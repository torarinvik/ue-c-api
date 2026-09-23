/* Audio, actor/component utilities, configuration, and hit subscriptions. */
    static bool IsValidAudioPlayback(double volumeMultiplier, double pitchMultiplier, uec_vector3 location)
    {
        return IsRepresentableFloat(volumeMultiplier) && IsRepresentableFloat(pitchMultiplier) &&
            volumeMultiplier >= 0.0 && pitchMultiplier > 0.0 && IsFiniteVector(location);
    }

    static float ToAudioFloat(double value)
    {
        return static_cast<float>(value);
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
        if (!IsValidAudioPlayback(volumeMultiplier, pitchMultiplier, location)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UWorld* world = worldHandle->Value.Get();
        USoundBase* sound = Cast<USoundBase>(soundHandle->Value.Get());
        if (world == nullptr || sound == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        UGameplayStatics::PlaySoundAtLocation(
            world,
            sound,
            FVector(location.x, location.y, location.z),
            ToAudioFloat(volumeMultiplier),
            ToAudioFloat(pitchMultiplier),
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
        if (IsLatentFunction(function) || function->HasAnyFunctionFlags(FUNC_Net) ||
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
        if (retained == nullptr) return HandleCreationFailureResult();
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

    uec_result UEC_CALL GetConfigInteger(uec_context* rawContext,
                                         uec_string_view section,
                                         uec_string_view key,
                                         int64_t* outValue)
    {
        if (outValue != nullptr) *outValue = 0;
        if (outValue == nullptr || !IsValidStringView(section) || section.size == 0 ||
            !IsValidStringView(key) || key.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (GConfig == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        int32 value = 0;
        if (!GConfig->GetInt(*ToFString(section), *ToFString(key), value, GGameIni)) {
            return UEC_RESULT_NOT_INITIALIZED;
        }
        *outValue = static_cast<int64_t>(value);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetConfigInteger(uec_context* rawContext,
                                         uec_string_view section,
                                         uec_string_view key,
                                         int64_t value)
    {
        if (!IsValidStringView(section) || section.size == 0 ||
            !IsValidStringView(key) || key.size == 0 ||
            value < TNumericLimits<int32>::Lowest() || value > TNumericLimits<int32>::Max()) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (GConfig == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        GConfig->SetInt(*ToFString(section), *ToFString(key), static_cast<int32>(value), GGameIni);
        GConfig->Flush(false, GGameIni);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetConfigBool(uec_context* rawContext, uec_string_view section,
                                      uec_string_view key, uec_bool* outValue)
    {
        if (outValue != nullptr) *outValue = UEC_FALSE;
        if (outValue == nullptr || !IsValidStringView(section) || section.size == 0 ||
            !IsValidStringView(key) || key.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (GConfig == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        bool value = false;
        if (!GConfig->GetBool(*ToFString(section), *ToFString(key), value, GGameIni)) {
            return UEC_RESULT_NOT_INITIALIZED;
        }
        *outValue = value ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    static void RemoveCollisionSubscription(
        const TSharedPtr<FUECCollisionSubscription>& subscription)
    {
        if (!subscription.IsValid()) return;
        if (UPrimitiveComponent* component = subscription->Component.Get())
        {
            if (UECComponentHitBridge* bridge = subscription->Bridge.Get())
            {
                component->OnComponentHit.RemoveDynamic(
                    bridge, &UECComponentHitBridge::HandleComponentHit);
            }
        }
        if (UECComponentHitBridge* bridge = subscription->Bridge.Get())
        {
            bridge->GetNativeHitEvent().Remove(subscription->Handle);
        }
        subscription->Bridge.Reset();
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
        UECComponentHitBridge* bridge = NewObject<UECComponentHitBridge>(component);
        if (bridge == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        subscription->Bridge.Reset(bridge);
        component->OnComponentHit.AddDynamic(
            bridge, &UECComponentHitBridge::HandleComponentHit);
        TWeakPtr<FUECCollisionSubscription> weakSubscription = subscription;
        subscription->Handle = bridge->GetNativeHitEvent().AddLambda(
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
                RemoveCollisionSubscription(current);
                GCollisionSubscriptions.Remove(current->Id);
            });
        if (IsShuttingDown())
        {
            RemoveCollisionSubscription(subscription);
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
            RemoveCollisionSubscription(subscription);
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
                RemoveCollisionSubscription(pair.Value);
                pair.Value->Cancelled = true;
            }
        }
        GCollisionSubscriptions.Empty();
    }
