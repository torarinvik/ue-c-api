    uec_result UEC_CALL GetControllerPawn(uec_actor* rawController, uec_actor** outPawn)
    {
        if (outPawn != nullptr) *outPawn = nullptr;
        if (outPawn == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* controllerHandle = reinterpret_cast<FUECActor*>(rawController);
        if (!IsValidActor(controllerHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        APlayerController* controller = Cast<APlayerController>(controllerHandle->Value.Get());
        if (controller == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        APawn* pawn = controller->GetPawn();
        if (pawn == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FUECActor* handle = MakeActorHandle(pawn);
        if (handle == nullptr) return HandleCreationFailureResult();
        *outPawn = reinterpret_cast<uec_actor*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL PossessPawn(uec_actor* rawController, uec_actor* rawPawn)
    {
        auto* controllerHandle = reinterpret_cast<FUECActor*>(rawController);
        auto* pawnHandle = reinterpret_cast<FUECActor*>(rawPawn);
        if (!IsValidActor(controllerHandle) || !IsValidActor(pawnHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        APlayerController* controller = Cast<APlayerController>(controllerHandle->Value.Get());
        APawn* pawn = Cast<APawn>(pawnHandle->Value.Get());
        if (controller == nullptr || pawn == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (controller->GetWorld() == nullptr || controller->GetWorld() != pawn->GetWorld()) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        const uec_result authorityResult = RequireWorldAuthority(controller->GetWorld());
        if (authorityResult != UEC_RESULT_OK) return authorityResult;
        controller->Possess(pawn);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetControllerViewTarget(uec_actor* rawController, uec_actor* rawViewTarget)
    {
        auto* controllerHandle = reinterpret_cast<FUECActor*>(rawController);
        auto* viewTargetHandle = reinterpret_cast<FUECActor*>(rawViewTarget);
        if (!IsValidActor(controllerHandle) || !IsValidActor(viewTargetHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        APlayerController* controller = Cast<APlayerController>(controllerHandle->Value.Get());
        AActor* viewTarget = viewTargetHandle->Value.Get();
        if (controller == nullptr || viewTarget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (controller->GetWorld() == nullptr || controller->GetWorld() != viewTarget->GetWorld()) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        controller->SetViewTarget(viewTarget);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetInputKeyDown(uec_actor* rawController,
                                        uec_string_view keyName,
                                        uec_bool* outDown)
    {
        if (outDown != nullptr) *outDown = UEC_FALSE;
        if (outDown == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* controllerHandle = reinterpret_cast<FUECActor*>(rawController);
        if (!IsValidActor(controllerHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        APlayerController* controller = Cast<APlayerController>(controllerHandle->Value.Get());
        if (controller == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidStringView(keyName)) return UEC_RESULT_INVALID_ARGUMENT;
        const FString name = ToFString(keyName);
        if (name.IsEmpty()) return UEC_RESULT_INVALID_ARGUMENT;
        const FKey key{FName(*name)};
        *outDown = controller->IsInputKeyDown(key) ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetInputKeyValue(uec_actor* rawController,
                                         uec_string_view keyName,
                                         double* outValue)
    {
        if (outValue != nullptr) *outValue = 0.0;
        if (outValue == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* controllerHandle = reinterpret_cast<FUECActor*>(rawController);
        if (!IsValidActor(controllerHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        APlayerController* controller = Cast<APlayerController>(controllerHandle->Value.Get());
        if (controller == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidStringView(keyName)) return UEC_RESULT_INVALID_ARGUMENT;
        const FString name = ToFString(keyName);
        if (name.IsEmpty()) return UEC_RESULT_INVALID_ARGUMENT;
        const FKey key{FName(*name)};
        *outValue = static_cast<double>(controller->GetInputAnalogKeyState(key));
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetInputActionValue(uec_actor* rawController,
                                            uec_object* rawAction,
                                            uec_input_action_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_input_action_value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        outValue->kind = UEC_INPUT_ACTION_VALUE_BOOLEAN;
        outValue->bool_value = UEC_FALSE;
        outValue->reserved[0] = outValue->reserved[1] = outValue->reserved[2] = 0;
        outValue->axis = {};
        auto* controllerHandle = reinterpret_cast<FUECActor*>(rawController);
        auto* actionHandle = reinterpret_cast<FUECObject*>(rawAction);
        if (!IsValidActor(controllerHandle) || !IsValidObject(actionHandle)) {
            return UEC_RESULT_INVALID_HANDLE;
        }
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        APlayerController* controller = Cast<APlayerController>(controllerHandle->Value.Get());
        UInputAction* action = Cast<UInputAction>(actionHandle->Value.Get());
        if (controller == nullptr || action == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        UEnhancedPlayerInput* playerInput = Cast<UEnhancedPlayerInput>(controller->PlayerInput);
        if (playerInput == nullptr) return UEC_RESULT_NOT_INITIALIZED;

        const FInputActionValue value = playerInput->GetActionValue(action);
        return WriteInputActionValue(value, *outValue)
            ? UEC_RESULT_OK : UEC_RESULT_UNSUPPORTED;
    }

    uec_result UEC_CALL GetControllerEnhancedInputSubsystem(
        uec_actor* rawController,
        uec_object** outSubsystem)
    {
        if (outSubsystem != nullptr) *outSubsystem = nullptr;
        if (outSubsystem == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* controllerHandle = reinterpret_cast<FUECActor*>(rawController);
        if (!IsValidActor(controllerHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        APlayerController* controller = Cast<APlayerController>(controllerHandle->Value.Get());
        if (controller == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        ULocalPlayer* localPlayer = controller->GetLocalPlayer();
        if (localPlayer == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        UEnhancedInputLocalPlayerSubsystem* subsystem =
            localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
        if (subsystem == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FUECObject* handle = MakeObjectHandle(subsystem);
        if (handle == nullptr) {
            return HandleCreationFailureResult();
        }
        *outSubsystem = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorVelocity(uec_actor* rawActor, uec_vector3* outVelocity)
    {
        if (outVelocity != nullptr) *outVelocity = {};
        if (outVelocity == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const FVector velocity = actor->GetVelocity();
        *outVelocity = {velocity.X, velocity.Y, velocity.Z};
        return UEC_RESULT_OK;
    }

    static uec_result GetPhysicsComponent(FUECSceneComponent* componentHandle,
                                          UPrimitiveComponent*& outComponent)
    {
        outComponent = nullptr;
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UPrimitiveComponent* component = Cast<UPrimitiveComponent>(componentHandle->Value.Get());
        if (component == nullptr || !component->IsSimulatingPhysics()) return UEC_RESULT_UNSUPPORTED;
        outComponent = component;
        return UEC_RESULT_OK;
    }

    static uec_result GetSimulatingPhysicsComponent(FUECSceneComponent* componentHandle,
                                                     UPrimitiveComponent*& outComponent)
    {
        const uec_result componentResult = GetPhysicsComponent(componentHandle, outComponent);
        if (componentResult != UEC_RESULT_OK) return componentResult;
        const uec_result authorityResult = RequireWorldAuthority(outComponent->GetWorld());
        if (authorityResult != UEC_RESULT_OK) return authorityResult;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetComponentPhysicsVelocity(uec_scene_component* rawComponent,
                                                    uec_vector3 velocity,
                                                    uec_bool addToCurrent)
    {
        if (!IsFiniteVector(velocity) || !IsValidBool(addToCurrent)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        UPrimitiveComponent* component = nullptr;
        const uec_result result = GetSimulatingPhysicsComponent(componentHandle, component);
        if (result != UEC_RESULT_OK) return result;
        const FVector value(velocity.x, velocity.y, velocity.z);
        component->SetPhysicsLinearVelocity(
            addToCurrent != UEC_FALSE ? component->GetPhysicsLinearVelocity(NAME_None) + value : value,
            false, NAME_None);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ApplyComponentImpulse(uec_scene_component* rawComponent,
                                              uec_vector3 impulse,
                                              uec_bool velocityChange)
    {
        if (!IsFiniteVector(impulse) || !IsValidBool(velocityChange)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        UPrimitiveComponent* component = nullptr;
        const uec_result result = GetSimulatingPhysicsComponent(componentHandle, component);
        if (result != UEC_RESULT_OK) return result;
        component->AddImpulse(FVector(impulse.x, impulse.y, impulse.z), NAME_None,
                              velocityChange != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ApplyComponentForce(uec_scene_component* rawComponent, uec_vector3 force)
    {
        if (!IsFiniteVector(force)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        UPrimitiveComponent* component = nullptr;
        const uec_result result = GetSimulatingPhysicsComponent(componentHandle, component);
        if (result != UEC_RESULT_OK) return result;
        component->AddForce(FVector(force.x, force.y, force.z), NAME_None, false);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetComponentPhysicsAngularVelocity(uec_scene_component* rawComponent,
                                                           uec_vector3* outVelocity)
    {
        if (outVelocity != nullptr) *outVelocity = {};
        if (outVelocity == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        UPrimitiveComponent* component = nullptr;
        const uec_result result = GetPhysicsComponent(componentHandle, component);
        if (result != UEC_RESULT_OK) return result;
        const FVector velocity = component->GetPhysicsAngularVelocityInRadians(NAME_None);
        *outVelocity = {velocity.X, velocity.Y, velocity.Z};
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetComponentPhysicsAngularVelocity(uec_scene_component* rawComponent,
                                                           uec_vector3 velocity,
                                                           uec_bool addToCurrent)
    {
        if (!IsFiniteVector(velocity) || !IsValidBool(addToCurrent)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        UPrimitiveComponent* component = nullptr;
        const uec_result result = GetSimulatingPhysicsComponent(componentHandle, component);
        if (result != UEC_RESULT_OK) return result;
        component->SetPhysicsAngularVelocityInRadians(
            FVector(velocity.x, velocity.y, velocity.z), addToCurrent != UEC_FALSE, NAME_None);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ApplyComponentTorque(uec_scene_component* rawComponent,
                                             uec_vector3 torque,
                                             uec_bool accelerationChange)
    {
        if (!IsFiniteVector(torque) || !IsValidBool(accelerationChange)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        UPrimitiveComponent* component = nullptr;
        const uec_result result = GetSimulatingPhysicsComponent(componentHandle, component);
        if (result != UEC_RESULT_OK) return result;
        component->AddTorqueInRadians(FVector(torque.x, torque.y, torque.z), NAME_None,
                                      accelerationChange != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ApplyComponentAngularImpulse(uec_scene_component* rawComponent,
                                                     uec_vector3 impulse,
                                                     uec_bool velocityChange)
    {
        if (!IsFiniteVector(impulse) || !IsValidBool(velocityChange)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        UPrimitiveComponent* component = nullptr;
        const uec_result result = GetSimulatingPhysicsComponent(componentHandle, component);
        if (result != UEC_RESULT_OK) return result;
        component->AddAngularImpulseInRadians(FVector(impulse.x, impulse.y, impulse.z), NAME_None,
                                              velocityChange != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    static UPrimitiveComponent* GetActorPrimitiveRoot(FUECActor* actorHandle)
    {
        AActor* actor = actorHandle == nullptr ? nullptr : actorHandle->Value.Get();
        return actor == nullptr ? nullptr : Cast<UPrimitiveComponent>(actor->GetRootComponent());
    }

    static uec_result GetActorPhysicsPrimitiveRoot(FUECActor* actorHandle,
                                                   UPrimitiveComponent*& outComponent)
    {
        outComponent = nullptr;
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UPrimitiveComponent* component = GetActorPrimitiveRoot(actorHandle);
        if (component == nullptr || !component->IsSimulatingPhysics()) return UEC_RESULT_UNSUPPORTED;
        outComponent = component;
        return UEC_RESULT_OK;
    }

    static uec_result GetSimulatingActorPrimitiveRoot(FUECActor* actorHandle,
                                                       UPrimitiveComponent*& outComponent)
    {
        const uec_result componentResult = GetActorPhysicsPrimitiveRoot(actorHandle, outComponent);
        if (componentResult != UEC_RESULT_OK) return componentResult;
        const uec_result authorityResult = RequireWorldAuthority(outComponent->GetWorld());
        if (authorityResult != UEC_RESULT_OK) return authorityResult;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetActorPhysicsVelocity(uec_actor* rawActor,
                                                uec_vector3 velocity,
                                                uec_bool addToCurrent)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsFiniteVector(velocity) || !IsValidBool(addToCurrent)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UPrimitiveComponent* component = GetActorPrimitiveRoot(actorHandle);
        if (component == nullptr || !component->IsSimulatingPhysics()) return UEC_RESULT_UNSUPPORTED;
        const uec_result authorityResult = RequireWorldAuthority(component->GetWorld());
        if (authorityResult != UEC_RESULT_OK) return authorityResult;
        const FVector value(velocity.x, velocity.y, velocity.z);
        if (addToCurrent != UEC_FALSE)
        {
            component->SetPhysicsLinearVelocity(
                component->GetPhysicsLinearVelocity(NAME_None) + value, false, NAME_None);
        }
        else
        {
            component->SetPhysicsLinearVelocity(value, false, NAME_None);
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ApplyActorImpulse(uec_actor* rawActor,
                                          uec_vector3 impulse,
                                          uec_bool velocityChange)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsFiniteVector(impulse) || !IsValidBool(velocityChange)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UPrimitiveComponent* component = GetActorPrimitiveRoot(actorHandle);
        if (component == nullptr || !component->IsSimulatingPhysics()) return UEC_RESULT_UNSUPPORTED;
        const uec_result authorityResult = RequireWorldAuthority(component->GetWorld());
        if (authorityResult != UEC_RESULT_OK) return authorityResult;
        component->AddImpulse(FVector(impulse.x, impulse.y, impulse.z), NAME_None, velocityChange != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ApplyActorForce(uec_actor* rawActor, uec_vector3 force)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsFiniteVector(force)) return UEC_RESULT_INVALID_ARGUMENT;
        UPrimitiveComponent* component = GetActorPrimitiveRoot(actorHandle);
        if (component == nullptr || !component->IsSimulatingPhysics()) return UEC_RESULT_UNSUPPORTED;
        const uec_result authorityResult = RequireWorldAuthority(component->GetWorld());
        if (authorityResult != UEC_RESULT_OK) return authorityResult;
        component->AddForce(FVector(force.x, force.y, force.z), NAME_None, false);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorPhysicsAngularVelocity(uec_actor* rawActor,
                                                       uec_vector3* outVelocity)
    {
        if (outVelocity != nullptr) *outVelocity = {};
        if (outVelocity == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        UPrimitiveComponent* component = nullptr;
        const uec_result result = GetActorPhysicsPrimitiveRoot(actorHandle, component);
        if (result != UEC_RESULT_OK) return result;
        const FVector velocity = component->GetPhysicsAngularVelocityInRadians(NAME_None);
        *outVelocity = {velocity.X, velocity.Y, velocity.Z};
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetActorPhysicsAngularVelocity(uec_actor* rawActor,
                                                       uec_vector3 velocity,
                                                       uec_bool addToCurrent)
    {
        if (!IsFiniteVector(velocity) || !IsValidBool(addToCurrent)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        UPrimitiveComponent* component = nullptr;
        const uec_result result = GetSimulatingActorPrimitiveRoot(actorHandle, component);
        if (result != UEC_RESULT_OK) return result;
        component->SetPhysicsAngularVelocityInRadians(
            FVector(velocity.x, velocity.y, velocity.z), addToCurrent != UEC_FALSE, NAME_None);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ApplyActorTorque(uec_actor* rawActor,
                                         uec_vector3 torque,
                                         uec_bool accelerationChange)
    {
        if (!IsFiniteVector(torque) || !IsValidBool(accelerationChange)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        UPrimitiveComponent* component = nullptr;
        const uec_result result = GetSimulatingActorPrimitiveRoot(actorHandle, component);
        if (result != UEC_RESULT_OK) return result;
        component->AddTorqueInRadians(FVector(torque.x, torque.y, torque.z), NAME_None,
                                      accelerationChange != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ApplyActorAngularImpulse(uec_actor* rawActor,
                                                 uec_vector3 impulse,
                                                 uec_bool velocityChange)
    {
        if (!IsFiniteVector(impulse) || !IsValidBool(velocityChange)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        UPrimitiveComponent* component = nullptr;
        const uec_result result = GetSimulatingActorPrimitiveRoot(actorHandle, component);
        if (result != UEC_RESULT_OK) return result;
        component->AddAngularImpulseInRadians(FVector(impulse.x, impulse.y, impulse.z), NAME_None,
                                              velocityChange != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL AddPawnMovementInput(uec_actor* rawPawn,
                                             uec_vector3 worldDirection,
                                             double scale,
                                             uec_bool force)
    {
        auto* pawnHandle = reinterpret_cast<FUECActor*>(rawPawn);
        if (!IsValidActor(pawnHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!FMath::IsFinite(worldDirection.x) || !FMath::IsFinite(worldDirection.y) ||
            !FMath::IsFinite(worldDirection.z) || !IsRepresentableFloat(scale) ||
            !IsValidBool(force)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        APawn* pawn = Cast<APawn>(pawnHandle->Value.Get());
        if (pawn == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        pawn->AddMovementInput(
            FVector(worldDirection.x, worldDirection.y, worldDirection.z),
            static_cast<float>(scale),
            force != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL JumpCharacter(uec_actor* rawCharacter)
    {
        auto* characterHandle = reinterpret_cast<FUECActor*>(rawCharacter);
        if (!IsValidActor(characterHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        ACharacter* character = Cast<ACharacter>(characterHandle->Value.Get());
        if (character == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        character->Jump();
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL StopCharacterJumping(uec_actor* rawCharacter)
    {
        auto* characterHandle = reinterpret_cast<FUECActor*>(rawCharacter);
        if (!IsValidActor(characterHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        ACharacter* character = Cast<ACharacter>(characterHandle->Value.Get());
        if (character == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        character->StopJumping();
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetComponentMaterialScalar(uec_scene_component* rawComponent,
                                                    uec_string_view parameterName,
                                                    double value)
    {
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(parameterName) ||
            parameterName.size == 0 || !IsRepresentableFloat(value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UMeshComponent* component = Cast<UMeshComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        component->SetScalarParameterValueOnMaterials(
            FName(*ToFString(parameterName)), static_cast<float>(value));
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetComponentMaterialVector(uec_scene_component* rawComponent,
                                                    uec_string_view parameterName,
                                                    uec_vector3 value)
    {
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(parameterName) ||
            parameterName.size == 0 || !FMath::IsFinite(value.x) ||
            !FMath::IsFinite(value.y) || !FMath::IsFinite(value.z)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UMeshComponent* component = Cast<UMeshComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        component->SetVectorParameterValueOnMaterials(
            FName(*ToFString(parameterName)), FVector(value.x, value.y, value.z));
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL AddInputMappingContext(uec_actor* rawController,
                                               uec_object* rawMappingContext,
                                               int32_t priority)
    {
        auto* controllerHandle = reinterpret_cast<FUECActor*>(rawController);
        auto* contextHandle = reinterpret_cast<FUECObject*>(rawMappingContext);
        if (!IsValidActor(controllerHandle) || !IsValidObject(contextHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        APlayerController* controller = Cast<APlayerController>(controllerHandle->Value.Get());
        UInputMappingContext* mappingContext = Cast<UInputMappingContext>(contextHandle->Value.Get());
        if (controller == nullptr || mappingContext == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        ULocalPlayer* localPlayer = controller->GetLocalPlayer();
        if (localPlayer == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        UEnhancedInputLocalPlayerSubsystem* subsystem =
            localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
        if (subsystem == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FModifyContextOptions options;
        subsystem->AddMappingContext(mappingContext, priority, options);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL RemoveInputMappingContext(uec_actor* rawController,
                                                  uec_object* rawMappingContext)
    {
        auto* controllerHandle = reinterpret_cast<FUECActor*>(rawController);
        auto* contextHandle = reinterpret_cast<FUECObject*>(rawMappingContext);
        if (!IsValidActor(controllerHandle) || !IsValidObject(contextHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        APlayerController* controller = Cast<APlayerController>(controllerHandle->Value.Get());
        UInputMappingContext* mappingContext = Cast<UInputMappingContext>(contextHandle->Value.Get());
        if (controller == nullptr || mappingContext == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        ULocalPlayer* localPlayer = controller->GetLocalPlayer();
        if (localPlayer == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        UEnhancedInputLocalPlayerSubsystem* subsystem =
            localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
        if (subsystem == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FModifyContextOptions options;
        subsystem->RemoveMappingContext(mappingContext, options);
        return UEC_RESULT_OK;
    }

    static bool IsValidInputActionValue(const uec_input_action_value& value)
    {
        if (!IsRepresentableFloat(value.axis.x) || !IsRepresentableFloat(value.axis.y) ||
            !IsRepresentableFloat(value.axis.z) || !IsValidBool(value.bool_value)) return false;
        switch (value.kind)
        {
        case UEC_INPUT_ACTION_VALUE_BOOLEAN:
        case UEC_INPUT_ACTION_VALUE_AXIS_1D:
        case UEC_INPUT_ACTION_VALUE_AXIS_2D:
        case UEC_INPUT_ACTION_VALUE_AXIS_3D: return true;
        default: return false;
        }
    }

    uec_result UEC_CALL InjectInputActionValue(uec_actor* rawController,
                                               uec_object* rawAction,
                                               const uec_input_action_value* value)
    {
        if (value == nullptr || value->struct_size < sizeof(uec_input_action_value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsValidInputActionValue(*value)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* controllerHandle = reinterpret_cast<FUECActor*>(rawController);
        auto* actionHandle = reinterpret_cast<FUECObject*>(rawAction);
        if (!IsValidActor(controllerHandle) || !IsValidObject(actionHandle)) {
            return UEC_RESULT_INVALID_HANDLE;
        }
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        APlayerController* controller = Cast<APlayerController>(controllerHandle->Value.Get());
        UInputAction* action = Cast<UInputAction>(actionHandle->Value.Get());
        if (controller == nullptr || action == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        UEnhancedPlayerInput* playerInput = Cast<UEnhancedPlayerInput>(controller->PlayerInput);
        if (playerInput == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FInputActionValue inputValue;
        switch (value->kind)
        {
        case UEC_INPUT_ACTION_VALUE_BOOLEAN:
            inputValue = FInputActionValue(value->bool_value != UEC_FALSE);
            break;
        case UEC_INPUT_ACTION_VALUE_AXIS_1D:
            inputValue = FInputActionValue(static_cast<float>(value->axis.x));
            break;
        case UEC_INPUT_ACTION_VALUE_AXIS_2D:
            inputValue = FInputActionValue(FVector2D(value->axis.x, value->axis.y));
            break;
        case UEC_INPUT_ACTION_VALUE_AXIS_3D:
            inputValue = FInputActionValue(FVector(value->axis.x, value->axis.y, value->axis.z));
            break;
        default:
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        const TArray<UInputModifier*> modifiers;
        const TArray<UInputTrigger*> triggers;
        playerInput->InjectInputForAction(action, inputValue, modifiers, triggers);
        return UEC_RESULT_OK;
    }

    static bool ToInputTriggerEvent(uec_input_trigger_event event, ETriggerEvent& outEvent)
    {
        switch (event)
        {
        case UEC_INPUT_TRIGGER_STARTED: outEvent = ETriggerEvent::Started; return true;
        case UEC_INPUT_TRIGGER_ONGOING: outEvent = ETriggerEvent::Ongoing; return true;
        case UEC_INPUT_TRIGGER_TRIGGERED: outEvent = ETriggerEvent::Triggered; return true;
        case UEC_INPUT_TRIGGER_CANCELED: outEvent = ETriggerEvent::Canceled; return true;
        case UEC_INPUT_TRIGGER_COMPLETED: outEvent = ETriggerEvent::Completed; return true;
        default: return false;
        }
    }

    uec_result UEC_CALL BindInputAction(uec_actor* rawActor,
                                         uec_object* rawAction,
                                         uec_input_trigger_event triggerEvent,
                                         uec_input_action_callback callback,
                                         void* userData,
                                         uint64_t* outBindingId)
    {
        if (outBindingId != nullptr) *outBindingId = 0;
        if (callback == nullptr || outBindingId == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        ETriggerEvent engineEvent;
        if (!ToInputTriggerEvent(triggerEvent, engineEvent)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        auto* actionHandle = reinterpret_cast<FUECObject*>(rawAction);
        if (!IsValidActor(actorHandle) || !IsValidObject(actionHandle)) {
            return UEC_RESULT_INVALID_HANDLE;
        }
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        UInputAction* action = Cast<UInputAction>(actionHandle->Value.Get());
        if (actor == nullptr || action == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        UEnhancedInputComponent* inputComponent = Cast<UEnhancedInputComponent>(actor->InputComponent);
        if (inputComponent == nullptr) return UEC_RESULT_UNSUPPORTED;
        EnsureActorDestroyedHandler(actor->GetWorld());
        if (GInputBindings.Num() >= MaxSubscriptions) return UEC_RESULT_QUEUE_FULL;

        auto binding = MakeShared<FUECInputBinding>();
        if (!AllocateMonotonicId(GNextInputBindingId, binding->Id)) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        binding->Component = inputComponent;
        binding->Callback = callback;
        binding->UserData = userData;
        TWeakPtr<FUECInputBinding> weakBinding = binding;
        FEnhancedInputActionEventBinding& engineBinding = inputComponent->BindActionValueLambda(
            action,
            engineEvent,
            [weakBinding](const FInputActionValue& inputValue)
            {
                TSharedPtr<FUECInputBinding> current = weakBinding.Pin();
                if (!current.IsValid() || current->Cancelled || current->Callback == nullptr ||
                    IsShuttingDown()) return;
                uec_input_action_value value{};
                if (!WriteInputActionValue(inputValue, value)) return;
                current->InCallback = true;
                FUECCallbackScope callbackScope;
                current->Callback(current->Id, value, current->UserData);
                current->InCallback = false;
                if (current->Cancelled || IsShuttingDown())
                {
                    if (UEnhancedInputComponent* component = current->Component.Get())
                    {
                        component->RemoveBindingByHandle(current->EngineHandle);
                    }
                    GInputBindings.Remove(current->Id);
                }
            });
        binding->EngineHandle = engineBinding.GetHandle();
        if (binding->EngineHandle == 0)
        {
            inputComponent->RemoveBindingByHandle(engineBinding.GetHandle());
            return UEC_RESULT_INTERNAL_ERROR;
        }
        if (IsShuttingDown())
        {
            inputComponent->RemoveBindingByHandle(binding->EngineHandle);
            binding->Cancelled = true;
            return UEC_RESULT_SHUTTING_DOWN;
        }
        GInputBindings.Add(binding->Id, binding);
        *outBindingId = binding->Id;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL UnbindInputAction(uec_context* rawContext, uint64_t bindingId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECInputBinding>* bindingPtr = GInputBindings.Find(bindingId);
        if (bindingPtr == nullptr || !bindingPtr->IsValid()) return UEC_RESULT_INVALID_ARGUMENT;
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
        return UEC_RESULT_OK;
    }
