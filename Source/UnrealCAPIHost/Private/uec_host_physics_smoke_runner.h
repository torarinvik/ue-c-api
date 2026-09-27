#pragma once

#include "Containers/Ticker.h"
#include "CoreMinimal.h"
#include "uec_api.h"

class FUECPhysicsSmokeRunner
{
public:
    using FStart = uec_result (UEC_CALL *)(void);
    using FPoll = uec_bool (UEC_CALL *)(uec_result* outResult);
    using FCancel = void (UEC_CALL *)(void);
    using FOnComplete = TFunction<void(uec_result)>;

    uec_result Start(FStart start, FPoll poll, FCancel cancel, FOnComplete onComplete,
                     double firstPollDelay, double pollInterval)
    {
        if (TickerHandle.IsValid() || start == nullptr || poll == nullptr ||
            cancel == nullptr || !onComplete) return UEC_RESULT_INVALID_ARGUMENT;
        const uec_result result = start();
        if (result != UEC_RESULT_OK) return result;
        PollFunction = poll;
        CancelFunction = cancel;
        OnComplete = MoveTemp(onComplete);
        const double now = FPlatformTime::Seconds();
        Deadline = now + 10.0;
        NextPollTime = now + firstPollDelay;
        PollInterval = pollInterval;
        TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateRaw(this, &FUECPhysicsSmokeRunner::Poll), 0.05f);
        return UEC_RESULT_OK;
    }

    void Shutdown()
    {
        if (TickerHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
            if (CancelFunction != nullptr) CancelFunction();
            TickerHandle.Reset();
        }
        OnComplete = nullptr;
        PollFunction = nullptr;
        CancelFunction = nullptr;
    }

private:
    bool Poll(float)
    {
        const double now = FPlatformTime::Seconds();
        if (now < NextPollTime) return true;
        NextPollTime = now + PollInterval;
        uec_result result = UEC_RESULT_NOT_INITIALIZED;
        const bool complete = PollFunction(&result) == UEC_TRUE;
        if (!complete && now < Deadline) return true;
        if (!complete) {
            if (CancelFunction != nullptr) CancelFunction();
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        TickerHandle.Reset();
        PollFunction = nullptr;
        CancelFunction = nullptr;
        FOnComplete callback = MoveTemp(OnComplete);
        if (callback) callback(result);
        return false;
    }

    FTSTicker::FDelegateHandle TickerHandle;
    FOnComplete OnComplete;
    FPoll PollFunction = nullptr;
    FCancel CancelFunction = nullptr;
    double Deadline = 0.0;
    double NextPollTime = 0.0;
    double PollInterval = 0.1;
};
