/* Bounded asynchronous invocation for reflected latent actor functions.
 *
 * This adapter deliberately has a narrower contract than synchronous mixed
 * invocation. The reflected function must be latent and expose exactly one
 * FLatentActionInfo parameter. Return values and other out parameters are
 * rejected because their storage belongs to the initial ProcessEvent frame,
 * which ends before the latent continuation resumes. The caller supplies all
 * other input parameters using the same tagged records as ABI 131.
 *
 * Each pending invocation receives a distinct UObject callback target. Unreal
 * addresses latent completions by callback target, execution function, and
 * UUID; the per-request target allows concurrent operations to use one stable
 * UFUNCTION without guessing which operation completed. The request table
 * keeps the callback target strongly referenced until completion or cancel.
 *
 * Cancellation suppresses the C callback and asks the world's latent-action
 * manager to remove work for that callback target. Unreal documents that an
 * action already being processed may not be removed before its execution, so
 * callers must treat cancellation as callback suppression plus best-effort
 * engine-action removal. Actor destruction, world cleanup, travel, and module
 * shutdown use the same cleanup path.
 */

    struct FUECLatentFunctionRequest final
    {
        uint64 Id = 0;
        TWeakObjectPtr<AActor> Actor;
        TWeakObjectPtr<UWorld> World;
        TStrongObjectPtr<UECLatentCallProxy> CallbackTarget;
        FDelegateHandle CompletionHandle;
        uec_latent_function_callback Callback = nullptr;
        void* UserData = nullptr;
        bool Cancelled = false;
        bool InCallback = false;
    };

    static constexpr uint32 MaxLatentFunctionRequests = 1024;
    static constexpr int32 LatentFunctionRequestUUID = 1;

    static bool IsLatentInfoParameter(FProperty* property)
    {
        if (property == nullptr || !property->HasAnyPropertyFlags(CPF_Parm)) return false;
        FStructProperty* structProperty = CastField<FStructProperty>(property);
        return structProperty != nullptr &&
               structProperty->Struct == FLatentActionInfo::StaticStruct();
    }

    static uec_result CollectLatentFunctionParameters(
        UFunction* function,
        TArray<FProperty*>& inputParameters,
        FStructProperty** outLatentInfoProperty)
    {
        inputParameters.Reset();
        if (outLatentInfoProperty != nullptr) *outLatentInfoProperty = nullptr;
        if (function == nullptr || outLatentInfoProperty == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }

        for (TFieldIterator<FProperty> iterator(function); iterator; ++iterator)
        {
            FProperty* property = *iterator;
            if (!property->HasAnyPropertyFlags(CPF_Parm)) continue;
            if (IsLatentInfoParameter(property))
            {
                if (*outLatentInfoProperty != nullptr) return UEC_RESULT_UNSUPPORTED;
                *outLatentInfoProperty = CastFieldChecked<FStructProperty>(property);
                continue;
            }
            if (property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm)) {
                return UEC_RESULT_UNSUPPORTED;
            }
            inputParameters.Add(property);
        }

        return *outLatentInfoProperty != nullptr ? UEC_RESULT_OK : UEC_RESULT_UNSUPPORTED;
    }

    static bool IsSupportedLatentInputProperty(const FProperty* property)
    {
        if (property == nullptr || !property->HasAnyPropertyFlags(CPF_Parm)) return false;
        return IsInvocationScalarProperty(property) ||
               CastField<FObjectPropertyBase>(property) != nullptr ||
               GetPropertyKind(property) != UEC_PROPERTY_UNKNOWN;
    }

    static uec_result ValidateLatentInputProperties(
        const TArray<FProperty*>& inputParameters)
    {
        for (const FProperty* property : inputParameters)
        {
            if (!IsSupportedLatentInputProperty(property)) return UEC_RESULT_UNSUPPORTED;
            if (property->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm |
                                              CPF_ReferenceParm)) {
                return UEC_RESULT_UNSUPPORTED;
            }
        }
        return UEC_RESULT_OK;
    }

    static uec_result ValidateLatentFunctionTarget(
        AActor* actor,
        UFunction* function,
        UWorld** outWorld)
    {
        if (outWorld != nullptr) *outWorld = nullptr;
        if (actor == nullptr || function == nullptr || outWorld == nullptr) {
            return UEC_RESULT_INVALID_HANDLE;
        }
        UWorld* world = actor->GetWorld();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!function->HasAnyFunctionFlags(FUNC_Latent) ||
            function->HasAnyFunctionFlags(FUNC_Net)) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (!function->HasAnyFunctionFlags(
                FUNC_BlueprintCallable | FUNC_Native | FUNC_BlueprintEvent)) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly) &&
            world->GetNetMode() == NM_Client) {
            return UEC_RESULT_UNSUPPORTED;
        }
        *outWorld = world;
        return UEC_RESULT_OK;
    }

    static uec_result RegisterLatentFunctionRequest(
        const TSharedPtr<FUECLatentFunctionRequest>& request)
    {
        if (!request.IsValid()) return UEC_RESULT_INVALID_ARGUMENT;
        FScopeLock lock(&GHandleMutex);
        if (GShuttingDown) return UEC_RESULT_SHUTTING_DOWN;
        if (GLatentFunctionRequests.Num() >= MaxLatentFunctionRequests) {
            return UEC_RESULT_QUEUE_FULL;
        }
        if (!AllocateMonotonicId(GNextLatentFunctionRequestId, request->Id)) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        GLatentFunctionRequests.Add(request->Id, request);
        return UEC_RESULT_OK;
    }

    static void RemoveLatentFunctionCallbackTarget(
        const TSharedPtr<FUECLatentFunctionRequest>& request,
        bool removeEngineActions)
    {
        if (!request.IsValid()) return;
        UECLatentCallProxy* target = request->CallbackTarget.Get();
        UWorld* world = request->World.Get();
        if (removeEngineActions && target != nullptr && world != nullptr)
        {
            world->GetLatentActionManager().RemoveActionsForObject(
                TWeakObjectPtr<UObject>(target));
        }
        if (target != nullptr && request->CompletionHandle.IsValid())
        {
            target->OnCompleted.Remove(request->CompletionHandle);
        }
        request->CompletionHandle.Reset();
    }

    static bool CancelLatentFunctionRequestInternal(uint64 requestId)
    {
        TSharedPtr<FUECLatentFunctionRequest>* requestPointer =
            GLatentFunctionRequests.Find(requestId);
        if (requestPointer == nullptr || !requestPointer->IsValid()) return false;

        TSharedPtr<FUECLatentFunctionRequest> request = *requestPointer;
        request->Cancelled = true;
        request->Callback = nullptr;
        request->UserData = nullptr;
        RemoveLatentFunctionCallbackTarget(request, true);
        GLatentFunctionRequests.Remove(requestId);
        return true;
    }

    static void CompleteLatentFunctionRequest(
        const TWeakPtr<FUECLatentFunctionRequest>& weakRequest)
    {
        TSharedPtr<FUECLatentFunctionRequest> request = weakRequest.Pin();
        if (!request.IsValid()) return;

        TSharedPtr<FUECLatentFunctionRequest>* current =
            GLatentFunctionRequests.Find(request->Id);
        if (current == nullptr || !current->IsValid() || current->Get() != request.Get()) {
            return;
        }
        if (request->Cancelled || IsShuttingDown()) {
            GLatentFunctionRequests.Remove(request->Id);
            request->Cancelled = true;
            request->Callback = nullptr;
            request->UserData = nullptr;
            request->CompletionHandle.Reset();
            return;
        }

        GLatentFunctionRequests.Remove(request->Id);
        const uint64 requestId = request->Id;
        const uec_latent_function_callback callback = request->Callback;
        void* userData = request->UserData;
        request->InCallback = true;
        if (callback != nullptr)
        {
            FUECCallbackScope callbackScope;
            callback(requestId, UEC_RESULT_OK, userData);
        }
        request->InCallback = false;
        request->Cancelled = true;
        request->Callback = nullptr;
        request->UserData = nullptr;
        request->CompletionHandle.Reset();
    }

    static uec_result MarshalLatentFunctionInputs(
        AActor* actor,
        const TArray<FProperty*>& inputParameters,
        const uec_function_argument* arguments,
        uint32_t argumentCount,
        uint8* parameterMemory)
    {
        if (actor == nullptr || parameterMemory == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (argumentCount != static_cast<uint32>(inputParameters.Num())) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        for (uint32 index = 0; index < argumentCount; ++index)
        {
            FProperty* property = inputParameters[static_cast<int32>(index)];
            const uec_result result = SetInvocationFunctionArgument(
                property, parameterMemory, arguments[index], actor);
            if (result != UEC_RESULT_OK) return result;
        }
        return UEC_RESULT_OK;
    }

    static void SetLatentFunctionInfo(
        FStructProperty* latentInfoProperty,
        uint8* parameterMemory,
        UECLatentCallProxy* callbackTarget)
    {
        check(latentInfoProperty != nullptr);
        check(parameterMemory != nullptr);
        check(callbackTarget != nullptr);
        void* valueMemory = latentInfoProperty->ContainerPtrToValuePtr<void>(parameterMemory);
        FLatentActionInfo* info = static_cast<FLatentActionInfo*>(valueMemory);
        *info = FLatentActionInfo(0, LatentFunctionRequestUUID,
                                  TEXT("OnLatentActionCompleted"), callbackTarget);
    }

    static bool HasRegisteredLatentAction(UWorld* world, UECLatentCallProxy* callbackTarget)
    {
        if (world == nullptr || callbackTarget == nullptr) return false;
        return world->GetLatentActionManager().FindExistingAction<FPendingLatentAction>(
                   callbackTarget, LatentFunctionRequestUUID) != nullptr;
    }

    static uec_result UEC_CALL InvokeActorFunctionLatent(
        uec_actor* rawActor,
        uec_string_view functionName,
        const uec_function_argument* arguments,
        uint32_t argumentCount,
        uec_latent_function_callback callback,
        void* userData,
        uint64_t* outRequestId)
    {
        if (outRequestId != nullptr) *outRequestId = 0;
        if (outRequestId == nullptr || callback == nullptr ||
            (argumentCount != 0 && arguments == nullptr) ||
            !IsValidStringView(functionName) || functionName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        const uec_result argumentRecordsResult = ValidateFunctionArgumentRecords(
            arguments, argumentCount);
        if (argumentRecordsResult != UEC_RESULT_OK) return argumentRecordsResult;

        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;

        UFunction* function = actor->FindFunction(FName(*ToFString(functionName)));
        if (function == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        UWorld* world = nullptr;
        const uec_result targetResult = ValidateLatentFunctionTarget(actor, function, &world);
        if (targetResult != UEC_RESULT_OK) return targetResult;

        TArray<FProperty*> inputParameters;
        FStructProperty* latentInfoProperty = nullptr;
        const uec_result parametersResult = CollectLatentFunctionParameters(
            function, inputParameters, &latentInfoProperty);
        if (parametersResult != UEC_RESULT_OK) return parametersResult;
        const uec_result inputPropertiesResult =
            ValidateLatentInputProperties(inputParameters);
        if (inputPropertiesResult != UEC_RESULT_OK) return inputPropertiesResult;
        if (argumentCount != static_cast<uint32>(inputParameters.Num())) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (GLatentFunctionRequests.Num() >= MaxLatentFunctionRequests) {
            return UEC_RESULT_QUEUE_FULL;
        }

        EnsureActorDestroyedHandler(world);
        if (!GActorDestroyedHandlers.Contains(world)) return UEC_RESULT_INTERNAL_ERROR;

        FStructOnScope parameters(function);
        uint8* parameterMemory = parameters.GetStructMemory();
        const uec_result marshalResult = MarshalLatentFunctionInputs(
            actor, inputParameters, arguments, argumentCount, parameterMemory);
        if (marshalResult != UEC_RESULT_OK) return marshalResult;

        UECLatentCallProxy* callbackTarget = NewObject<UECLatentCallProxy>(GetTransientPackage());
        if (callbackTarget == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        SetLatentFunctionInfo(latentInfoProperty, parameterMemory, callbackTarget);

        TSharedPtr<FUECLatentFunctionRequest> request =
            MakeShared<FUECLatentFunctionRequest>();
        request->Actor = actor;
        request->World = world;
        request->CallbackTarget = TStrongObjectPtr<UECLatentCallProxy>(callbackTarget);
        request->Callback = callback;
        request->UserData = userData;
        TWeakPtr<FUECLatentFunctionRequest> weakRequest = request;
        request->CompletionHandle = callbackTarget->OnCompleted.AddLambda(
            [weakRequest]() { CompleteLatentFunctionRequest(weakRequest); });

        const uec_result registerResult = RegisterLatentFunctionRequest(request);
        if (registerResult != UEC_RESULT_OK)
        {
            request->Cancelled = true;
            request->Callback = nullptr;
            request->UserData = nullptr;
            RemoveLatentFunctionCallbackTarget(request, false);
            return registerResult;
        }

        *outRequestId = request->Id;
        actor->ProcessEvent(function, parameterMemory);

        if (!GLatentFunctionRequests.Contains(request->Id)) {
            return UEC_RESULT_OK;
        }
        if (!HasRegisteredLatentAction(world, callbackTarget))
        {
            CancelLatentFunctionRequestInternal(request->Id);
            *outRequestId = 0;
            return UEC_RESULT_UNSUPPORTED;
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL CancelActorFunctionLatent(
        uec_context* rawContext,
        uint64_t requestId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (requestId == 0) return UEC_RESULT_INVALID_ARGUMENT;
        return CancelLatentFunctionRequestInternal(requestId)
                   ? UEC_RESULT_OK : UEC_RESULT_INVALID_ARGUMENT;
    }

    static void CancelLatentFunctionRequestsForActor(AActor* actor)
    {
        if (actor == nullptr) return;
        TArray<uint64> requestIds;
        for (const TPair<uint64, TSharedPtr<FUECLatentFunctionRequest>>& pair :
             GLatentFunctionRequests)
        {
            if (pair.Value.IsValid() && pair.Value->Actor.Get() == actor) {
                requestIds.Add(pair.Key);
            }
        }
        for (uint64 requestId : requestIds) {
            CancelLatentFunctionRequestInternal(requestId);
        }
    }

    static void CancelLatentFunctionRequestsForWorld(UWorld* world)
    {
        if (world == nullptr) return;
        TArray<uint64> requestIds;
        for (const TPair<uint64, TSharedPtr<FUECLatentFunctionRequest>>& pair :
             GLatentFunctionRequests)
        {
            if (pair.Value.IsValid() && pair.Value->World.Get() == world) {
                requestIds.Add(pair.Key);
            }
        }
        for (uint64 requestId : requestIds) {
            CancelLatentFunctionRequestInternal(requestId);
        }
    }

    static void CancelAllLatentFunctionRequests()
    {
        TArray<uint64> requestIds;
        GLatentFunctionRequests.GetKeys(requestIds);
        for (uint64 requestId : requestIds) {
            CancelLatentFunctionRequestInternal(requestId);
        }
        GLatentFunctionRequests.Empty();
    }
