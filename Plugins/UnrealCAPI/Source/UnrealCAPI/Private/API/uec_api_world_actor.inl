    uec_result UEC_CALL GetCapabilities(uec_context* rawContext, uec_capabilities* outCapabilities)
    {
        if (outCapabilities != nullptr) *outCapabilities = 0;
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
            UEC_CAPABILITY_PRESENTATION | UEC_CAPABILITY_RETAINED_OBJECTS |
            UEC_CAPABILITY_CONFIGURATION | UEC_CAPABILITY_STREAMING |
            UEC_CAPABILITY_REFLECTION_CONTAINERS | UEC_CAPABILITY_COLLISION_DETAILS |
            UEC_CAPABILITY_EVENT_BRIDGE | UEC_CAPABILITY_ASYNC_LATENT_FUNCTIONS;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetLastError(uec_context* rawContext,
                                     char* buffer,
                                     size_t bufferSize,
                                     size_t* requiredSize)
    {
        if (requiredSize != nullptr) *requiredSize = 0;
        if (requiredSize == nullptr)
        {
            SetLastErrorMessage(TEXT("Required-size output is null"));
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        return CopyLastErrorMessage(buffer, bufferSize, requiredSize);
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
        FScopeLock lock(&GHandleMutex);
        if (GShuttingDown || !IsValidContextNoLock(context)) {
            SetLastErrorMessage(TEXT("Invalid or stale context handle"));
            return UEC_RESULT_INVALID_HANDLE;
        }
        context->Header.bReleased = true;
        return UEC_RESULT_OK;
    }

    static FUECWorld* MakeWorldHandle(UWorld* world,
                                      EWorldType::Type worldType,
                                      int32 pieInstance)
    {
        if (world == nullptr) return nullptr;
        auto* handle = new FUECWorld();
        if (!InitializeHandle(handle->Header, EUECHandleKind::World))
        {
            delete handle;
            return nullptr;
        }
        handle->Value = world;
        handle->Kind = ToWorldKind(worldType);
        handle->PIEInstance = pieInstance;
        {
            FScopeLock lock(&GHandleMutex);
            if (GShuttingDown)
            {
                delete handle;
                return nullptr;
            }
            GWorlds.Add(handle);
        }
        return handle;
    }

    uec_result UEC_CALL GetDefaultWorld(uec_context* rawContext, uec_world** outWorld)
    {
        if (outWorld != nullptr) *outWorld = nullptr;
        if (outWorld == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (GEngine == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        UWorld* activeWorld = nullptr;
        EWorldType::Type activeWorldType = EWorldType::None;
        int32 activePIEInstance = INDEX_NONE;
        for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
        {
            UWorld* world = worldContext.World();
            if (world == nullptr ||
                (worldContext.WorldType != EWorldType::Game &&
                 worldContext.WorldType != EWorldType::PIE)) {
                continue;
            }
            if (activeWorld != nullptr) {
                SetLastErrorMessage(TEXT(
                    "Multiple active Game/PIE worlds; select a world explicitly by kind and index"));
                return UEC_RESULT_AMBIGUOUS_CONTEXT;
            }
            activeWorld = world;
            activeWorldType = worldContext.WorldType;
            activePIEInstance = worldContext.PIEInstance;
        }
        if (activeWorld == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FUECWorld* handle = MakeWorldHandle(
            activeWorld, activeWorldType, activePIEInstance);
        if (handle == nullptr) return HandleCreationFailureResult();
        *outWorld = reinterpret_cast<uec_world*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetWorldCount(uec_context* rawContext, uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (GEngine == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
        {
            if (worldContext.World() != nullptr &&
                (worldContext.WorldType == EWorldType::Game || worldContext.WorldType == EWorldType::PIE))
            {
                if (*outCount == UINT32_MAX) return UEC_RESULT_INTERNAL_ERROR;
                ++(*outCount);
            }
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetWorldAt(uec_context* rawContext, uint32_t index, uec_world** outWorld)
    {
        if (outWorld != nullptr) *outWorld = nullptr;
        if (outWorld == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
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
            FUECWorld* handle = MakeWorldHandle(
                world, worldContext.WorldType, worldContext.PIEInstance);
            if (handle == nullptr) return HandleCreationFailureResult();
            *outWorld = reinterpret_cast<uec_world*>(handle);
            return UEC_RESULT_OK;
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    static bool IsSupportedWorldKind(uec_world_kind kind)
    {
        return kind >= UEC_WORLD_KIND_GAME && kind <= UEC_WORLD_KIND_INACTIVE;
    }

    uec_result UEC_CALL GetWorldCountByKind(uec_context* rawContext,
                                            uec_world_kind kind,
                                            uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr || !IsSupportedWorldKind(kind)) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (GEngine == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
        {
            if (worldContext.World() != nullptr && ToWorldKind(worldContext.WorldType) == kind)
            {
                if (*outCount == UINT32_MAX) return UEC_RESULT_INTERNAL_ERROR;
                ++(*outCount);
            }
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetWorldAtByKind(uec_context* rawContext,
                                         uec_world_kind kind,
                                         uint32_t index,
                                         uec_world** outWorld)
    {
        if (outWorld != nullptr) *outWorld = nullptr;
        if (outWorld == nullptr || !IsSupportedWorldKind(kind)) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (GEngine == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        uint32_t current = 0;
        for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
        {
            UWorld* world = worldContext.World();
            if (world == nullptr || ToWorldKind(worldContext.WorldType) != kind) continue;
            if (current++ != index) continue;
            FUECWorld* handle = MakeWorldHandle(
                world, worldContext.WorldType, worldContext.PIEInstance);
            if (handle == nullptr) return HandleCreationFailureResult();
            *outWorld = reinterpret_cast<uec_world*>(handle);
            return UEC_RESULT_OK;
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    uec_result UEC_CALL GetWorldKind(uec_world* rawWorld, uec_world_kind* outKind)
    {
        if (outKind != nullptr) *outKind = UEC_WORLD_KIND_UNKNOWN;
        if (outKind == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* world = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(world)) return UEC_RESULT_INVALID_HANDLE;
        *outKind = world->Kind;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetWorldPIEInstance(uec_world* rawWorld, int32_t* outInstance)
    {
        if (outInstance != nullptr) *outInstance = -1;
        if (outInstance == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* world = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(world)) return UEC_RESULT_INVALID_HANDLE;
        *outInstance = world->PIEInstance;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetWorldNetMode(uec_world* rawWorld, uec_net_mode* outMode)
    {
        if (outMode != nullptr) *outMode = UEC_NET_MODE_UNKNOWN;
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

    uec_result UEC_CALL GetWorldHasAuthority(uec_world* rawWorld, uec_bool* outHasAuthority)
    {
        if (outHasAuthority != nullptr) *outHasAuthority = UEC_FALSE;
        if (outHasAuthority == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        *outHasAuthority = world->GetNetMode() == NM_Client ? UEC_FALSE : UEC_TRUE;
        return UEC_RESULT_OK;
    }

    static FUECObject* MakeObjectHandle(UObject* object);

    uec_result UEC_CALL GetWorldGameMode(uec_world* rawWorld, uec_object** outGameMode)
    {
        if (outGameMode != nullptr) *outGameMode = nullptr;
        if (outGameMode == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        AGameModeBase* gameMode = world->GetAuthGameMode();
        if (gameMode == nullptr) return UEC_RESULT_UNSUPPORTED;
        FUECObject* handle = MakeObjectHandle(gameMode);
        if (handle == nullptr) return HandleCreationFailureResult();
        *outGameMode = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetWorldGameState(uec_world* rawWorld, uec_object** outGameState)
    {
        if (outGameState != nullptr) *outGameState = nullptr;
        if (outGameState == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        AGameStateBase* gameState = world->GetGameState();
        if (gameState == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FUECObject* handle = MakeObjectHandle(gameState);
        if (handle == nullptr) return HandleCreationFailureResult();
        *outGameState = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }


    uec_result UEC_CALL GetControllerLocalPlayer(
        uec_actor* rawController,
        uec_object** outLocalPlayer)
    {
        if (outLocalPlayer != nullptr) *outLocalPlayer = nullptr;
        if (outLocalPlayer == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* controllerHandle = reinterpret_cast<FUECActor*>(rawController);
        if (!IsValidActor(controllerHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        APlayerController* controller = Cast<APlayerController>(controllerHandle->Value.Get());
        if (controller == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        ULocalPlayer* localPlayer = controller->GetLocalPlayer();
        if (localPlayer == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        UWorld* controllerWorld = controller->GetWorld();
        if (controllerWorld == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        if (localPlayer->GetWorld() != controllerWorld)
            return UEC_RESULT_INVALID_ARGUMENT;
        FUECObject* handle = MakeObjectHandle(localPlayer);
        if (handle == nullptr) return HandleCreationFailureResult();
        *outLocalPlayer = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    static void CancelTimersFor(UWorld* world)
    {
        if (world == nullptr) return;
        TArray<uint64> timerIds;
        for (const TPair<uint64, TSharedPtr<FUECTimerState>>& pair : GTimers)
        {
            if (pair.Value.IsValid() && pair.Value->World.Get() == world) timerIds.Add(pair.Key);
        }
        for (uint64 timerId : timerIds)
        {
            TSharedPtr<FUECTimerState>* statePtr = GTimers.Find(timerId);
            if (statePtr == nullptr || !statePtr->IsValid()) continue;
            TSharedPtr<FUECTimerState> state = *statePtr;
            world->GetTimerManager().ClearTimer(state->Handle);
            state->Cancelled = true;
            GTimers.Remove(timerId);
        }
    }

    static void CancelTickSubscriptionsFor(UWorld* world)
    {
        if (world == nullptr) return;
        TArray<uint64> subscriptionIds;
        for (const TPair<uint64, TSharedPtr<FUECTickSubscription>>& pair : GTickSubscriptions)
        {
            if (pair.Value.IsValid() && pair.Value->World.Get() == world) {
                subscriptionIds.Add(pair.Key);
            }
        }
        for (uint64 subscriptionId : subscriptionIds)
        {
            TSharedPtr<FUECTickSubscription>* subscriptionPtr =
                GTickSubscriptions.Find(subscriptionId);
            if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid()) continue;
            TSharedPtr<FUECTickSubscription> subscription = *subscriptionPtr;
            subscription->Cancelled = true;
            if (!subscription->InCallback) FTSTicker::RemoveTicker(subscription->Handle);
            GTickSubscriptions.Remove(subscriptionId);
        }
    }

    static void InvalidateWorldHandles(UWorld* world)
    {
        if (world == nullptr) return;
        CancelActorSubscriptionsForWorld(world);
        // Invalidate access without consuming the consumer's right to release each handle.
        for (const FUECWorld* candidate : GWorlds)
        {
            if (candidate == nullptr || candidate->Value.Get() != world) continue;
            auto* mutableCandidate = const_cast<FUECWorld*>(candidate);
            InvalidateHandle(mutableCandidate->Header);
            mutableCandidate->Value.Reset();
        }
        for (const FUECActor* candidate : GActors)
        {
            AActor* actor = candidate == nullptr ? nullptr : candidate->Value.Get();
            if (actor == nullptr || actor->GetWorld() != world) continue;
            CancelActorSubscriptions(actor);
            auto* mutableCandidate = const_cast<FUECActor*>(candidate);
            InvalidateHandle(mutableCandidate->Header);
            mutableCandidate->Value.Reset();
        }
        for (const FUECSceneComponent* candidate : GComponents)
        {
            USceneComponent* component = candidate == nullptr ? nullptr : candidate->Value.Get();
            if (component == nullptr || component->GetWorld() != world) continue;
            auto* mutableCandidate = const_cast<FUECSceneComponent*>(candidate);
            InvalidateHandle(mutableCandidate->Header);
            mutableCandidate->Value.Reset();
        }
        if (GEngine == nullptr) return;
        for (const FUECObject* candidate : GObjects)
        {
            UObject* object = candidate == nullptr ? nullptr : candidate->Value.Get();
            if (object == nullptr || GEngine->GetWorldFromContextObject(
                object, EGetWorldErrorMode::ReturnNull) != world) continue;
            auto* mutableCandidate = const_cast<FUECObject*>(candidate);
            InvalidateHandle(mutableCandidate->Header);
            mutableCandidate->Value.Reset();
            mutableCandidate->StrongValue.Reset();
        }
    }

    static void HandleWorldCleanup(UWorld* world, bool, bool)
    {
        if (IsShuttingDown() || world == nullptr) return;
        RemoveActorDestroyedHandler(world);
        CancelTimersFor(world);
        CancelTickSubscriptionsFor(world);
        CancelAudioSubscriptionsForWorld(world);
        CancelWidgetSubscriptionsForWorld(world);
        CancelAnimationSubscriptionsForWorld(world);
        CancelStreamingRequestsFor(world);
        InvalidateWorldHandles(world);
    }

#if WITH_EDITOR
    static void HandleObjectsReplaced(
        const FCoreUObjectDelegates::FReplacementObjectMap& replacements)
    {
        if (replacements.IsEmpty()) return;
        FScopeLock lock(&GHandleMutex);
        if (GShuttingDown) return;
        for (const FUECClass* candidate : GClasses)
        {
            auto* handle = const_cast<FUECClass*>(candidate);
            if (handle != nullptr && !handle->Header.bReleased &&
                replacements.Contains(handle->Value.Get()))
            {
                handle->Header.bInvalidated = true;
            }
        }
    }
#endif

    static FUECActor* MakeActorHandle(AActor* actor)
    {
        if (actor == nullptr) return nullptr;
        auto* handle = new FUECActor();
        if (!InitializeHandle(handle->Header, EUECHandleKind::Actor))
        {
            delete handle;
            return nullptr;
        }
        handle->Value = actor;
        {
            FScopeLock lock(&GHandleMutex);
            if (GShuttingDown)
            {
                delete handle;
                return nullptr;
            }
            GActors.Add(handle);
        }
        return handle;
    }

    static FUECObject* MakeObjectHandle(UObject* object)
    {
        if (object == nullptr) return nullptr;
        auto* handle = new FUECObject();
        if (!InitializeHandle(handle->Header, EUECHandleKind::Object))
        {
            delete handle;
            return nullptr;
        }
        handle->Value = object;
        {
            FScopeLock lock(&GHandleMutex);
            if (GShuttingDown)
            {
                delete handle;
                return nullptr;
            }
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
        if (outGameInstance != nullptr) *outGameInstance = nullptr;
        if (outGameInstance == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UGameInstance* gameInstance = world->GetGameInstance();
        if (gameInstance == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FUECObject* handle = MakeObjectHandle(gameInstance);
        if (handle == nullptr) return HandleCreationFailureResult();
        *outGameInstance = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetFirstPlayerController(uec_world* rawWorld, uec_actor** outController)
    {
        if (outController != nullptr) *outController = nullptr;
        if (outController == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        APlayerController* controller = UGameplayStatics::GetPlayerController(world, 0);
        if (controller == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FUECActor* handle = MakeActorHandle(controller);
        if (handle == nullptr) return HandleCreationFailureResult();
        *outController = reinterpret_cast<uec_actor*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetPlayerController(uec_world* rawWorld,
                                            uint32_t playerIndex,
                                            uec_actor** outController)
    {
        if (outController != nullptr) *outController = nullptr;
        if (outController == nullptr || playerIndex > static_cast<uint32>(INT32_MAX)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        APlayerController* controller = UGameplayStatics::GetPlayerController(
            world, static_cast<int32>(playerIndex));
        if (controller == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FUECActor* handle = MakeActorHandle(controller);
        if (handle == nullptr) return HandleCreationFailureResult();
        *outController = reinterpret_cast<uec_actor*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ReleaseWorld(uec_world* rawWorld)
    {
        auto* world = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsRegisteredHandle(world, GWorlds, EUECHandleKind::World,
                                TEXT("Invalid or stale world handle"))) {
            return UEC_RESULT_INVALID_HANDLE;
        }
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TombstoneHandle(world->Header);
        world->Value.Reset();
        return UEC_RESULT_OK;
    }

#include "uec_api_streaming.inl"
