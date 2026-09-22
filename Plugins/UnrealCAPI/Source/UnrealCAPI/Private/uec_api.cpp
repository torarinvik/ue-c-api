#include "uec_api.h"
#include "CoreMinimal.h"
#include "Math/NumericLimits.h"
#include "Async/Async.h"
#include "Containers/Ticker.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Engine/LevelStreaming.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SaveGame.h"
#include "InputCoreTypes.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionShape.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Animation/AnimationAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Camera/CameraComponent.h"
#include "Sound/SoundBase.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "HAL/CriticalSection.h"
#include "Modules/ModuleManager.h"
#include "Misc/ScopeLock.h"
#include "Misc/ConfigCacheIni.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/StrongObjectPtrTemplates.h"
#include "UObject/StructOnScope.h"
namespace
{
    constexpr char kModuleName[] = "UnrealCAPI";
    enum class EUECHandleKind : uint8
    {
        Context,
        World,
        Actor,
        SceneComponent,
        Class,
        Object
    };
    struct FUECHandleHeader final
    {
        EUECHandleKind Kind = EUECHandleKind::Context;
        uint64 Generation = 0;
        bool bReleased = false;
    };
    static uint64 AllocateHandleGeneration();
    static bool InitializeHandle(FUECHandleHeader& header, EUECHandleKind kind);
    struct FUECContext final
    {
        FUECHandleHeader Header;
    };
    struct FUECWorld final
    {
        FUECHandleHeader Header;
        TWeakObjectPtr<UWorld> Value;
        uec_world_kind Kind = UEC_WORLD_KIND_UNKNOWN;
        int32 PIEInstance = -1;
    };
    struct FUECActor final
    {
        FUECHandleHeader Header;
        TWeakObjectPtr<AActor> Value;
    };
    struct FUECSceneComponent final
    {
        FUECHandleHeader Header;
        TWeakObjectPtr<USceneComponent> Value;
    };
    struct FUECTimerState final
    {
        uint64 Id = 0;
        TWeakObjectPtr<UWorld> World;
        FTimerHandle Handle;
        uec_timer_callback Callback = nullptr;
        void* UserData = nullptr;
        bool Looping = false;
        bool Cancelled = false;
    };
    struct FUECTickSubscription final
    {
        uint64 Id = 0;
        TWeakObjectPtr<UWorld> World;
        uec_tick_callback Callback = nullptr;
        void* UserData = nullptr;
        FTSTicker::FDelegateHandle Handle;
        bool Cancelled = false;
        bool InCallback = false;
    };
    struct FUECAudioSubscription final
    {
        uint64 Id = 0;
        TWeakObjectPtr<UAudioComponent> Component;
        FDelegateHandle Handle;
        uec_audio_finished_callback Callback = nullptr;
        void* UserData = nullptr;
        bool Cancelled = false;
        bool InCallback = false;
    };
    struct FUECWidgetSubscription final
    {
        uint64 Id = 0;
        TWeakObjectPtr<UButton> Button;
        FDelegateHandle Handle;
        uec_widget_event_callback Callback = nullptr;
        void* UserData = nullptr;
        bool Cancelled = false;
        bool InCallback = false;
    };
    struct FUECAnimationSubscription final
    {
        uint64 Id = 0;
        TWeakObjectPtr<USkeletalMeshComponent> Component;
        FTSTicker::FDelegateHandle Handle;
        uec_animation_finished_callback Callback = nullptr;
        void* UserData = nullptr;
        bool Cancelled = false;
        bool InCallback = false;
        bool WasPlaying = false;
    };
    struct FUECCollisionSubscription final
    {
        uint64 Id = 0;
        TWeakObjectPtr<UPrimitiveComponent> Component;
        FDelegateHandle Handle;
        uec_component_hit_callback Callback = nullptr;
        void* UserData = nullptr;
        bool Cancelled = false;
        bool InCallback = false;
    };
    struct FUECClass final
    {
        FUECHandleHeader Header;
        TWeakObjectPtr<UClass> Value;
    };
    struct FUECObject final
    {
        FUECHandleHeader Header;
        TWeakObjectPtr<UObject> Value;
        TStrongObjectPtr<UObject> StrongValue;
    };
    struct FUECObjectLoadRequest final
    {
        uint64 Id = 0;
        FSoftObjectPath Path;
        TSharedPtr<FStreamableHandle> Handle;
        uec_object_load_callback Callback = nullptr;
        void* UserData = nullptr;
        bool Cancelled = false;
    };
    struct FUECGameThreadRequest final
    {
        uint64 Id = 0;
        uec_game_thread_callback Callback = nullptr;
        void* UserData = nullptr;
        bool Cancelled = false;
    };
    struct FUECTravelRequest final
    {
        uint64 Id = 0;
        TWeakObjectPtr<UWorld> PreviousWorld;
        FString LevelPath;
        uec_travel_callback Callback = nullptr;
        void* UserData = nullptr;
        bool Cancelled = false;
    };
    struct FUECSaveGameRequest final
    {
        uint64 Id = 0;
        uec_save_game_callback Callback = nullptr;
        void* UserData = nullptr;
        bool Cancelled = false;
    };
    struct FUECInputBinding final
    {
        uint64 Id = 0;
        TWeakObjectPtr<UEnhancedInputComponent> Component;
        uint32 EngineHandle = 0;
        uec_input_action_callback Callback = nullptr;
        void* UserData = nullptr;
        bool Cancelled = false;
        bool InCallback = false;
    };
    FCriticalSection GHandleMutex;
    bool GShuttingDown = false;
    uint64 GNextHandleGeneration = 1;
    TSet<const FUECContext*> GContexts;
    TSet<const FUECWorld*> GWorlds;
    TSet<const FUECActor*> GActors;
    TSet<const FUECSceneComponent*> GComponents;
    TMap<uint64, TSharedPtr<FUECTimerState>> GTimers;
    TMap<uint64, TSharedPtr<FUECTickSubscription>> GTickSubscriptions;
    TMap<uint64, TSharedPtr<FUECAudioSubscription>> GAudioSubscriptions;
    TMap<uint64, TSharedPtr<FUECWidgetSubscription>> GWidgetSubscriptions;
    TMap<uint64, TSharedPtr<FUECAnimationSubscription>> GAnimationSubscriptions;
    TMap<uint64, TSharedPtr<FUECCollisionSubscription>> GCollisionSubscriptions;
    TSet<const FUECClass*> GClasses;
    TSet<const FUECObject*> GObjects;
    TMap<uint64, TSharedPtr<FUECObjectLoadRequest>> GObjectLoadRequests;
    TMap<uint64, TSharedPtr<FUECGameThreadRequest>> GGameThreadRequests;
    TMap<uint64, TSharedPtr<FUECTravelRequest>> GTravelRequests;
    TMap<uint64, TSharedPtr<FUECSaveGameRequest>> GSaveGameRequests;
    TMap<uint64, TSharedPtr<FUECInputBinding>> GInputBindings;
    int32 GActiveCallbacks = 0;
    uint64 GNextObjectLoadRequestId = 1;
    uint64 GNextGameThreadRequestId = 1;
    uint64 GNextTravelRequestId = 1;
    uint64 GNextTimerId = 1;
    uint64 GNextTickSubscriptionId = 1;
    uint64 GNextAudioSubscriptionId = 1;
    uint64 GNextWidgetSubscriptionId = 1;
    uint64 GNextAnimationSubscriptionId = 1;
    uint64 GNextCollisionSubscriptionId = 1;
    uint64 GNextSaveGameRequestId = 1;
    uint64 GNextInputBindingId = 1;
    #include "API/uec_api_runtime.inl"
    #include "API/uec_api_world_actor.inl"
    #include "API/uec_api_actor_component.inl"
    #include "API/uec_api_reflection.inl"
    #include "API/uec_api_reflection_containers.inl"
    #include "API/uec_api_reflection_map_set.inl"
    #include "API/uec_api_reflection_invoke.inl"
    #include "API/uec_api_presentation.inl"
    #include "API/uec_api_gameplay.inl"
    #include "API/uec_api_input.inl"
    #include "API/uec_api_async.inl"
    const uec_api GApi = { sizeof(uec_api), UEC_ABI_MAJOR, UEC_ABI_MINOR,
        &GetCapabilities, &GetLastError, &Log, &ReleaseContext,
        &GetWorldCount, &GetWorldAt, &GetWorldKind, &GetWorldName, &TravelWorld,
        &GetFirstPlayerController, &GetControllerPawn, &PossessPawn,
        &SetControllerViewTarget, &GetInputKeyDown, &GetInputKeyValue,
        &GetActorVelocity, &SetActorPhysicsVelocity, &ApplyActorImpulse,
        &ApplyActorForce, &GetDefaultWorld,
        &ReleaseWorld, &SpawnActor, &ReleaseActor, &DestroyActor,
        &GetActorTransform, &SetActorTransform, &GetActorName, &ActorHasTag,
        &GetActorRootComponent, &GetActorComponentCount, &GetActorComponentAt,
        &ReleaseSceneComponent, &GetComponentTransform,
        &SetComponentTransform, &SetComponentVisible, &SetComponentActive,
        &SetTimer, &ClearTimer, &FindClass, &ReleaseClass, &GetClassName,
        &ClassIsA, &GetClassPropertyCount, &GetClassPropertyAt,
        &GetActorPropertyValue, &GetActorPropertyString,
        &SetActorPropertyValue, &SetActorPropertyString, &LineTrace,
        &InvokeActorFunction, &LoadObjectHandle, &ReleaseObject,
        &GetObjectName, &ObjectIsA, &RequestObjectLoad, &CancelObjectLoad,
        &SweepTrace, &OverlapShape, &PlaySoundAtLocation,
        &CreateWidget, &AddWidgetToViewport, &RemoveWidgetFromParent,
        &GetCameraFieldOfView, &SetCameraFieldOfView,
        &GetObjectPropertyValue, &GetObjectPropertyString,
        &SetObjectPropertyValue, &SetObjectPropertyString,
        &CreateSaveGame, &SaveGameToSlot, &LoadGameFromSlot, &DeleteGameSlot,
        &RunOnGameThread, &CancelGameThreadRequest,
        &AddPawnMovementInput, &JumpCharacter, &StopCharacterJumping,
        &SetStaticMesh, &SetSkeletalMesh,
        &PlaySkeletalAnimation, &StopSkeletalAnimation,
        &SetComponentMaterialScalar, &SetComponentMaterialVector,
        &RetainObject, &GetComponentClassName, &ComponentIsA,
        &AttachSceneComponent, &DetachSceneComponent,
        &GetActorClassName, &ActorIsA,
        &AddInputMappingContext, &RemoveInputMappingContext,
        &GetClassFunctionCount, &GetClassFunctionAt,
        &SetComponentCollisionEnabled, &SetComponentCollisionResponse,
        &IsObjectPathLoaded, &SpawnSoundAttached, &StopAudioComponent,
        &GetInputActionValue, &LineTraceFiltered, &InjectInputActionValue,
        &GetActorPropertyObject, &SetActorPropertyObject,
        &GetObjectPropertyObject, &SetObjectPropertyObject,
        &AsyncSaveGameToSlot, &AsyncLoadGameFromSlot, &CancelSaveGameRequest,
        &GetActorCountByClass, &GetActorAtByClass, &DestroyAudioComponent,
        &BindInputAction, &UnbindInputAction,
        &GetPlayerController, &GetWorldGameInstance,
        &InvokeActorFunctionText,
        &SubscribeWorldTick, &UnsubscribeWorldTick,
        &BindAudioFinished, &UnbindAudioFinished,
        &GetObjectPath, &GetObjectClassName,
        &SetWidgetVisibility, &SetTextBlockText,
        &BindButtonClicked, &UnbindButtonClicked,
        &GetComponentVelocity,
        &GetWorldPIEInstance,
        &GetWorldNetMode,
        &GetActorTagCount,
        &GetActorTagAt,
        &GetActorBounds,
        &FindPlayerStart,
        &BindAnimationFinished,
        &UnbindAnimationFinished,
        &GetWorldHasAuthority,
        &GetWorldGameMode,
        &GetWorldGameState,
        &GetActorComponentCountByClass,
        &GetActorComponentAtByClass,
        &GetConfigString,
        &SetConfigString,
        &GetStreamingLevelCount,
        &GetStreamingLevelAt,
        &SetStreamingLevelState,
        &IsClassPathLoaded,
        &BindComponentHit,
        &UnbindComponentHit,
        &SweepTraceFiltered,
        &OverlapShapeFiltered,
        &SetActorTag,
        &GetRuntimeStats,
        &GetWorldCountByKind, &GetWorldAtByKind,
        &InvokeActorFunctionValue,
        &InvokeActorFunctionValues, &GetClassFunctionParameterAt,
        &InvokeActorFunctionTextValues, &FindObjectHandle,
        &TravelWorldAsync, &CancelTravelRequest, &GetComponentVisible, &GetComponentActive, &GetClassFunctionFlags, &GetWidgetVisibility, &GetTextBlockText, &GetComponentCollisionEnabled, &GetAudioComponentPlaying, &SetStreamingLevelStateAsync, &CancelStreamingLevelRequest, &GetComponentCollisionResponse, &GetConfigInteger, &SetConfigInteger, &BindActorDestroyed, &UnbindActorDestroyed, &GetConfigBool, &GetActorPropertyArrayCount, &GetActorPropertyArrayElementText, &GetObjectPropertyArrayCount, &GetObjectPropertyArrayElementText, &GetObjectPropertyMapCount, &GetObjectPropertyMapEntryText, &GetObjectPropertySetCount, &GetObjectPropertySetElementText, &GetActorPropertySoftPath, &GetObjectPropertySoftPath, &GetActorPropertyMapCount, &GetActorPropertyMapEntryText, &GetActorPropertySetCount, &GetActorPropertySetElementText, &SetActorPropertySoftPath, &SetObjectPropertySoftPath, &GetActorPropertyStructFieldText, &GetObjectPropertyStructFieldText, &SetActorPropertyStructFieldText, &SetObjectPropertyStructFieldText, &SetActorPropertyArrayElementText, &SetObjectPropertyArrayElementText, &SetActorPropertyMapValueText, &SetObjectPropertyMapValueText, &GetClassPropertyFlags, &GetActorPropertyArrayElementValue, &GetObjectPropertyArrayElementValue, &GetActorPropertyMapValue, &GetObjectPropertyMapValue, &GetActorPropertySetElementValue, &GetObjectPropertySetElementValue, &GetActorPropertyStructFieldValue, &GetObjectPropertyStructFieldValue, &SetActorPropertyArrayElementValue, &SetObjectPropertyArrayElementValue, &SetActorPropertyMapValue, &SetObjectPropertyMapValue, &SetActorPropertyStructFieldValue, &SetObjectPropertyStructFieldValue, &GetClassPropertyDefaultText, &GetClassPropertyReferenceClassPath, &GetClassPropertyEnumValueCount, &GetClassPropertyEnumValueAt, &GetClassPropertyStructFieldCount, &GetClassPropertyStructFieldAt, &GetActorPropertyClass, &SetActorPropertyClass, &GetObjectPropertyClass, &SetObjectPropertyClass, &GetClassPropertyStructPath, &GetClassPropertyContainerKinds, &SetActorPropertySetElementText, &SetObjectPropertySetElementText, &SetActorPropertySetElementValue, &SetObjectPropertySetElementValue
    };
}
class FUnrealCAPIModule final : public IModuleInterface
{ FDelegateHandle WorldCleanupHandle; FDelegateHandle PostLoadMapHandle;
public:
    void StartupModule() override
    {
        {
            FScopeLock lock(&GHandleMutex);
            GShuttingDown = false;
        }
        WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddStatic(&HandleWorldCleanup);
        PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddStatic(&HandlePostLoadMap);
        UE_LOG(LogTemp, Log, TEXT("%s runtime module started (ABI %u.%u)"),
            UTF8_TO_TCHAR(kModuleName), UEC_ABI_MAJOR, UEC_ABI_MINOR);
    }
    void ShutdownModule() override
    {
        FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
        FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
        {
            FScopeLock lock(&GHandleMutex);
            GShuttingDown = true;
        }
        LogOutstandingResources();
        ClearAllTimers();
        ClearAllTickSubscriptions();
        ClearAllAudioSubscriptions();
        ClearAllWidgetSubscriptions();
        ClearAllAnimationSubscriptions();
        ClearAllCollisionSubscriptions();
        CancelAllObjectLoads();
        CancelAllGameThreadRequests();
        CancelAllTravelRequests(); CancelAllStreamingRequests();
        CancelAllSaveGameRequests();
        CancelAllInputBindings();
        RemoveAllActorDestroyedHandlers(); ClearAllActorDestroyedSubscriptions();
        ClearAllHandles();
        UE_LOG(LogTemp, Log, TEXT("%s runtime module stopped"), UTF8_TO_TCHAR(kModuleName));
    }
};
IMPLEMENT_MODULE(FUnrealCAPIModule, UnrealCAPI)
UEC_API uec_result UEC_CALL uec_get_api(uint32_t requestedMajor,
                                        uint32_t requestedMinor,
                                        const uec_api** outApi,
                                        uec_context** outContext)
{
    if (outApi == nullptr || outContext == nullptr)
    {
        SetLastErrorMessage(TEXT("API and context outputs are required"));
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    *outApi = nullptr;
    *outContext = nullptr;
    if (requestedMajor != UEC_ABI_MAJOR || requestedMinor > UEC_ABI_MINOR)
    {
        SetLastErrorMessage(TEXT("Requested ABI version is unsupported"));
        return UEC_RESULT_UNSUPPORTED;
    }
    auto* context = new FUECContext();
    if (!InitializeHandle(context->Header, EUECHandleKind::Context))
    {
        SetLastErrorMessage(TEXT("Handle generation allocation failed"));
        delete context;
        return UEC_RESULT_INTERNAL_ERROR;
    }
    {
        FScopeLock lock(&GHandleMutex);
        if (GShuttingDown)
        {
            SetLastErrorMessage(TEXT("The Unreal C API is shutting down"));
            delete context;
            return UEC_RESULT_SHUTTING_DOWN;
        }
        GContexts.Add(context);
        *outApi = &GApi;
        *outContext = reinterpret_cast<uec_context*>(context);
    }
    return UEC_RESULT_OK;
}
