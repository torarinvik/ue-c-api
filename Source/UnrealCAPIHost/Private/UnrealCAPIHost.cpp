#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"

#include "uec_api.h"

extern "C" uec_result UEC_CALL uec_host_smoke_bootstrap(void);
extern "C" uec_result UEC_CALL uec_host_event_bridge_smoke(void);
extern "C" uec_result UEC_CALL uec_host_latent_smoke_start(void);
extern "C" uec_bool UEC_CALL uec_host_latent_smoke_poll(uec_result* out_result);
extern "C" void UEC_CALL uec_host_latent_smoke_cancel(void);

DEFINE_LOG_CATEGORY_STATIC(LogUnrealCAPIHost, Log, All);

class FUnrealCAPIHostModule final : public FDefaultGameModuleImpl
{
    FTSTicker::FDelegateHandle EventBridgeSmokeHandle;
    FTSTicker::FDelegateHandle LatentSmokeHandle;
    float LatentSmokeElapsed = 0.0f;

    bool RunEventBridgeSmoke(float)
    {
        if (GEngine == nullptr) return true;
        bool hasRuntimeWorld = false;
        for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
        {
            if (worldContext.World() != nullptr &&
                (worldContext.WorldType == EWorldType::Game ||
                 worldContext.WorldType == EWorldType::PIE)) {
                hasRuntimeWorld = true;
                break;
            }
        }
        if (!hasRuntimeWorld) return true;

        const uec_result result = uec_host_event_bridge_smoke();
        if (result == UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Log, TEXT("C event bridge smoke completed"));
            const uec_result latentResult = uec_host_latent_smoke_start();
            if (latentResult == UEC_RESULT_OK) {
                LatentSmokeElapsed = 0.0f;
                LatentSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
                    FTickerDelegate::CreateRaw(this, &FUnrealCAPIHostModule::RunLatentSmoke),
                    0.1f);
            }
            else {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C latent invocation smoke failed to start with result %d"),
                    static_cast<int32>(latentResult));
            }
        }
        else {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C event bridge smoke failed with result %d"), static_cast<int32>(result));
        }
        EventBridgeSmokeHandle.Reset();
        return false;
    }

    bool RunLatentSmoke(float deltaSeconds)
    {
        LatentSmokeElapsed += deltaSeconds;
        uec_result result = UEC_RESULT_NOT_INITIALIZED;
        const bool complete = uec_host_latent_smoke_poll(&result) == UEC_TRUE;
        if (!complete && LatentSmokeElapsed < 10.0f) return true;
        if (!complete) {
            uec_host_latent_smoke_cancel();
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Log, TEXT("C latent invocation smoke completed"));
        }
        else {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C latent invocation smoke failed with result %d"),
                static_cast<int32>(result));
        }
        LatentSmokeHandle.Reset();
        return false;
    }

public:
    void StartupModule() override
    {
        const uec_result result = uec_host_smoke_bootstrap();
        if (result == UEC_RESULT_OK)
        {
            UE_LOG(LogUnrealCAPIHost, Log, TEXT("C consumer bootstrap completed"));
        }
        else
        {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C consumer bootstrap failed with result %d"), static_cast<int32>(result));
        }
        EventBridgeSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateRaw(this, &FUnrealCAPIHostModule::RunEventBridgeSmoke), 0.1f);
    }

    void ShutdownModule() override
    {
        if (EventBridgeSmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(EventBridgeSmokeHandle);
        }
        if (LatentSmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(LatentSmokeHandle);
            uec_host_latent_smoke_cancel();
        }
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FUnrealCAPIHostModule, UnrealCAPIHost, "UnrealCAPIHost");
