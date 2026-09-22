    /* Actor lifetime and transform operations. */
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
        if (transform == nullptr || !IsFiniteTransform(*transform) || !IsValidBool(sweep)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
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
        if (!IsValidStringView(tag)) return UEC_RESULT_INVALID_ARGUMENT;
        *outHasTag = actor->ActorHasTag(FName(*ToFString(tag))) ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    /* Component handles and component state. */
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
        if (static_cast<uint64>(components.Num()) > UINT32_MAX) return UEC_RESULT_INTERNAL_ERROR;
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

    /* Tag enumeration preserves Unreal's reflected tag order for the call. */
    uec_result UEC_CALL GetActorTagCount(uec_actor* rawActor, uint32_t* outCount)
    {
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
        if (outStart == nullptr || playerIndex > static_cast<uint32_t>(INT32_MAX)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outStart = nullptr;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        APlayerController* controller = UGameplayStatics::GetPlayerController(
            world, static_cast<int32>(playerIndex));
        AActor* start = UGameplayStatics::FindPlayerStart(world, controller);
        if (start == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FUECActor* handle = MakeActorHandle(start);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outStart = reinterpret_cast<uec_actor*>(handle);
        return UEC_RESULT_OK;
    }
