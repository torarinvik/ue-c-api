    uec_result UEC_CALL LineTrace(uec_world* rawWorld,
                                  uec_vector3 start,
                                  uec_vector3 end,
                                  uec_trace_channel channel,
                                  uec_bool traceComplex,
                                  uec_hit_result* outHit)
    {
        if (outHit != nullptr) *outHit = {};
        if (outHit == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsFiniteVector(start) || !IsFiniteVector(end) || !IsValidBool(traceComplex)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;

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
            auto* actorHandle = MakeActorHandle(actor);
            if (actorHandle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
            outHit->actor = reinterpret_cast<uec_actor*>(actorHandle);
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SweepTrace(uec_world* rawWorld,
                                   uec_vector3 start,
                                   uec_vector3 end,
                                   const uec_collision_shape* descriptor,
                                   uec_trace_channel channel,
                                   uec_bool traceComplex,
                                   uec_hit_result* outHit)
    {
        if (outHit != nullptr) *outHit = {};
        if (outHit == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsFiniteVector(start) || !IsFiniteVector(end) || !IsValidBool(traceComplex)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FCollisionShape collisionShape;
        const uec_result shapeResult = MakeCollisionShape(descriptor, collisionShape);
        if (shapeResult != UEC_RESULT_OK) return shapeResult;
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;

        FHitResult hit;
        FCollisionQueryParams queryParams;
        queryParams.bTraceComplex = traceComplex != UEC_FALSE;
        const bool didHit = world->SweepSingleByChannel(
            hit,
            FVector(start.x, start.y, start.z),
            FVector(end.x, end.y, end.z),
            FQuat::Identity,
            collisionChannel,
            collisionShape,
            queryParams,
            FCollisionResponseParams::DefaultResponseParam);
        if (!didHit) return UEC_RESULT_OK;
        outHit->blocking_hit = hit.bBlockingHit ? UEC_TRUE : UEC_FALSE;
        outHit->location = {hit.Location.X, hit.Location.Y, hit.Location.Z};
        outHit->normal = {hit.Normal.X, hit.Normal.Y, hit.Normal.Z};
        outHit->distance = hit.Distance;
        if (AActor* actor = hit.GetActor())
        {
            FUECActor* actorHandle = MakeActorHandle(actor);
            if (actorHandle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
            outHit->actor = reinterpret_cast<uec_actor*>(actorHandle);
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL OverlapShape(uec_world* rawWorld,
                                     uec_vector3 center,
                                     const uec_collision_shape* descriptor,
                                     uec_trace_channel channel,
                                     uint32_t maxHits,
                                     uec_actor** outActors,
                                     uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr || (maxHits != 0 && outActors == nullptr))
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        for (uint32_t index = 0; index < maxHits; ++index) outActors[index] = nullptr;
        if (!IsFiniteVector(center)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FCollisionShape collisionShape;
        const uec_result shapeResult = MakeCollisionShape(descriptor, collisionShape);
        if (shapeResult != UEC_RESULT_OK) return shapeResult;
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;

        TArray<FOverlapResult> overlaps;
        FCollisionQueryParams queryParams;
        const bool hasOverlap = world->OverlapMultiByChannel(
            overlaps,
            FVector(center.x, center.y, center.z),
            FQuat::Identity,
            collisionChannel,
            collisionShape,
            queryParams,
            FCollisionResponseParams::DefaultResponseParam);
        if (!hasOverlap) return UEC_RESULT_OK;

        TSet<AActor*> actors;
        for (const FOverlapResult& overlap : overlaps)
        {
            if (AActor* actor = overlap.GetActor()) actors.Add(actor);
        }
        uint32_t copied = 0;
        for (AActor* actor : actors)
        {
            if (copied == maxHits) break;
            FUECActor* actorHandle = MakeActorHandle(actor);
            if (actorHandle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
            outActors[copied++] = reinterpret_cast<uec_actor*>(actorHandle);
            *outCount = copied;
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL PlaySoundAtLocation(uec_world* rawWorld,
                                            uec_object* rawSound,
                                            uec_vector3 location,
                                            double volumeMultiplier,
                                            double pitchMultiplier)
    {
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        auto* soundHandle = reinterpret_cast<FUECObject*>(rawSound);
        if (!IsValidWorld(worldHandle) || !IsValidObject(soundHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsRepresentableFloat(volumeMultiplier) || !IsRepresentableFloat(pitchMultiplier) ||
            volumeMultiplier < 0.0 || pitchMultiplier <= 0.0 || !IsFiniteVector(location)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UWorld* world = worldHandle->Value.Get();
        USoundBase* sound = Cast<USoundBase>(soundHandle->Value.Get());
        if (world == nullptr || sound == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        UGameplayStatics::PlaySoundAtLocation(
            world,
            sound,
            FVector(location.x, location.y, location.z),
            static_cast<float>(volumeMultiplier),
            static_cast<float>(pitchMultiplier),
            0.0f,
            nullptr,
            nullptr,
            nullptr);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetComponentVelocity(uec_scene_component* rawComponent,
                                             uec_vector3* outVelocity)
    {
        if (outVelocity != nullptr) *outVelocity = {};
        if (outVelocity == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UPrimitiveComponent* component = Cast<UPrimitiveComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        const FVector velocity = component->GetComponentVelocity();
        *outVelocity = {velocity.X, velocity.Y, velocity.Z};
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL InvokeActorFunction(uec_actor* rawActor, uec_string_view functionName)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(functionName) || functionName.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        UFunction* function = actor->FindFunction(FName(*ToFString(functionName)));
        if (function == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (function->HasAnyFunctionFlags(FUNC_Latent | FUNC_Net) ||
            (function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly) &&
             actor->GetWorld() != nullptr && actor->GetWorld()->GetNetMode() == NM_Client))
        {
            return UEC_RESULT_UNSUPPORTED;
        }
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

    uec_result UEC_CALL RetainObject(uec_object* rawObject, uec_object** outRetainedObject)
    {
        if (outRetainedObject != nullptr) *outRetainedObject = nullptr;
        if (outRetainedObject == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FUECObject* retained = MakeRetainedObjectHandle(object);
        if (retained == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outRetainedObject = reinterpret_cast<uec_object*>(retained);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetComponentClassName(uec_scene_component* rawComponent,
                                              char* buffer,
                                              size_t bufferSize,
                                              size_t* requiredSize)
    {
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USceneComponent* component = componentHandle->Value.Get();
        if (component == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return CopyFStringToUtf8(component->GetClass()->GetPathName(), buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL ComponentIsA(uec_scene_component* rawComponent,
                                     uec_string_view classPath,
                                     uec_bool* outIsA)
    {
        if (outIsA != nullptr) *outIsA = UEC_FALSE;
        if (outIsA == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USceneComponent* component = componentHandle->Value.Get();
        if (component == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(classPath) || classPath.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        UClass* klass = LoadClass<USceneComponent>(nullptr, *ToFString(classPath));
        if (klass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outIsA = component->IsA(klass) ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL AttachSceneComponent(uec_scene_component* rawChild,
                                             uec_scene_component* rawParent,
                                             uec_bool keepWorldTransform,
                                             uec_string_view socketName)
    {
        if (!IsValidBool(keepWorldTransform)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* childHandle = reinterpret_cast<FUECSceneComponent*>(rawChild);
        auto* parentHandle = reinterpret_cast<FUECSceneComponent*>(rawParent);
        if (!IsValidComponent(childHandle) || !IsValidComponent(parentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(socketName)) return UEC_RESULT_INVALID_ARGUMENT;
        USceneComponent* child = childHandle->Value.Get();
        USceneComponent* parent = parentHandle->Value.Get();
        if (child == nullptr || parent == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (child == parent || child->GetWorld() != parent->GetWorld()) return UEC_RESULT_INVALID_ARGUMENT;
        const FAttachmentTransformRules rules = keepWorldTransform != UEC_FALSE
            ? FAttachmentTransformRules::KeepWorldTransform
            : FAttachmentTransformRules::KeepRelativeTransform;
        return child->AttachToComponent(parent, rules, FName(*ToFString(socketName)))
            ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
    }

    uec_result UEC_CALL DetachSceneComponent(uec_scene_component* rawComponent,
                                             uec_bool keepWorldTransform)
    {
        if (!IsValidBool(keepWorldTransform)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USceneComponent* component = componentHandle->Value.Get();
        if (component == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const FDetachmentTransformRules rules = keepWorldTransform != UEC_FALSE
            ? FDetachmentTransformRules::KeepWorldTransform
            : FDetachmentTransformRules::KeepRelativeTransform;
        component->DetachFromComponent(rules);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorClassName(uec_actor* rawActor,
                                          char* buffer,
                                          size_t bufferSize,
                                          size_t* requiredSize)
    {
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return CopyFStringToUtf8(actor->GetClass()->GetPathName(), buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL ActorIsA(uec_actor* rawActor,
                                 uec_string_view classPath,
                                 uec_bool* outIsA)
    {
        if (outIsA != nullptr) *outIsA = UEC_FALSE;
        if (outIsA == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(classPath) || classPath.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        UClass* klass = LoadClass<AActor>(nullptr, *ToFString(classPath));
        if (klass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outIsA = actor->IsA(klass) ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetComponentCollisionEnabled(uec_scene_component* rawComponent,
                                                     uec_collision_enabled enabled)
    {
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UPrimitiveComponent* component = Cast<UPrimitiveComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        ECollisionEnabled::Type collisionEnabled;
        switch (enabled)
        {
        case UEC_COLLISION_DISABLED: collisionEnabled = ECollisionEnabled::NoCollision; break;
        case UEC_COLLISION_QUERY_ONLY: collisionEnabled = ECollisionEnabled::QueryOnly; break;
        case UEC_COLLISION_PHYSICS_ONLY: collisionEnabled = ECollisionEnabled::PhysicsOnly; break;
        case UEC_COLLISION_QUERY_AND_PHYSICS: collisionEnabled = ECollisionEnabled::QueryAndPhysics; break;
        default: return UEC_RESULT_INVALID_ARGUMENT;
        }
        component->SetCollisionEnabled(collisionEnabled);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetComponentCollisionResponse(uec_scene_component* rawComponent,
                                                      uec_trace_channel channel,
                                                      uec_bool block)
    {
        if (!IsValidBool(block)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UPrimitiveComponent* component = Cast<UPrimitiveComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;
        component->SetCollisionResponseToChannel(
            collisionChannel, block != UEC_FALSE ? ECR_Block : ECR_Ignore);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL LineTraceFiltered(uec_world* rawWorld,
                                          uec_vector3 start,
                                          uec_vector3 end,
                                          uec_trace_channel channel,
                                          uec_bool traceComplex,
                                          const uec_actor* const* ignoredActors,
                                          uint32_t ignoredActorCount,
                                          uec_hit_result* outHit)
    {
        if (outHit != nullptr) *outHit = {};
        if (outHit == nullptr || (ignoredActorCount != 0 && ignoredActors == nullptr) ||
            !IsFiniteVector(start) || !IsFiniteVector(end) || !IsValidBool(traceComplex)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;

        FCollisionQueryParams queryParams;
        queryParams.bTraceComplex = traceComplex != UEC_FALSE;
        for (uint32_t index = 0; index < ignoredActorCount; ++index)
        {
            const auto* actorHandle = reinterpret_cast<const FUECActor*>(ignoredActors[index]);
            if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
            AActor* actor = actorHandle->Value.Get();
            if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
            queryParams.AddIgnoredActor(actor);
        }

        FHitResult hit;
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
            FUECActor* actorHandle = MakeActorHandle(actor);
            if (actorHandle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
            outHit->actor = reinterpret_cast<uec_actor*>(actorHandle);
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SweepTraceFiltered(uec_world* rawWorld,
                                           uec_vector3 start,
                                           uec_vector3 end,
                                           const uec_collision_shape* descriptor,
                                           uec_trace_channel channel,
                                           uec_bool traceComplex,
                                           const uec_actor* const* ignoredActors,
                                           uint32_t ignoredActorCount,
                                           uec_hit_result* outHit)
    {
        if (outHit != nullptr) *outHit = {};
        if (outHit == nullptr || (ignoredActorCount != 0 && ignoredActors == nullptr) ||
            !IsFiniteVector(start) || !IsFiniteVector(end) || !IsValidBool(traceComplex)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FCollisionShape collisionShape;
        const uec_result shapeResult = MakeCollisionShape(descriptor, collisionShape);
        if (shapeResult != UEC_RESULT_OK) return shapeResult;
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;
        FCollisionQueryParams queryParams;
        queryParams.bTraceComplex = traceComplex != UEC_FALSE;
        for (uint32_t index = 0; index < ignoredActorCount; ++index)
        {
            const auto* actorHandle = reinterpret_cast<const FUECActor*>(ignoredActors[index]);
            if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
            AActor* actor = actorHandle->Value.Get();
            if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
            queryParams.AddIgnoredActor(actor);
        }
        FHitResult hit;
        const bool didHit = world->SweepSingleByChannel(
            hit,
            FVector(start.x, start.y, start.z),
            FVector(end.x, end.y, end.z),
            FQuat::Identity,
            collisionChannel,
            collisionShape,
            queryParams,
            FCollisionResponseParams::DefaultResponseParam);
        if (!didHit) return UEC_RESULT_OK;
        outHit->blocking_hit = hit.bBlockingHit ? UEC_TRUE : UEC_FALSE;
        outHit->location = {hit.Location.X, hit.Location.Y, hit.Location.Z};
        outHit->normal = {hit.Normal.X, hit.Normal.Y, hit.Normal.Z};
        outHit->distance = hit.Distance;
        if (AActor* actor = hit.GetActor())
        {
            FUECActor* actorHandle = MakeActorHandle(actor);
            if (actorHandle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
            outHit->actor = reinterpret_cast<uec_actor*>(actorHandle);
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL OverlapShapeFiltered(uec_world* rawWorld,
                                             uec_vector3 center,
                                             const uec_collision_shape* descriptor,
                                             uec_trace_channel channel,
                                             uint32_t maxHits,
                                             const uec_actor* const* ignoredActors,
                                             uint32_t ignoredActorCount,
                                             uec_actor** outActors,
                                             uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr || (maxHits != 0 && outActors == nullptr) ||
            (ignoredActorCount != 0 && ignoredActors == nullptr) || !IsFiniteVector(center)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        for (uint32_t index = 0; index < maxHits; ++index) outActors[index] = nullptr;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FCollisionShape collisionShape;
        const uec_result shapeResult = MakeCollisionShape(descriptor, collisionShape);
        if (shapeResult != UEC_RESULT_OK) return shapeResult;
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;
        FCollisionQueryParams queryParams;
        for (uint32_t index = 0; index < ignoredActorCount; ++index)
        {
            const auto* actorHandle = reinterpret_cast<const FUECActor*>(ignoredActors[index]);
            if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
            AActor* actor = actorHandle->Value.Get();
            if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
            queryParams.AddIgnoredActor(actor);
        }
        TArray<FOverlapResult> overlaps;
        const bool hasOverlap = world->OverlapMultiByChannel(
            overlaps,
            FVector(center.x, center.y, center.z),
            FQuat::Identity,
            collisionChannel,
            collisionShape,
            queryParams,
            FCollisionResponseParams::DefaultResponseParam);
        if (!hasOverlap) return UEC_RESULT_OK;
        TSet<AActor*> actors;
        for (const FOverlapResult& overlap : overlaps)
        {
            if (AActor* actor = overlap.GetActor()) actors.Add(actor);
        }
        uint32_t copied = 0;
        for (AActor* actor : actors)
        {
            if (copied == maxHits) break;
            FUECActor* actorHandle = MakeActorHandle(actor);
            if (actorHandle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
            outActors[copied++] = reinterpret_cast<uec_actor*>(actorHandle);
            *outCount = copied;
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetConfigString(uec_context* rawContext,
                                        uec_string_view section,
                                        uec_string_view key,
                                        char* buffer,
                                        size_t bufferSize,
                                        size_t* requiredSize)
    {
        if (requiredSize != nullptr) *requiredSize = 0;
        if (requiredSize == nullptr || !IsValidStringView(section) || section.size == 0 ||
            !IsValidStringView(key) || key.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (GConfig == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FString value;
        if (!GConfig->GetString(*ToFString(section), *ToFString(key), value, GGameIni)) {
            return UEC_RESULT_NOT_INITIALIZED;
        }
        return CopyFStringToUtf8(value, buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL SetConfigString(uec_context* rawContext,
                                        uec_string_view section,
                                        uec_string_view key,
                                        uec_string_view value)
    {
        if (!IsValidStringView(section) || section.size == 0 ||
            !IsValidStringView(key) || key.size == 0 || !IsValidStringView(value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (GConfig == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        GConfig->SetString(*ToFString(section), *ToFString(key), *ToFString(value), GGameIni);
        GConfig->Flush(false, GGameIni);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL BindComponentHit(uec_scene_component* rawComponent,
                                         uec_component_hit_callback callback,
                                         void* userData,
                                         uint64_t* outSubscriptionId)
    {
        if (outSubscriptionId != nullptr) *outSubscriptionId = 0;
        if (callback == nullptr || outSubscriptionId == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UPrimitiveComponent* component = Cast<UPrimitiveComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        EnsureActorDestroyedHandler(component->GetWorld());
        if (GCollisionSubscriptions.Num() >= MaxSubscriptions) return UEC_RESULT_QUEUE_FULL;

        uint64 subscriptionId = 0;
        if (!AllocateMonotonicId(GNextCollisionSubscriptionId, subscriptionId)) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        auto subscription = MakeShared<FUECCollisionSubscription>();
        subscription->Id = subscriptionId;
        subscription->Component = component;
        subscription->Callback = callback;
        subscription->UserData = userData;
        TWeakPtr<FUECCollisionSubscription> weakSubscription = subscription;
        subscription->Handle = component->OnComponentHit().AddLambda(
            [weakSubscription](UPrimitiveComponent*, AActor* otherActor,
                               UPrimitiveComponent*, FVector normalImpulse, const FHitResult&)
            {
                TSharedPtr<FUECCollisionSubscription> current = weakSubscription.Pin();
                if (!current.IsValid() || current->Cancelled || IsShuttingDown()) return;
                current->InCallback = true;
                uec_actor* otherHandle = nullptr;
                if (otherActor != nullptr)
                {
                    if (FUECActor* handle = MakeActorHandle(otherActor)) {
                        otherHandle = reinterpret_cast<uec_actor*>(handle);
                    }
                }
                FUECCallbackScope callbackScope;
                current->Callback(current->Id,
                                  otherHandle,
                                  {normalImpulse.X, normalImpulse.Y, normalImpulse.Z},
                                  current->UserData);
                current->InCallback = false;
                current->Cancelled = true;
                if (UPrimitiveComponent* component = current->Component.Get())
                {
                    component->OnComponentHit().Remove(current->Handle);
                }
                GCollisionSubscriptions.Remove(current->Id);
            });
        if (IsShuttingDown())
        {
            component->OnComponentHit().Remove(subscription->Handle);
            subscription->Cancelled = true;
            return UEC_RESULT_SHUTTING_DOWN;
        }
        GCollisionSubscriptions.Add(subscriptionId, subscription);
        *outSubscriptionId = subscriptionId;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL UnbindComponentHit(uec_context* rawContext,
                                           uint64_t subscriptionId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECCollisionSubscription>* subscriptionPtr =
            GCollisionSubscriptions.Find(subscriptionId);
        if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid()) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
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
        return UEC_RESULT_OK;
    }

    static void ClearAllCollisionSubscriptions()
    {
        for (const TPair<uint64, TSharedPtr<FUECCollisionSubscription>>& pair : GCollisionSubscriptions)
        {
            if (pair.Value.IsValid())
            {
                if (UPrimitiveComponent* component = pair.Value->Component.Get())
                {
                    component->OnComponentHit().Remove(pair.Value->Handle);
                }
                pair.Value->Cancelled = true;
            }
        }
        GCollisionSubscriptions.Empty();
    }
