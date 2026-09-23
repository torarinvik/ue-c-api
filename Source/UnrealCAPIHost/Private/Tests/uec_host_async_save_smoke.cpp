#include "CoreMinimal.h"

#include "uec_api.h"

namespace
{
    enum class ESaveSmokeStage : uint8
    {
        Saving,
        Loading
    };

    struct FAsyncSaveSmokeState
    {
        const uec_api* Api = nullptr;
        uec_context* Context = nullptr;
        uec_object* SaveObject = nullptr;
        FString SlotName;
        uec_runtime_stats Baseline{};
        uint64_t RequestId = 0;
        ESaveSmokeStage Stage = ESaveSmokeStage::Saving;
        uec_result Result = UEC_RESULT_NOT_INITIALIZED;
        bool Started = false;
        bool Complete = false;
    };

    FAsyncSaveSmokeState GAsyncSaveSmokeState;

    void UEC_CALL OnAsyncSaveSmokeComplete(uint64_t requestId,
                                          uec_result result,
                                          uec_object* saveGame,
                                          uec_bool success,
                                          void* userData);

    void FinishAsyncSaveSmoke(FAsyncSaveSmokeState& state, uec_result result)
    {
        if (state.Complete) return;
        if (state.RequestId != 0 && state.Api != nullptr && state.Context != nullptr &&
            state.Api->cancel_save_game_request != nullptr) {
            state.Api->cancel_save_game_request(state.Context, state.RequestId);
            state.RequestId = 0;
        }
        if (state.SaveObject != nullptr && state.Api != nullptr &&
            state.Api->release_object != nullptr) {
            const uec_result releaseResult = state.Api->release_object(state.SaveObject);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
            state.SaveObject = nullptr;
        }
        if (state.Context != nullptr && state.Api != nullptr &&
            !state.SlotName.IsEmpty() &&
            state.Api->delete_game_slot != nullptr) {
            FTCHARToUTF8 slotUtf8(*state.SlotName);
            const uec_string_view slotName{slotUtf8.Get(),
                                           static_cast<size_t>(slotUtf8.Length())};
            uec_bool deleted = UEC_FALSE;
            state.Api->delete_game_slot(state.Context, slotName, 0, &deleted);
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

    bool CallbackStatsAreValid(FAsyncSaveSmokeState& state, uint32 expectedObjects)
    {
        if (state.Baseline.active_callbacks == UINT32_MAX) return false;
        uec_runtime_stats observed{};
        observed.struct_size = sizeof(observed);
        return state.Api != nullptr && state.Context != nullptr &&
            state.Api->get_runtime_stats != nullptr &&
            state.Api->get_runtime_stats(state.Context, &observed) == UEC_RESULT_OK &&
            observed.pending_requests == state.Baseline.pending_requests &&
            observed.active_callbacks == state.Baseline.active_callbacks + 1u &&
            observed.live_contexts == state.Baseline.live_contexts &&
            observed.live_objects == expectedObjects;
    }

    void StartAsyncLoad(FAsyncSaveSmokeState& state)
    {
        state.Stage = ESaveSmokeStage::Loading;
        FTCHARToUTF8 slotUtf8(*state.SlotName);
        const uec_string_view slotName{slotUtf8.Get(),
                                       static_cast<size_t>(slotUtf8.Length())};
        state.Result = state.Api->async_load_game_from_slot(
            state.Context, slotName, 0, &OnAsyncSaveSmokeComplete, &state,
            &state.RequestId);
        if (state.Result != UEC_RESULT_OK) {
            FinishAsyncSaveSmoke(state, state.Result);
        }
    }

    void UEC_CALL OnAsyncSaveSmokeComplete(uint64_t requestId,
                                          uec_result result,
                                          uec_object* saveGame,
                                          uec_bool success,
                                          void* userData)
    {
        auto* state = static_cast<FAsyncSaveSmokeState*>(userData);
        if (state == nullptr || state->Complete) return;
        if (requestId == 0 || requestId != state->RequestId) {
            if (saveGame != nullptr && state->Api != nullptr) {
                state->Api->release_object(saveGame);
            }
            FinishAsyncSaveSmoke(*state, UEC_RESULT_INTERNAL_ERROR);
            return;
        }
        state->RequestId = 0;

        if (state->Stage == ESaveSmokeStage::Saving) {
            const bool valid = result == UEC_RESULT_OK && success == UEC_TRUE &&
                saveGame == nullptr &&
                CallbackStatsAreValid(*state, state->Baseline.live_objects);
            if (!valid || state->SaveObject == nullptr) {
                if (saveGame != nullptr) state->Api->release_object(saveGame);
                FinishAsyncSaveSmoke(*state, UEC_RESULT_INTERNAL_ERROR);
                return;
            }
            const uec_result releaseResult = state->Api->release_object(state->SaveObject);
            if (releaseResult != UEC_RESULT_OK) {
                FinishAsyncSaveSmoke(*state, releaseResult);
                return;
            }
            state->SaveObject = nullptr;
            if (state->Baseline.live_objects == 0u) {
                FinishAsyncSaveSmoke(*state, UEC_RESULT_INTERNAL_ERROR);
                return;
            }
            --state->Baseline.live_objects;
            StartAsyncLoad(*state);
            return;
        }

        const bool valid = result == UEC_RESULT_OK && success == UEC_TRUE &&
            saveGame != nullptr &&
            state->Baseline.live_objects != UINT32_MAX &&
            CallbackStatsAreValid(*state, state->Baseline.live_objects + 1u);
        if (!valid) {
            if (saveGame != nullptr) state->Api->release_object(saveGame);
            FinishAsyncSaveSmoke(*state, UEC_RESULT_INTERNAL_ERROR);
            return;
        }
        const uec_result releaseResult = state->Api->release_object(saveGame);
        if (releaseResult != UEC_RESULT_OK) {
            FinishAsyncSaveSmoke(*state, releaseResult);
            return;
        }

        FTCHARToUTF8 slotUtf8(*state->SlotName);
        const uec_string_view slotName{slotUtf8.Get(),
                                       static_cast<size_t>(slotUtf8.Length())};
        uec_bool deleted = UEC_FALSE;
        const uec_result deleteResult = state->Api->delete_game_slot(
            state->Context, slotName, 0, &deleted);
        if (deleteResult == UEC_RESULT_OK && deleted == UEC_TRUE) {
            state->SlotName.Empty();
        }
        FinishAsyncSaveSmoke(
            *state,
            deleteResult == UEC_RESULT_OK && deleted == UEC_TRUE
                ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR);
    }
}

extern "C" uec_result UEC_CALL uec_host_async_save_smoke_start(void)
{
    FAsyncSaveSmokeState& state = GAsyncSaveSmokeState;
    if (state.Started) return UEC_RESULT_INVALID_ARGUMENT;
    state = FAsyncSaveSmokeState{};
    state.Started = true;
    state.SlotName = FString::Printf(
        TEXT("UECAPI_Smoke_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));

    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR,
                                    &state.Api, &state.Context);
    if (result != UEC_RESULT_OK) {
        FinishAsyncSaveSmoke(state, result);
        return result;
    }
    if (state.Api == nullptr || state.Context == nullptr ||
        state.Api->create_save_game == nullptr ||
        state.Api->async_save_game_to_slot == nullptr ||
        state.Api->async_load_game_from_slot == nullptr ||
        state.Api->cancel_save_game_request == nullptr ||
        state.Api->delete_game_slot == nullptr ||
        state.Api->get_runtime_stats == nullptr ||
        state.Api->release_object == nullptr ||
        state.Api->release_context == nullptr) {
        FinishAsyncSaveSmoke(state, UEC_RESULT_INTERNAL_ERROR);
        return UEC_RESULT_INTERNAL_ERROR;
    }

    FTCHARToUTF8 classUtf8(TEXT("/Script/UnrealCAPIHost.UECAPIHostSaveGame"));
    const uec_string_view classPath{classUtf8.Get(),
                                    static_cast<size_t>(classUtf8.Length())};
    result = state.Api->create_save_game(state.Context, classPath, &state.SaveObject);
    if (result != UEC_RESULT_OK) {
        UE_LOG(LogTemp, Error, TEXT("Async save smoke create failed: %d"),
               static_cast<int32>(result));
        FinishAsyncSaveSmoke(state, result);
        return result;
    }
    state.Baseline.struct_size = sizeof(state.Baseline);
    result = state.Api->get_runtime_stats(state.Context, &state.Baseline);
    if (result != UEC_RESULT_OK || state.Baseline.pending_requests != 0u ||
        state.Baseline.active_callbacks != 0u) {
        UE_LOG(LogTemp, Error,
               TEXT("Async save smoke baseline failed: result=%d requests=%u callbacks=%u"),
               static_cast<int32>(result), state.Baseline.pending_requests,
               state.Baseline.active_callbacks);
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        FinishAsyncSaveSmoke(state, result);
        return result;
    }

    FTCHARToUTF8 slotUtf8(*state.SlotName);
    const uec_string_view slotName{slotUtf8.Get(),
                                   static_cast<size_t>(slotUtf8.Length())};
    result = state.Api->async_save_game_to_slot(
        state.SaveObject, slotName, 0, &OnAsyncSaveSmokeComplete, &state,
        &state.RequestId);
    if (result != UEC_RESULT_OK) {
        UE_LOG(LogTemp, Error, TEXT("Async save smoke request failed: %d"),
               static_cast<int32>(result));
        FinishAsyncSaveSmoke(state, result);
        return result;
    }
    return UEC_RESULT_OK;
}

extern "C" uec_bool UEC_CALL uec_host_async_save_smoke_poll(uec_result* outResult)
{
    if (outResult == nullptr) return UEC_FALSE;
    const FAsyncSaveSmokeState& state = GAsyncSaveSmokeState;
    *outResult = state.Complete ? state.Result : UEC_RESULT_NOT_INITIALIZED;
    return state.Complete ? UEC_TRUE : UEC_FALSE;
}

extern "C" void UEC_CALL uec_host_async_save_smoke_cancel(void)
{
    FinishAsyncSaveSmoke(GAsyncSaveSmokeState, UEC_RESULT_INTERNAL_ERROR);
}
