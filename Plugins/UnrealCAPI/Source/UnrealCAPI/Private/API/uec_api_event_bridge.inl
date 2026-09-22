/* Blueprint event bridge components and their C callback subscriptions. */

    static UECEventBridgeComponent* FindActorEventBridgeComponent(AActor* actor)
    {
        if (actor == nullptr) return nullptr;
        return Cast<UECEventBridgeComponent>(
            actor->GetComponentByClass(UECEventBridgeComponent::StaticClass()));
    }

    static UECEventBridgeComponent* CreateActorEventBridgeComponent(AActor* actor)
    {
        if (actor == nullptr || actor->GetWorld() == nullptr) return nullptr;
        UECEventBridgeComponent* component = NewObject<UECEventBridgeComponent>(actor);
        if (component == nullptr) return nullptr;
        actor->AddInstanceComponent(component);
        component->RegisterComponent();
        if (!component->IsRegistered() || component->GetOwner() != actor)
        {
            component->DestroyComponent();
            return nullptr;
        }
        return component;
    }

    uec_result UEC_CALL GetOrCreateActorEventBridge(uec_actor* rawActor,
                                                     uec_object** outBridge)
    {
        if (outBridge != nullptr) *outBridge = nullptr;
        if (outBridge == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr || actor->GetWorld() == nullptr) return UEC_RESULT_INVALID_HANDLE;

        UECEventBridgeComponent* component = FindActorEventBridgeComponent(actor);
        bool created = false;
        if (component == nullptr)
        {
            const uec_result authorityResult = RequireWorldAuthority(actor->GetWorld());
            if (authorityResult != UEC_RESULT_OK) return authorityResult;
            component = CreateActorEventBridgeComponent(actor);
            if (component == nullptr) return UEC_RESULT_INTERNAL_ERROR;
            created = true;
        }
        FUECObject* handle = MakeObjectHandle(component);
        if (handle == nullptr)
        {
            if (created) component->DestroyComponent();
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outBridge = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL DestroyActorEventBridge(uec_object* rawBridge)
    {
        auto* bridgeHandle = reinterpret_cast<FUECObject*>(rawBridge);
        if (!IsValidObject(bridgeHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UECEventBridgeComponent* component = Cast<UECEventBridgeComponent>(bridgeHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        AActor* actor = component->GetOwner();
        if (actor == nullptr || actor->GetWorld() == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const uec_result authorityResult = RequireWorldAuthority(actor->GetWorld());
        if (authorityResult != UEC_RESULT_OK) return authorityResult;

        for (const FUECObject* candidate : GObjects)
        {
            if (candidate == nullptr || candidate->Value.Get() != component) continue;
            auto* mutableCandidate = const_cast<FUECObject*>(candidate);
            TombstoneHandle(mutableCandidate->Header);
            mutableCandidate->Value.Reset();
            mutableCandidate->StrongValue.Reset();
        }
        component->DestroyComponent();
        return UEC_RESULT_OK;
    }

    static void RemoveEventBridgeSubscription(
        const TSharedPtr<FUECEventBridgeSubscription>& subscription)
    {
        if (!subscription.IsValid()) return;
        subscription->Cancelled = true;
        if (!subscription->InCallback)
        {
            if (UECEventBridgeComponent* component = subscription->Component.Get()) {
                component->GetNativeEvent().Remove(subscription->Handle);
                component->GetNativeDestroyedEvent().Remove(subscription->DestroyedHandle);
            }
        }
        GEventBridgeSubscriptions.Remove(subscription->Id);
    }

    uec_result UEC_CALL BindActorEventBridge(uec_object* rawBridge,
                                              uec_event_bridge_callback callback,
                                              void* userData,
                                              uint64_t* outSubscriptionId)
    {
        if (outSubscriptionId != nullptr) *outSubscriptionId = 0;
        if (callback == nullptr || outSubscriptionId == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* bridgeHandle = reinterpret_cast<FUECObject*>(rawBridge);
        if (!IsValidObject(bridgeHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UECEventBridgeComponent* component = Cast<UECEventBridgeComponent>(bridgeHandle->Value.Get());
        if (component == nullptr || component->GetOwner() == nullptr || component->GetWorld() == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (GEventBridgeSubscriptions.Num() >= MaxSubscriptions) return UEC_RESULT_QUEUE_FULL;

        uint64 id = 0;
        if (!AllocateMonotonicId(GNextEventBridgeSubscriptionId, id)) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        auto subscription = MakeShared<FUECEventBridgeSubscription>();
        subscription->Id = id;
        subscription->Component = component;
        subscription->Callback = callback;
        subscription->UserData = userData;
        TWeakPtr<FUECEventBridgeSubscription> weakSubscription = subscription;
        subscription->Handle = component->GetNativeEvent().AddLambda(
            [weakSubscription](int64 eventId, int64 integerValue, double realValue,
                               const FString& textValue)
            {
                TSharedPtr<FUECEventBridgeSubscription> current = weakSubscription.Pin();
                if (!current.IsValid() || current->Cancelled || IsShuttingDown()) return;
                current->InCallback = true;
                FTCHARToUTF8 converted(*textValue);
                const uec_string_view text = {
                    converted.Get(), static_cast<size_t>(converted.Length())};
                FUECCallbackScope callbackScope;
                current->Callback(current->Id, static_cast<int64>(eventId),
                                  static_cast<int64>(integerValue), realValue,
                                  text, current->UserData);
                current->InCallback = false;
                if (current->Cancelled || IsShuttingDown()) {
                    if (UECEventBridgeComponent* component = current->Component.Get()) {
                        component->GetNativeEvent().Remove(current->Handle);
                        component->GetNativeDestroyedEvent().Remove(current->DestroyedHandle);
                    }
                    GEventBridgeSubscriptions.Remove(current->Id);
                }
            });
        subscription->DestroyedHandle = component->GetNativeDestroyedEvent().AddLambda(
            [weakSubscription](UECEventBridgeComponent*)
            {
                TSharedPtr<FUECEventBridgeSubscription> current = weakSubscription.Pin();
                if (current.IsValid()) RemoveEventBridgeSubscription(current);
            });
        if (IsShuttingDown())
        {
            component->GetNativeEvent().Remove(subscription->Handle);
            component->GetNativeDestroyedEvent().Remove(subscription->DestroyedHandle);
            subscription->Cancelled = true;
            return UEC_RESULT_SHUTTING_DOWN;
        }
        GEventBridgeSubscriptions.Add(id, subscription);
        *outSubscriptionId = id;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL UnbindActorEventBridge(uec_context* rawContext,
                                                uint64_t subscriptionId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECEventBridgeSubscription>* subscription =
            GEventBridgeSubscriptions.Find(subscriptionId);
        if (subscription == nullptr || !subscription->IsValid()) return UEC_RESULT_INVALID_ARGUMENT;
        RemoveEventBridgeSubscription(*subscription);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL EmitActorEventBridge(uec_object* rawBridge,
                                              int64_t eventId,
                                              int64_t integerValue,
                                              double realValue,
                                              uec_string_view textValue)
    {
        auto* bridgeHandle = reinterpret_cast<FUECObject*>(rawBridge);
        if (!IsValidObject(bridgeHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!FMath::IsFinite(realValue) || !IsValidStringView(textValue)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UECEventBridgeComponent* component = Cast<UECEventBridgeComponent>(bridgeHandle->Value.Get());
        if (component == nullptr || component->GetOwner() == nullptr) return UEC_RESULT_INVALID_HANDLE;
        component->EmitEvent(eventId, integerValue, realValue,
                             ToFString(textValue));
        return UEC_RESULT_OK;
    }

    static void CancelEventBridgeSubscriptions(AActor* actor)
    {
        if (actor == nullptr) return;
        TArray<uint64> subscriptionIds;
        for (const TPair<uint64, TSharedPtr<FUECEventBridgeSubscription>>& pair :
             GEventBridgeSubscriptions)
        {
            if (!pair.Value.IsValid()) continue;
            UECEventBridgeComponent* component = pair.Value->Component.Get();
            if (component != nullptr && component->GetOwner() == actor) {
                subscriptionIds.Add(pair.Key);
            }
        }
        for (uint64 id : subscriptionIds)
        {
            TSharedPtr<FUECEventBridgeSubscription>* subscription =
                GEventBridgeSubscriptions.Find(id);
            if (subscription != nullptr) RemoveEventBridgeSubscription(*subscription);
        }
    }

    static void CancelEventBridgeSubscriptionsForWorld(UWorld* world)
    {
        if (world == nullptr) return;
        TArray<uint64> subscriptionIds;
        for (const TPair<uint64, TSharedPtr<FUECEventBridgeSubscription>>& pair :
             GEventBridgeSubscriptions)
        {
            if (!pair.Value.IsValid()) continue;
            UECEventBridgeComponent* component = pair.Value->Component.Get();
            if (component != nullptr && component->GetWorld() == world) {
                subscriptionIds.Add(pair.Key);
            }
        }
        for (uint64 id : subscriptionIds)
        {
            TSharedPtr<FUECEventBridgeSubscription>* subscription =
                GEventBridgeSubscriptions.Find(id);
            if (subscription != nullptr) RemoveEventBridgeSubscription(*subscription);
        }
    }

    static void ClearAllEventBridgeSubscriptions()
    {
        TArray<uint64> subscriptionIds;
        GEventBridgeSubscriptions.GetKeys(subscriptionIds);
        for (uint64 id : subscriptionIds)
        {
            TSharedPtr<FUECEventBridgeSubscription>* subscription =
                GEventBridgeSubscriptions.Find(id);
            if (subscription != nullptr) RemoveEventBridgeSubscription(*subscription);
        }
        GEventBridgeSubscriptions.Empty();
    }

    uec_result UEC_CALL BindActorDestroyed(uec_actor* rawActor, uec_actor_destroyed_callback callback, void* userData, uint64_t* outSubscriptionId)
    {
        if (outSubscriptionId != nullptr) *outSubscriptionId = 0;
        if (callback == nullptr || outSubscriptionId == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr || actor->GetWorld() == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (GActorDestroyedSubscriptions.Num() >= MaxSubscriptions) return UEC_RESULT_QUEUE_FULL;
        EnsureActorDestroyedHandler(actor->GetWorld());
        if (!GActorDestroyedHandlers.Contains(actor->GetWorld())) return UEC_RESULT_INTERNAL_ERROR;
        uint64 id = 0;
        if (!AllocateMonotonicId(GNextActorDestroyedSubscriptionId, id)) return UEC_RESULT_INTERNAL_ERROR;
        auto subscription = MakeShared<FUECActorDestroyedSubscription>();
        subscription->Id = id; subscription->Actor = actor; subscription->Callback = callback; subscription->UserData = userData;
        GActorDestroyedSubscriptions.Add(id, subscription); *outSubscriptionId = id;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL UnbindActorDestroyed(uec_context* rawContext, uint64_t subscriptionId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECActorDestroyedSubscription>* subscription = GActorDestroyedSubscriptions.Find(subscriptionId);
        if (subscription == nullptr || !subscription->IsValid()) return UEC_RESULT_INVALID_ARGUMENT;
        (*subscription)->Cancelled = true; GActorDestroyedSubscriptions.Remove(subscriptionId);
        return UEC_RESULT_OK;
    }

    static void ClearAllActorDestroyedSubscriptions()
    {
        for (const TPair<uint64, TSharedPtr<FUECActorDestroyedSubscription>>& pair : GActorDestroyedSubscriptions)
            if (pair.Value.IsValid()) pair.Value->Cancelled = true;
        GActorDestroyedSubscriptions.Empty();
    }


    static void InvalidateDestroyedActorHandles(AActor* actor)
    {
        if (actor == nullptr) return;
        for (const FUECActor* candidate : GActors)
        {
            if (candidate == nullptr || candidate->Value.Get() != actor) continue;
            auto* mutableCandidate = const_cast<FUECActor*>(candidate);
            TombstoneHandle(mutableCandidate->Header);
            mutableCandidate->Value.Reset();
        }
        for (const FUECSceneComponent* candidate : GComponents)
        {
            USceneComponent* component = candidate == nullptr ? nullptr : candidate->Value.Get();
            if (component == nullptr || component->GetOwner() != actor) continue;
            auto* mutableCandidate = const_cast<FUECSceneComponent*>(candidate);
            TombstoneHandle(mutableCandidate->Header);
            mutableCandidate->Value.Reset();
        }
        for (const FUECObject* candidate : GObjects)
        {
            UObject* object = candidate == nullptr ? nullptr : candidate->Value.Get();
            UActorComponent* component = object == nullptr ? nullptr : Cast<UActorComponent>(object);
            if (object != actor && (component == nullptr || component->GetOwner() != actor)) continue;
            auto* mutableCandidate = const_cast<FUECObject*>(candidate);
            TombstoneHandle(mutableCandidate->Header);
            mutableCandidate->Value.Reset();
            mutableCandidate->StrongValue.Reset();
        }
    }

    static void HandleActorDestroyed(AActor* actor)
    {
        if (actor == nullptr) return;
        TArray<TSharedPtr<FUECActorDestroyedSubscription>> completed;
        TArray<uint64> completedIds;
        for (const TPair<uint64, TSharedPtr<FUECActorDestroyedSubscription>>& pair : GActorDestroyedSubscriptions)
        {
            if (pair.Value.IsValid() && pair.Value->Actor.Get() == actor) { completedIds.Add(pair.Key); completed.Add(pair.Value); }
        }
        for (uint64 id : completedIds) GActorDestroyedSubscriptions.Remove(id);
        for (const TSharedPtr<FUECActorDestroyedSubscription>& subscription : completed)
        {
            if (!subscription.IsValid() || subscription->Cancelled || subscription->Callback == nullptr) continue;
            subscription->InCallback = true; FUECCallbackScope callbackScope;
            subscription->Callback(subscription->Id, subscription->UserData);
            subscription->InCallback = false; subscription->Cancelled = true;
        }
        CancelActorSubscriptions(actor);
        InvalidateDestroyedActorHandles(actor);
    }

    static void EnsureActorDestroyedHandler(UWorld* world)
    {
        if (world == nullptr || GActorDestroyedHandlers.Contains(world)) return;
        const FDelegateHandle handler = world->AddOnActorDestroyedHandler(
            FOnActorDestroyed::FDelegate::CreateStatic(&HandleActorDestroyed));
        if (handler.IsValid()) GActorDestroyedHandlers.Add(world, handler);
    }

    static void RemoveActorDestroyedHandler(UWorld* world)
    {
        if (world == nullptr) return;
        FDelegateHandle* handler = GActorDestroyedHandlers.Find(world);
        if (handler == nullptr) return;
        world->RemoveOnActorDestroyedHandler(*handler);
        GActorDestroyedHandlers.Remove(world);
    }

    static void RemoveAllActorDestroyedHandlers()
    {
        for (const TPair<UWorld*, FDelegateHandle>& pair : GActorDestroyedHandlers)
        {
            if (pair.Key != nullptr) pair.Key->RemoveOnActorDestroyedHandler(pair.Value);
        }
        GActorDestroyedHandlers.Empty();
    }

    static void CancelActorSubscriptions(AActor* actor)
    {
        if (actor == nullptr) return;
        CancelLatentFunctionRequestsForActor(actor);
        TArray<uint64> collisionIds;
        for (const TPair<uint64, TSharedPtr<FUECCollisionSubscription>>& pair : GCollisionSubscriptions)
        {
            if (!pair.Value.IsValid()) continue;
            UPrimitiveComponent* component = pair.Value->Component.Get();
            if (component != nullptr && component->GetOwner() == actor) collisionIds.Add(pair.Key);
        }
        for (uint64 subscriptionId : collisionIds)
        {
            TSharedPtr<FUECCollisionSubscription>* subscriptionPtr =
                GCollisionSubscriptions.Find(subscriptionId);
            if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid()) continue;
            TSharedPtr<FUECCollisionSubscription> subscription = *subscriptionPtr;
            subscription->Cancelled = true;
            if (!subscription->InCallback)
            {
                if (UPrimitiveComponent* component = subscription->Component.Get())
                {
                    component->OnComponentHit().Remove(subscription->Handle);
                }
            }
            GCollisionSubscriptions.Remove(subscriptionId);
        }

        TArray<uint64> inputIds;
        for (const TPair<uint64, TSharedPtr<FUECInputBinding>>& pair : GInputBindings)
        {
            if (!pair.Value.IsValid()) continue;
            UEnhancedInputComponent* component = pair.Value->Component.Get();
            if (component != nullptr && component->GetOwner() == actor) inputIds.Add(pair.Key);
        }
        for (uint64 bindingId : inputIds)
        {
            TSharedPtr<FUECInputBinding>* bindingPtr = GInputBindings.Find(bindingId);
            if (bindingPtr == nullptr || !bindingPtr->IsValid()) continue;
            TSharedPtr<FUECInputBinding> binding = *bindingPtr;
            binding->Cancelled = true;
            if (!binding->InCallback)
            {
                if (UEnhancedInputComponent* component = binding->Component.Get())
                {
                    component->RemoveBindingByHandle(binding->EngineHandle);
                }
            }
            GInputBindings.Remove(bindingId);
        }
        CancelEventBridgeSubscriptions(actor);
    }

    static void CancelActorSubscriptionsForWorld(UWorld* world)
    {
        if (world == nullptr) return;
        CancelLatentFunctionRequestsForWorld(world);
        TSet<AActor*> actors;
        for (const TPair<uint64, TSharedPtr<FUECCollisionSubscription>>& pair : GCollisionSubscriptions)
        {
            if (!pair.Value.IsValid()) continue;
            UPrimitiveComponent* component = pair.Value->Component.Get();
            if (component != nullptr && component->GetWorld() == world && component->GetOwner() != nullptr)
            {
                actors.Add(component->GetOwner());
            }
        }
        for (const TPair<uint64, TSharedPtr<FUECInputBinding>>& pair : GInputBindings)
        {
            if (!pair.Value.IsValid()) continue;
            UEnhancedInputComponent* component = pair.Value->Component.Get();
            if (component != nullptr && component->GetWorld() == world && component->GetOwner() != nullptr)
            {
                actors.Add(component->GetOwner());
            }
        }
        for (AActor* actor : actors) CancelActorSubscriptions(actor);
        CancelEventBridgeSubscriptionsForWorld(world);
        TArray<uint64> destroyedIds;
        for (const TPair<uint64, TSharedPtr<FUECActorDestroyedSubscription>>& pair : GActorDestroyedSubscriptions)
        {
            if (pair.Value.IsValid() && pair.Value->Actor.IsValid() && pair.Value->Actor->GetWorld() == world) destroyedIds.Add(pair.Key);
        }
        for (uint64 id : destroyedIds) { TSharedPtr<FUECActorDestroyedSubscription>* subscription = GActorDestroyedSubscriptions.Find(id); if (subscription != nullptr && subscription->IsValid()) (*subscription)->Cancelled = true; GActorDestroyedSubscriptions.Remove(id); }
    }
