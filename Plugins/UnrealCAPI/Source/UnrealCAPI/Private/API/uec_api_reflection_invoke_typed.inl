/* Typed reflected function invocation and argument marshaling. */
    static uec_result SetInvocationPropertyValue(FProperty* property,
                                                 void* container,
                                                 const uec_property_value& value)
    {
        if (property == nullptr || container == nullptr || value.struct_size < sizeof(value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (FBoolProperty* boolProperty = CastField<FBoolProperty>(property))
        {
            if (value.kind != UEC_PROPERTY_BOOL || !IsValidBool(value.bool_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            boolProperty->SetPropertyValue_InContainer(container, value.bool_value != UEC_FALSE);
            return UEC_RESULT_OK;
        }
        if (FEnumProperty* enumProperty = CastField<FEnumProperty>(property))
        {
            if ((value.kind != UEC_PROPERTY_INTEGER && value.kind != UEC_PROPERTY_ENUM) ||
                !IsIntegerValueInRange(enumProperty->GetUnderlyingProperty(), value.integer_value) ||
                !IsValidEnumValue(property, value.integer_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            enumProperty->GetUnderlyingProperty()->SetNumericPropertyValueFromString_InContainer(
                container, *LexToString(value.integer_value));
            return UEC_RESULT_OK;
        }
        if (FNumericProperty* numericProperty = CastField<FNumericProperty>(property))
        {
            if (numericProperty->IsFloatingPoint())
            {
                if ((value.kind != UEC_PROPERTY_FLOAT && value.kind != UEC_PROPERTY_DOUBLE) ||
                    !FMath::IsFinite(value.real_value) ||
                    (CastField<FFloatProperty>(property) && !IsRepresentableFloat(value.real_value))) {
                    return UEC_RESULT_INVALID_ARGUMENT;
                }
                numericProperty->SetFloatingPointPropertyValue(
                    numericProperty->ContainerPtrToValuePtr<void>(container), value.real_value);
                return UEC_RESULT_OK;
            }
            if (numericProperty->IsInteger())
            {
                if ((value.kind != UEC_PROPERTY_INTEGER && value.kind != UEC_PROPERTY_ENUM) ||
                    (numericProperty->IsEnum() && !IsValidEnumValue(property, value.integer_value)) ||
                    !IsIntegerValueInRange(numericProperty, value.integer_value)) {
                    return UEC_RESULT_INVALID_ARGUMENT;
                }
                numericProperty->SetNumericPropertyValueFromString_InContainer(
                    container, *LexToString(value.integer_value));
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_UNSUPPORTED;
    }

    static uec_result ReadInvocationPropertyValue(FProperty* property,
                                                  const void* container,
                                                  uec_property_value* outValue)
    {
        if (property == nullptr || container == nullptr || outValue == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        outValue->kind = UEC_PROPERTY_UNKNOWN;
        outValue->bool_value = UEC_FALSE;
        outValue->integer_value = 0;
        outValue->real_value = 0.0;
        if (const FBoolProperty* boolProperty = CastField<FBoolProperty>(property))
        {
            outValue->kind = UEC_PROPERTY_BOOL;
            outValue->bool_value = boolProperty->GetPropertyValue_InContainer(container)
                ? UEC_TRUE : UEC_FALSE;
            return UEC_RESULT_OK;
        }
        if (const FEnumProperty* enumProperty = CastField<FEnumProperty>(property))
        {
            outValue->kind = UEC_PROPERTY_ENUM;
            return TryReadIntegerProperty(enumProperty->GetUnderlyingProperty(), container,
                                          outValue->integer_value)
                ? UEC_RESULT_OK : UEC_RESULT_UNSUPPORTED;
        }
        if (const FNumericProperty* numericProperty = CastField<FNumericProperty>(property))
        {
            if (numericProperty->IsFloatingPoint())
            {
                outValue->kind = CastField<FFloatProperty>(property)
                    ? UEC_PROPERTY_FLOAT : UEC_PROPERTY_DOUBLE;
                outValue->real_value = numericProperty->GetFloatingPointPropertyValue_InContainer(container);
                return UEC_RESULT_OK;
            }
            if (numericProperty->IsInteger())
            {
                outValue->kind = UEC_PROPERTY_INTEGER;
                return TryReadIntegerProperty(numericProperty, container,
                                              outValue->integer_value)
                    ? UEC_RESULT_OK : UEC_RESULT_UNSUPPORTED;
            }
        }
        return UEC_RESULT_UNSUPPORTED;
    }

    uec_result UEC_CALL InvokeActorFunctionValue(
        uec_actor* rawActor,
        uec_string_view functionName,
        const uec_property_value* argumentValues,
        uint32_t argumentCount,
        uec_property_value* outReturnValue)
    {
        if (outReturnValue != nullptr)
        {
            if (outReturnValue->struct_size < sizeof(uec_property_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            outReturnValue->kind = UEC_PROPERTY_UNKNOWN;
            outReturnValue->bool_value = UEC_FALSE;
            outReturnValue->integer_value = 0;
            outReturnValue->real_value = 0.0;
        }
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(functionName) || functionName.size == 0 ||
            (argumentCount != 0 && argumentValues == nullptr)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UFunction* function = actor->FindFunction(FName(*ToFString(functionName)));
        if (function == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (function->HasAnyFunctionFlags(FUNC_Latent | FUNC_Net) ||
            (function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly) &&
             actor->GetWorld() != nullptr && actor->GetWorld()->GetNetMode() == NM_Client)) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (!function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_Native | FUNC_BlueprintEvent)) {
            return UEC_RESULT_UNSUPPORTED;
        }

        TArray<FProperty*> inputParameters;
        TArray<FProperty*> outputParameters;
        FProperty* outputProperty = nullptr;
        for (TFieldIterator<FProperty> iterator(function); iterator; ++iterator)
        {
            FProperty* parameter = *iterator;
            if (!parameter->HasAnyPropertyFlags(CPF_Parm)) continue;
            if (parameter->HasAnyPropertyFlags(CPF_ReturnParm)) {
                outputProperty = parameter;
                continue;
            }
            if (parameter->HasAnyPropertyFlags(CPF_OutParm)) outputParameters.Add(parameter);
            if (!parameter->HasAnyPropertyFlags(CPF_OutParm) ||
                parameter->HasAnyPropertyFlags(CPF_ReferenceParm)) inputParameters.Add(parameter);
        }
        if (argumentCount != static_cast<uint32_t>(inputParameters.Num())) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (outputProperty == nullptr && outputParameters.Num() != 0) outputProperty = outputParameters[0];
        if (outputProperty != nullptr && outReturnValue == nullptr) return UEC_RESULT_INVALID_ARGUMENT;

        FStructOnScope parameters(function);
        uint8* parameterMemory = parameters.GetStructMemory();
        for (uint32_t index = 0; index < argumentCount; ++index)
        {
            const uec_result result = SetInvocationPropertyValue(
                inputParameters[static_cast<int32>(index)], parameterMemory, argumentValues[index]);
            if (result != UEC_RESULT_OK) return result;
        }
        actor->ProcessEvent(function, parameterMemory);
        if (outputProperty == nullptr) return UEC_RESULT_OK;
        return ReadInvocationPropertyValue(outputProperty, parameterMemory, outReturnValue);
    }

    uec_result UEC_CALL InvokeActorFunctionValues(
        uec_actor* rawActor,
        uec_string_view functionName,
        const uec_property_value* argumentValues,
        uint32_t argumentCount,
        uec_property_value* outValues,
        uint32_t outCapacity,
        uint32_t* outCount)
    {
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outCount = 0;
        if (outCapacity != 0 && outValues == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(functionName) || functionName.size == 0 ||
            (argumentCount != 0 && argumentValues == nullptr)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UFunction* function = actor->FindFunction(FName(*ToFString(functionName)));
        if (function == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (function->HasAnyFunctionFlags(FUNC_Latent | FUNC_Net) ||
            (function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly) &&
             actor->GetWorld() != nullptr && actor->GetWorld()->GetNetMode() == NM_Client)) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (!function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_Native | FUNC_BlueprintEvent)) {
            return UEC_RESULT_UNSUPPORTED;
        }

        TArray<FProperty*> inputParameters;
        TArray<FProperty*> outputParameters;
        FProperty* returnProperty = nullptr;
        for (TFieldIterator<FProperty> iterator(function); iterator; ++iterator)
        {
            FProperty* parameter = *iterator;
            if (!parameter->HasAnyPropertyFlags(CPF_Parm)) continue;
            if (parameter->HasAnyPropertyFlags(CPF_ReturnParm)) {
                returnProperty = parameter;
                continue;
            }
            if (parameter->HasAnyPropertyFlags(CPF_OutParm)) outputParameters.Add(parameter);
            if (!parameter->HasAnyPropertyFlags(CPF_OutParm) ||
                parameter->HasAnyPropertyFlags(CPF_ReferenceParm)) inputParameters.Add(parameter);
        }
        if (argumentCount != static_cast<uint32_t>(inputParameters.Num())) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }

        TArray<FProperty*> outputProperties;
        if (returnProperty != nullptr) outputProperties.Add(returnProperty);
        outputProperties.Append(outputParameters);
        if (static_cast<uint64>(outputProperties.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outCount = static_cast<uint32_t>(outputProperties.Num());
        if (static_cast<uint64>(outputProperties.Num()) > outCapacity) {
            return UEC_RESULT_BUFFER_TOO_SMALL;
        }
        for (uint32_t index = 0; index < *outCount; ++index)
        {
            if (outValues[index].struct_size < sizeof(uec_property_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
        }

        FStructOnScope parameters(function);
        uint8* parameterMemory = parameters.GetStructMemory();
        for (uint32_t index = 0; index < argumentCount; ++index)
        {
            const uec_result result = SetInvocationPropertyValue(
                inputParameters[static_cast<int32>(index)], parameterMemory, argumentValues[index]);
            if (result != UEC_RESULT_OK) return result;
        }
        actor->ProcessEvent(function, parameterMemory);
        for (uint32_t index = 0; index < *outCount; ++index)
        {
            const uec_result result = ReadInvocationPropertyValue(
                outputProperties[static_cast<int32>(index)], parameterMemory, &outValues[index]);
            if (result != UEC_RESULT_OK) return result;
        }
        return UEC_RESULT_OK;
    }

    static uec_result SetInvocationFunctionArgument(FProperty* property,
                                                    void* container,
                                                    const uec_function_argument& argument,
                                                    AActor* actor)
    {
        if (property == nullptr || container == nullptr || actor == nullptr ||
            argument.struct_size < sizeof(uec_function_argument)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (FClassProperty* classProperty = CastField<FClassProperty>(property))
        {
            if (argument.kind != UEC_PROPERTY_CLASS || argument.object_value != nullptr ||
                argument.world_value != nullptr) return UEC_RESULT_INVALID_ARGUMENT;
            UClass* value = nullptr;
            if (argument.class_value != nullptr)
            {
                auto* handle = reinterpret_cast<FUECClass*>(argument.class_value);
                if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
                value = handle->Value.Get();
                if (value == nullptr) return UEC_RESULT_INVALID_HANDLE;
                if (classProperty->MetaClass != nullptr && !value->IsChildOf(classProperty->MetaClass)) {
                    return UEC_RESULT_INVALID_ARGUMENT;
                }
            }
            classProperty->SetObjectPropertyValue_InContainer(container, value);
            return UEC_RESULT_OK;
        }
        if (FObjectPropertyBase* objectProperty = CastField<FObjectPropertyBase>(property))
        {
            if (argument.kind != UEC_PROPERTY_OBJECT || argument.class_value != nullptr ||
                (argument.object_value != nullptr && argument.world_value != nullptr)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            UObject* value = nullptr;
            if (argument.object_value != nullptr)
            {
                auto* handle = reinterpret_cast<FUECObject*>(argument.object_value);
                if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
                value = handle->Value.Get();
                if (value == nullptr) return UEC_RESULT_INVALID_HANDLE;
                // Enforce same-world inputs without relying on editor-only function metadata.
                UWorld* valueWorld = Cast<UWorld>(value);
                if (valueWorld == nullptr && GEngine != nullptr) {
                    valueWorld = GEngine->GetWorldFromContextObject(
                        value, EGetWorldErrorMode::ReturnNull);
                }
                if (valueWorld != nullptr && valueWorld != actor->GetWorld()) {
                    return UEC_RESULT_INVALID_ARGUMENT;
                }
            }
            else if (argument.world_value != nullptr)
            {
                auto* handle = reinterpret_cast<FUECWorld*>(argument.world_value);
                if (!IsValidWorld(handle)) return UEC_RESULT_INVALID_HANDLE;
                value = handle->Value.Get();
                if (value == nullptr) return UEC_RESULT_INVALID_HANDLE;
                if (value != actor->GetWorld()) return UEC_RESULT_INVALID_ARGUMENT;
            }
            if (value != nullptr && objectProperty->PropertyClass != nullptr &&
                !value->IsA(objectProperty->PropertyClass)) return UEC_RESULT_INVALID_ARGUMENT;
            objectProperty->SetObjectPropertyValue_InContainer(container, value);
            return UEC_RESULT_OK;
        }

        uec_property_value scalar = {};
        scalar.struct_size = sizeof(scalar);
        scalar.kind = argument.kind;
        scalar.bool_value = argument.bool_value;
        scalar.integer_value = argument.integer_value;
        scalar.real_value = argument.real_value;
        const uec_result scalarResult = SetInvocationPropertyValue(property, container, scalar);
        if (scalarResult != UEC_RESULT_UNSUPPORTED) return scalarResult;
        const uec_property_kind expectedKind = GetPropertyKind(property);
        if (expectedKind == UEC_PROPERTY_UNKNOWN || argument.kind != expectedKind ||
            !IsValidStringView(argument.text_value)) return UEC_RESULT_INVALID_ARGUMENT;
        const FString text = ToFString(argument.text_value);
        return property->ImportText_InContainer(*text, container, actor, PPF_None, GWarn) != nullptr
            ? UEC_RESULT_OK : UEC_RESULT_INVALID_ARGUMENT;
    }

    static bool IsTextBackedFunctionArgumentKind(uec_property_kind kind)
    {
        switch (kind)
        {
        case UEC_PROPERTY_STRING:
        case UEC_PROPERTY_NAME:
        case UEC_PROPERTY_TEXT:
        case UEC_PROPERTY_STRUCT:
        case UEC_PROPERTY_ARRAY:
        case UEC_PROPERTY_MAP:
        case UEC_PROPERTY_SET:
        case UEC_PROPERTY_SOFT_OBJECT:
        case UEC_PROPERTY_SOFT_CLASS:
            return true;
        default:
            return false;
        }
    }

    static uec_result ValidateFunctionArgumentRecords(
        const uec_function_argument* arguments,
        uint32_t argumentCount)
    {
        if (argumentCount != 0 && arguments == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        for (uint32_t index = 0; index < argumentCount; ++index)
        {
            const uec_function_argument& argument = arguments[index];
            if (argument.struct_size < sizeof(uec_function_argument) ||
                (argument.kind != UEC_PROPERTY_OBJECT &&
                 (argument.object_value != nullptr || argument.world_value != nullptr)) ||
                (argument.kind != UEC_PROPERTY_CLASS && argument.class_value != nullptr) ||
                (argument.object_value != nullptr && argument.world_value != nullptr)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            const bool hasText = argument.text_value.data != nullptr ||
                argument.text_value.size != 0;
            if (hasText && !IsTextBackedFunctionArgumentKind(argument.kind)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            if (hasText && !IsValidStringView(argument.text_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
        }
        return UEC_RESULT_OK;
    }

    static bool IsInvocationScalarProperty(const FProperty* property)
    {
        return CastField<FBoolProperty>(property) != nullptr ||
            CastField<FEnumProperty>(property) != nullptr ||
            CastField<FNumericProperty>(property) != nullptr;
    }

    static void ResetInvocationFunctionOutput(uec_function_output& output)
    {
        output.kind = UEC_PROPERTY_UNKNOWN;
        output.bool_value = UEC_FALSE;
        output.integer_value = 0;
        output.real_value = 0.0;
        output.object_value = nullptr;
        output.class_value = nullptr;
        output.text_required_size = 0;
    }

    static void DiscardInvocationObjectHandle(FUECObject* handle)
    {
        if (handle == nullptr) return;
        TombstoneHandle(handle->Header);
        handle->Value.Reset();
    }

    static void DiscardInvocationClassHandle(FUECClass* handle)
    {
        if (handle == nullptr) return;
        TombstoneHandle(handle->Header);
        handle->Value.Reset();
    }

    uec_result UEC_CALL InvokeActorFunctionArguments(
        uec_actor* rawActor,
        uec_string_view functionName,
        const uec_function_argument* arguments,
        uint32_t argumentCount,
        uec_function_output* outputs,
        uint32_t outputCapacity,
        uint32_t* outCount)
    {
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outCount = 0;
        if ((argumentCount != 0 && arguments == nullptr) ||
            (outputCapacity != 0 && outputs == nullptr) || !IsValidStringView(functionName) ||
            functionName.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
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
        if (function->HasAnyFunctionFlags(FUNC_Latent | FUNC_Net) ||
            (function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly) &&
             actor->GetWorld() != nullptr && actor->GetWorld()->GetNetMode() == NM_Client)) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (!function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_Native | FUNC_BlueprintEvent)) {
            return UEC_RESULT_UNSUPPORTED;
        }

        TArray<FProperty*> inputParameters;
        TArray<FProperty*> outputProperties;
        FProperty* returnProperty = nullptr;
        for (TFieldIterator<FProperty> iterator(function); iterator; ++iterator)
        {
            FProperty* parameter = *iterator;
            if (!parameter->HasAnyPropertyFlags(CPF_Parm)) continue;
            if (parameter->HasAnyPropertyFlags(CPF_ReturnParm)) {
                returnProperty = parameter;
                continue;
            }
            if (parameter->HasAnyPropertyFlags(CPF_OutParm)) outputProperties.Add(parameter);
            if (!parameter->HasAnyPropertyFlags(CPF_OutParm) ||
                parameter->HasAnyPropertyFlags(CPF_ReferenceParm)) inputParameters.Add(parameter);
        }
        if (returnProperty != nullptr) outputProperties.Insert(returnProperty, 0);
        if (argumentCount != static_cast<uint32_t>(inputParameters.Num())) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (static_cast<uint64>(outputProperties.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outCount = static_cast<uint32_t>(outputProperties.Num());
        if (static_cast<uint64>(outputProperties.Num()) > outputCapacity) {
            return UEC_RESULT_BUFFER_TOO_SMALL;
        }
        for (uint32_t index = 0; index < *outCount; ++index)
        {
            uec_function_output& output = outputs[index];
            if (output.struct_size < sizeof(uec_function_output) ||
                (output.text_buffer_size != 0 && output.text_buffer == nullptr)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            FProperty* property = outputProperties[static_cast<int32>(index)];
            if (!IsInvocationScalarProperty(property) &&
                CastField<FObjectPropertyBase>(property) == nullptr &&
                GetPropertyKind(property) == UEC_PROPERTY_UNKNOWN) return UEC_RESULT_UNSUPPORTED;
        }

        FStructOnScope parameters(function);
        uint8* parameterMemory = parameters.GetStructMemory();
        for (uint32_t index = 0; index < argumentCount; ++index)
        {
            const uec_result result = SetInvocationFunctionArgument(
                inputParameters[static_cast<int32>(index)], parameterMemory,
                arguments[index], actor);
            if (result != UEC_RESULT_OK) return result;
        }
        for (uint32_t index = 0; index < *outCount; ++index) {
            ResetInvocationFunctionOutput(outputs[index]);
        }
        actor->ProcessEvent(function, parameterMemory);

        TArray<uec_property_value> scalarValues;
        TArray<FString> textValues;
        TArray<UObject*> objectValues;
        TArray<UClass*> classValues;
        TArray<uint8> outputKinds;
        scalarValues.SetNum(*outCount);
        textValues.SetNum(*outCount);
        objectValues.SetNumZeroed(*outCount);
        classValues.SetNumZeroed(*outCount);
        outputKinds.SetNumZeroed(*outCount);
        for (uec_property_value& value : scalarValues) {
            value.struct_size = sizeof(uec_property_value);
        }
        for (uint32_t index = 0; index < *outCount; ++index)
        {
            FProperty* property = outputProperties[static_cast<int32>(index)];
            if (FClassProperty* classProperty = CastField<FClassProperty>(property)) {
                classValues[static_cast<int32>(index)] = Cast<UClass>(
                    classProperty->GetObjectPropertyValue_InContainer(parameterMemory));
                outputKinds[static_cast<int32>(index)] = 3;
            }
            else if (FObjectPropertyBase* objectProperty = CastField<FObjectPropertyBase>(property)) {
                objectValues[static_cast<int32>(index)] =
                    objectProperty->GetObjectPropertyValue_InContainer(parameterMemory);
                outputKinds[static_cast<int32>(index)] = 2;
            }
            else if (IsInvocationScalarProperty(property)) {
                const uec_result result = ReadInvocationPropertyValue(
                    property, parameterMemory, &scalarValues[static_cast<int32>(index)]);
                if (result != UEC_RESULT_OK) return result;
                outputKinds[static_cast<int32>(index)] = 1;
            }
            else if (!property->ExportText_InContainer(0, textValues[static_cast<int32>(index)],
                                                       parameterMemory, nullptr, actor,
                                                       PPF_None, actor)) {
                return UEC_RESULT_UNSUPPORTED;
            }
        }

        TArray<FUECObject*> createdObjects;
        TArray<FUECClass*> createdClasses;
        createdObjects.SetNumZeroed(*outCount);
        createdClasses.SetNumZeroed(*outCount);
        for (uint32_t index = 0; index < *outCount; ++index)
        {
            if (outputKinds[static_cast<int32>(index)] == 2 &&
                objectValues[static_cast<int32>(index)] != nullptr) {
                createdObjects[static_cast<int32>(index)] =
                    MakeObjectHandle(objectValues[static_cast<int32>(index)]);
                if (createdObjects[static_cast<int32>(index)] == nullptr) {
                    for (FUECObject* handle : createdObjects) DiscardInvocationObjectHandle(handle);
                    return UEC_RESULT_INTERNAL_ERROR;
                }
            }
            if (outputKinds[static_cast<int32>(index)] == 3 &&
                classValues[static_cast<int32>(index)] != nullptr) {
                createdClasses[static_cast<int32>(index)] =
                    MakeClassPropertyHandle(classValues[static_cast<int32>(index)]);
                if (createdClasses[static_cast<int32>(index)] == nullptr) {
                    for (FUECObject* handle : createdObjects) DiscardInvocationObjectHandle(handle);
                    for (FUECClass* handle : createdClasses) DiscardInvocationClassHandle(handle);
                    return UEC_RESULT_INTERNAL_ERROR;
                }
            }
        }

        uec_result finalResult = UEC_RESULT_OK;
        for (uint32_t index = 0; index < *outCount; ++index)
        {
            uec_function_output& output = outputs[index];
            const uint8 kind = outputKinds[static_cast<int32>(index)];
            if (kind == 1) {
                const uec_property_value& value = scalarValues[static_cast<int32>(index)];
                output.kind = value.kind;
                output.bool_value = value.bool_value;
                output.integer_value = value.integer_value;
                output.real_value = value.real_value;
            }
            else if (kind == 2) {
                output.kind = UEC_PROPERTY_OBJECT;
                output.object_value = reinterpret_cast<uec_object*>(
                    createdObjects[static_cast<int32>(index)]);
            }
            else if (kind == 3) {
                output.kind = UEC_PROPERTY_CLASS;
                output.class_value = reinterpret_cast<uec_class*>(
                    createdClasses[static_cast<int32>(index)]);
            }
            else {
                output.kind = GetPropertyKind(outputProperties[static_cast<int32>(index)]);
                const uec_result result = CopyFStringToUtf8(
                    textValues[static_cast<int32>(index)], output.text_buffer,
                    output.text_buffer_size, &output.text_required_size);
                if (result != UEC_RESULT_OK && finalResult == UEC_RESULT_OK) finalResult = result;
            }
        }
        return finalResult;
    }
