#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"

#include "uec_api.h"

extern "C" uec_result UEC_CALL uec_host_smoke_bootstrap(void);
extern "C" uec_result UEC_CALL uec_host_collision_smoke(void);
extern "C" uec_result UEC_CALL uec_host_event_bridge_smoke(void);
extern "C" uec_result UEC_CALL uec_host_latent_smoke_start(void);
extern "C" uec_bool UEC_CALL uec_host_latent_smoke_poll(uec_result* out_result);
extern "C" void UEC_CALL uec_host_latent_smoke_cancel(void);
extern "C" uec_result UEC_CALL uec_host_queue_smoke_start(void);
extern "C" uec_bool UEC_CALL uec_host_queue_smoke_poll(uec_result* out_result);
extern "C" void UEC_CALL uec_host_queue_smoke_cancel(void);
extern "C" uec_result UEC_CALL uec_host_async_save_smoke_start(void);
extern "C" uec_bool UEC_CALL uec_host_async_save_smoke_poll(uec_result* out_result);
extern "C" void UEC_CALL uec_host_async_save_smoke_cancel(void);
extern "C" uec_result UEC_CALL uec_host_object_load_smoke_start(void);
extern "C" uec_bool UEC_CALL uec_host_object_load_smoke_poll(uec_result* out_result);
extern "C" void UEC_CALL uec_host_object_load_smoke_cancel(void);
extern "C" uec_result UEC_CALL uec_host_gameplay_example_smoke_start(void);
extern "C" uec_bool UEC_CALL uec_host_gameplay_example_smoke_poll(uec_result* out_result);
extern "C" void UEC_CALL uec_host_gameplay_example_smoke_cancel(void);
extern "C" uec_result UEC_CALL uec_host_travel_smoke_start(void);
extern "C" uec_bool UEC_CALL uec_host_travel_smoke_poll(uec_result* out_result);
extern "C" void UEC_CALL uec_host_travel_smoke_cancel(void);

DEFINE_LOG_CATEGORY_STATIC(LogUnrealCAPIHost, Log, All);

class FUnrealCAPIHostModule final : public FDefaultGameModuleImpl
{
    FTSTicker::FDelegateHandle EventBridgeSmokeHandle;
    FTSTicker::FDelegateHandle LatentSmokeHandle;
    FTSTicker::FDelegateHandle QueueSmokeHandle;
    FTSTicker::FDelegateHandle AsyncSaveSmokeHandle;
    FTSTicker::FDelegateHandle ObjectLoadSmokeHandle;
    FTSTicker::FDelegateHandle GameplayExampleSmokeHandle;
    FTSTicker::FDelegateHandle TravelSmokeHandle;
    float LatentSmokeElapsed = 0.0f;
    float QueueSmokeElapsed = 0.0f;
    float AsyncSaveSmokeElapsed = 0.0f;
    float ObjectLoadSmokeElapsed = 0.0f;
    float GameplayExampleSmokeElapsed = 0.0f;
    float TravelSmokeElapsed = 0.0f;

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

        const uec_result collisionResult = uec_host_collision_smoke();
        if (collisionResult != UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C collision smoke failed with result %d"),
                static_cast<int32>(collisionResult));
            EventBridgeSmokeHandle.Reset();
            return false;
        }
        UE_LOG(LogUnrealCAPIHost, Log, TEXT("C collision smoke completed"));

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
            const uec_result queueResult = uec_host_queue_smoke_start();
            if (queueResult == UEC_RESULT_OK) {
                QueueSmokeElapsed = 0.0f;
                QueueSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
                    FTickerDelegate::CreateRaw(this, &FUnrealCAPIHostModule::RunQueueSmoke),
                    0.1f);
            }
            else {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C game-thread queue smoke failed to start with result %d"),
                    static_cast<int32>(queueResult));
            }
        }
        else {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C latent invocation smoke failed with result %d"),
                static_cast<int32>(result));
        }
        LatentSmokeHandle.Reset();
        return false;
    }

    bool RunQueueSmoke(float deltaSeconds)
    {
        QueueSmokeElapsed += deltaSeconds;
        uec_result result = UEC_RESULT_NOT_INITIALIZED;
        const bool complete = uec_host_queue_smoke_poll(&result) == UEC_TRUE;
        if (!complete && QueueSmokeElapsed < 30.0f) return true;
        if (!complete) {
            uec_host_queue_smoke_cancel();
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Log, TEXT("C game-thread queue smoke completed"));
            const uec_result saveResult = uec_host_async_save_smoke_start();
            if (saveResult == UEC_RESULT_OK) {
                AsyncSaveSmokeElapsed = 0.0f;
                AsyncSaveSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
                    FTickerDelegate::CreateRaw(this, &FUnrealCAPIHostModule::RunAsyncSaveSmoke),
                    0.1f);
            }
            else {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C async save smoke failed to start with result %d"),
                    static_cast<int32>(saveResult));
            }
        }
        else {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C game-thread queue smoke failed with result %d"),
                static_cast<int32>(result));
        }
        QueueSmokeHandle.Reset();
        return false;
    }

    bool RunAsyncSaveSmoke(float deltaSeconds)
    {
        AsyncSaveSmokeElapsed += deltaSeconds;
        uec_result result = UEC_RESULT_NOT_INITIALIZED;
        const bool complete = uec_host_async_save_smoke_poll(&result) == UEC_TRUE;
        if (!complete && AsyncSaveSmokeElapsed < 30.0f) return true;
        if (!complete) {
            uec_host_async_save_smoke_cancel();
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Log, TEXT("C async save smoke completed"));
            const uec_result loadResult = uec_host_object_load_smoke_start();
            if (loadResult == UEC_RESULT_OK) {
                ObjectLoadSmokeElapsed = 0.0f;
                ObjectLoadSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
                    FTickerDelegate::CreateRaw(this, &FUnrealCAPIHostModule::RunObjectLoadSmoke),
                    0.1f);
            }
            else {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C async object load smoke failed to start with result %d"),
                    static_cast<int32>(loadResult));
            }
        }
        else {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C async save smoke failed with result %d"), static_cast<int32>(result));
        }
        AsyncSaveSmokeHandle.Reset();
        return false;
    }

    bool RunObjectLoadSmoke(float deltaSeconds)
    {
        ObjectLoadSmokeElapsed += deltaSeconds;
        uec_result result = UEC_RESULT_NOT_INITIALIZED;
        const bool complete = uec_host_object_load_smoke_poll(&result) == UEC_TRUE;
        if (!complete && ObjectLoadSmokeElapsed < 10.0f) return true;
        if (!complete) {
            uec_host_object_load_smoke_cancel();
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Log, TEXT("C async object load smoke completed"));
            const uec_result exampleResult = uec_host_gameplay_example_smoke_start();
            if (exampleResult == UEC_RESULT_OK) {
                GameplayExampleSmokeElapsed = 0.0f;
                GameplayExampleSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
                    FTickerDelegate::CreateRaw(
                        this, &FUnrealCAPIHostModule::RunGameplayExampleSmoke),
                    0.1f);
            }
            else {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C gameplay example smoke failed to start with result %d"),
                    static_cast<int32>(exampleResult));
            }
        }
        else {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C async object load smoke failed with result %d"),
                static_cast<int32>(result));
        }
        ObjectLoadSmokeHandle.Reset();
        return false;
    }

    bool RunGameplayExampleSmoke(float deltaSeconds)
    {
        GameplayExampleSmokeElapsed += deltaSeconds;
        uec_result result = UEC_RESULT_NOT_INITIALIZED;
        const bool complete = uec_host_gameplay_example_smoke_poll(&result) == UEC_TRUE;
        if (!complete && GameplayExampleSmokeElapsed < 10.0f) return true;
        if (!complete) {
            uec_host_gameplay_example_smoke_cancel();
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Log, TEXT("C gameplay example smoke completed"));
            const uec_result travelResult = uec_host_travel_smoke_start();
            if (travelResult == UEC_RESULT_OK) {
                TravelSmokeElapsed = 0.0f;
                TravelSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
                    FTickerDelegate::CreateRaw(this, &FUnrealCAPIHostModule::RunTravelSmoke),
                    0.1f);
            }
            else {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C travel smoke failed to start with result %d"),
                    static_cast<int32>(travelResult));
            }
        }
        else {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C gameplay example smoke failed with result %d"),
                static_cast<int32>(result));
        }
        GameplayExampleSmokeHandle.Reset();
        return false;
    }

    bool RunTravelSmoke(float deltaSeconds)
    {
        TravelSmokeElapsed += deltaSeconds;
        uec_result result = UEC_RESULT_NOT_INITIALIZED;
        const bool complete = uec_host_travel_smoke_poll(&result) == UEC_TRUE;
        if (!complete && TravelSmokeElapsed < 30.0f) return true;
        if (!complete) {
            uec_host_travel_smoke_cancel();
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Log, TEXT("C travel smoke completed"));
        }
        else {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C travel smoke failed with result %d"), static_cast<int32>(result));
        }
        TravelSmokeHandle.Reset();
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
        if (QueueSmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(QueueSmokeHandle);
            uec_host_queue_smoke_cancel();
        }
        if (AsyncSaveSmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(AsyncSaveSmokeHandle);
            uec_host_async_save_smoke_cancel();
        }
        if (ObjectLoadSmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(ObjectLoadSmokeHandle);
            uec_host_object_load_smoke_cancel();
        }
        if (GameplayExampleSmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(GameplayExampleSmokeHandle);
            uec_host_gameplay_example_smoke_cancel();
        }
        if (TravelSmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(TravelSmokeHandle);
            uec_host_travel_smoke_cancel();
        }
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FUnrealCAPIHostModule, UnrealCAPIHost, "UnrealCAPIHost");
