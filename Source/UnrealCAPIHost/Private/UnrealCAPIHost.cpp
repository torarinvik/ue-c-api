#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"

#include "uec_api.h"

extern "C" uec_result UEC_CALL uec_host_smoke_bootstrap(void);
extern "C" uec_result UEC_CALL uec_host_event_bridge_smoke(void);

DEFINE_LOG_CATEGORY_STATIC(LogUnrealCAPIHost, Log, All);

class FUnrealCAPIHostModule final : public FDefaultGameModuleImpl
{
    FTSTicker::FDelegateHandle EventBridgeSmokeHandle;

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
        }
        else {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C event bridge smoke failed with result %d"), static_cast<int32>(result));
        }
        EventBridgeSmokeHandle.Reset();
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
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FUnrealCAPIHostModule, UnrealCAPIHost, "UnrealCAPIHost");
