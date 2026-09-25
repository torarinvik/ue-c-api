#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformMisc.h"
#include "uec_api.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#include "UnrealEdGlobals.h"
#endif

extern "C" uec_result UEC_CALL uec_host_dedicated_server_context_smoke(void);

namespace
{
    FTSTicker::FDelegateHandle DedicatedServerSmokeHandle;
    double DedicatedServerSmokeDeadline = 0.0;

    bool FinishDedicatedServerSmokeAfterPIE(float)
    {
#if WITH_EDITOR
        if (GEditor != nullptr && GEditor->PlayWorld != nullptr) return true;
#endif
        DedicatedServerSmokeHandle.Reset();
        FPlatformMisc::RequestExit(false);
        return false;
    }

    void RequestDedicatedServerSmokeExit()
    {
#if WITH_EDITOR
        if (GEditor != nullptr && GEditor->PlayWorld != nullptr)
        {
            GEditor->RequestEndPlayMap();
            DedicatedServerSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateStatic(&FinishDedicatedServerSmokeAfterPIE), 0.05f);
            return;
        }
#endif
        FPlatformMisc::RequestExit(false);
    }

    bool PollDedicatedServerContext(float)
    {
        const uec_result result = uec_host_dedicated_server_context_smoke();
        if (result == UEC_RESULT_NOT_INITIALIZED &&
            FPlatformTime::Seconds() < DedicatedServerSmokeDeadline) {
            return true;
        }

        if (result == UEC_RESULT_OK) {
            UE_LOG(LogTemp, Log,
                TEXT("C dedicated-server runtime context smoke completed"));
        } else {
            UE_LOG(LogTemp, Error,
                TEXT("C dedicated-server runtime context smoke failed with result %d"),
                static_cast<int32>(result));
        }
        RequestDedicatedServerSmokeExit();
        return false;
    }
}

extern "C" void UEC_CALL uec_host_dedicated_server_context_smoke_start(void)
{
    DedicatedServerSmokeDeadline = FPlatformTime::Seconds() + 60.0;
    DedicatedServerSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateStatic(&PollDedicatedServerContext), 0.1f);
}
