    static FUECSceneComponent* MakeSceneComponentHandle(USceneComponent* component)
    {
        if (component == nullptr) return nullptr;
        auto* handle = new FUECSceneComponent();
        if (!InitializeHandle(handle->Header, EUECHandleKind::SceneComponent))
        {
            delete handle;
            return nullptr;
        }
        handle->Value = component;
        {
            FScopeLock lock(&GHandleMutex);
            if (GShuttingDown)
            {
                delete handle;
                return nullptr;
            }
            GComponents.Add(handle);
        }
        return handle;
    }

    static UClass* LoadSceneComponentClass(uec_string_view classPath)
    {
        if (!IsValidStringView(classPath) || classPath.size == 0) return nullptr;
        UClass* componentClass = LoadClass<USceneComponent>(nullptr, *ToFString(classPath));
        return componentClass != nullptr && componentClass->IsChildOf(USceneComponent::StaticClass())
            ? componentClass : nullptr;
    }

    static TMap<UWorld*, FDelegateHandle> GActorDestroyedHandlers;

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
    }

    static void CancelActorSubscriptionsForWorld(UWorld* world)
    {
        if (world == nullptr) return;
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
    }

    /* Actor lifetime and transform operations. */
    uec_result UEC_CALL SpawnActor(uec_world* rawWorld, uec_string_view classPath,
                                   const uec_transform* transform, uec_actor** outActor)
    {
        if (outActor != nullptr) *outActor = nullptr;
        if (outActor == nullptr || transform == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidStringView(classPath) || !IsFiniteTransform(*transform)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const uec_result authorityResult = RequireWorldAuthority(world);
        if (authorityResult != UEC_RESULT_OK) return authorityResult;
        UClass* actorClass = LoadClass<AActor>(nullptr, *ToFString(classPath));
        if (actorClass == nullptr || !actorClass->IsChildOf(AActor::StaticClass())) return UEC_RESULT_INVALID_ARGUMENT;
        AActor* actor = world->SpawnActor<AActor>(actorClass, ToFTransform(*transform));
        if (actor == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        auto* handle = MakeActorHandle(actor);
        if (handle == nullptr)
        {
            actor->Destroy();
            return UEC_RESULT_INTERNAL_ERROR;
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
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const uec_result authorityResult = RequireWorldAuthority(actor->GetWorld());
        if (authorityResult != UEC_RESULT_OK) return authorityResult;
        CancelActorSubscriptions(actor);
        TombstoneHandle(handle->Header);
        handle->Value.Reset();
        return actor->Destroy() ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
    }

    uec_result UEC_CALL ReleaseActor(uec_actor* rawActor)
    {
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TombstoneHandle(handle->Header);
        handle->Value.Reset();
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorTransform(uec_actor* rawActor, uec_transform* outTransform)
    {
        if (outTransform != nullptr) *outTransform = {};
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
        if (transform == nullptr || !IsFiniteTransform(*transform) || !IsValidBool(sweep)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const uec_result authorityResult = RequireWorldAuthority(actor->GetWorld());
        if (authorityResult != UEC_RESULT_OK) return authorityResult;
        actor->SetActorTransform(ToFTransform(*transform), sweep != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorName(uec_actor* rawActor, char* buffer, size_t bufferSize, size_t* requiredSize)
    {
        if (requiredSize != nullptr) *requiredSize = 0;
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
        if (outHasTag != nullptr) *outHasTag = UEC_FALSE;
        if (outHasTag == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(tag)) return UEC_RESULT_INVALID_ARGUMENT;
        *outHasTag = actor->ActorHasTag(FName(*ToFString(tag))) ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetActorTag(uec_actor* rawActor,
                                    uec_string_view tag,
                                    uec_bool enabled)
    {
        if (!IsValidStringView(tag) || tag.size == 0 || !IsValidBool(enabled)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const uec_result authorityResult = RequireWorldAuthority(actor->GetWorld());
        if (authorityResult != UEC_RESULT_OK) return authorityResult;
        const FName tagName(*ToFString(tag));
        if (enabled != UEC_FALSE)
        {
            actor->Tags.AddUnique(tagName);
        }
        else
        {
            actor->Tags.Remove(tagName);
        }
        return UEC_RESULT_OK;
    }

    /* Component handles and component state. */
    uec_result UEC_CALL GetActorRootComponent(uec_actor* rawActor, uec_scene_component** outComponent)
    {
        if (outComponent != nullptr) *outComponent = nullptr;
        if (outComponent == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        USceneComponent* component = actor->GetRootComponent();
        if (component == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        auto* handle = MakeSceneComponentHandle(component);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outComponent = reinterpret_cast<uec_scene_component*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorComponentCount(uec_actor* rawActor, uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        TArray<USceneComponent*> components;
        actor->GetComponents<USceneComponent>(components);
        if (static_cast<uint64>(components.Num()) > UINT32_MAX) return UEC_RESULT_INTERNAL_ERROR;
        *outCount = static_cast<uint32_t>(components.Num());
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorComponentAt(uec_actor* rawActor,
                                            uint32_t index,
                                            uec_scene_component** outComponent)
    {
        if (outComponent != nullptr) *outComponent = nullptr;
        if (outComponent == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        TArray<USceneComponent*> components;
        actor->GetComponents<USceneComponent>(components);
        if (index >= static_cast<uint32_t>(components.Num())) return UEC_RESULT_INVALID_ARGUMENT;
        USceneComponent* component = components[static_cast<int32>(index)];
        if (component == nullptr) return UEC_RESULT_INVALID_HANDLE;
        auto* handle = MakeSceneComponentHandle(component);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outComponent = reinterpret_cast<uec_scene_component*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorComponentCountByClass(uec_actor* rawActor,
                                                       uec_string_view classPath,
                                                       uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* componentClass = LoadSceneComponentClass(classPath);
        if (componentClass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        TArray<USceneComponent*> components;
        actor->GetComponents<USceneComponent>(components);
        for (USceneComponent* component : components)
        {
            if (component != nullptr && component->IsA(componentClass))
            {
                if (*outCount == UINT32_MAX) return UEC_RESULT_INTERNAL_ERROR;
                ++(*outCount);
            }
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorComponentAtByClass(uec_actor* rawActor,
                                                    uec_string_view classPath,
                                                    uint32_t index,
                                                    uec_scene_component** outComponent)
    {
        if (outComponent != nullptr) *outComponent = nullptr;
        if (outComponent == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* componentClass = LoadSceneComponentClass(classPath);
        if (componentClass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        TArray<USceneComponent*> components;
        actor->GetComponents<USceneComponent>(components);
        uint32_t current = 0;
        for (USceneComponent* component : components)
        {
            if (component == nullptr || !component->IsA(componentClass)) continue;
            if (current++ != index) continue;
            FUECSceneComponent* handle = MakeSceneComponentHandle(component);
            if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
            *outComponent = reinterpret_cast<uec_scene_component*>(handle);
            return UEC_RESULT_OK;
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    uec_result UEC_CALL ReleaseSceneComponent(uec_scene_component* rawComponent)
    {
        auto* handle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TombstoneHandle(handle->Header);
        handle->Value.Reset();
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetComponentTransform(uec_scene_component* rawComponent, uec_transform* outTransform)
    {
        if (outTransform != nullptr) *outTransform = {};
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
        if (transform == nullptr || !IsFiniteTransform(*transform) || !IsValidBool(sweep)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
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
        if (!IsValidBool(visible) || !IsValidBool(propagateToChildren)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
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
        if (!IsValidBool(active) || !IsValidBool(reset)) return UEC_RESULT_INVALID_ARGUMENT;
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

    /* Shared registry cleanup is kept with the handles it invalidates. */
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

    /* Indexed queries and player-start selection. */
    uec_result UEC_CALL GetActorCountByClass(uec_world* rawWorld,
                                             uec_string_view classPath,
                                             uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(classPath) || classPath.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UClass* actorClass = LoadClass<AActor>(nullptr, *ToFString(classPath));
        if (actorClass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
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
        if (outActor != nullptr) *outActor = nullptr;
        if (outActor == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
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

    /* Tag enumeration preserves Unreal's reflected tag order for the call. */
    uec_result UEC_CALL GetActorTagCount(uec_actor* rawActor, uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (static_cast<uint64>(actor->Tags.Num()) > UINT32_MAX) return UEC_RESULT_INTERNAL_ERROR;
        *outCount = static_cast<uint32_t>(actor->Tags.Num());
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorTagAt(uec_actor* rawActor,
                                      uint32_t index,
                                      char* buffer,
                                      size_t bufferSize,
                                      size_t* requiredSize)
    {
        if (requiredSize != nullptr) *requiredSize = 0;
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (index >= static_cast<uint32_t>(actor->Tags.Num())) return UEC_RESULT_INVALID_ARGUMENT;
        return CopyFStringToUtf8(actor->Tags[static_cast<int32>(index)].ToString(),
                                 buffer, bufferSize, requiredSize);
    }

    /* Bounds are reported in the same world-space units as transforms. */
    /* The output remains caller-owned and contains no Unreal layout. */
    uec_result UEC_CALL GetActorBounds(uec_actor* rawActor,
                                       uec_vector3* outOrigin,
                                       uec_vector3* outExtent)
    {
        if (outOrigin != nullptr) *outOrigin = {};
        if (outExtent != nullptr) *outExtent = {};
        if (outOrigin == nullptr || outExtent == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FVector origin;
        FVector extent;
        actor->GetActorBounds(false, origin, extent);
        *outOrigin = {origin.X, origin.Y, origin.Z};
        *outExtent = {extent.X, extent.Y, extent.Z};
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL FindPlayerStart(uec_world* rawWorld,
                                        uint32_t playerIndex,
                                        uec_actor** outStart)
    {
        if (outStart != nullptr) *outStart = nullptr;
        if (outStart == nullptr || playerIndex > static_cast<uint32_t>(INT32_MAX)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        APlayerController* controller = UGameplayStatics::GetPlayerController(
            world, static_cast<int32>(playerIndex));
        AGameModeBase* gameMode = world->GetAuthGameMode();
        if (gameMode == nullptr) return UEC_RESULT_UNSUPPORTED;
        AActor* start = gameMode->FindPlayerStart(controller, FString());
        if (start == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FUECActor* handle = MakeActorHandle(start);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outStart = reinterpret_cast<uec_actor*>(handle);
        return UEC_RESULT_OK;
    }
