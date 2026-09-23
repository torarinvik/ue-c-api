#include "CoreMinimal.h"
#include "Async/Async.h"
#include "Async/ParallelFor.h"
#include "HAL/Event.h"
#include "HAL/PlatformProcess.h"

#include "uec_api.h"

namespace
{
    constexpr int32 QueueCapacity = 1024;
    constexpr int32 ExtraSubmissions = 128;
    constexpr uint32 CancellationBatch = 128;

    struct FQueueSmokeState;

    struct FQueueSubmission
    {
        FQueueSmokeState* Owner = nullptr;
        uec_result Result = UEC_RESULT_INTERNAL_ERROR;
        uint64 RequestId = 42;
        bool Cancelled = false;
        bool CallbackExecuted = false;
    };

    struct FQueueSmokeState
    {
        const uec_api* Api = nullptr;
        uec_context* Context = nullptr;
        TArray<FQueueSubmission> Submissions;
        TArray<uint64> AcceptedIds;
        uec_runtime_stats Baseline{};
        uec_result Result = UEC_RESULT_NOT_INITIALIZED;
        uint32 AcceptedCount = 0;
        uint32 RejectedCount = 0;
        uint32 CancelledCount = 0;
        uint32 ExpectedCallbackCount = 0;
        uint32 CallbackCount = 0;
        bool CallbackStatsValid = true;
        bool Started = false;
        bool Complete = false;
    };

    FQueueSmokeState GQueueSmokeState;

    bool HasRuntimeStatsReturnedToBaseline(FQueueSmokeState& state)
    {
        uec_runtime_stats observed{};
        observed.struct_size = sizeof(observed);
        if (state.Api == nullptr || state.Api->get_runtime_stats == nullptr ||
            state.Context == nullptr ||
            state.Api->get_runtime_stats(state.Context, &observed) != UEC_RESULT_OK) {
            return false;
        }
        return observed.pending_requests == state.Baseline.pending_requests &&
            observed.active_callbacks == state.Baseline.active_callbacks &&
            observed.live_contexts == state.Baseline.live_contexts &&
            observed.live_worlds == state.Baseline.live_worlds &&
            observed.live_actors == state.Baseline.live_actors &&
            observed.live_components == state.Baseline.live_components &&
            observed.live_classes == state.Baseline.live_classes &&
            observed.live_objects == state.Baseline.live_objects;
    }

    void FinishQueueSmoke(FQueueSmokeState& state,
                         uec_result result,
                         bool cancelPending)
    {
        if (state.Complete) return;
        if (cancelPending && state.Api != nullptr && state.Context != nullptr &&
            state.Api->cancel_game_thread_request != nullptr) {
            for (const FQueueSubmission& submission : state.Submissions) {
                if (submission.Result != UEC_RESULT_OK || submission.RequestId == 0) continue;
                const uec_result cancelResult = state.Api->cancel_game_thread_request(
                    state.Context, submission.RequestId);
                if (cancelResult != UEC_RESULT_OK &&
                    cancelResult != UEC_RESULT_INVALID_ARGUMENT && result == UEC_RESULT_OK) {
                    result = cancelResult;
                }
            }
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

    void UEC_CALL CountQueuedCallback(void* userData)
    {
        auto* submission = static_cast<FQueueSubmission*>(userData);
        if (submission == nullptr || submission->Owner == nullptr) return;
        FQueueSmokeState* state = submission->Owner;
        if (state == nullptr || state->Complete) return;
        submission->CallbackExecuted = true;
        ++state->CallbackCount;
        uec_runtime_stats observed{};
        observed.struct_size = sizeof(observed);
        if (state->Api == nullptr || state->Context == nullptr ||
            state->Api->get_runtime_stats == nullptr ||
            state->Api->get_runtime_stats(state->Context, &observed) != UEC_RESULT_OK ||
            state->Baseline.active_callbacks == UINT32_MAX ||
            observed.active_callbacks != state->Baseline.active_callbacks + 1u ||
            observed.live_contexts != state->Baseline.live_contexts ||
            submission->Cancelled) {
            state->CallbackStatsValid = false;
        }
    }
}

extern "C" uec_result UEC_CALL uec_host_queue_smoke_start(void)
{
    FQueueSmokeState& state = GQueueSmokeState;
    if (state.Started) return UEC_RESULT_INVALID_ARGUMENT;
    state = FQueueSmokeState{};
    state.Started = true;

    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR,
                                    &state.Api, &state.Context);
    if (result != UEC_RESULT_OK) {
        FinishQueueSmoke(state, result, false);
        return result;
    }
    if (state.Api == nullptr || state.Context == nullptr ||
        state.Api->get_runtime_stats == nullptr ||
        state.Api->run_on_game_thread == nullptr ||
        state.Api->cancel_game_thread_request == nullptr ||
        state.Api->release_context == nullptr) {
        FinishQueueSmoke(state, UEC_RESULT_INTERNAL_ERROR, false);
        return UEC_RESULT_INTERNAL_ERROR;
    }

    state.Baseline.struct_size = sizeof(state.Baseline);
    result = state.Api->get_runtime_stats(state.Context, &state.Baseline);
    if (result != UEC_RESULT_OK || state.Baseline.pending_requests != 0u ||
        state.Baseline.active_callbacks != 0u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        FinishQueueSmoke(state, result, false);
        return result;
    }

    state.Submissions.SetNum(QueueCapacity + ExtraSubmissions);
    FEvent* submissionsComplete = FPlatformProcess::GetSynchEventFromPool(true);
    if (submissionsComplete == nullptr) {
        FinishQueueSmoke(state, UEC_RESULT_INTERNAL_ERROR, false);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    TFuture<void> submitter = Async(EAsyncExecution::ThreadPool, [&state, submissionsComplete]()
    {
        ParallelFor(state.Submissions.Num(), [&state](int32 index)
        {
            FQueueSubmission& submission = state.Submissions[index];
            submission.Owner = &state;
            submission.RequestId = 42;
            submission.Result = state.Api->run_on_game_thread(
                state.Context, &CountQueuedCallback, &submission, &submission.RequestId);
        });
        submissionsComplete->Trigger();
    });
    submissionsComplete->Wait();
    submitter.Get();
    FPlatformProcess::ReturnSynchEventToPool(submissionsComplete);

    state.AcceptedIds.Reserve(QueueCapacity);
    for (FQueueSubmission& submission : state.Submissions) {
        if (submission.Result == UEC_RESULT_OK) {
            if (submission.RequestId == 0u) {
                result = UEC_RESULT_INTERNAL_ERROR;
                break;
            }
            state.AcceptedIds.Add(submission.RequestId);
            ++state.AcceptedCount;
        } else if (submission.Result == UEC_RESULT_QUEUE_FULL) {
            if (submission.RequestId != 0u) {
                result = UEC_RESULT_INTERNAL_ERROR;
                break;
            }
            ++state.RejectedCount;
        } else {
            result = submission.Result;
            break;
        }
    }
    state.AcceptedIds.Sort();
    for (int32 index = 1; index < state.AcceptedIds.Num(); ++index) {
        if (state.AcceptedIds[index - 1] == state.AcceptedIds[index]) {
            result = UEC_RESULT_INTERNAL_ERROR;
            break;
        }
    }
    if (result == UEC_RESULT_OK &&
        (state.AcceptedCount != static_cast<uint32>(QueueCapacity) ||
         state.RejectedCount != static_cast<uint32>(ExtraSubmissions) ||
         state.CallbackCount != 0u)) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }

    for (FQueueSubmission& submission : state.Submissions) {
        if (result != UEC_RESULT_OK || submission.Result != UEC_RESULT_OK ||
            state.CancelledCount == CancellationBatch) {
            continue;
        }
        const uec_result cancelResult = state.Api->cancel_game_thread_request(
            state.Context, submission.RequestId);
        if (cancelResult != UEC_RESULT_OK) {
            result = cancelResult;
            break;
        }
        submission.Cancelled = true;
        ++state.CancelledCount;
    }
    if (result == UEC_RESULT_OK && state.CancelledCount != CancellationBatch) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    state.ExpectedCallbackCount = state.AcceptedCount - state.CancelledCount;
    if (result == UEC_RESULT_OK && state.CallbackCount != 0u) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }

    uec_runtime_stats observed{};
    observed.struct_size = sizeof(observed);
    if (result == UEC_RESULT_OK) {
        result = state.Api->get_runtime_stats(state.Context, &observed);
        if (result == UEC_RESULT_OK &&
            observed.pending_requests != state.Baseline.pending_requests +
                state.ExpectedCallbackCount) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result != UEC_RESULT_OK) {
        FinishQueueSmoke(state, result, true);
        return result;
    }
    return UEC_RESULT_OK;
}

extern "C" uec_bool UEC_CALL uec_host_queue_smoke_poll(uec_result* outResult)
{
    if (outResult == nullptr) return UEC_FALSE;
    FQueueSmokeState& state = GQueueSmokeState;
    if (!state.Complete && state.CallbackCount >= state.ExpectedCallbackCount) {
        bool callbacksMatchExpectedSubmissions = true;
        for (const FQueueSubmission& submission : state.Submissions) {
            if (submission.Result == UEC_RESULT_OK &&
                submission.Cancelled == submission.CallbackExecuted) {
                callbacksMatchExpectedSubmissions = false;
                break;
            }
        }
        const bool valid = state.CallbackCount == state.ExpectedCallbackCount &&
            callbacksMatchExpectedSubmissions && state.CallbackStatsValid &&
            state.AcceptedCount == static_cast<uint32>(QueueCapacity) &&
            state.RejectedCount == static_cast<uint32>(ExtraSubmissions) &&
            state.CancelledCount == CancellationBatch &&
            HasRuntimeStatsReturnedToBaseline(state);
        FinishQueueSmoke(state,
                         valid ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR,
                         !valid);
    }
    *outResult = state.Complete ? state.Result : UEC_RESULT_NOT_INITIALIZED;
    return state.Complete ? UEC_TRUE : UEC_FALSE;
}

extern "C" void UEC_CALL uec_host_queue_smoke_cancel(void)
{
    FinishQueueSmoke(GQueueSmokeState, UEC_RESULT_INTERNAL_ERROR, true);
}
