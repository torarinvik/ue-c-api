    uec_result UEC_CALL GetCapabilities(uec_context* rawContext, uec_capabilities* outCapabilities)
    {
        if (outCapabilities == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        *outCapabilities = UEC_CAPABILITY_BOOTSTRAP | UEC_CAPABILITY_LOGGING |
            UEC_CAPABILITY_WORLD | UEC_CAPABILITY_ACTORS | UEC_CAPABILITY_COMPONENTS |
            UEC_CAPABILITY_TIMERS | UEC_CAPABILITY_CLASS_METADATA | UEC_CAPABILITY_REFLECTION |
            UEC_CAPABILITY_COLLISION | UEC_CAPABILITY_ASSETS | UEC_CAPABILITY_ASYNC_ASSETS;
        *outCapabilities |= UEC_CAPABILITY_LEVEL_TRAVEL | UEC_CAPABILITY_PLAYER_FLOW |
            UEC_CAPABILITY_INPUT | UEC_CAPABILITY_PHYSICS | UEC_CAPABILITY_COLLISION_QUERIES |
            UEC_CAPABILITY_AUDIO | UEC_CAPABILITY_UI | UEC_CAPABILITY_CAMERA |
            UEC_CAPABILITY_SAVE_DATA | UEC_CAPABILITY_THREADING | UEC_CAPABILITY_MOVEMENT |
            UEC_CAPABILITY_PRESENTATION | UEC_CAPABILITY_RETAINED_OBJECTS;
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
        if (!IsValidStringView(message))
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
                handle->PIEInstance = worldContext.PIEInstance;
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
            handle->PIEInstance = worldContext.PIEInstance;
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

    uec_result UEC_CALL GetWorldPIEInstance(uec_world* rawWorld, int32_t* outInstance)
    {
        if (outInstance == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* world = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(world)) return UEC_RESULT_INVALID_HANDLE;
        *outInstance = world->PIEInstance;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetWorldNetMode(uec_world* rawWorld, uec_net_mode* outMode)
    {
        if (outMode == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        switch (world->GetNetMode())
        {
        case NM_Standalone: *outMode = UEC_NET_MODE_STANDALONE; break;
        case NM_DedicatedServer: *outMode = UEC_NET_MODE_DEDICATED_SERVER; break;
        case NM_ListenServer: *outMode = UEC_NET_MODE_LISTEN_SERVER; break;
        case NM_Client: *outMode = UEC_NET_MODE_CLIENT; break;
        default: *outMode = UEC_NET_MODE_UNKNOWN; break;
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetWorldName(uec_world* rawWorld,
                                     char* buffer,
                                     size_t bufferSize,
                                     size_t* requiredSize)
    {
        auto* handle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(handle)) return UEC_RESULT_INVALID_HANDLE;
        UWorld* world = handle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return CopyFStringToUtf8(world->GetMapName(), buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL TravelWorld(uec_world* rawWorld, uec_string_view levelPath)
    {
        auto* handle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = handle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const FString path = ToFString(levelPath);
        if (path.IsEmpty()) return UEC_RESULT_INVALID_ARGUMENT;
        UGameplayStatics::OpenLevel(world, FName(*path));
        return UEC_RESULT_OK;
    }

    static FUECActor* MakeActorHandle(AActor* actor)
    {
        if (actor == nullptr) return nullptr;
        auto* handle = new FUECActor();
        handle->Value = actor;
        {
            FScopeLock lock(&GHandleMutex);
            GActors.Add(handle);
        }
        return handle;
    }

    static FUECObject* MakeObjectHandle(UObject* object)
    {
        if (object == nullptr) return nullptr;
        auto* handle = new FUECObject();
        handle->Value = object;
        {
            FScopeLock lock(&GHandleMutex);
            GObjects.Add(handle);
        }
        return handle;
    }

    static FUECObject* MakeRetainedObjectHandle(UObject* object)
    {
        FUECObject* handle = MakeObjectHandle(object);
        if (handle != nullptr) handle->StrongValue = TStrongObjectPtr<UObject>(object);
        return handle;
    }

    uec_result UEC_CALL GetWorldGameInstance(uec_world* rawWorld,
                                             uec_object** outGameInstance)
    {
        if (outGameInstance == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outGameInstance = nullptr;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UGameInstance* gameInstance = world->GetGameInstance();
        if (gameInstance == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FUECObject* handle = MakeObjectHandle(gameInstance);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outGameInstance = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetFirstPlayerController(uec_world* rawWorld, uec_actor** outController)
    {
        if (outController == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outController = nullptr;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        APlayerController* controller = UGameplayStatics::GetPlayerController(world, 0);
        FUECActor* handle = MakeActorHandle(controller);
        if (handle == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        *outController = reinterpret_cast<uec_actor*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetPlayerController(uec_world* rawWorld,
                                            uint32_t playerIndex,
                                            uec_actor** outController)
    {
        if (outController == nullptr || playerIndex > static_cast<uint32>(INT32_MAX)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outController = nullptr;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        APlayerController* controller = UGameplayStatics::GetPlayerController(
            world, static_cast<int32>(playerIndex));
        FUECActor* handle = MakeActorHandle(controller);
        if (handle == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        *outController = reinterpret_cast<uec_actor*>(handle);
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
        if (!IsValidStringView(classPath) || !IsFiniteTransform(*transform)) return UEC_RESULT_INVALID_ARGUMENT;
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
        if (transform == nullptr || !IsFiniteTransform(*transform)) return UEC_RESULT_INVALID_ARGUMENT;
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
        if (transform == nullptr || !IsFiniteTransform(*transform)) return UEC_RESULT_INVALID_ARGUMENT;
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

    uec_result UEC_CALL SubscribeWorldTick(uec_world* rawWorld,
                                           uec_tick_callback callback,
                                           void* userData,
                                           uint64_t* outSubscriptionId)
    {
        if (callback == nullptr || outSubscriptionId == nullptr)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;

        const uint64 subscriptionId = GNextTickSubscriptionId++;
        auto subscription = MakeShared<FUECTickSubscription>();
        subscription->Id = subscriptionId;
        subscription->World = world;
        subscription->Callback = callback;
        subscription->UserData = userData;
        TWeakPtr<FUECTickSubscription> weakSubscription = subscription;
        subscription->Handle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([weakSubscription](float deltaSeconds)
            {
                TSharedPtr<FUECTickSubscription> current = weakSubscription.Pin();
                if (!current.IsValid() || current->Cancelled || IsShuttingDown())
                {
                    return false;
                }
                if (current->World.Get() == nullptr)
                {
                    GTickSubscriptions.Remove(current->Id);
                    return false;
                }
                current->InCallback = true;
                current->Callback(current->Id, static_cast<double>(deltaSeconds), current->UserData);
                current->InCallback = false;
                if (current->Cancelled || IsShuttingDown())
                {
                    GTickSubscriptions.Remove(current->Id);
                    return false;
                }
                return true;
            }),
            0.0f);
        GTickSubscriptions.Add(subscriptionId, subscription);
        *outSubscriptionId = subscriptionId;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL UnsubscribeWorldTick(uec_context* rawContext,
                                             uint64_t subscriptionId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECTickSubscription>* subscriptionPtr = GTickSubscriptions.Find(subscriptionId);
        if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid())
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        TSharedPtr<FUECTickSubscription> subscription = *subscriptionPtr;
        subscription->Cancelled = true;
        if (!subscription->InCallback)
        {
            FTSTicker::RemoveTicker(subscription->Handle);
        }
        GTickSubscriptions.Remove(subscriptionId);
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

    static void ClearAllTickSubscriptions()
    {
        for (const TPair<uint64, TSharedPtr<FUECTickSubscription>>& pair : GTickSubscriptions)
        {
            if (pair.Value.IsValid())
            {
                pair.Value->Cancelled = true;
                FTSTicker::RemoveTicker(pair.Value->Handle);
            }
        }
        GTickSubscriptions.Empty();
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

    uec_result UEC_CALL GetActorCountByClass(uec_world* rawWorld,
                                             uec_string_view classPath,
                                             uint32_t* outCount)
    {
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(classPath) || classPath.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UClass* actorClass = LoadClass<AActor>(nullptr, *ToFString(classPath));
        if (actorClass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outCount = 0;
        for (TActorIterator<AActor> iterator(world); iterator; ++iterator)
        {
            if (iterator->IsA(actorClass))
            {
                if (*outCount == UINT32_MAX) return UEC_RESULT_INTERNAL_ERROR;
                ++(*outCount);
            }
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorAtByClass(uec_world* rawWorld,
                                          uec_string_view classPath,
                                          uint32_t index,
                                          uec_actor** outActor)
    {
        if (outActor == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outActor = nullptr;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(classPath) || classPath.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UClass* actorClass = LoadClass<AActor>(nullptr, *ToFString(classPath));
        if (actorClass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        uint32_t current = 0;
        for (TActorIterator<AActor> iterator(world); iterator; ++iterator)
        {
            AActor* actor = *iterator;
            if (!actor->IsA(actorClass)) continue;
            if (current++ != index) continue;
            FUECActor* handle = MakeActorHandle(actor);
            if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
            *outActor = reinterpret_cast<uec_actor*>(handle);
            return UEC_RESULT_OK;
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }
