/* Mixed reflected function arguments and outputs. */
    static uec_result SetInvocationFunctionArgument(FProperty* property,
                                                    void* container,
                                                    const uec_function_argument& argument,
                                                    AActor* actor)
    {
        const size_t legacySize = offsetof(uec_function_argument, struct_value);
        if (property == nullptr || container == nullptr || actor == nullptr ||
            argument.struct_size < legacySize ||
            (argument.struct_size > legacySize &&
             argument.struct_size < sizeof(uec_function_argument))) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (argument.struct_size >= sizeof(uec_function_argument) &&
            argument.struct_value.kind != UEC_FUNCTION_STRUCT_NONE) {
            if (argument.kind != UEC_PROPERTY_STRUCT ||
                argument.text_value.data != nullptr || argument.text_value.size != 0) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            return SetInvocationStructValue(property, container, argument.struct_value);
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
        if (FStrProperty* stringProperty = CastField<FStrProperty>(property))
        {
            FString value;
            int32 consumedCharacters = 0;
            if (!FParse::QuotedString(*text, value, &consumedCharacters) ||
                consumedCharacters != text.Len()) return UEC_RESULT_INVALID_ARGUMENT;
            stringProperty->SetPropertyValue_InContainer(container, value);
            return UEC_RESULT_OK;
        }
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
            const size_t legacySize = offsetof(uec_function_argument, struct_value);
            if (argument.struct_size < legacySize ||
                (argument.struct_size > legacySize &&
                 argument.struct_size < sizeof(uec_function_argument)) ||
                (argument.kind != UEC_PROPERTY_OBJECT &&
                 (argument.object_value != nullptr || argument.world_value != nullptr)) ||
                (argument.kind != UEC_PROPERTY_CLASS && argument.class_value != nullptr) ||
                (argument.object_value != nullptr && argument.world_value != nullptr)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            const bool hasText = argument.text_value.data != nullptr ||
                argument.text_value.size != 0;
            if (argument.struct_size >= sizeof(uec_function_argument) &&
                (!IsSupportedInvocationStructKind(argument.struct_value.kind) ||
                 (argument.struct_value.kind != UEC_FUNCTION_STRUCT_NONE &&
                  (argument.kind != UEC_PROPERTY_STRUCT || hasText)))) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
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
        if (output.struct_size >= sizeof(uec_function_output)) {
            output.struct_value = {};
        }
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
        if (IsLatentFunction(function) || function->HasAnyFunctionFlags(FUNC_Net) ||
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
        TArray<uec_function_struct_kind> requestedStructKinds;
        requestedStructKinds.SetNumZeroed(*outCount);
        for (uint32_t index = 0; index < *outCount; ++index)
        {
            uec_function_output& output = outputs[index];
            const size_t legacySize = offsetof(uec_function_output, struct_value);
            if (output.struct_size < legacySize ||
                (output.struct_size > legacySize &&
                 output.struct_size < sizeof(uec_function_output)) ||
                (output.text_buffer_size != 0 && output.text_buffer == nullptr)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            FProperty* property = outputProperties[static_cast<int32>(index)];
            if (output.struct_size >= sizeof(uec_function_output)) {
                const uec_function_struct_kind requestedKind = output.struct_value.kind;
                if (!IsSupportedInvocationStructKind(requestedKind) ||
                    (requestedKind != UEC_FUNCTION_STRUCT_NONE &&
                     GetInvocationStructKind(property) != requestedKind)) {
                    return UEC_RESULT_INVALID_ARGUMENT;
                }
                requestedStructKinds[static_cast<int32>(index)] = requestedKind;
            }
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
        TArray<uec_function_struct_value> structValues;
        TArray<uint8> outputKinds;
        scalarValues.SetNum(*outCount);
        textValues.SetNum(*outCount);
        objectValues.SetNumZeroed(*outCount);
        classValues.SetNumZeroed(*outCount);
        structValues.SetNumZeroed(*outCount);
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
            else if (requestedStructKinds[static_cast<int32>(index)] !=
                     UEC_FUNCTION_STRUCT_NONE) {
                const uec_result result = ReadInvocationStructValue(
                    property, parameterMemory, &structValues[static_cast<int32>(index)]);
                if (result != UEC_RESULT_OK) return result;
                outputKinds[static_cast<int32>(index)] = 4;
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
                    return HandleCreationFailureResult();
                }
            }
            if (outputKinds[static_cast<int32>(index)] == 3 &&
                classValues[static_cast<int32>(index)] != nullptr) {
                createdClasses[static_cast<int32>(index)] =
                    MakeClassPropertyHandle(classValues[static_cast<int32>(index)]);
                if (createdClasses[static_cast<int32>(index)] == nullptr) {
                    for (FUECObject* handle : createdObjects) DiscardInvocationObjectHandle(handle);
                    for (FUECClass* handle : createdClasses) DiscardInvocationClassHandle(handle);
                    return HandleCreationFailureResult();
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
            else if (kind == 4) {
                output.kind = UEC_PROPERTY_STRUCT;
                output.struct_value = structValues[static_cast<int32>(index)];
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
