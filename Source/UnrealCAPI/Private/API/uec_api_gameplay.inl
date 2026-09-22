    uec_result UEC_CALL LineTrace(uec_world* rawWorld,
                                  uec_vector3 start,
                                  uec_vector3 end,
                                  uec_trace_channel channel,
                                  uec_bool traceComplex,
                                  uec_hit_result* outHit)
    {
        if (outHit == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsFiniteVector(start) || !IsFiniteVector(end)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        ECollisionChannel collisionChannel;
        if (!ToCollisionChannel(channel, collisionChannel)) return UEC_RESULT_INVALID_ARGUMENT;

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

    uec_result UEC_CALL SweepTrace(uec_world* rawWorld,
                                   uec_vector3 start,
                                   uec_vector3 end,
                                   const uec_collision_shape* descriptor,
                                   uec_trace_channel channel,
                                   uec_bool traceComplex,
                                   uec_hit_result* outHit)
    {
        if (outHit == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsFiniteVector(start) || !IsFiniteVector(end)) return UEC_RESULT_INVALID_ARGUMENT;
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

        *outHit = {};
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
            outHit->actor = reinterpret_cast<uec_actor*>(MakeActorHandle(actor));
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
        if (outCount == nullptr || (maxHits != 0 && outActors == nullptr))
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

        *outCount = 0;
        for (uint32_t index = 0; index < maxHits; ++index) outActors[index] = nullptr;
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
        *outCount = static_cast<uint32_t>(actors.Num());
        uint32_t copied = 0;
        for (AActor* actor : actors)
        {
            if (copied == maxHits) break;
            outActors[copied++] = reinterpret_cast<uec_actor*>(MakeActorHandle(actor));
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
        if (!FMath::IsFinite(volumeMultiplier) || !FMath::IsFinite(pitchMultiplier) ||
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

    uec_result UEC_CALL CreateWidget(uec_world* rawWorld,
                                     uec_string_view widgetClassPath,
                                     uec_object** outWidget)
    {
        if (outWidget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outWidget = nullptr;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (widgetClassPath.data == nullptr && widgetClassPath.size != 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        APlayerController* controller = world->GetFirstPlayerController();
        if (controller == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        UClass* widgetClass = LoadClass<UUserWidget>(nullptr, *ToFString(widgetClassPath));
        if (widgetClass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        UUserWidget* widget = CreateWidget<UUserWidget>(controller, widgetClass);
        FUECObject* handle = MakeObjectHandle(widget);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outWidget = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL AddWidgetToViewport(uec_object* rawWidget, int32_t zOrder)
    {
        auto* widgetHandle = reinterpret_cast<FUECObject*>(rawWidget);
        if (!IsValidObject(widgetHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UUserWidget* widget = Cast<UUserWidget>(widgetHandle->Value.Get());
        if (widget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        widget->AddToViewport(zOrder);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL RemoveWidgetFromParent(uec_object* rawWidget)
    {
        auto* widgetHandle = reinterpret_cast<FUECObject*>(rawWidget);
        if (!IsValidObject(widgetHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UUserWidget* widget = Cast<UUserWidget>(widgetHandle->Value.Get());
        if (widget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        widget->RemoveFromParent();
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetWidgetVisibility(uec_object* rawWidget,
                                            uec_widget_visibility visibility)
    {
        auto* widgetHandle = reinterpret_cast<FUECObject*>(rawWidget);
        if (!IsValidObject(widgetHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWidget* widget = Cast<UWidget>(widgetHandle->Value.Get());
        if (widget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        ESlateVisibility engineVisibility;
        switch (visibility)
        {
        case UEC_WIDGET_VISIBLE: engineVisibility = ESlateVisibility::Visible; break;
        case UEC_WIDGET_COLLAPSED: engineVisibility = ESlateVisibility::Collapsed; break;
        case UEC_WIDGET_HIDDEN: engineVisibility = ESlateVisibility::Hidden; break;
        default: return UEC_RESULT_INVALID_ARGUMENT;
        }
        widget->SetVisibility(engineVisibility);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetTextBlockText(uec_object* rawWidget,
                                         uec_string_view text)
    {
        auto* widgetHandle = reinterpret_cast<FUECObject*>(rawWidget);
        if (!IsValidObject(widgetHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(text)) return UEC_RESULT_INVALID_ARGUMENT;
        UTextBlock* textBlock = Cast<UTextBlock>(widgetHandle->Value.Get());
        if (textBlock == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        textBlock->SetText(FText::FromString(ToFString(text)));
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL BindButtonClicked(uec_object* rawButton,
                                          uec_widget_event_callback callback,
                                          void* userData,
                                          uint64_t* outSubscriptionId)
    {
        if (callback == nullptr || outSubscriptionId == nullptr)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* buttonHandle = reinterpret_cast<FUECObject*>(rawButton);
        if (!IsValidObject(buttonHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UButton* button = Cast<UButton>(buttonHandle->Value.Get());
        if (button == nullptr) return UEC_RESULT_INVALID_ARGUMENT;

        const uint64 subscriptionId = GNextWidgetSubscriptionId++;
        auto subscription = MakeShared<FUECWidgetSubscription>();
        subscription->Id = subscriptionId;
        subscription->Button = button;
        subscription->Callback = callback;
        subscription->UserData = userData;
        TWeakPtr<FUECWidgetSubscription> weakSubscription = subscription;
        subscription->Handle = button->OnClicked.AddLambda(
            [weakSubscription]()
            {
                TSharedPtr<FUECWidgetSubscription> current = weakSubscription.Pin();
                if (!current.IsValid() || current->Cancelled || IsShuttingDown()) return;
                current->InCallback = true;
                current->Callback(current->Id, current->UserData);
                current->InCallback = false;
                current->Cancelled = true;
                GWidgetSubscriptions.Remove(current->Id);
            });
        GWidgetSubscriptions.Add(subscriptionId, subscription);
        *outSubscriptionId = subscriptionId;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL UnbindButtonClicked(uec_context* rawContext,
                                            uint64_t subscriptionId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECWidgetSubscription>* subscriptionPtr = GWidgetSubscriptions.Find(subscriptionId);
        if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid())
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        TSharedPtr<FUECWidgetSubscription> subscription = *subscriptionPtr;
        subscription->Cancelled = true;
        if (!subscription->InCallback)
        {
            if (UButton* button = subscription->Button.Get())
            {
                button->OnClicked.Remove(subscription->Handle);
            }
        }
        GWidgetSubscriptions.Remove(subscriptionId);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetCameraFieldOfView(uec_scene_component* rawComponent,
                                             double* outDegrees)
    {
        if (outDegrees == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UCameraComponent* camera = Cast<UCameraComponent>(componentHandle->Value.Get());
        if (camera == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outDegrees = static_cast<double>(camera->FieldOfView);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetCameraFieldOfView(uec_scene_component* rawComponent,
                                             double degrees)
    {
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!FMath::IsFinite(degrees) || degrees <= 0.0 || degrees >= 360.0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UCameraComponent* camera = Cast<UCameraComponent>(componentHandle->Value.Get());
        if (camera == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        camera->SetFieldOfView(static_cast<float>(degrees));
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

    uec_result UEC_CALL RetainObject(uec_object* rawObject, uec_object** outRetainedObject)
    {
        if (outRetainedObject == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outRetainedObject = nullptr;
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
        if (outIsA == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USceneComponent* component = componentHandle->Value.Get();
        if (component == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (classPath.data == nullptr && classPath.size != 0) return UEC_RESULT_INVALID_ARGUMENT;
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
        auto* childHandle = reinterpret_cast<FUECSceneComponent*>(rawChild);
        auto* parentHandle = reinterpret_cast<FUECSceneComponent*>(rawParent);
        if (!IsValidComponent(childHandle) || !IsValidComponent(parentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (socketName.data == nullptr && socketName.size != 0) return UEC_RESULT_INVALID_ARGUMENT;
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
        if (outIsA == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (classPath.data == nullptr && classPath.size != 0) return UEC_RESULT_INVALID_ARGUMENT;
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

    uec_result UEC_CALL SpawnSoundAttached(uec_scene_component* rawAttachTo,
                                           uec_object* rawSound,
                                           uec_string_view socketName,
                                           double volumeMultiplier,
                                           double pitchMultiplier,
                                           uec_object** outAudioComponent)
    {
        if (outAudioComponent == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawAttachTo);
        auto* soundHandle = reinterpret_cast<FUECObject*>(rawSound);
        if (!IsValidComponent(componentHandle) || !IsValidObject(soundHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(socketName) ||
            !FMath::IsFinite(volumeMultiplier) || !FMath::IsFinite(pitchMultiplier) ||
            volumeMultiplier < 0.0 || pitchMultiplier <= 0.0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        *outAudioComponent = nullptr;
        USceneComponent* attachTo = componentHandle->Value.Get();
        USoundBase* sound = Cast<USoundBase>(soundHandle->Value.Get());
        if (attachTo == nullptr || sound == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        UAudioComponent* audio = UGameplayStatics::SpawnSoundAttached(
            sound,
            attachTo,
            FName(*ToFString(socketName)),
            FVector::ZeroVector,
            EAttachLocation::KeepRelativeOffset,
            true,
            static_cast<float>(volumeMultiplier),
            static_cast<float>(pitchMultiplier),
            0.0f,
            nullptr,
            nullptr,
            false);
        FUECObject* handle = MakeObjectHandle(audio);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outAudioComponent = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL StopAudioComponent(uec_object* rawAudioComponent)
    {
        auto* audioHandle = reinterpret_cast<FUECObject*>(rawAudioComponent);
        if (!IsValidObject(audioHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UAudioComponent* audio = Cast<UAudioComponent>(audioHandle->Value.Get());
        if (audio == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        audio->Stop();
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL BindAudioFinished(uec_object* rawAudioComponent,
                                          uec_audio_finished_callback callback,
                                          void* userData,
                                          uint64_t* outSubscriptionId)
    {
        if (callback == nullptr || outSubscriptionId == nullptr)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* audioHandle = reinterpret_cast<FUECObject*>(rawAudioComponent);
        if (!IsValidObject(audioHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UAudioComponent* audio = Cast<UAudioComponent>(audioHandle->Value.Get());
        if (audio == nullptr) return UEC_RESULT_INVALID_ARGUMENT;

        const uint64 subscriptionId = GNextAudioSubscriptionId++;
        auto subscription = MakeShared<FUECAudioSubscription>();
        subscription->Id = subscriptionId;
        subscription->Component = audio;
        subscription->Callback = callback;
        subscription->UserData = userData;
        TWeakPtr<FUECAudioSubscription> weakSubscription = subscription;
        subscription->Handle = audio->OnAudioFinishedNative.AddLambda(
            [weakSubscription](UAudioComponent*)
            {
                TSharedPtr<FUECAudioSubscription> current = weakSubscription.Pin();
                if (!current.IsValid() || current->Cancelled || IsShuttingDown()) return;
                current->InCallback = true;
                current->Callback(current->Id, current->UserData);
                current->InCallback = false;
                current->Cancelled = true;
                GAudioSubscriptions.Remove(current->Id);
            });
        GAudioSubscriptions.Add(subscriptionId, subscription);
        *outSubscriptionId = subscriptionId;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL UnbindAudioFinished(uec_context* rawContext,
                                            uint64_t subscriptionId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECAudioSubscription>* subscriptionPtr = GAudioSubscriptions.Find(subscriptionId);
        if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid())
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        TSharedPtr<FUECAudioSubscription> subscription = *subscriptionPtr;
        subscription->Cancelled = true;
        if (!subscription->InCallback)
        {
            if (UAudioComponent* audio = subscription->Component.Get())
            {
                audio->OnAudioFinishedNative.Remove(subscription->Handle);
            }
        }
        GAudioSubscriptions.Remove(subscriptionId);
        return UEC_RESULT_OK;
    }

    static void CancelAudioSubscriptionsFor(UAudioComponent* audio)
    {
        if (audio == nullptr) return;
        TArray<uint64> subscriptionIds;
        for (const TPair<uint64, TSharedPtr<FUECAudioSubscription>>& pair : GAudioSubscriptions)
        {
            if (pair.Value.IsValid() && pair.Value->Component.Get() == audio)
            {
                subscriptionIds.Add(pair.Key);
            }
        }
        for (uint64 subscriptionId : subscriptionIds)
        {
            TSharedPtr<FUECAudioSubscription>* subscriptionPtr = GAudioSubscriptions.Find(subscriptionId);
            if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid()) continue;
            (*subscriptionPtr)->Cancelled = true;
            audio->OnAudioFinishedNative.Remove((*subscriptionPtr)->Handle);
            GAudioSubscriptions.Remove(subscriptionId);
        }
    }

    uec_result UEC_CALL DestroyAudioComponent(uec_object* rawAudioComponent)
    {
        auto* audioHandle = reinterpret_cast<FUECObject*>(rawAudioComponent);
        if (!IsValidObject(audioHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UAudioComponent* audio = Cast<UAudioComponent>(audioHandle->Value.Get());
        if (audio == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        CancelAudioSubscriptionsFor(audio);
        audio->Stop();
        audio->DestroyComponent();
        return UEC_RESULT_OK;
    }

    static void ClearAllAudioSubscriptions()
    {
        for (const TPair<uint64, TSharedPtr<FUECAudioSubscription>>& pair : GAudioSubscriptions)
        {
            if (!pair.Value.IsValid()) continue;
            pair.Value->Cancelled = true;
            if (UAudioComponent* audio = pair.Value->Component.Get())
            {
                audio->OnAudioFinishedNative.Remove(pair.Value->Handle);
            }
        }
        GAudioSubscriptions.Empty();
    }

    static void ClearAllWidgetSubscriptions()
    {
        for (const TPair<uint64, TSharedPtr<FUECWidgetSubscription>>& pair : GWidgetSubscriptions)
        {
            if (!pair.Value.IsValid()) continue;
            pair.Value->Cancelled = true;
            if (UButton* button = pair.Value->Button.Get())
            {
                button->OnClicked.Remove(pair.Value->Handle);
            }
        }
        GWidgetSubscriptions.Empty();
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
        if (outHit == nullptr || (ignoredActorCount != 0 && ignoredActors == nullptr) ||
            !IsFiniteVector(start) || !IsFiniteVector(end)) {
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

        *outHit = {};
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
