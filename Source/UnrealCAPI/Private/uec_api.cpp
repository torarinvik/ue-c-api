#include "uec_api.h"

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "HAL/CriticalSection.h"
#include "Modules/ModuleManager.h"
#include "Misc/ScopeLock.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

namespace
{
    constexpr char kModuleName[] = "UnrealCAPI";

    struct FUECContext final
    {
        uint64 Generation = 1;
        bool bReleased = false;
    };

    struct FUECWorld final
    {
        TWeakObjectPtr<UWorld> Value;
        uec_world_kind Kind = UEC_WORLD_KIND_UNKNOWN;
    };
    struct FUECActor final { TWeakObjectPtr<AActor> Value; };
    struct FUECSceneComponent final { TWeakObjectPtr<USceneComponent> Value; };
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
    struct FUECClass final { TWeakObjectPtr<UClass> Value; };
    struct FUECObject final { TWeakObjectPtr<UObject> Value; };

    FCriticalSection GHandleMutex;
    TSet<const FUECContext*> GContexts;
    TSet<const FUECWorld*> GWorlds;
    TSet<const FUECActor*> GActors;
    TSet<const FUECSceneComponent*> GComponents;
    TMap<uint64, TSharedPtr<FUECTimerState>> GTimers;
    TSet<const FUECClass*> GClasses;
    TSet<const FUECObject*> GObjects;
    uint64 GNextTimerId = 1;

    static bool IsValidContext(uec_context* rawContext)
    {
        const auto* context = reinterpret_cast<const FUECContext*>(rawContext);
        FScopeLock lock(&GHandleMutex);
        return context != nullptr && GContexts.Contains(context) && !context->bReleased;
    }

    static bool IsValidWorld(const FUECWorld* world)
    {
        FScopeLock lock(&GHandleMutex);
        return world != nullptr && GWorlds.Contains(world);
    }

    static bool IsValidActor(const FUECActor* actor)
    {
        FScopeLock lock(&GHandleMutex);
        return actor != nullptr && GActors.Contains(actor);
    }

    static bool IsValidComponent(const FUECSceneComponent* component)
    {
        FScopeLock lock(&GHandleMutex);
        return component != nullptr && GComponents.Contains(component);
    }

    static bool IsValidClass(const FUECClass* klass)
    {
        FScopeLock lock(&GHandleMutex);
        return klass != nullptr && GClasses.Contains(klass);
    }

    static bool IsValidObject(const FUECObject* object)
    {
        FScopeLock lock(&GHandleMutex);
        return object != nullptr && GObjects.Contains(object);
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

    static FString ToFString(uec_string_view value)
    {
        if (value.data == nullptr || value.size == 0) return FString();
        FUTF8ToTCHAR converter(value.data, static_cast<int32>(value.size));
        return FString(converter.Length(), converter.Get());
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

    uec_result UEC_CALL GetCapabilities(uec_context* rawContext, uec_capabilities* outCapabilities)
    {
        if (outCapabilities == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        *outCapabilities = UEC_CAPABILITY_BOOTSTRAP | UEC_CAPABILITY_LOGGING |
            UEC_CAPABILITY_WORLD | UEC_CAPABILITY_ACTORS | UEC_CAPABILITY_COMPONENTS |
            UEC_CAPABILITY_TIMERS | UEC_CAPABILITY_CLASS_METADATA | UEC_CAPABILITY_REFLECTION |
            UEC_CAPABILITY_COLLISION | UEC_CAPABILITY_ASSETS;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetLastError(uec_context*, char* buffer, size_t bufferSize, size_t* requiredSize)
    {
        static constexpr char Message[] = "No error";
        const size_t required = sizeof(Message); // includes the NUL terminator
        if (requiredSize != nullptr)
        {
            *requiredSize = required;
        }
        if (buffer == nullptr || bufferSize < required)
        {
            return UEC_RESULT_BUFFER_TOO_SMALL;
        }
        FMemory::Memcpy(buffer, Message, required);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL Log(uec_context* rawContext, uec_string_view message)
    {
        if (!IsValidContext(rawContext))
        {
            return UEC_RESULT_INVALID_HANDLE;
        }
        if (message.data == nullptr && message.size != 0)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }

        FString Text = ToFString(message);
        UE_LOG(LogTemp, Log, TEXT("[UEC] %s"), *Text);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ReleaseContext(uec_context* rawContext)
    {
        auto* context = reinterpret_cast<FUECContext*>(rawContext);
        if (!IsValidContext(rawContext))
        {
            return UEC_RESULT_INVALID_HANDLE;
        }
        context->bReleased = true;
        {
            FScopeLock lock(&GHandleMutex);
            GContexts.Remove(context);
        }
        delete context;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetDefaultWorld(uec_context* rawContext, uec_world** outWorld)
    {
        if (outWorld == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outWorld = nullptr;
        if (GEngine == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
        {
            UWorld* world = worldContext.World();
            if (world != nullptr && (worldContext.WorldType == EWorldType::Game || worldContext.WorldType == EWorldType::PIE))
            {
                auto* handle = new FUECWorld();
                handle->Value = world;
                handle->Kind = ToWorldKind(worldContext.WorldType);
                {
                    FScopeLock lock(&GHandleMutex);
                    GWorlds.Add(handle);
                }
                *outWorld = reinterpret_cast<uec_world*>(handle);
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_NOT_INITIALIZED;
    }

    uec_result UEC_CALL GetWorldCount(uec_context* rawContext, uint32_t* outCount)
    {
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outCount = 0;
        if (GEngine == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
        {
            if (worldContext.World() != nullptr &&
                (worldContext.WorldType == EWorldType::Game || worldContext.WorldType == EWorldType::PIE))
            {
                ++(*outCount);
            }
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetWorldAt(uec_context* rawContext, uint32_t index, uec_world** outWorld)
    {
        if (outWorld == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outWorld = nullptr;
        if (GEngine == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        uint32_t current = 0;
        for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
        {
            UWorld* world = worldContext.World();
            if (world == nullptr || (worldContext.WorldType != EWorldType::Game && worldContext.WorldType != EWorldType::PIE))
            {
                continue;
            }
            if (current++ != index) continue;
            auto* handle = new FUECWorld();
            handle->Value = world;
            handle->Kind = ToWorldKind(worldContext.WorldType);
            {
                FScopeLock lock(&GHandleMutex);
                GWorlds.Add(handle);
            }
            *outWorld = reinterpret_cast<uec_world*>(handle);
            return UEC_RESULT_OK;
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    uec_result UEC_CALL GetWorldKind(uec_world* rawWorld, uec_world_kind* outKind)
    {
        if (outKind == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* world = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(world)) return UEC_RESULT_INVALID_HANDLE;
        *outKind = world->Kind;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ReleaseWorld(uec_world* rawWorld)
    {
        auto* world = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(world)) return UEC_RESULT_INVALID_HANDLE;
        {
            FScopeLock lock(&GHandleMutex);
            GWorlds.Remove(world);
        }
        delete world;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SpawnActor(uec_world* rawWorld, uec_string_view classPath,
                                   const uec_transform* transform, uec_actor** outActor)
    {
        if (outActor == nullptr || transform == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outActor = nullptr;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UClass* actorClass = LoadClass<AActor>(nullptr, *ToFString(classPath));
        if (actorClass == nullptr || !actorClass->IsChildOf(AActor::StaticClass())) return UEC_RESULT_INVALID_ARGUMENT;
        AActor* actor = world->SpawnActor<AActor>(actorClass, ToFTransform(*transform));
        if (actor == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        auto* handle = new FUECActor();
        handle->Value = actor;
        {
            FScopeLock lock(&GHandleMutex);
            GActors.Add(handle);
        }
        *outActor = reinterpret_cast<uec_actor*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL DestroyActor(uec_actor* rawActor)
    {
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        {
            FScopeLock lock(&GHandleMutex);
            GActors.Remove(handle);
        }
        delete handle;
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return actor->Destroy() ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
    }

    uec_result UEC_CALL ReleaseActor(uec_actor* rawActor)
    {
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        {
            FScopeLock lock(&GHandleMutex);
            GActors.Remove(handle);
        }
        delete handle;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorTransform(uec_actor* rawActor, uec_transform* outTransform)
    {
        if (outTransform == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        *outTransform = FromFTransform(actor->GetActorTransform());
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetActorTransform(uec_actor* rawActor, const uec_transform* transform, uec_bool sweep)
    {
        if (transform == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        actor->SetActorTransform(ToFTransform(*transform), sweep != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorName(uec_actor* rawActor, char* buffer, size_t bufferSize, size_t* requiredSize)
    {
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;

        const FString name = actor->GetName();
        FTCHARToUTF8 utf8(*name);
        const size_t required = static_cast<size_t>(utf8.Length()) + 1;
        *requiredSize = required;
        if (buffer == nullptr || bufferSize < required) return UEC_RESULT_BUFFER_TOO_SMALL;
        FMemory::Memcpy(buffer, utf8.Get(), required - 1);
        buffer[required - 1] = '\0';
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ActorHasTag(uec_actor* rawActor, uec_string_view tag, uec_bool* outHasTag)
    {
        if (outHasTag == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (tag.data == nullptr && tag.size != 0) return UEC_RESULT_INVALID_ARGUMENT;
        *outHasTag = actor->ActorHasTag(FName(*ToFString(tag))) ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorRootComponent(uec_actor* rawActor, uec_scene_component** outComponent)
    {
        if (outComponent == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outComponent = nullptr;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        USceneComponent* component = actor->GetRootComponent();
        if (component == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        auto* handle = new FUECSceneComponent();
        handle->Value = component;
        {
            FScopeLock lock(&GHandleMutex);
            GComponents.Add(handle);
        }
        *outComponent = reinterpret_cast<uec_scene_component*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorComponentCount(uec_actor* rawActor, uint32_t* outCount)
    {
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outCount = 0;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        TArray<USceneComponent*> components;
        actor->GetComponents<USceneComponent>(components);
        *outCount = static_cast<uint32_t>(components.Num());
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorComponentAt(uec_actor* rawActor,
                                            uint32_t index,
                                            uec_scene_component** outComponent)
    {
        if (outComponent == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outComponent = nullptr;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        TArray<USceneComponent*> components;
        actor->GetComponents<USceneComponent>(components);
        if (index >= static_cast<uint32_t>(components.Num())) return UEC_RESULT_INVALID_ARGUMENT;
        USceneComponent* component = components[static_cast<int32>(index)];
        if (component == nullptr) return UEC_RESULT_INVALID_HANDLE;
        auto* handle = new FUECSceneComponent();
        handle->Value = component;
        {
            FScopeLock lock(&GHandleMutex);
            GComponents.Add(handle);
        }
        *outComponent = reinterpret_cast<uec_scene_component*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ReleaseSceneComponent(uec_scene_component* rawComponent)
    {
        auto* handle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        {
            FScopeLock lock(&GHandleMutex);
            GComponents.Remove(handle);
        }
        delete handle;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetComponentTransform(uec_scene_component* rawComponent, uec_transform* outTransform)
    {
        if (outTransform == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USceneComponent* component = handle->Value.Get();
        if (component == nullptr) return UEC_RESULT_INVALID_HANDLE;
        *outTransform = FromFTransform(component->GetComponentTransform());
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetComponentTransform(uec_scene_component* rawComponent,
                                               const uec_transform* transform,
                                               uec_bool sweep)
    {
        if (transform == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USceneComponent* component = handle->Value.Get();
        if (component == nullptr) return UEC_RESULT_INVALID_HANDLE;
        component->SetWorldTransform(ToFTransform(*transform), sweep != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetComponentVisible(uec_scene_component* rawComponent,
                                             uec_bool visible,
                                             uec_bool propagateToChildren)
    {
        auto* handle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USceneComponent* component = handle->Value.Get();
        if (component == nullptr) return UEC_RESULT_INVALID_HANDLE;
        component->SetVisibility(visible != UEC_FALSE, propagateToChildren != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetComponentActive(uec_scene_component* rawComponent,
                                            uec_bool active,
                                            uec_bool reset)
    {
        auto* handle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USceneComponent* component = handle->Value.Get();
        if (component == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (active != UEC_FALSE)
        {
            component->Activate(reset != UEC_FALSE);
        }
        else
        {
            component->Deactivate();
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetTimer(uec_world* rawWorld,
                                 double intervalSeconds,
                                 uec_bool looping,
                                 uec_timer_callback callback,
                                 void* userData,
                                 uint64_t* outTimerId)
    {
        if (outTimerId == nullptr || callback == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!FMath::IsFinite(intervalSeconds) || intervalSeconds <= 0.0) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;

        const uint64 timerId = GNextTimerId++;
        auto state = MakeShared<FUECTimerState>();
        state->Id = timerId;
        state->World = world;
        state->Callback = callback;
        state->UserData = userData;
        state->Looping = looping != UEC_FALSE;
        TWeakPtr<FUECTimerState> weakState = state;
        FTimerDelegate delegate;
        delegate.BindLambda([weakState]()
        {
            TSharedPtr<FUECTimerState> current = weakState.Pin();
            if (!current.IsValid() || current->Cancelled || current->Callback == nullptr) return;
            current->Callback(current->Id, current->UserData);
            if (!current->Looping)
            {
                GTimers.Remove(current->Id);
            }
        });
        world->GetTimerManager().SetTimer(state->Handle, delegate, intervalSeconds, state->Looping);
        GTimers.Add(timerId, state);
        *outTimerId = timerId;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ClearTimer(uec_world* rawWorld, uint64_t timerId)
    {
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECTimerState>* statePtr = GTimers.Find(timerId);
        if (statePtr == nullptr || !statePtr->IsValid()) return UEC_RESULT_INVALID_ARGUMENT;
        TSharedPtr<FUECTimerState> state = *statePtr;
        UWorld* world = worldHandle->Value.Get();
        if (world != nullptr)
        {
            world->GetTimerManager().ClearTimer(state->Handle);
        }
        state->Cancelled = true;
        GTimers.Remove(timerId);
        return UEC_RESULT_OK;
    }

    static void ClearAllTimers()
    {
        for (const TPair<uint64, TSharedPtr<FUECTimerState>>& pair : GTimers)
        {
            if (pair.Value.IsValid())
            {
                if (UWorld* world = pair.Value->World.Get())
                {
                    world->GetTimerManager().ClearTimer(pair.Value->Handle);
                }
                pair.Value->Cancelled = true;
            }
        }
        GTimers.Empty();
    }

    static void ClearAllHandles()
    {
        FScopeLock lock(&GHandleMutex);
        for (const FUECContext* handle : GContexts) delete const_cast<FUECContext*>(handle);
        for (const FUECWorld* handle : GWorlds) delete const_cast<FUECWorld*>(handle);
        for (const FUECActor* handle : GActors) delete const_cast<FUECActor*>(handle);
        for (const FUECSceneComponent* handle : GComponents) delete const_cast<FUECSceneComponent*>(handle);
        for (const FUECClass* handle : GClasses) delete const_cast<FUECClass*>(handle);
        GContexts.Empty();
        GWorlds.Empty();
        GActors.Empty();
        GComponents.Empty();
        GClasses.Empty();
        for (const FUECObject* handle : GObjects) delete const_cast<FUECObject*>(handle);
        GObjects.Empty();
    }

    uec_result UEC_CALL FindClass(uec_context* rawContext,
                                  uec_string_view classPath,
                                  uec_class** outClass)
    {
        if (outClass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outClass = nullptr;
        UClass* klass = LoadClass<UObject>(nullptr, *ToFString(classPath));
        if (klass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = new FUECClass();
        handle->Value = klass;
        {
            FScopeLock lock(&GHandleMutex);
            GClasses.Add(handle);
        }
        *outClass = reinterpret_cast<uec_class*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ReleaseClass(uec_class* rawClass)
    {
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        {
            FScopeLock lock(&GHandleMutex);
            GClasses.Remove(handle);
        }
        delete handle;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetClassName(uec_class* rawClass,
                                     char* buffer,
                                     size_t bufferSize,
                                     size_t* requiredSize)
    {
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        UClass* klass = handle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return CopyFStringToUtf8(klass->GetName(), buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL ClassIsA(uec_class* rawClass,
                                 uec_string_view parentClassPath,
                                 uec_bool* outIsA)
    {
        if (outIsA == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = handle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UClass* parent = LoadClass<UObject>(nullptr, *ToFString(parentClassPath));
        if (parent == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outIsA = klass->IsChildOf(parent) ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetClassPropertyCount(uec_class* rawClass, uint32_t* outCount)
    {
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = handle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        *outCount = 0;
        for (TFieldIterator<FProperty> iterator(klass, EFieldIteratorFlags::IncludeSuper); iterator; ++iterator)
        {
            ++(*outCount);
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetClassPropertyAt(uec_class* rawClass,
                                            uint32_t index,
                                            char* nameBuffer,
                                            size_t nameBufferSize,
                                            size_t* nameRequiredSize,
                                            uec_property_kind* outKind)
    {
        if (nameRequiredSize == nullptr || outKind == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = handle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        uint32_t current = 0;
        for (TFieldIterator<FProperty> iterator(klass, EFieldIteratorFlags::IncludeSuper); iterator; ++iterator)
        {
            if (current++ != index) continue;
            FProperty* property = *iterator;
            *outKind = GetPropertyKind(property);
            return CopyFStringToUtf8(property->GetName(), nameBuffer, nameBufferSize, nameRequiredSize);
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    uec_result UEC_CALL GetActorPropertyValue(uec_actor* rawActor,
                                              uec_string_view propertyName,
                                              uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value))
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (propertyName.data == nullptr && propertyName.size != 0) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;

        outValue->kind = GetPropertyKind(property);
        outValue->bool_value = UEC_FALSE;
        outValue->integer_value = 0;
        outValue->real_value = 0.0;
        if (FBoolProperty* boolProperty = CastField<FBoolProperty>(property))
        {
            outValue->bool_value = boolProperty->GetPropertyValue_InContainer(actor) ? UEC_TRUE : UEC_FALSE;
            return UEC_RESULT_OK;
        }
        if (FNumericProperty* numericProperty = CastField<FNumericProperty>(property))
        {
            if (numericProperty->IsFloatingPoint())
            {
                outValue->real_value = numericProperty->GetFloatingPointPropertyValue_InContainer(actor);
                return UEC_RESULT_OK;
            }
            if (numericProperty->IsInteger())
            {
                outValue->integer_value = IsUnsignedIntegerProperty(property)
                    ? static_cast<int64>(numericProperty->GetUnsignedIntPropertyValue_InContainer(actor))
                    : numericProperty->GetSignedIntPropertyValue_InContainer(actor);
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_UNSUPPORTED;
    }

    uec_result UEC_CALL GetActorPropertyString(uec_actor* rawActor,
                                               uec_string_view propertyName,
                                               char* buffer,
                                               size_t bufferSize,
                                               size_t* requiredSize,
                                               uec_property_kind* outKind)
    {
        if (requiredSize == nullptr || outKind == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (propertyName.data == nullptr && propertyName.size != 0) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        FString value;
        *outKind = GetPropertyKind(property);
        if (const FStrProperty* stringProperty = CastField<FStrProperty>(property))
        {
            value = stringProperty->GetPropertyValue_InContainer(actor);
        }
        else if (const FNameProperty* nameProperty = CastField<FNameProperty>(property))
        {
            value = nameProperty->GetPropertyValue_InContainer(actor).ToString();
        }
        else if (const FTextProperty* textProperty = CastField<FTextProperty>(property))
        {
            value = textProperty->GetPropertyValue_InContainer(actor).ToString();
        }
        else
        {
            return UEC_RESULT_UNSUPPORTED;
        }
        return CopyFStringToUtf8(value, buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL SetActorPropertyValue(uec_actor* rawActor,
                                              uec_string_view propertyName,
                                              const uec_property_value* value)
    {
        if (value == nullptr || value->struct_size < sizeof(uec_property_value)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (propertyName.data == nullptr && propertyName.size != 0) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (FBoolProperty* boolProperty = CastField<FBoolProperty>(property))
        {
            if (value->kind != UEC_PROPERTY_BOOL) return UEC_RESULT_INVALID_ARGUMENT;
            boolProperty->SetPropertyValue_InContainer(actor, value->bool_value != UEC_FALSE);
            return UEC_RESULT_OK;
        }
        if (FNumericProperty* numericProperty = CastField<FNumericProperty>(property))
        {
            if (numericProperty->IsFloatingPoint())
            {
                if (value->kind != UEC_PROPERTY_FLOAT && value->kind != UEC_PROPERTY_DOUBLE) return UEC_RESULT_INVALID_ARGUMENT;
                numericProperty->SetFloatingPointPropertyValue(
                    numericProperty->ContainerPtrToValuePtr<void>(actor), value->real_value);
                return UEC_RESULT_OK;
            }
            if (numericProperty->IsInteger())
            {
                if (value->kind != UEC_PROPERTY_INTEGER && value->kind != UEC_PROPERTY_ENUM) return UEC_RESULT_INVALID_ARGUMENT;
                const FString text = LexToString(value->integer_value);
                numericProperty->SetNumericPropertyValueFromString_InContainer(actor, *text);
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_UNSUPPORTED;
    }

    uec_result UEC_CALL SetActorPropertyString(uec_actor* rawActor,
                                               uec_string_view propertyName,
                                               uec_string_view value)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if ((propertyName.data == nullptr && propertyName.size != 0) ||
            (value.data == nullptr && value.size != 0)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        const FString text = ToFString(value);
        if (const FStrProperty* stringProperty = CastField<FStrProperty>(property))
        {
            stringProperty->SetPropertyValue_InContainer(actor, text);
            return UEC_RESULT_OK;
        }
        if (const FNameProperty* nameProperty = CastField<FNameProperty>(property))
        {
            nameProperty->SetPropertyValue_InContainer(actor, FName(*text));
            return UEC_RESULT_OK;
        }
        if (const FTextProperty* textProperty = CastField<FTextProperty>(property))
        {
            textProperty->SetPropertyValue_InContainer(actor, FText::FromString(text));
            return UEC_RESULT_OK;
        }
        return UEC_RESULT_UNSUPPORTED;
    }

    uec_result UEC_CALL LineTrace(uec_world* rawWorld,
                                  uec_vector3 start,
                                  uec_vector3 end,
                                  uec_trace_channel channel,
                                  uec_bool traceComplex,
                                  uec_hit_result* outHit)
    {
        if (outHit == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        ECollisionChannel collisionChannel;
        switch (channel)
        {
        case UEC_TRACE_VISIBILITY: collisionChannel = ECC_Visibility; break;
        case UEC_TRACE_CAMERA: collisionChannel = ECC_Camera; break;
        case UEC_TRACE_WORLD_STATIC: collisionChannel = ECC_WorldStatic; break;
        case UEC_TRACE_WORLD_DYNAMIC: collisionChannel = ECC_WorldDynamic; break;
        case UEC_TRACE_PAWN: collisionChannel = ECC_Pawn; break;
        case UEC_TRACE_PHYSICS_BODY: collisionChannel = ECC_PhysicsBody; break;
        default: return UEC_RESULT_INVALID_ARGUMENT;
        }

        *outHit = {};
        FHitResult hit;
        FCollisionQueryParams queryParams;
        queryParams.bTraceComplex = traceComplex != UEC_FALSE;
        const bool didHit = world->LineTraceSingleByChannel(
            hit,
            FVector(start.x, start.y, start.z),
            FVector(end.x, end.y, end.z),
            collisionChannel,
            queryParams);
        if (!didHit) return UEC_RESULT_OK;
        outHit->blocking_hit = hit.bBlockingHit ? UEC_TRUE : UEC_FALSE;
        outHit->location = {hit.Location.X, hit.Location.Y, hit.Location.Z};
        outHit->normal = {hit.Normal.X, hit.Normal.Y, hit.Normal.Z};
        outHit->distance = hit.Distance;
        if (AActor* actor = hit.GetActor())
        {
            auto* actorHandle = new FUECActor();
            actorHandle->Value = actor;
            {
                FScopeLock lock(&GHandleMutex);
                GActors.Add(actorHandle);
            }
            outHit->actor = reinterpret_cast<uec_actor*>(actorHandle);
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL InvokeActorFunction(uec_actor* rawActor, uec_string_view functionName)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (functionName.data == nullptr && functionName.size != 0) return UEC_RESULT_INVALID_ARGUMENT;
        UFunction* function = actor->FindFunction(FName(*ToFString(functionName)));
        if (function == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (function->HasAnyFunctionFlags(FUNC_Latent)) return UEC_RESULT_UNSUPPORTED;
        if (!function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_Native | FUNC_BlueprintEvent))
        {
            return UEC_RESULT_UNSUPPORTED;
        }
        for (TFieldIterator<FProperty> iterator(function); iterator; ++iterator)
        {
            const FProperty* parameter = *iterator;
            if (parameter->HasAnyPropertyFlags(CPF_Parm)) return UEC_RESULT_UNSUPPORTED;
        }
        actor->ProcessEvent(function, nullptr);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL LoadObjectHandle(uec_context* rawContext,
                                         uec_string_view objectPath,
                                         uec_object** outObject)
    {
        if (outObject == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outObject = nullptr;
        UObject* object = LoadObject<UObject>(nullptr, *ToFString(objectPath));
        if (object == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = new FUECObject();
        handle->Value = object;
        {
            FScopeLock lock(&GHandleMutex);
            GObjects.Add(handle);
        }
        *outObject = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ReleaseObject(uec_object* rawObject)
    {
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        {
            FScopeLock lock(&GHandleMutex);
            GObjects.Remove(handle);
        }
        delete handle;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetObjectName(uec_object* rawObject,
                                      char* buffer,
                                      size_t bufferSize,
                                      size_t* requiredSize)
    {
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        UObject* object = handle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return CopyFStringToUtf8(object->GetName(), buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL ObjectIsA(uec_object* rawObject,
                                  uec_string_view classPath,
                                  uec_bool* outIsA)
    {
        if (outIsA == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UClass* klass = LoadClass<UObject>(nullptr, *ToFString(classPath));
        if (klass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outIsA = object->IsA(klass) ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    const uec_api GApi = {
        sizeof(uec_api), UEC_ABI_MAJOR, UEC_ABI_MINOR,
        &GetCapabilities, &GetLastError, &Log, &ReleaseContext,
        &GetWorldCount, &GetWorldAt, &GetWorldKind, &GetDefaultWorld,
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
        &GetObjectName, &ObjectIsA
    };
}

class FUnrealCAPIModule final : public IModuleInterface
{
public:
    void StartupModule() override
    {
        UE_LOG(LogTemp, Log, TEXT("%s runtime module started (ABI %u.%u)"),
            UTF8_TO_TCHAR(kModuleName), UEC_ABI_MAJOR, UEC_ABI_MINOR);
    }

    void ShutdownModule() override
    {
        ClearAllTimers();
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
    {
        FScopeLock lock(&GHandleMutex);
        GContexts.Add(context);
    }
    *outApi = &GApi;
    *outContext = reinterpret_cast<uec_context*>(context);
    return UEC_RESULT_OK;
}
