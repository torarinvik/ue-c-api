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
            UEC_CAPABILITY_REFLECTION_CONTAINERS;
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
        if (!IsValidContext(rawContext))
        {
            return UEC_RESULT_INVALID_HANDLE;
        }
        TombstoneHandle(context->Header);
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
        for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
        {
            UWorld* world = worldContext.World();
            if (world != nullptr && (worldContext.WorldType == EWorldType::Game || worldContext.WorldType == EWorldType::PIE))
            {
                FUECWorld* handle = MakeWorldHandle(
                    world, worldContext.WorldType, worldContext.PIEInstance);
                if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
                *outWorld = reinterpret_cast<uec_world*>(handle);
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_NOT_INITIALIZED;
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
            if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
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
            if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
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
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
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
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outGameState = reinterpret_cast<uec_object*>(handle);
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
        for (const FUECWorld* candidate : GWorlds)
        {
            if (candidate == nullptr || candidate->Value.Get() != world) continue;
            auto* mutableCandidate = const_cast<FUECWorld*>(candidate);
            TombstoneHandle(mutableCandidate->Header);
            mutableCandidate->Value.Reset();
        }
        for (const FUECActor* candidate : GActors)
        {
            AActor* actor = candidate == nullptr ? nullptr : candidate->Value.Get();
            if (actor == nullptr || actor->GetWorld() != world) continue;
            CancelActorSubscriptions(actor);
            auto* mutableCandidate = const_cast<FUECActor*>(candidate);
            TombstoneHandle(mutableCandidate->Header);
            mutableCandidate->Value.Reset();
        }
        for (const FUECSceneComponent* candidate : GComponents)
        {
            USceneComponent* component = candidate == nullptr ? nullptr : candidate->Value.Get();
            if (component == nullptr || component->GetWorld() != world) continue;
            auto* mutableCandidate = const_cast<FUECSceneComponent*>(candidate);
            TombstoneHandle(mutableCandidate->Header);
            mutableCandidate->Value.Reset();
        }
        if (GEngine == nullptr) return;
        for (const FUECObject* candidate : GObjects)
        {
            UObject* object = candidate == nullptr ? nullptr : candidate->Value.Get();
            if (object == nullptr || GEngine->GetWorldFromContextObject(
                object, EGetWorldErrorMode::ReturnNull) != world) continue;
            auto* mutableCandidate = const_cast<FUECObject*>(candidate);
            TombstoneHandle(mutableCandidate->Header);
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
        CancelStreamingRequestsFor(world);
        InvalidateWorldHandles(world);
    }

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
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
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
        FUECActor* handle = MakeActorHandle(controller);
        if (handle == nullptr) return UEC_RESULT_NOT_INITIALIZED;
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
        FUECActor* handle = MakeActorHandle(controller);
        if (handle == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        *outController = reinterpret_cast<uec_actor*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ReleaseWorld(uec_world* rawWorld)
    {
        auto* world = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(world)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TombstoneHandle(world->Header);
        world->Value.Reset();
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetTimer(uec_world* rawWorld,
                                 double intervalSeconds,
                                 uec_bool looping,
                                 uec_timer_callback callback,
                                 void* userData,
                                 uint64_t* outTimerId)
    {
        if (outTimerId != nullptr) *outTimerId = 0;
        if (outTimerId == nullptr || callback == nullptr || !IsValidBool(looping)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsRepresentableFloat(intervalSeconds) || intervalSeconds <= 0.0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (GTimers.Num() >= MaxSubscriptions) return UEC_RESULT_QUEUE_FULL;

        uint64 timerId = 0;
        if (!AllocateMonotonicId(GNextTimerId, timerId)) return UEC_RESULT_INTERNAL_ERROR;
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
            if (!current.IsValid() || current->Cancelled || current->Callback == nullptr ||
                IsShuttingDown()) return;
            if (current->World.Get() == nullptr)
            {
                GTimers.Remove(current->Id);
                return;
            }
            FUECCallbackScope callbackScope;
            current->Callback(current->Id, current->UserData);
            if (!current->Looping)
            {
                GTimers.Remove(current->Id);
            }
        });
        world->GetTimerManager().SetTimer(state->Handle, delegate, intervalSeconds, state->Looping);
        if (IsShuttingDown())
        {
            world->GetTimerManager().ClearTimer(state->Handle);
            state->Cancelled = true;
            return UEC_RESULT_SHUTTING_DOWN;
        }
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
        if (outSubscriptionId != nullptr) *outSubscriptionId = 0;
        if (callback == nullptr || outSubscriptionId == nullptr)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (GTickSubscriptions.Num() >= MaxSubscriptions) return UEC_RESULT_QUEUE_FULL;

        uint64 subscriptionId = 0;
        if (!AllocateMonotonicId(GNextTickSubscriptionId, subscriptionId)) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
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
                FUECCallbackScope callbackScope;
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
        if (IsShuttingDown())
        {
            FTSTicker::RemoveTicker(subscription->Handle);
            subscription->Cancelled = true;
            return UEC_RESULT_SHUTTING_DOWN;
        }
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
#include "uec_api_streaming.inl"
