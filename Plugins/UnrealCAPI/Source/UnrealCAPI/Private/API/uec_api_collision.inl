/* Collision queries and collision response adapters. */
    static constexpr uint32_t MaxCollisionQueryActors = UEC_MAX_COLLISION_QUERY_ACTORS;

    static void ResetHitResult(uec_hit_result* outHit)
    {
        if (outHit != nullptr) *outHit = {};
    }

    static bool IsValidTraceEndpoints(uec_vector3 start, uec_vector3 end, uec_bool traceComplex)
    {
        return IsFiniteVector(start) && IsFiniteVector(end) && IsValidBool(traceComplex);
    }

    static void ResetActorOutputs(uec_actor** outActors, uint32_t maxHits, uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outActors == nullptr || maxHits > MaxCollisionQueryActors) return;
        for (uint32_t index = 0; index < maxHits; ++index) outActors[index] = nullptr;
    }

    static uec_result CopyActorHandles(const TSet<AActor*>& actors,
                                       uint32_t maxHits,
                                       uec_actor** outActors,
                                       uint32_t* outCount)
    {
        if (maxHits > MaxCollisionQueryActors || outActors == nullptr || outCount == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        const uint32_t resultLimit = FMath::Min(maxHits, static_cast<uint32_t>(actors.Num()));
        TArray<FUECActor*> handles;
        handles.Reserve(static_cast<int32>(resultLimit));
        for (AActor* actor : actors)
        {
            if (handles.Num() >= static_cast<int32>(resultLimit)) break;
            FUECActor* handle = MakeActorHandle(actor);
            if (handle == nullptr)
            {
                for (FUECActor* createdHandle : handles)
                {
                    TombstoneHandle(createdHandle->Header);
                    createdHandle->Value.Reset();
                }
                return HandleCreationFailureResult();
            }
            handles.Add(handle);
        }

        for (int32 index = 0; index < handles.Num(); ++index)
        {
            outActors[index] = reinterpret_cast<uec_actor*>(handles[index]);
        }
        *outCount = static_cast<uint32_t>(handles.Num());
        return UEC_RESULT_OK;
    }

    static FCollisionQueryParams MakeTraceQueryParams(uec_bool traceComplex)
    {
        FCollisionQueryParams queryParams;
        queryParams.bTraceComplex = traceComplex != UEC_FALSE;
        return queryParams;
    }

    static FVector ToUnrealVector(uec_vector3 value)
    {
        return FVector(value.x, value.y, value.z);
    }

    static uec_result AddIgnoredActors(FCollisionQueryParams& queryParams,
                                       const uec_actor* const* ignoredActors,
                                       uint32_t ignoredActorCount)
    {
        if (ignoredActorCount > MaxCollisionQueryActors ||
            (ignoredActorCount != 0 && ignoredActors == nullptr)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        for (uint32_t index = 0; index < ignoredActorCount; ++index)
        {
            const auto* actorHandle = reinterpret_cast<const FUECActor*>(ignoredActors[index]);
            if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
            AActor* actor = actorHandle->Value.Get();
            if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
            queryParams.AddIgnoredActor(actor);
        }
        return UEC_RESULT_OK;
    }

    static uec_result CopyHitResult(const FHitResult& hit, uec_hit_result* outHit)
    {
        if (outHit == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        uec_hit_result result = {};
        result.blocking_hit = hit.bBlockingHit ? UEC_TRUE : UEC_FALSE;
        result.location = {hit.Location.X, hit.Location.Y, hit.Location.Z};
        result.normal = {hit.Normal.X, hit.Normal.Y, hit.Normal.Z};
        result.distance = hit.Distance;
        if (AActor* actor = hit.GetActor())
        {
            FUECActor* actorHandle = MakeActorHandle(actor);
            if (actorHandle == nullptr) return HandleCreationFailureResult();
            result.actor = reinterpret_cast<uec_actor*>(actorHandle);
        }
        *outHit = result;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL LineTrace(uec_world* rawWorld,
                                  uec_vector3 start,
                                  uec_vector3 end,
                                  uec_trace_channel channel,
                                  uec_bool traceComplex,
                                  uec_hit_result* outHit)
    {
        ResetHitResult(outHit);
        if (outHit == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidTraceEndpoints(start, end, traceComplex)) {
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
        FCollisionQueryParams queryParams = MakeTraceQueryParams(traceComplex);
        const bool didHit = world->LineTraceSingleByChannel(
            hit,
            ToUnrealVector(start),
            ToUnrealVector(end),
            collisionChannel,
            queryParams);
        if (!didHit) return UEC_RESULT_OK;
        return CopyHitResult(hit, outHit);
    }

    uec_result UEC_CALL SweepTrace(uec_world* rawWorld,
                                   uec_vector3 start,
                                   uec_vector3 end,
                                   const uec_collision_shape* descriptor,
                                   uec_trace_channel channel,
                                   uec_bool traceComplex,
                                   uec_hit_result* outHit)
    {
        ResetHitResult(outHit);
        if (outHit == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidTraceEndpoints(start, end, traceComplex)) {
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
        FCollisionQueryParams queryParams = MakeTraceQueryParams(traceComplex);
        const bool didHit = world->SweepSingleByChannel(
            hit,
            ToUnrealVector(start),
            ToUnrealVector(end),
            FQuat::Identity,
            collisionChannel,
            collisionShape,
            queryParams,
            FCollisionResponseParams::DefaultResponseParam);
        if (!didHit) return UEC_RESULT_OK;
        return CopyHitResult(hit, outHit);
    }

    uec_result UEC_CALL OverlapShape(uec_world* rawWorld,
                                     uec_vector3 center,
                                     const uec_collision_shape* descriptor,
                                     uec_trace_channel channel,
                                     uint32_t maxHits,
                                     uec_actor** outActors,
                                     uint32_t* outCount)
    {
        ResetActorOutputs(outActors, maxHits, outCount);
        if (outCount == nullptr || maxHits > MaxCollisionQueryActors ||
            (maxHits != 0 && outActors == nullptr))
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
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
        if (maxHits == 0) return UEC_RESULT_OK;

        TArray<FOverlapResult> overlaps;
        FCollisionQueryParams queryParams;
        const bool hasOverlap = world->OverlapMultiByChannel(
            overlaps,
            ToUnrealVector(center),
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
            if (static_cast<uint32_t>(actors.Num()) >= maxHits) break;
        }
        return CopyActorHandles(actors, maxHits, outActors, outCount);
    }

    uec_result UEC_CALL SetComponentCollisionEnabled(uec_scene_component* rawComponent,
                                                     uec_collision_enabled enabled)
    {
        ECollisionEnabled::Type collisionEnabled;
        switch (enabled)
        {
        case UEC_COLLISION_DISABLED: collisionEnabled = ECollisionEnabled::NoCollision; break;
        case UEC_COLLISION_QUERY_ONLY: collisionEnabled = ECollisionEnabled::QueryOnly; break;
        case UEC_COLLISION_PHYSICS_ONLY: collisionEnabled = ECollisionEnabled::PhysicsOnly; break;
        case UEC_COLLISION_QUERY_AND_PHYSICS: collisionEnabled = ECollisionEnabled::QueryAndPhysics; break;
        default: return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UPrimitiveComponent* component = Cast<UPrimitiveComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        component->SetCollisionEnabled(collisionEnabled);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetComponentCollisionEnabled(uec_scene_component* rawComponent,
                                                     uec_collision_enabled* outEnabled)
    {
        if (outEnabled != nullptr) *outEnabled = UEC_COLLISION_DISABLED;
        if (outEnabled == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UPrimitiveComponent* component = Cast<UPrimitiveComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        switch (component->GetCollisionEnabled())
        {
        case ECollisionEnabled::NoCollision: *outEnabled = UEC_COLLISION_DISABLED; break;
        case ECollisionEnabled::QueryOnly: *outEnabled = UEC_COLLISION_QUERY_ONLY; break;
        case ECollisionEnabled::PhysicsOnly: *outEnabled = UEC_COLLISION_PHYSICS_ONLY; break;
        case ECollisionEnabled::QueryAndPhysics: *outEnabled = UEC_COLLISION_QUERY_AND_PHYSICS; break;
        default: return UEC_RESULT_UNSUPPORTED;
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetComponentCollisionResponse(uec_scene_component* rawComponent,
                                                      uec_trace_channel channel,
                                                      uec_bool block)
    {
        if (!IsValidBool(block)) return UEC_RESULT_INVALID_ARGUMENT;
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UPrimitiveComponent* component = Cast<UPrimitiveComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        component->SetCollisionResponseToChannel(
            collisionChannel, block != UEC_FALSE ? ECR_Block : ECR_Ignore);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetComponentCollisionChannelResponse(
        uec_scene_component* rawComponent,
        uec_trace_channel channel,
        uec_collision_response response)
    {
        ECollisionResponse engineResponse;
        switch (response)
        {
        case UEC_COLLISION_RESPONSE_IGNORE: engineResponse = ECR_Ignore; break;
        case UEC_COLLISION_RESPONSE_OVERLAP: engineResponse = ECR_Overlap; break;
        case UEC_COLLISION_RESPONSE_BLOCK: engineResponse = ECR_Block; break;
        default: return UEC_RESULT_INVALID_ARGUMENT;
        }
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UPrimitiveComponent* component = Cast<UPrimitiveComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        component->SetCollisionResponseToChannel(collisionChannel, engineResponse);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetComponentCollisionResponse(uec_scene_component* rawComponent,
                                                      uec_trace_channel channel,
                                                      uec_collision_response* outResponse)
    {
        if (outResponse != nullptr) *outResponse = UEC_COLLISION_RESPONSE_IGNORE;
        if (outResponse == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UPrimitiveComponent* component = Cast<UPrimitiveComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;
        switch (component->GetCollisionResponseToChannel(collisionChannel))
        {
        case ECR_Ignore: *outResponse = UEC_COLLISION_RESPONSE_IGNORE; break;
        case ECR_Overlap: *outResponse = UEC_COLLISION_RESPONSE_OVERLAP; break;
        case ECR_Block: *outResponse = UEC_COLLISION_RESPONSE_BLOCK; break;
        default: return UEC_RESULT_UNSUPPORTED;
        }
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
        ResetHitResult(outHit);
        if (outHit == nullptr || ignoredActorCount > MaxCollisionQueryActors ||
            (ignoredActorCount != 0 && ignoredActors == nullptr) ||
            !IsValidTraceEndpoints(start, end, traceComplex)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;

        FCollisionQueryParams queryParams = MakeTraceQueryParams(traceComplex);
        const uec_result ignoredResult = AddIgnoredActors(queryParams, ignoredActors, ignoredActorCount);
        if (ignoredResult != UEC_RESULT_OK) return ignoredResult;

        FHitResult hit;
        const bool didHit = world->LineTraceSingleByChannel(
            hit,
            ToUnrealVector(start),
            ToUnrealVector(end),
            collisionChannel,
            queryParams);
        if (!didHit) return UEC_RESULT_OK;
        return CopyHitResult(hit, outHit);
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
        ResetHitResult(outHit);
        if (outHit == nullptr || ignoredActorCount > MaxCollisionQueryActors ||
            (ignoredActorCount != 0 && ignoredActors == nullptr) ||
            !IsValidTraceEndpoints(start, end, traceComplex)) {
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
        FCollisionQueryParams queryParams = MakeTraceQueryParams(traceComplex);
        const uec_result ignoredResult = AddIgnoredActors(queryParams, ignoredActors, ignoredActorCount);
        if (ignoredResult != UEC_RESULT_OK) return ignoredResult;
        FHitResult hit;
        const bool didHit = world->SweepSingleByChannel(
            hit,
            ToUnrealVector(start),
            ToUnrealVector(end),
            FQuat::Identity,
            collisionChannel,
            collisionShape,
            queryParams,
            FCollisionResponseParams::DefaultResponseParam);
        if (!didHit) return UEC_RESULT_OK;
        return CopyHitResult(hit, outHit);
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
        ResetActorOutputs(outActors, maxHits, outCount);
        if (outCount == nullptr || maxHits > MaxCollisionQueryActors ||
            ignoredActorCount > MaxCollisionQueryActors ||
            (maxHits != 0 && outActors == nullptr) ||
            (ignoredActorCount != 0 && ignoredActors == nullptr) || !IsFiniteVector(center)) {
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
        const uec_result ignoredResult = AddIgnoredActors(queryParams, ignoredActors, ignoredActorCount);
        if (ignoredResult != UEC_RESULT_OK) return ignoredResult;
        if (maxHits == 0) return UEC_RESULT_OK;
        TArray<FOverlapResult> overlaps;
        const bool hasOverlap = world->OverlapMultiByChannel(
            overlaps,
            ToUnrealVector(center),
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
            if (static_cast<uint32_t>(actors.Num()) >= maxHits) break;
        }
        return CopyActorHandles(actors, maxHits, outActors, outCount);
    }

    static uec_result PrepareHitResultDetails(uec_hit_result_details* outHit)
    {
        if (outHit == nullptr || outHit->struct_size < sizeof(uec_hit_result_details)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        outHit->struct_size = sizeof(uec_hit_result_details);
        outHit->reserved = 0;
        outHit->hit = {};
        outHit->impact_point = {};
        outHit->impact_normal = {};
        outHit->trace_start = {};
        outHit->trace_end = {};
        outHit->penetration_depth = 0.0;
        outHit->item = -1;
        outHit->face_index = -1;
        outHit->component = nullptr;
        return UEC_RESULT_OK;
    }

    static uec_result CopyHitResultDetails(const FHitResult& hit,
                                           uec_hit_result_details* outHit)
    {
        uec_hit_result_details result = {};
        result.struct_size = sizeof(result);
        result.item = -1;
        result.face_index = -1;
        const uec_result baseResult = CopyHitResult(hit, &result.hit);
        if (baseResult != UEC_RESULT_OK) return baseResult;
        result.impact_point = {hit.ImpactPoint.X, hit.ImpactPoint.Y, hit.ImpactPoint.Z};
        result.impact_normal = {hit.ImpactNormal.X, hit.ImpactNormal.Y, hit.ImpactNormal.Z};
        result.trace_start = {hit.TraceStart.X, hit.TraceStart.Y, hit.TraceStart.Z};
        result.trace_end = {hit.TraceEnd.X, hit.TraceEnd.Y, hit.TraceEnd.Z};
        result.penetration_depth = hit.PenetrationDepth;
        result.item = hit.Item;
        result.face_index = hit.FaceIndex;
        if (UPrimitiveComponent* component = hit.GetComponent())
        {
            FUECSceneComponent* componentHandle = MakeSceneComponentHandle(component);
            if (componentHandle == nullptr)
            {
                if (result.hit.actor != nullptr)
                {
                    ReleaseActor(result.hit.actor);
                }
                return HandleCreationFailureResult();
            }
            result.component = reinterpret_cast<uec_scene_component*>(componentHandle);
        }
        *outHit = result;
        return UEC_RESULT_OK;
    }

    static uec_result TraceDetailedInternal(
        uec_world* rawWorld,
        uec_vector3 start,
        uec_vector3 end,
        const uec_collision_shape* descriptor,
        uec_trace_channel channel,
        uec_bool traceComplex,
        const uec_actor* const* ignoredActors,
        uint32_t ignoredActorCount,
        uec_hit_result_details* outHit)
    {
        const uec_result outputResult = PrepareHitResultDetails(outHit);
        if (outputResult != UEC_RESULT_OK) return outputResult;
        if (!IsValidTraceEndpoints(start, end, traceComplex) ||
            ignoredActorCount > MaxCollisionQueryActors ||
            (ignoredActorCount != 0 && ignoredActors == nullptr)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;
        FCollisionQueryParams queryParams = MakeTraceQueryParams(traceComplex);
        const uec_result ignoredResult = AddIgnoredActors(
            queryParams, ignoredActors, ignoredActorCount);
        if (ignoredResult != UEC_RESULT_OK) return ignoredResult;
        FHitResult hit;
        bool didHit = false;
        if (descriptor == nullptr)
        {
            didHit = world->LineTraceSingleByChannel(
                hit,
                ToUnrealVector(start),
                ToUnrealVector(end),
                collisionChannel,
                queryParams);
        }
        else
        {
            FCollisionShape collisionShape;
            const uec_result shapeResult = MakeCollisionShape(descriptor, collisionShape);
            if (shapeResult != UEC_RESULT_OK) return shapeResult;
            didHit = world->SweepSingleByChannel(
                hit,
                ToUnrealVector(start),
                ToUnrealVector(end),
                FQuat::Identity,
                collisionChannel,
                collisionShape,
                queryParams,
                FCollisionResponseParams::DefaultResponseParam);
        }
        if (!didHit) return UEC_RESULT_OK;
        return CopyHitResultDetails(hit, outHit);
    }

    uec_result UEC_CALL TraceDetailed(uec_world* rawWorld,
                                      uec_vector3 start,
                                      uec_vector3 end,
                                      const uec_collision_shape* descriptor,
                                      uec_trace_channel channel,
                                      uec_bool traceComplex,
                                      uec_hit_result_details* outHit)
    {
        return TraceDetailedInternal(rawWorld, start, end, descriptor, channel,
                                     traceComplex, nullptr, 0, outHit);
    }

    uec_result UEC_CALL TraceDetailedFiltered(
        uec_world* rawWorld,
        uec_vector3 start,
        uec_vector3 end,
        const uec_collision_shape* descriptor,
        uec_trace_channel channel,
        uec_bool traceComplex,
        const uec_actor* const* ignoredActors,
        uint32_t ignoredActorCount,
        uec_hit_result_details* outHit)
    {
        return TraceDetailedInternal(rawWorld, start, end, descriptor, channel,
                                     traceComplex, ignoredActors, ignoredActorCount, outHit);
    }
