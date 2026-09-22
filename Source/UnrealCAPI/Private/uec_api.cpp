#include "uec_api.h"

#include "CoreMinimal.h"
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
    TMap<uint64, TSharedPtr<FUECSaveGameRequest>> GSaveGameRequests;
    TMap<uint64, TSharedPtr<FUECInputBinding>> GInputBindings;
    uint64 GNextObjectLoadRequestId = 1;
    uint64 GNextGameThreadRequestId = 1;
    uint64 GNextTimerId = 1;
    uint64 GNextTickSubscriptionId = 1;
    uint64 GNextAudioSubscriptionId = 1;
    uint64 GNextWidgetSubscriptionId = 1;
    uint64 GNextAnimationSubscriptionId = 1;
    uint64 GNextCollisionSubscriptionId = 1;
    uint64 GNextSaveGameRequestId = 1;
    uint64 GNextInputBindingId = 1;
    constexpr int32 MaxQueuedObjectLoads = 1024;
    constexpr int32 MaxQueuedGameThreadRequests = 1024;

    static uint64 AllocateHandleGeneration()
    {
        FScopeLock lock(&GHandleMutex);
        if (GNextHandleGeneration == 0) return 0;
        return GNextHandleGeneration++;
    }

    static bool InitializeHandle(FUECHandleHeader& header, EUECHandleKind kind)
    {
        const uint64 generation = AllocateHandleGeneration();
        if (generation == 0) return false;
        header.Kind = kind;
        header.Generation = generation;
        header.bReleased = false;
        return true;
    }

    static void TombstoneHandle(FUECHandleHeader& header)
    {
        FScopeLock lock(&GHandleMutex);
        header.bReleased = true;
    }

    static bool IsShuttingDown()
    {
        FScopeLock lock(&GHandleMutex);
        return GShuttingDown;
    }

    static bool IsValidContext(uec_context* rawContext)
    {
        const auto* context = reinterpret_cast<const FUECContext*>(rawContext);
        FScopeLock lock(&GHandleMutex);
        return context != nullptr && !GShuttingDown && GContexts.Contains(context) &&
            context->Header.Kind == EUECHandleKind::Context &&
            context->Header.Generation != 0 && !context->Header.bReleased;
    }

    static bool IsValidWorld(const FUECWorld* world)
    {
        FScopeLock lock(&GHandleMutex);
        return world != nullptr && !GShuttingDown && GWorlds.Contains(world) &&
            world->Header.Kind == EUECHandleKind::World && world->Header.Generation != 0 &&
            !world->Header.bReleased;
    }

    static bool IsValidActor(const FUECActor* actor)
    {
        FScopeLock lock(&GHandleMutex);
        return actor != nullptr && !GShuttingDown && GActors.Contains(actor) &&
            actor->Header.Kind == EUECHandleKind::Actor && actor->Header.Generation != 0 &&
            !actor->Header.bReleased;
    }

    static bool IsValidComponent(const FUECSceneComponent* component)
    {
        FScopeLock lock(&GHandleMutex);
        return component != nullptr && !GShuttingDown && GComponents.Contains(component) &&
            component->Header.Kind == EUECHandleKind::SceneComponent &&
            component->Header.Generation != 0 && !component->Header.bReleased;
    }

    static bool IsValidClass(const FUECClass* klass)
    {
        FScopeLock lock(&GHandleMutex);
        return klass != nullptr && !GShuttingDown && GClasses.Contains(klass) &&
            klass->Header.Kind == EUECHandleKind::Class && klass->Header.Generation != 0 &&
            !klass->Header.bReleased;
    }

    static bool IsValidObject(const FUECObject* object)
    {
        FScopeLock lock(&GHandleMutex);
        return object != nullptr && !GShuttingDown && GObjects.Contains(object) &&
            object->Header.Kind == EUECHandleKind::Object && object->Header.Generation != 0 &&
            !object->Header.bReleased;
    }

    static uec_property_kind GetPropertyKind(const FProperty* property)
    {
        if (CastField<FBoolProperty>(property)) return UEC_PROPERTY_BOOL;
        if (CastField<FIntProperty>(property) || CastField<FInt64Property>(property) ||
            CastField<FUInt32Property>(property) || CastField<FUInt64Property>(property) ||
            CastField<FByteProperty>(property) || CastField<FInt16Property>(property) ||
            CastField<FUInt16Property>(property) || CastField<FInt8Property>(property) ||
            CastField<FUInt8Property>(property)) return UEC_PROPERTY_INTEGER;
        if (CastField<FFloatProperty>(property)) return UEC_PROPERTY_FLOAT;
        if (CastField<FDoubleProperty>(property)) return UEC_PROPERTY_DOUBLE;
        if (CastField<FEnumProperty>(property)) return UEC_PROPERTY_ENUM;
        if (CastField<FStrProperty>(property)) return UEC_PROPERTY_STRING;
        if (CastField<FNameProperty>(property)) return UEC_PROPERTY_NAME;
        if (CastField<FTextProperty>(property)) return UEC_PROPERTY_TEXT;
        if (CastField<FClassProperty>(property)) return UEC_PROPERTY_CLASS;
        if (CastField<FObjectPropertyBase>(property)) return UEC_PROPERTY_OBJECT;
        if (CastField<FStructProperty>(property)) return UEC_PROPERTY_STRUCT;
        if (CastField<FArrayProperty>(property)) return UEC_PROPERTY_ARRAY;
        if (CastField<FMapProperty>(property)) return UEC_PROPERTY_MAP;
        if (CastField<FSetProperty>(property)) return UEC_PROPERTY_SET;
        return UEC_PROPERTY_UNKNOWN;
    }

    static bool IsUnsignedIntegerProperty(const FProperty* property)
    {
        return CastField<FByteProperty>(property) || CastField<FUInt8Property>(property) ||
            CastField<FUInt16Property>(property) || CastField<FUInt32Property>(property) ||
            CastField<FUInt64Property>(property);
    }

    static uec_result CopyFStringToUtf8(const FString& value, char* buffer,
                                        size_t bufferSize, size_t* requiredSize)
    {
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        FTCHARToUTF8 utf8(*value);
        const size_t required = static_cast<size_t>(utf8.Length()) + 1;
        *requiredSize = required;
        if (buffer == nullptr || bufferSize < required) return UEC_RESULT_BUFFER_TOO_SMALL;
        FMemory::Memcpy(buffer, utf8.Get(), required - 1);
        buffer[required - 1] = '\0';
        return UEC_RESULT_OK;
    }

    static bool IsValidStringView(uec_string_view value);

    static FString ToFString(uec_string_view value)
    {
        if (!IsValidStringView(value) || value.size == 0) return FString();
        FUTF8ToTCHAR converter(value.data, static_cast<int32>(value.size));
        return FString(converter.Length(), converter.Get());
    }

    static bool IsValidUtf8(uec_string_view value)
    {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(value.data);
        size_t index = 0;
        while (index < value.size)
        {
            const uint8_t first = bytes[index++];
            if (first == 0u) return false;
            if (first <= 0x7Fu) continue;
            uint32 codePoint = 0;
            size_t continuationCount = 0;
            uint32 minimum = 0;
            if (first >= 0xC2u && first <= 0xDFu)
            {
                codePoint = first & 0x1Fu;
                continuationCount = 1;
                minimum = 0x80u;
            }
            else if (first >= 0xE0u && first <= 0xEFu)
            {
                codePoint = first & 0x0Fu;
                continuationCount = 2;
                minimum = 0x800u;
            }
            else if (first >= 0xF0u && first <= 0xF4u)
            {
                codePoint = first & 0x07u;
                continuationCount = 3;
                minimum = 0x10000u;
            }
            else
            {
                return false;
            }
            if (index + continuationCount > value.size) return false;
            for (size_t continuation = 0; continuation < continuationCount; ++continuation)
            {
                const uint8_t next = bytes[index++];
                if ((next & 0xC0u) != 0x80u) return false;
                codePoint = (codePoint << 6u) | (next & 0x3Fu);
            }
            if (codePoint < minimum || codePoint > 0x10FFFFu ||
                (codePoint >= 0xD800u && codePoint <= 0xDFFFu))
            {
                return false;
            }
        }
        return true;
    }

    static bool IsValidStringView(uec_string_view value)
    {
        return (value.data != nullptr || value.size == 0) &&
            value.size <= static_cast<size_t>(INT32_MAX) &&
            (value.size == 0 || IsValidUtf8(value));
    }

    static bool IsFiniteVector(const uec_vector3& value)
    {
        return FMath::IsFinite(value.x) && FMath::IsFinite(value.y) && FMath::IsFinite(value.z);
    }

    static bool IsValidBool(uec_bool value)
    {
        return value == UEC_FALSE || value == UEC_TRUE;
    }

    static uec_result RequireWorldAuthority(const UWorld* world)
    {
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return world->GetNetMode() == NM_Client ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_OK;
    }

    static bool IsFiniteTransform(const uec_transform& value)
    {
        return IsFiniteVector(value.translation) && IsFiniteVector(value.scale) &&
            FMath::IsFinite(value.rotation.x) && FMath::IsFinite(value.rotation.y) &&
            FMath::IsFinite(value.rotation.z) && FMath::IsFinite(value.rotation.w);
    }

    static bool WriteInputActionValue(const FInputActionValue& value,
                                      uec_input_action_value& outValue)
    {
        outValue.struct_size = sizeof(uec_input_action_value);
        outValue.kind = UEC_INPUT_ACTION_VALUE_BOOLEAN;
        outValue.bool_value = UEC_FALSE;
        outValue.reserved[0] = 0;
        outValue.reserved[1] = 0;
        outValue.reserved[2] = 0;
        outValue.axis = {0.0, 0.0, 0.0};
        switch (value.GetValueType())
        {
        case EInputActionValueType::Boolean:
            outValue.bool_value = value.Get<bool>() ? UEC_TRUE : UEC_FALSE;
            return true;
        case EInputActionValueType::Axis1D:
            outValue.kind = UEC_INPUT_ACTION_VALUE_AXIS_1D;
            outValue.axis.x = static_cast<double>(value.Get<float>());
            return true;
        case EInputActionValueType::Axis2D:
        {
            outValue.kind = UEC_INPUT_ACTION_VALUE_AXIS_2D;
            const FVector2D axis = value.Get<FVector2D>();
            outValue.axis.x = axis.X;
            outValue.axis.y = axis.Y;
            return true;
        }
        case EInputActionValueType::Axis3D:
        {
            outValue.kind = UEC_INPUT_ACTION_VALUE_AXIS_3D;
            const FVector axis = value.Get<FVector>();
            outValue.axis.x = axis.X;
            outValue.axis.y = axis.Y;
            outValue.axis.z = axis.Z;
            return true;
        }
        default:
            return false;
        }
    }

    static FTransform ToFTransform(const uec_transform& value)
    {
        return FTransform(
            FQuat(value.rotation.x, value.rotation.y, value.rotation.z, value.rotation.w),
            FVector(value.translation.x, value.translation.y, value.translation.z),
            FVector(value.scale.x, value.scale.y, value.scale.z));
    }

    static uec_transform FromFTransform(const FTransform& value)
    {
        const FVector translation = value.GetTranslation();
        const FQuat rotation = value.GetRotation();
        const FVector scale = value.GetScale3D();
        return {{translation.X, translation.Y, translation.Z},
                {rotation.X, rotation.Y, rotation.Z, rotation.W},
                {scale.X, scale.Y, scale.Z}};
    }

    static uec_world_kind ToWorldKind(EWorldType::Type type)
    {
        switch (type)
        {
        case EWorldType::Game: return UEC_WORLD_KIND_GAME;
        case EWorldType::PIE: return UEC_WORLD_KIND_PIE;
        case EWorldType::Editor: return UEC_WORLD_KIND_EDITOR;
        case EWorldType::GamePreview: return UEC_WORLD_KIND_GAME_PREVIEW;
        case EWorldType::Inactive: return UEC_WORLD_KIND_INACTIVE;
        default: return UEC_WORLD_KIND_UNKNOWN;
        }
    }

    static bool ToCollisionChannel(uec_trace_channel channel, ECollisionChannel& outChannel)
    {
        switch (channel)
        {
        case UEC_TRACE_VISIBILITY: outChannel = ECC_Visibility; return true;
        case UEC_TRACE_CAMERA: outChannel = ECC_Camera; return true;
        case UEC_TRACE_WORLD_STATIC: outChannel = ECC_WorldStatic; return true;
        case UEC_TRACE_WORLD_DYNAMIC: outChannel = ECC_WorldDynamic; return true;
        case UEC_TRACE_PAWN: outChannel = ECC_Pawn; return true;
        case UEC_TRACE_PHYSICS_BODY: outChannel = ECC_PhysicsBody; return true;
        default: return false;
        }
    }

    static uec_result MakeCollisionShape(const uec_collision_shape* descriptor,
                                         FCollisionShape& outShape)
    {
        if (descriptor == nullptr || descriptor->struct_size < sizeof(uec_collision_shape))
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        switch (descriptor->kind)
        {
        case UEC_COLLISION_SHAPE_SPHERE:
            if (!FMath::IsFinite(descriptor->radius) || descriptor->radius <= 0.0) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            outShape = FCollisionShape::MakeSphere(static_cast<float>(descriptor->radius));
            return UEC_RESULT_OK;
        case UEC_COLLISION_SHAPE_BOX:
            if (!FMath::IsFinite(descriptor->half_extents.x) ||
                !FMath::IsFinite(descriptor->half_extents.y) ||
                !FMath::IsFinite(descriptor->half_extents.z) ||
                descriptor->half_extents.x <= 0.0 || descriptor->half_extents.y <= 0.0 ||
                descriptor->half_extents.z <= 0.0) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            outShape = FCollisionShape::MakeBox(FVector(
                descriptor->half_extents.x, descriptor->half_extents.y, descriptor->half_extents.z));
            return UEC_RESULT_OK;
        case UEC_COLLISION_SHAPE_CAPSULE:
            if (!FMath::IsFinite(descriptor->radius) || !FMath::IsFinite(descriptor->half_height) ||
                descriptor->radius <= 0.0 || descriptor->half_height < descriptor->radius) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            outShape = FCollisionShape::MakeCapsule(
                static_cast<float>(descriptor->radius), static_cast<float>(descriptor->half_height));
            return UEC_RESULT_OK;
        default:
            return UEC_RESULT_INVALID_ARGUMENT;
        }
    }


    #include "API/uec_api_world_actor.inl"
    #include "API/uec_api_actor_component.inl"
    #include "API/uec_api_reflection.inl"
    #include "API/uec_api_presentation.inl"
    #include "API/uec_api_gameplay.inl"
    #include "API/uec_api_input.inl"
    #include "API/uec_api_async.inl"

    const uec_api GApi = {
        sizeof(uec_api), UEC_ABI_MAJOR, UEC_ABI_MINOR,
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
        &OverlapShapeFiltered
    };
}
class FUnrealCAPIModule final : public IModuleInterface
{
public:
    void StartupModule() override
    {
        {
            FScopeLock lock(&GHandleMutex);
            GShuttingDown = false;
        }
        UE_LOG(LogTemp, Log, TEXT("%s runtime module started (ABI %u.%u)"),
            UTF8_TO_TCHAR(kModuleName), UEC_ABI_MAJOR, UEC_ABI_MINOR);
    }

    void ShutdownModule() override
    {
        {
            FScopeLock lock(&GHandleMutex);
            GShuttingDown = true;
        }
        ClearAllTimers();
        ClearAllTickSubscriptions();
        ClearAllAudioSubscriptions();
        ClearAllWidgetSubscriptions();
        ClearAllAnimationSubscriptions();
        ClearAllCollisionSubscriptions();
        CancelAllObjectLoads();
        CancelAllGameThreadRequests();
        CancelAllSaveGameRequests();
        CancelAllInputBindings();
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
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    *outApi = nullptr;
    *outContext = nullptr;

    if (requestedMajor != UEC_ABI_MAJOR || requestedMinor > UEC_ABI_MINOR)
    {
        return UEC_RESULT_UNSUPPORTED;
    }

    auto* context = new FUECContext();
    if (!InitializeHandle(context->Header, EUECHandleKind::Context))
    {
        delete context;
        return UEC_RESULT_INTERNAL_ERROR;
    }
    {
        FScopeLock lock(&GHandleMutex);
        if (GShuttingDown)
        {
            delete context;
            return UEC_RESULT_SHUTTING_DOWN;
        }
        GContexts.Add(context);
        *outApi = &GApi;
        *outContext = reinterpret_cast<uec_context*>(context);
    }
    return UEC_RESULT_OK;
}
