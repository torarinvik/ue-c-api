#include "CoreMinimal.h"

#include "uec_api.h"

DEFINE_LOG_CATEGORY_STATIC(LogObjectLoadSmoke, Log, All);

namespace
{
    enum class EObjectLoadSmokeStage : uint8
    {
        ObserveCancellation,
        FailingLoad,
        StartSuccessfulLoad,
        Loading,
        ConfirmRetention
    };

    struct FObjectLoadSmokeState
    {
        const uec_api* Api = nullptr;
        uec_context* Context = nullptr;
        uec_runtime_stats Baseline{};
        uint64_t RequestId = 0;
        uint64_t CancelledRequestId = 0;
        uec_object* RetainedObject = nullptr;
        uec_result Result = UEC_RESULT_NOT_INITIALIZED;
        EObjectLoadSmokeStage Stage = EObjectLoadSmokeStage::ObserveCancellation;
        uec_result PendingResult = UEC_RESULT_NOT_INITIALIZED;
        uint32 PollsInStage = 0;
        bool Started = false;
        bool Complete = false;
    };

    FObjectLoadSmokeState GObjectLoadSmokeState;

    void FinishObjectLoadSmoke(FObjectLoadSmokeState& state, uec_result result)
    {
        if (state.Complete) return;
        if (state.RequestId != 0 && state.Api != nullptr && state.Context != nullptr &&
            state.Api->cancel_object_load != nullptr) {
            state.Api->cancel_object_load(state.Context, state.RequestId);
            state.RequestId = 0;
        }
        if (state.RetainedObject != nullptr && state.Api != nullptr &&
            state.Api->release_object != nullptr) {
            const uec_result releaseResult = state.Api->release_object(state.RetainedObject);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
            state.RetainedObject = nullptr;
        }
        if (state.Context != nullptr && state.Api != nullptr &&
            state.Api->release_context != nullptr) {
            const uec_result releaseResult = state.Api->release_context(state.Context);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
        }
        state.Context = nullptr;
        state.Result = result;
        state.Complete = true;
    }

    bool CallbackStatsAreValid(FObjectLoadSmokeState& state,
                               uec_object* loadedObject)
    {
        if (state.Baseline.active_callbacks == UINT32_MAX ||
            state.Baseline.live_objects == UINT32_MAX) return false;
        uec_runtime_stats observed{};
        observed.struct_size = sizeof(observed);
        return state.Api != nullptr && state.Context != nullptr &&
            state.Api->get_runtime_stats != nullptr &&
            state.Api->get_runtime_stats(state.Context, &observed) == UEC_RESULT_OK &&
            observed.pending_requests == state.Baseline.pending_requests &&
            observed.active_callbacks == state.Baseline.active_callbacks + 1u &&
            observed.live_contexts == state.Baseline.live_contexts &&
            observed.live_objects == state.Baseline.live_objects +
                (loadedObject != nullptr ? 1u : 0u);
    }

    bool RuntimeStatsReturnedToBaseline(FObjectLoadSmokeState& state)
    {
        uec_runtime_stats observed{};
        observed.struct_size = sizeof(observed);
        return state.Api != nullptr && state.Context != nullptr &&
            state.Api->get_runtime_stats != nullptr &&
            state.Api->get_runtime_stats(state.Context, &observed) == UEC_RESULT_OK &&
            observed.pending_requests == state.Baseline.pending_requests &&
            observed.active_callbacks == state.Baseline.active_callbacks &&
            observed.live_contexts == state.Baseline.live_contexts &&
            observed.live_objects == state.Baseline.live_objects;
    }

    bool VerifyMissingPathQueries(FObjectLoadSmokeState& state)
    {
        const FString missingPath = FString::Printf(
            TEXT("/UECAPI/PathQuery_%s.Missing"),
            *FGuid::NewGuid().ToString(EGuidFormats::Digits));
        FTCHARToUTF8 pathUtf8(*missingPath);
        const uec_string_view path{pathUtf8.Get(),
                                   static_cast<size_t>(pathUtf8.Length())};
        uec_bool objectLoaded = UEC_TRUE;
        uec_bool classLoaded = UEC_TRUE;
        if (state.Api == nullptr || state.Context == nullptr ||
            state.Api->is_object_path_loaded == nullptr ||
            state.Api->is_class_path_loaded == nullptr) return false;
        const uec_result objectResult = state.Api->is_object_path_loaded(
            state.Context, path, &objectLoaded);
        const uec_result classResult = state.Api->is_class_path_loaded(
            state.Context, path, &classLoaded);
        const bool valid = objectResult == UEC_RESULT_OK && classResult == UEC_RESULT_OK &&
            objectLoaded == UEC_FALSE && classLoaded == UEC_FALSE;
        if (!valid) {
            UE_LOG(LogTemp, Error,
                TEXT("Missing path query failed: object=%d/%u class=%d/%u path=%s"),
                static_cast<int32>(objectResult), static_cast<uint32>(objectLoaded),
                static_cast<int32>(classResult), static_cast<uint32>(classLoaded),
                *missingPath);
        }
        return valid;
    }

    bool RetainedObjectStatsAreValid(FObjectLoadSmokeState& state)
    {
        if (state.Baseline.live_objects == UINT32_MAX) return false;
        uec_runtime_stats observed{};
        observed.struct_size = sizeof(observed);
        return state.Api != nullptr && state.Context != nullptr &&
            state.Api->get_runtime_stats(state.Context, &observed) == UEC_RESULT_OK &&
            observed.pending_requests == state.Baseline.pending_requests &&
            observed.active_callbacks == state.Baseline.active_callbacks + 1u &&
            observed.live_contexts == state.Baseline.live_contexts &&
            observed.live_objects == state.Baseline.live_objects + 1u;
    }

    using FGetObjectText = uec_result (UEC_CALL *)(uec_object*, char*, size_t, size_t*);

    bool CheckObjectText(const uec_api* api,
                         uec_object* object,
                         FGetObjectText getter,
                         const char* expected)
    {
        char actual[256]{};
        size_t requiredSize = 0u;
        if (api == nullptr || object == nullptr || getter == nullptr || expected == nullptr ||
            getter(object, actual, sizeof(actual), &requiredSize) != UEC_RESULT_OK) {
            return false;
        }
        size_t expectedSize = 1u;
        while (expected[expectedSize - 1u] != '\0') ++expectedSize;
        if (requiredSize != expectedSize || requiredSize > sizeof(actual)) return false;
        for (size_t index = 0u; index < expectedSize; ++index) {
            if (actual[index] != expected[index]) return false;
        }
        return true;
    }

    bool VerifySynchronousObjectLookup(FObjectLoadSmokeState& state)
    {
        static constexpr char actorClassPath[] = "/Script/Engine.Actor";
        static constexpr char classClassPath[] = "/Script/CoreUObject.Class";
        const uec_string_view actorClass{
            actorClassPath, sizeof(actorClassPath) - 1u};
        const uec_string_view classClass{
            classClassPath, sizeof(classClassPath) - 1u};
        const FString missingPath = FString::Printf(
            TEXT("/UECAPI/SyncLookup_%s.Missing"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
        FTCHARToUTF8 missingPathUtf8(*missingPath);
        const uec_string_view missingObjectPath{
            missingPathUtf8.Get(), static_cast<size_t>(missingPathUtf8.Length())};
        uec_object* foundObject = nullptr;
        uec_object* loadedObject = nullptr;
        uec_object* missingObject = nullptr;
        uec_object* invalidOutput = nullptr;
        uec_bool isClass = UEC_FALSE;
        uec_runtime_stats observed{};
        uec_result result = state.Api->find_object(
            state.Context, actorClass, &foundObject);
        if (result != UEC_RESULT_OK || foundObject == nullptr ||
            !CheckObjectText(state.Api, foundObject, state.Api->get_object_name, "Actor") ||
            !CheckObjectText(state.Api, foundObject, state.Api->get_object_path,
                             actorClassPath) ||
            !CheckObjectText(state.Api, foundObject, state.Api->get_object_class_name,
                             classClassPath)) {
            goto cleanup;
        }
        result = state.Api->object_is_a(foundObject, classClass, &isClass);
        if (result != UEC_RESULT_OK || isClass != UEC_TRUE) goto cleanup;

        result = state.Api->load_object(state.Context, actorClass, &loadedObject);
        if (result != UEC_RESULT_OK || loadedObject == nullptr ||
            !CheckObjectText(state.Api, loadedObject, state.Api->get_object_path,
                             actorClassPath) ||
            !CheckObjectText(state.Api, loadedObject, state.Api->get_object_class_name,
                             classClassPath)) {
            goto cleanup;
        }
        isClass = UEC_FALSE;
        result = state.Api->object_is_a(loadedObject, classClass, &isClass);
        if (result != UEC_RESULT_OK || isClass != UEC_TRUE) goto cleanup;

        missingObject = reinterpret_cast<uec_object*>(state.Context);
        result = state.Api->find_object(
            state.Context, missingObjectPath, &missingObject);
        if (result != UEC_RESULT_NOT_INITIALIZED || missingObject != nullptr) {
            if (missingObject != nullptr &&
                missingObject != reinterpret_cast<uec_object*>(state.Context)) {
                (void)state.Api->release_object(missingObject);
            }
            missingObject = nullptr;
            goto cleanup;
        }
        invalidOutput = reinterpret_cast<uec_object*>(state.Context);
        result = state.Api->load_object(
            state.Context, missingObjectPath, &invalidOutput);
        if (result != UEC_RESULT_INVALID_ARGUMENT || invalidOutput != nullptr) {
            if (invalidOutput != nullptr &&
                invalidOutput != reinterpret_cast<uec_object*>(state.Context)) {
                (void)state.Api->release_object(invalidOutput);
            }
            invalidOutput = nullptr;
            goto cleanup;
        }

        result = UEC_RESULT_OK;

cleanup:
        if (missingObject != nullptr) (void)state.Api->release_object(missingObject);
        if (loadedObject != nullptr) (void)state.Api->release_object(loadedObject);
        if (foundObject != nullptr) (void)state.Api->release_object(foundObject);
        if (result != UEC_RESULT_OK) {
            UE_LOG(LogTemp, Error, TEXT("Synchronous object lookup failed: result=%d"),
                   static_cast<int32>(result));
            return false;
        }
        observed.struct_size = sizeof(observed);
        const uec_result statsResult = state.Api->get_runtime_stats(state.Context, &observed);
        const bool statsValid = statsResult == UEC_RESULT_OK &&
            observed.live_objects == state.Baseline.live_objects &&
            observed.pending_requests == state.Baseline.pending_requests &&
            observed.active_callbacks == state.Baseline.active_callbacks;
        if (!statsValid) {
            UE_LOG(LogTemp, Error,
                TEXT("Synchronous object lookup stats failed: result=%d objects=%u/%u pending=%u/%u callbacks=%u/%u"),
                static_cast<int32>(statsResult), observed.live_objects,
                state.Baseline.live_objects, observed.pending_requests,
                state.Baseline.pending_requests, observed.active_callbacks,
                state.Baseline.active_callbacks);
        }
        return statsValid;
    }

    void UEC_CALL OnObjectLoadSmokeComplete(uint64_t requestId,
                                            uec_result result,
                                            uec_object* loadedObject,
                                            void* userData)
    {
        auto* state = static_cast<FObjectLoadSmokeState*>(userData);
        if (state == nullptr) return;
        if (requestId == state->CancelledRequestId) {
            if (loadedObject != nullptr && state->Api != nullptr) {
                state->Api->release_object(loadedObject);
            }
            if (state->Complete) {
                state->Result = UEC_RESULT_INTERNAL_ERROR;
                return;
            }
            FinishObjectLoadSmoke(*state, UEC_RESULT_INTERNAL_ERROR);
            return;
        }
        if (state->Complete) return;
        if (requestId == 0 || requestId != state->RequestId) {
            if (loadedObject != nullptr && state->Api != nullptr) {
                state->Api->release_object(loadedObject);
            }
            FinishObjectLoadSmoke(*state, UEC_RESULT_INTERNAL_ERROR);
            return;
        }
        state->RequestId = 0;

        if (state->Stage == EObjectLoadSmokeStage::FailingLoad) {
            const bool validFailure = result == UEC_RESULT_INTERNAL_ERROR &&
                loadedObject == nullptr && CallbackStatsAreValid(*state, nullptr);
            if (loadedObject != nullptr && state->Api != nullptr) {
                state->Api->release_object(loadedObject);
            }
            if (!validFailure) {
                FinishObjectLoadSmoke(*state, UEC_RESULT_INTERNAL_ERROR);
                return;
            }
            state->PendingResult = UEC_RESULT_OK;
            state->PollsInStage = 0;
            state->Stage = EObjectLoadSmokeStage::StartSuccessfulLoad;
            return;
        }
        if (state->Stage != EObjectLoadSmokeStage::Loading) {
            if (loadedObject != nullptr && state->Api != nullptr) {
                state->Api->release_object(loadedObject);
            }
            FinishObjectLoadSmoke(*state, UEC_RESULT_INTERNAL_ERROR);
            return;
        }

        static constexpr char expectedPath[] = "/Script/Engine.Actor";
        char pathBuffer[128]{};
        size_t requiredSize = 0;
        const uec_result pathResult = loadedObject != nullptr && state->Api != nullptr &&
                state->Api->get_object_path != nullptr
            ? state->Api->get_object_path(loadedObject, pathBuffer, sizeof(pathBuffer),
                                          &requiredSize)
            : UEC_RESULT_INTERNAL_ERROR;
        const bool valid = result == UEC_RESULT_OK && loadedObject != nullptr &&
            state->Stage == EObjectLoadSmokeStage::Loading &&
            CallbackStatsAreValid(*state, loadedObject) && pathResult == UEC_RESULT_OK &&
            requiredSize == sizeof(expectedPath) &&
            FMemory::Memcmp(pathBuffer, expectedPath, sizeof(expectedPath)) == 0;
        uec_bool objectPathLoaded = UEC_FALSE;
        uec_bool classPathLoaded = UEC_FALSE;
        const uec_string_view loadedPath{expectedPath, sizeof(expectedPath) - 1u};
        const bool pathQueriesValid = valid &&
            state->Api->is_object_path_loaded(state->Context, loadedPath,
                                              &objectPathLoaded) == UEC_RESULT_OK &&
            state->Api->is_class_path_loaded(state->Context, loadedPath,
                                             &classPathLoaded) == UEC_RESULT_OK &&
            objectPathLoaded == UEC_TRUE && classPathLoaded == UEC_TRUE;
        uec_result completionResult = valid ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
        if (completionResult == UEC_RESULT_OK && !pathQueriesValid) {
            completionResult = UEC_RESULT_INTERNAL_ERROR;
        }
        uec_object* retainedObject = nullptr;
        if (completionResult == UEC_RESULT_OK &&
            state->Api->retain_object(loadedObject, &retainedObject) != UEC_RESULT_OK) {
            completionResult = UEC_RESULT_INTERNAL_ERROR;
        }
        if (loadedObject != nullptr && state->Api != nullptr &&
            state->Api->release_object != nullptr) {
            const uec_result releaseResult = state->Api->release_object(loadedObject);
            if (completionResult == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                completionResult = releaseResult;
            }
        }
        if (completionResult == UEC_RESULT_OK && !RetainedObjectStatsAreValid(*state)) {
            completionResult = UEC_RESULT_INTERNAL_ERROR;
        }
        if (completionResult == UEC_RESULT_OK) {
            state->RetainedObject = retainedObject;
            retainedObject = nullptr;
        }
        if (retainedObject != nullptr && state->Api != nullptr &&
            state->Api->release_object != nullptr) {
            (void)state->Api->release_object(retainedObject);
        }
        if (completionResult == UEC_RESULT_OK) {
            state->PendingResult = completionResult;
            state->PollsInStage = 0;
            state->Stage = EObjectLoadSmokeStage::ConfirmRetention;
        }
        else {
            FinishObjectLoadSmoke(*state, completionResult);
        }
    }
}

extern "C" uec_result UEC_CALL uec_host_object_load_smoke_start(void)
{
    FObjectLoadSmokeState& state = GObjectLoadSmokeState;
    if (state.Started) return UEC_RESULT_INVALID_ARGUMENT;
    state = FObjectLoadSmokeState{};
    state.Started = true;

    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR,
                                    &state.Api, &state.Context);
    if (result != UEC_RESULT_OK) {
        FinishObjectLoadSmoke(state, result);
        return result;
    }
    if (state.Api == nullptr || state.Context == nullptr ||
        state.Api->load_object == nullptr || state.Api->find_object == nullptr ||
        state.Api->get_object_name == nullptr || state.Api->get_object_path == nullptr ||
        state.Api->get_object_class_name == nullptr || state.Api->object_is_a == nullptr ||
        state.Api->request_object_load == nullptr ||
        state.Api->cancel_object_load == nullptr ||
        state.Api->retain_object == nullptr ||
        state.Api->is_object_path_loaded == nullptr ||
        state.Api->is_class_path_loaded == nullptr ||
        state.Api->get_object_path == nullptr ||
        state.Api->get_runtime_stats == nullptr ||
        state.Api->release_object == nullptr ||
        state.Api->release_context == nullptr) {
        FinishObjectLoadSmoke(state, UEC_RESULT_INTERNAL_ERROR);
        return UEC_RESULT_INTERNAL_ERROR;
    }

    state.Baseline.struct_size = sizeof(state.Baseline);
    result = state.Api->get_runtime_stats(state.Context, &state.Baseline);
    if (result != UEC_RESULT_OK || state.Baseline.pending_requests != 0u ||
        state.Baseline.active_callbacks != 0u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        FinishObjectLoadSmoke(state, result);
        return result;
    }
    if (!VerifySynchronousObjectLookup(state)) {
        FinishObjectLoadSmoke(state, UEC_RESULT_INTERNAL_ERROR);
        return UEC_RESULT_INTERNAL_ERROR;
    }

    if (!VerifyMissingPathQueries(state)) {
        FinishObjectLoadSmoke(state, UEC_RESULT_INTERNAL_ERROR);
        return UEC_RESULT_INTERNAL_ERROR;
    }

    FTCHARToUTF8 objectPathUtf8(TEXT("/Script/Engine.Actor"));
    const uec_string_view objectPath{objectPathUtf8.Get(),
                                     static_cast<size_t>(objectPathUtf8.Length())};
    result = state.Api->request_object_load(
        state.Context, objectPath, &OnObjectLoadSmokeComplete, &state, &state.RequestId);
    if (result != UEC_RESULT_OK) {
        UE_LOG(LogTemp, Error, TEXT("Object-load cancellation request failed: %d"),
               static_cast<int32>(result));
        FinishObjectLoadSmoke(state, result);
        return result;
    }
    state.CancelledRequestId = state.RequestId;
    result = state.Api->cancel_object_load(state.Context, state.RequestId);
    if (result != UEC_RESULT_OK) {
        UE_LOG(LogTemp, Error, TEXT("Object-load cancellation failed: %d"),
               static_cast<int32>(result));
        FinishObjectLoadSmoke(state, result);
        return result;
    }
    if (!RuntimeStatsReturnedToBaseline(state)) {
        uec_runtime_stats observed{};
        observed.struct_size = sizeof(observed);
        const uec_result statsResult = state.Api->get_runtime_stats(state.Context, &observed);
        UE_LOG(LogObjectLoadSmoke, Error,
            TEXT("Post-cancel stats mismatch: result=%d context=%u/%u objects=%u/%u requests=%u/%u callbacks=%u/%u"),
            static_cast<int32>(statsResult), observed.live_contexts,
            state.Baseline.live_contexts, observed.live_objects,
            state.Baseline.live_objects, observed.pending_requests,
            state.Baseline.pending_requests, observed.active_callbacks,
            state.Baseline.active_callbacks);
        FinishObjectLoadSmoke(state, UEC_RESULT_INTERNAL_ERROR);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    state.RequestId = 0;
    state.PollsInStage = 0;
    state.Stage = EObjectLoadSmokeStage::ObserveCancellation;
    return UEC_RESULT_OK;
}

static uec_result StartVerifiedObjectLoad(FObjectLoadSmokeState& state)
{
    FTCHARToUTF8 objectPathUtf8(TEXT("/Script/Engine.Actor"));
    const uec_string_view objectPath{objectPathUtf8.Get(),
                                     static_cast<size_t>(objectPathUtf8.Length())};
    const uec_result result = state.Api->request_object_load(
        state.Context, objectPath, &OnObjectLoadSmokeComplete, &state, &state.RequestId);
    if (result != UEC_RESULT_OK) {
        FinishObjectLoadSmoke(state, result);
        return state.Result;
    }
    state.Stage = EObjectLoadSmokeStage::Loading;
    state.PollsInStage = 0;
    return UEC_RESULT_OK;
}

static void StartMissingObjectLoad(FObjectLoadSmokeState& state)
{
    const FString path = FString::Printf(
        TEXT("/UECAPI/Smoke_%s.Missing"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    FTCHARToUTF8 objectPathUtf8(*path);
    const uec_string_view objectPath{objectPathUtf8.Get(),
                                     static_cast<size_t>(objectPathUtf8.Length())};
    const uec_result result = state.Api->request_object_load(
        state.Context, objectPath, &OnObjectLoadSmokeComplete, &state, &state.RequestId);
    if (result == UEC_RESULT_OK && state.RequestId != 0) {
        state.Stage = EObjectLoadSmokeStage::FailingLoad;
        state.PollsInStage = 0;
        return;
    }
    if (result == UEC_RESULT_INTERNAL_ERROR && RuntimeStatsReturnedToBaseline(state)) {
        state.Stage = EObjectLoadSmokeStage::StartSuccessfulLoad;
        state.PendingResult = UEC_RESULT_OK;
        state.PollsInStage = 0;
        return;
    }
    FinishObjectLoadSmoke(
        state, result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result);
}

extern "C" uec_bool UEC_CALL uec_host_object_load_smoke_poll(uec_result* outResult)
{
    if (outResult == nullptr) return UEC_FALSE;
    FObjectLoadSmokeState& state = GObjectLoadSmokeState;
    if (!state.Complete) {
        ++state.PollsInStage;
        if (state.Stage == EObjectLoadSmokeStage::ObserveCancellation &&
            state.PollsInStage >= 2u) {
            StartMissingObjectLoad(state);
        }
        else if (state.Stage == EObjectLoadSmokeStage::StartSuccessfulLoad &&
                 state.PollsInStage >= 1u) {
            StartVerifiedObjectLoad(state);
        }
        else if (state.Stage == EObjectLoadSmokeStage::ConfirmRetention &&
                 state.PollsInStage >= 2u) {
            CollectGarbage(RF_NoFlags);
            const bool retainedObjectValid = state.RetainedObject != nullptr &&
                CheckObjectText(state.Api, state.RetainedObject,
                                state.Api->get_object_path,
                                "/Script/Engine.Actor");
            const uec_result releaseResult = state.RetainedObject != nullptr
                ? state.Api->release_object(state.RetainedObject)
                : UEC_RESULT_INVALID_HANDLE;
            state.RetainedObject = nullptr;
            const bool baselineRestored = RuntimeStatsReturnedToBaseline(state);
            FinishObjectLoadSmoke(
                state, state.PendingResult == UEC_RESULT_OK && retainedObjectValid &&
                    releaseResult == UEC_RESULT_OK && baselineRestored
                    ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR);
        }
    }
    *outResult = state.Complete ? state.Result : UEC_RESULT_NOT_INITIALIZED;
    return state.Complete ? UEC_TRUE : UEC_FALSE;
}

extern "C" void UEC_CALL uec_host_object_load_smoke_cancel(void)
{
    FinishObjectLoadSmoke(GObjectLoadSmokeState, UEC_RESULT_INTERNAL_ERROR);
}
