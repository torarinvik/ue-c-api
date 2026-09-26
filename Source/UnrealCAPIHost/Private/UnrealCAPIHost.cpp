#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Modules/ModuleManager.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#include "UnrealEdGlobals.h"
#endif

#include "UECAPIHostCollisionSmokeActor.h"
#include "uec_host_smoke_exports.h"

DEFINE_LOG_CATEGORY_STATIC(LogUnrealCAPIHost, Log, All);

class FUnrealCAPIHostModule final : public FDefaultGameModuleImpl
{
    FTSTicker::FDelegateHandle EventBridgeSmokeHandle;
    FTSTicker::FDelegateHandle PhysicsSmokeHandle;
    FTSTicker::FDelegateHandle LatentSmokeHandle;
    FTSTicker::FDelegateHandle InputSmokeHandle;
    FTSTicker::FDelegateHandle QueueSmokeHandle;
    FTSTicker::FDelegateHandle AsyncSaveSmokeHandle;
    FTSTicker::FDelegateHandle ObjectLoadSmokeHandle;
    FTSTicker::FDelegateHandle GameplayExampleSmokeHandle;
    FTSTicker::FDelegateHandle TravelSmokeHandle;
    FTSTicker::FDelegateHandle PIEEndForRestartHandle;
    FTSTicker::FDelegateHandle ExitAfterPIESmokeHandle;
    FDelegateHandle ShutdownPendingPreExitHandle;
    float LatentSmokeElapsed = 0.0f;
    float InputSmokeElapsed = 0.0f;
    float QueueSmokeElapsed = 0.0f;
    float AsyncSaveSmokeElapsed = 0.0f;
    float ObjectLoadSmokeElapsed = 0.0f;
    float GameplayExampleSmokeElapsed = 0.0f;
    float TravelSmokeElapsed = 0.0f;
    double PhysicsSmokeDeadline = 0.0;
    double PhysicsSmokeNextPollTime = 0.0;
    double AuthoritySmokeDeadline = 0.0;
    bool bPIERestartStarted = false;
    bool bInputSmokeCompleted = false;
    int32 AnimationSmokeQuiescentTicks = 0;
    int32 PIERestartCyclesCompleted = 0;

    bool FinishTestRunAfterPIE(float)
    {
#if WITH_EDITOR
        if (GEditor != nullptr && GEditor->PlayWorld != nullptr) return true;
#endif
        ExitAfterPIESmokeHandle.Reset();
        FPlatformMisc::RequestExit(false);
        return false;
    }

    void RequestSmokeExit()
    {
#if WITH_EDITOR
        if (GEditor != nullptr && GEditor->PlayWorld != nullptr)
        {
            if (FParse::Param(FCommandLine::Get(), TEXT("uec-tests-pie-restart")) &&
                !bPIERestartStarted && !PIEEndForRestartHandle.IsValid())
            {
                const uec_result captureResult =
                    uec_host_pie_restart_smoke_capture(GEditor->PlayWorld);
                if (captureResult != UEC_RESULT_OK)
                {
                    UE_LOG(LogUnrealCAPIHost, Error,
                        TEXT("C PIE restart smoke failed to capture handles with result %d"),
                        static_cast<int32>(captureResult));
                    GEditor->RequestEndPlayMap();
                    ExitAfterPIESmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
                        FTickerDelegate::CreateRaw(
                            this, &FUnrealCAPIHostModule::FinishTestRunAfterPIE),
                        0.05f);
                    return;
                }
                PIEEndForRestartHandle = FTSTicker::GetCoreTicker().AddTicker(
                    FTickerDelegate::CreateRaw(
                        this, &FUnrealCAPIHostModule::EndPIEForRestart),
                    0.2f);
                return;
            }
            if (!ExitAfterPIESmokeHandle.IsValid())
            {
                GEditor->RequestEndPlayMap();
                ExitAfterPIESmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
                    FTickerDelegate::CreateRaw(this, &FUnrealCAPIHostModule::FinishTestRunAfterPIE),
                    0.05f);
            }
            return;
        }
#endif
        FPlatformMisc::RequestExit(false);
    }

#if WITH_EDITOR
    bool EndPIEForRestart(float)
    {
        PIEEndForRestartHandle.Reset();
        if (GEditor == nullptr || GEditor->PlayWorld == nullptr)
        {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C PIE restart smoke lost the first PlayWorld before shutdown"));
            uec_host_pie_restart_smoke_cancel();
            FPlatformMisc::RequestExit(false);
            return false;
        }
        GEditor->RequestEndPlayMap();
        ExitAfterPIESmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateRaw(this, &FUnrealCAPIHostModule::RestartPIEAfterCleanup),
            0.05f);
        return false;
    }

    bool RestartPIEAfterCleanup(float)
    {
        if (GEditor == nullptr)
        {
            ExitAfterPIESmokeHandle.Reset();
            uec_host_pie_restart_smoke_cancel();
            FPlatformMisc::RequestExit(false);
            return false;
        }
        if (GEditor->PlayWorld != nullptr) return true;

        ExitAfterPIESmokeHandle.Reset();
        bPIERestartStarted = true;
        FRequestPlaySessionParams parameters;
        GEditor->RequestPlaySession(parameters);
        EventBridgeSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateRaw(this, &FUnrealCAPIHostModule::RunEventBridgeSmoke),
            0.1f);
        return false;
    }
#endif

    void VerifyPendingWorkBeforeModuleShutdown()
    {
        uec_result result = uec_host_shutdown_pending_smoke_arm();
        if (result == UEC_RESULT_OK) {
            result = uec_host_shutdown_pending_smoke_verify();
        }
        if (result == UEC_RESULT_OK)
        {
            UE_LOG(LogUnrealCAPIHost, Log,
                TEXT("C shutdown pending-work smoke completed"));
        }
        else
        {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C shutdown pending-work smoke failed with result %d"),
                static_cast<int32>(result));
        }
    }

    void StartEventBridgeSmokeChain()
    {
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
    }

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

        if (FParse::Param(FCommandLine::Get(), TEXT("uec-tests-shutdown-pending")))
        {
            EventBridgeSmokeHandle.Reset();
            const uec_result prepareResult = uec_host_shutdown_pending_smoke_prepare();
            if (prepareResult != UEC_RESULT_OK)
            {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C shutdown pending-work smoke failed with result %d"),
                    static_cast<int32>(prepareResult));
                FPlatformMisc::RequestExit(false);
                return false;
            }
            ShutdownPendingPreExitHandle = FCoreDelegates::OnEnginePreExit.AddRaw(
                this, &FUnrealCAPIHostModule::VerifyPendingWorkBeforeModuleShutdown);
            UE_LOG(LogUnrealCAPIHost, Log,
                TEXT("C shutdown pending-work smoke scheduled"));
            FPlatformMisc::RequestExit(false);
            return false;
        }

        if (FParse::Param(FCommandLine::Get(), TEXT("uec-tests-multi-pie")))
        {
            const uec_result multiPIEResult = uec_host_multi_pie_smoke();
            if (multiPIEResult == UEC_RESULT_NOT_INITIALIZED) return true;
            if (multiPIEResult == UEC_RESULT_OK)
            {
                UE_LOG(LogUnrealCAPIHost, Log,
                    TEXT("C multi-PIE context smoke completed"));
            }
            else
            {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C multi-PIE context smoke failed with result %d"),
                    static_cast<int32>(multiPIEResult));
            }
            EventBridgeSmokeHandle.Reset();
            RequestSmokeExit();
            return false;
        }

        if (bPIERestartStarted)
        {
            const uec_result restartResult = uec_host_pie_restart_smoke_verify();
            if (restartResult != UEC_RESULT_OK)
            {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C PIE restart smoke failed with result %d"),
                    static_cast<int32>(restartResult));
                EventBridgeSmokeHandle.Reset();
                RequestSmokeExit();
                return false;
            }
            ++PIERestartCyclesCompleted;
            if (PIERestartCyclesCompleted < 3)
            {
                bPIERestartStarted = false;
                EventBridgeSmokeHandle.Reset();
                RequestSmokeExit();
                return false;
            }
            UE_LOG(LogUnrealCAPIHost, Log,
                TEXT("C PIE restart smoke completed after %d cleanup cycles"),
                PIERestartCyclesCompleted);
            EventBridgeSmokeHandle.Reset();
            RequestSmokeExit();
            return false;
        }

        if (FParse::Param(FCommandLine::Get(), TEXT("uec-tests-authority"))) {
            const double now = FPlatformTime::Seconds();
            if (AuthoritySmokeDeadline == 0.0) AuthoritySmokeDeadline = now + 60.0;
            const bool requireListenServer = FParse::Param(
                FCommandLine::Get(), TEXT("uec-tests-listen-server"));
            UWorld* clientWorld = nullptr;
            UWorld* listenServerWorld = nullptr;
            for (const FWorldContext& worldContext : GEngine->GetWorldContexts()) {
                UWorld* world = worldContext.World();
                if (world == nullptr || worldContext.WorldType != EWorldType::PIE) continue;
                if (world->GetNetMode() == NM_Client) clientWorld = world;
                else if (world->GetNetMode() == NM_ListenServer) listenServerWorld = world;
                if (clientWorld != nullptr &&
                    (!requireListenServer || listenServerWorld != nullptr)) break;
            }
            if ((clientWorld == nullptr || (requireListenServer && listenServerWorld == nullptr)) &&
                now < AuthoritySmokeDeadline) return true;

            uec_result result = UEC_RESULT_OK;
            if (clientWorld == nullptr ||
                (requireListenServer && listenServerWorld == nullptr)) {
                result = UEC_RESULT_NOT_INITIALIZED;
            }
            else {
                AUECAPIHostCollisionSmokeActor* actor =
                    clientWorld->SpawnActor<AUECAPIHostCollisionSmokeActor>(
                        FVector(18000.0, -24000.0, 50000.0), FRotator::ZeroRotator);
                UPrimitiveComponent* root = actor == nullptr ? nullptr :
                    Cast<UPrimitiveComponent>(actor->GetRootComponent());
                if (root == nullptr) {
                    result = UEC_RESULT_INTERNAL_ERROR;
                }
                else {
                    root->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                    root->SetSimulatePhysics(true);
                    if (!root->IsSimulatingPhysics()) result = UEC_RESULT_UNSUPPORTED;
                    else result = uec_host_authority_smoke();
                }
            }
            if (result == UEC_RESULT_OK && requireListenServer) {
                result = uec_host_listen_server_authority_smoke();
            }
            if (result == UEC_RESULT_OK) {
                if (requireListenServer) {
                    UE_LOG(LogUnrealCAPIHost, Log,
                        TEXT("C listen-server authority smoke completed"));
                }
                else {
                    UE_LOG(LogUnrealCAPIHost, Log,
                        TEXT("C client authority smoke completed"));
                }
            }
            else {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C client authority smoke failed with result %d"),
                    static_cast<int32>(result));
            }
            EventBridgeSmokeHandle.Reset();
            if (FParse::Param(FCommandLine::Get(), TEXT("uec-tests-exit"))) {
                RequestSmokeExit();
            }
            return false;
        }

        const uec_result reflectionScalarsResult =
            uec_host_reflection_scalars_smoke();
        if (reflectionScalarsResult != UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C reflected scalar property smoke failed with result %d"),
                static_cast<int32>(reflectionScalarsResult));
            EventBridgeSmokeHandle.Reset();
            return false;
        }
        UE_LOG(LogUnrealCAPIHost, Log,
            TEXT("C reflected scalar property smoke completed"));

        const uec_result reflectionContainersResult =
            uec_host_reflection_containers_smoke();
        if (reflectionContainersResult != UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C reflected container smoke failed with result %d"),
                static_cast<int32>(reflectionContainersResult));
            EventBridgeSmokeHandle.Reset();
            return false;
        }
        UE_LOG(LogUnrealCAPIHost, Log, TEXT("C reflected container smoke completed"));

        const uec_result playerFlowResult = uec_host_player_flow_smoke();
        if (playerFlowResult != UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C player-flow and camera smoke failed with result %d"),
                static_cast<int32>(playerFlowResult));
            EventBridgeSmokeHandle.Reset();
            return false;
        }
        UE_LOG(LogUnrealCAPIHost, Log, TEXT("C player-flow and camera smoke completed"));
        const uec_result animationSmokeResult = uec_host_animation_smoke_start();
        if (animationSmokeResult != UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C skeletal animation smoke failed to start with result %d"),
                static_cast<int32>(animationSmokeResult));
        }

        const uec_result blueprintInvocationResult =
            uec_host_blueprint_invocation_smoke();
        if (blueprintInvocationResult != UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C Blueprint invocation smoke failed with result %d"),
                static_cast<int32>(blueprintInvocationResult));
            EventBridgeSmokeHandle.Reset();
            return false;
        }
        UE_LOG(LogUnrealCAPIHost, Log, TEXT("C Blueprint invocation smoke completed"));

        const uec_result collisionResult = uec_host_collision_smoke();
        if (collisionResult != UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C collision smoke failed with result %d"),
                static_cast<int32>(collisionResult));
            EventBridgeSmokeHandle.Reset();
            return false;
        }
        UE_LOG(LogUnrealCAPIHost, Log, TEXT("C collision smoke completed"));

        const uec_result gcResult = uec_host_gc_smoke();
        if (gcResult != UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C GC lifetime smoke failed with result %d"),
                static_cast<int32>(gcResult));
            EventBridgeSmokeHandle.Reset();
            return false;
        }
        UE_LOG(LogUnrealCAPIHost, Log, TEXT("C GC lifetime smoke completed"));

        const uec_result physicsResult = uec_host_physics_smoke_start();
        if (physicsResult == UEC_RESULT_OK) {
            PhysicsSmokeNextPollTime = FPlatformTime::Seconds() + 0.5;
            PhysicsSmokeDeadline = FPlatformTime::Seconds() + 10.0;
            PhysicsSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateRaw(this, &FUnrealCAPIHostModule::RunPhysicsSmoke),
                0.1f);
        }
        else {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C physics smoke failed to start with result %d"),
                static_cast<int32>(physicsResult));
        }
        EventBridgeSmokeHandle.Reset();
        return false;
    }

    bool RunPhysicsSmoke(float)
    {
        const double now = FPlatformTime::Seconds();
        if (now < PhysicsSmokeNextPollTime) return true;
        PhysicsSmokeNextPollTime = now + 0.5;
        uec_result result = UEC_RESULT_NOT_INITIALIZED;
        const bool complete = uec_host_physics_smoke_poll(&result) == UEC_TRUE;
        if (!complete && now < PhysicsSmokeDeadline) return true;
        if (!complete) {
            uec_host_physics_smoke_cancel();
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            UE_LOG(LogUnrealCAPIHost, Log, TEXT("C physics smoke completed"));
            StartEventBridgeSmokeChain();
        }
        else {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C physics smoke failed with result %d"), static_cast<int32>(result));
        }
        PhysicsSmokeHandle.Reset();
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
            const uec_result inputResult = uec_host_input_smoke_start();
            if (inputResult == UEC_RESULT_OK) {
                InputSmokeElapsed = 0.0f;
                InputSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
                    FTickerDelegate::CreateRaw(this, &FUnrealCAPIHostModule::RunInputSmoke),
                    0.0f);
            }
            else {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C Enhanced Input smoke failed to start with result %d"),
                    static_cast<int32>(inputResult));
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

    bool RunInputSmoke(float deltaSeconds)
    {
        if (!bInputSmokeCompleted) {
            InputSmokeElapsed += deltaSeconds;
            uec_result result = UEC_RESULT_NOT_INITIALIZED;
            const bool complete = uec_host_input_smoke_poll(&result) == UEC_TRUE;
            if (!complete && InputSmokeElapsed < 10.0f) return true;
            if (!complete) {
                uec_host_input_smoke_cancel();
                result = UEC_RESULT_INTERNAL_ERROR;
            }
            if (result != UEC_RESULT_OK) {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C Enhanced Input smoke failed with result %d"),
                    static_cast<int32>(result));
                InputSmokeHandle.Reset();
                return false;
            }
            UE_LOG(LogUnrealCAPIHost, Log, TEXT("C Enhanced Input smoke completed"));
            bInputSmokeCompleted = true;
        }
        if (uec_host_animation_smoke_is_running() == UEC_TRUE) {
            AnimationSmokeQuiescentTicks = 0;
            return true;
        }
        if (AnimationSmokeQuiescentTicks++ == 0) return true;
        AnimationSmokeQuiescentTicks = 0;
        bInputSmokeCompleted = false;
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
        InputSmokeHandle.Reset();
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
            UE_LOG(LogUnrealCAPIHost, Log,
                TEXT("C travel smoke completed (named-widget helper included)"));
            if (FParse::Param(FCommandLine::Get(), TEXT("uec-tests-exit"))) {
                RequestSmokeExit();
            }
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
        const bool bSupportsDynamicReloading =
            FModuleManager::Get().LoadModuleChecked<IModuleInterface>(TEXT("UnrealCAPI"))
                .SupportsDynamicReloading();
        if (bSupportsDynamicReloading)
        {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C runtime module dynamic-unload policy smoke failed: unloading is enabled"));
        }
        else
        {
            UE_LOG(LogUnrealCAPIHost, Log,
                TEXT("C runtime module dynamic-unload policy smoke completed"));
        }

        const uec_result result = uec_host_smoke_bootstrap();
        if (result == UEC_RESULT_OK)
        {
            UE_LOG(LogUnrealCAPIHost, Log, TEXT("C consumer bootstrap completed"));
            const uec_result metadataResult = uec_host_reflection_metadata_smoke();
            if (metadataResult == UEC_RESULT_OK)
            {
                UE_LOG(LogUnrealCAPIHost, Log,
                    TEXT("C cooked reflection metadata smoke completed"));
            }
            else
            {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C cooked reflection metadata smoke failed with result %d"),
                    static_cast<int32>(metadataResult));
            }

            const uec_result persistenceResult = uec_host_persistence_smoke();
            if (persistenceResult == UEC_RESULT_OK)
            {
                UE_LOG(LogUnrealCAPIHost, Log,
                    TEXT("C persistence and configuration smoke completed"));
            }
            else
            {
                UE_LOG(LogUnrealCAPIHost, Error,
                    TEXT("C persistence and configuration smoke failed with result %d"),
                    static_cast<int32>(persistenceResult));
            }
        }
        else
        {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C consumer bootstrap failed with result %d"), static_cast<int32>(result));
        }
        if (FParse::Param(FCommandLine::Get(), TEXT("uec-tests-dedicated-server")))
        {
            uec_host_dedicated_server_context_smoke_start();
            return;
        }
        EventBridgeSmokeHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateRaw(this, &FUnrealCAPIHostModule::RunEventBridgeSmoke), 0.1f);
    }

    void ShutdownModule() override
    {
        if (ShutdownPendingPreExitHandle.IsValid()) {
            FCoreDelegates::OnEnginePreExit.Remove(ShutdownPendingPreExitHandle);
        }
        if (EventBridgeSmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(EventBridgeSmokeHandle);
        }
        if (PhysicsSmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(PhysicsSmokeHandle);
            uec_host_physics_smoke_cancel();
        }
        if (LatentSmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(LatentSmokeHandle);
            uec_host_latent_smoke_cancel();
        }
        if (InputSmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(InputSmokeHandle);
            uec_host_input_smoke_cancel();
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
        uec_host_animation_smoke_cancel();
        if (TravelSmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(TravelSmokeHandle);
            uec_host_travel_smoke_cancel();
        }
        if (PIEEndForRestartHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(PIEEndForRestartHandle);
        }
        if (ExitAfterPIESmokeHandle.IsValid()) {
            FTSTicker::GetCoreTicker().RemoveTicker(ExitAfterPIESmokeHandle);
        }
        uec_host_pie_restart_smoke_cancel();
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FUnrealCAPIHostModule, UnrealCAPIHost, "UnrealCAPIHost");
