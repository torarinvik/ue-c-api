#include "CoreMinimal.h"

#include "uec_api.h"

namespace
{
    enum class EObjectLoadSmokeStage : uint8
    {
        ObserveCancellation,
        FailingLoad,
        StartSuccessfulLoad,
        Loading,
        ConfirmCancellation
    };

    struct FObjectLoadSmokeState
    {
        const uec_api* Api = nullptr;
        uec_context* Context = nullptr;
        uec_runtime_stats Baseline{};
        uint64_t RequestId = 0;
        uint64_t CancelledRequestId = 0;
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

        FTCHARToUTF8 expectedPath(TEXT("/Script/Engine.Actor"));
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
            requiredSize == static_cast<size_t>(expectedPath.Length()) + 1u &&
            FMemory::Memcmp(pathBuffer, expectedPath.Get(),
                            static_cast<size_t>(expectedPath.Length())) == 0;
        uec_result completionResult = valid ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
        if (loadedObject != nullptr && state->Api != nullptr &&
            state->Api->release_object != nullptr) {
            const uec_result releaseResult = state->Api->release_object(loadedObject);
            if (completionResult == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                completionResult = releaseResult;
            }
        }
        if (completionResult == UEC_RESULT_OK) {
            state->PendingResult = completionResult;
            state->PollsInStage = 0;
            state->Stage = EObjectLoadSmokeStage::ConfirmCancellation;
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
        state.Api->request_object_load == nullptr ||
        state.Api->cancel_object_load == nullptr ||
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

    FTCHARToUTF8 objectPathUtf8(TEXT("/Script/Engine.Actor"));
    const uec_string_view objectPath{objectPathUtf8.Get(),
                                     static_cast<size_t>(objectPathUtf8.Length())};
    result = state.Api->request_object_load(
        state.Context, objectPath, &OnObjectLoadSmokeComplete, &state, &state.RequestId);
    if (result != UEC_RESULT_OK) {
        FinishObjectLoadSmoke(state, result);
        return result;
    }
    state.CancelledRequestId = state.RequestId;
    result = state.Api->cancel_object_load(state.Context, state.RequestId);
    if (result != UEC_RESULT_OK) {
        FinishObjectLoadSmoke(state, result);
        return result;
    }
    if (!RuntimeStatsReturnedToBaseline(state)) {
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
        else if (state.Stage == EObjectLoadSmokeStage::ConfirmCancellation &&
                 state.PollsInStage >= 2u) {
            FinishObjectLoadSmoke(state, state.PendingResult);
        }
    }
    *outResult = state.Complete ? state.Result : UEC_RESULT_NOT_INITIALIZED;
    return state.Complete ? UEC_TRUE : UEC_FALSE;
}

extern "C" void UEC_CALL uec_host_object_load_smoke_cancel(void)
{
    FinishObjectLoadSmoke(GObjectLoadSmokeState, UEC_RESULT_INTERNAL_ERROR);
}
