/* Reflected object property setters, function invocation, and object references. */
    uec_result UEC_CALL SetObjectPropertyValue(uec_object* rawObject,
                                               uec_string_view propertyName,
                                               const uec_property_value* value)
    {
        if (value == nullptr || value->struct_size < sizeof(uec_property_value)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(propertyName)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = object->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsWritableProperty(property)) return UEC_RESULT_UNSUPPORTED;
        if (FBoolProperty* boolProperty = CastField<FBoolProperty>(property))
        {
            if (value->kind != UEC_PROPERTY_BOOL || !IsValidBool(value->bool_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            boolProperty->SetPropertyValue_InContainer(object, value->bool_value != UEC_FALSE);
            return UEC_RESULT_OK;
        }
        if (FEnumProperty* enumProperty = CastField<FEnumProperty>(property))
        {
            if (value->kind != UEC_PROPERTY_INTEGER && value->kind != UEC_PROPERTY_ENUM) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            FNumericProperty* underlying = enumProperty->GetUnderlyingProperty();
            if (!IsIntegerValueInRange(underlying, value->integer_value) ||
                !IsValidEnumValue(enumProperty, value->integer_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            const FString text = LexToString(value->integer_value);
            underlying->SetNumericPropertyValueFromString_InContainer(object, *text);
            return UEC_RESULT_OK;
        }
        if (FNumericProperty* numericProperty = CastField<FNumericProperty>(property))
        {
            if (numericProperty->IsFloatingPoint())
            {
                if ((value->kind != UEC_PROPERTY_FLOAT && value->kind != UEC_PROPERTY_DOUBLE) ||
                    !FMath::IsFinite(value->real_value) ||
                    (CastField<FFloatProperty>(property) &&
                     !IsRepresentableFloat(value->real_value))) {
                    return UEC_RESULT_INVALID_ARGUMENT;
                }
                numericProperty->SetFloatingPointPropertyValue(
                    numericProperty->ContainerPtrToValuePtr<void>(object), value->real_value);
                return UEC_RESULT_OK;
            }
            if (numericProperty->IsInteger())
            {
                if (value->kind != UEC_PROPERTY_INTEGER && value->kind != UEC_PROPERTY_ENUM) return UEC_RESULT_INVALID_ARGUMENT;
                if (!IsIntegerValueInRange(numericProperty, value->integer_value)) {
                    return UEC_RESULT_INVALID_ARGUMENT;
                }
                const FString text = LexToString(value->integer_value);
                numericProperty->SetNumericPropertyValueFromString_InContainer(object, *text);
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_UNSUPPORTED;
    }

    uec_result UEC_CALL SetObjectPropertyString(uec_object* rawObject,
                                                uec_string_view propertyName,
                                                uec_string_view value)
    {
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(propertyName) || !IsValidStringView(value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        FProperty* property = object->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsWritableProperty(property)) return UEC_RESULT_UNSUPPORTED;
        const FString text = ToFString(value);
        if (const FStrProperty* stringProperty = CastField<FStrProperty>(property))
        {
            stringProperty->SetPropertyValue_InContainer(object, text);
            return UEC_RESULT_OK;
        }
        if (const FNameProperty* nameProperty = CastField<FNameProperty>(property))
        {
            nameProperty->SetPropertyValue_InContainer(object, FName(*text));
            return UEC_RESULT_OK;
        }
        if (const FTextProperty* textProperty = CastField<FTextProperty>(property))
        {
            textProperty->SetPropertyValue_InContainer(object, FText::FromString(text));
            return UEC_RESULT_OK;
        }
        if (property->ImportText_InContainer(*text, object, object, PPF_None, GWarn) == nullptr)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL InvokeActorFunctionText(
        uec_actor* rawActor,
        uec_string_view functionName,
        const uec_string_view* argumentValues,
        uint32_t argumentCount,
        char* returnBuffer,
        size_t returnBufferSize,
        size_t* returnRequiredSize,
        uec_property_kind* outReturnKind)
    {
        if (returnRequiredSize == nullptr || outReturnKind == nullptr)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        *returnRequiredSize = 0;
        *outReturnKind = UEC_PROPERTY_UNKNOWN;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(functionName) || functionName.size == 0)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (argumentCount != 0 && argumentValues == nullptr)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
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

        TArray<FProperty*> inputParameters;
        TArray<FProperty*> outputParameters;
        FProperty* outputProperty = nullptr;
        for (TFieldIterator<FProperty> iterator(function); iterator; ++iterator)
        {
            FProperty* parameter = *iterator;
            if (!parameter->HasAnyPropertyFlags(CPF_Parm)) continue;
            if (parameter->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                outputProperty = parameter;
                continue;
            }
            if (parameter->HasAnyPropertyFlags(CPF_OutParm))
            {
                outputParameters.Add(parameter);
            }
            if (!parameter->HasAnyPropertyFlags(CPF_OutParm) ||
                parameter->HasAnyPropertyFlags(CPF_ReferenceParm))
            {
                inputParameters.Add(parameter);
            }
        }
        if (argumentCount != static_cast<uint32_t>(inputParameters.Num()))
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        for (uint32_t index = 0; index < argumentCount; ++index)
        {
            if (!IsValidStringView(argumentValues[index])) return UEC_RESULT_INVALID_ARGUMENT;
        }

        FStructOnScope parameters(function);
        uint8* parameterMemory = parameters.GetStructMemory();
        for (int32 index = 0; index < inputParameters.Num(); ++index)
        {
            FProperty* parameter = inputParameters[index];
            const FString text = ToFString(argumentValues[index]);
            if (parameter->ImportText_InContainer(*text, parameterMemory, actor,
                                                   PPF_None, GWarn) == nullptr)
            {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
        }

        actor->ProcessEvent(function, parameterMemory);
        if (outputProperty == nullptr)
        {
            for (FProperty* parameter : outputParameters)
            {
                outputProperty = parameter;
                break;
            }
        }
        if (outputProperty == nullptr) return UEC_RESULT_OK;

        FString outputText;
        if (!outputProperty->ExportText_InContainer(0, outputText, parameterMemory,
                                                    nullptr, actor, PPF_None, actor))
        {
            return UEC_RESULT_UNSUPPORTED;
        }
        *outReturnKind = GetPropertyKind(outputProperty);
        return CopyFStringToUtf8(outputText, returnBuffer, returnBufferSize, returnRequiredSize);
    }

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
                !IsValidEnumValue(enumProperty, value.integer_value)) {
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

    uec_result UEC_CALL InvokeActorFunctionTextValues(
        uec_actor* rawActor,
        uec_string_view functionName,
        const uec_string_view* argumentValues,
        uint32_t argumentCount,
        uec_text_output* outValues,
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
        for (uint32_t index = 0; index < argumentCount; ++index) {
            if (!IsValidStringView(argumentValues[index])) return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (static_cast<uint64>(outputProperties.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outCount = static_cast<uint32_t>(outputProperties.Num());
        if (static_cast<uint64>(outputProperties.Num()) > outCapacity) {
            return UEC_RESULT_BUFFER_TOO_SMALL;
        }
        for (uint32_t index = 0; index < *outCount; ++index) {
            if (outValues[index].struct_size < sizeof(uec_text_output)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            outValues[index].kind = UEC_PROPERTY_UNKNOWN;
            outValues[index].required_size = 0;
        }
        FStructOnScope parameters(function);
        uint8* parameterMemory = parameters.GetStructMemory();
        for (uint32_t index = 0; index < argumentCount; ++index) {
            const FString text = ToFString(argumentValues[index]);
            if (inputParameters[static_cast<int32>(index)]->ImportText_InContainer(
                    *text, parameterMemory, actor, PPF_None, GWarn) == nullptr) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
        }
        actor->ProcessEvent(function, parameterMemory);
        TArray<FString> outputTexts;
        outputTexts.Reserve(outputProperties.Num());
        for (FProperty* property : outputProperties)
        {
            FString text;
            if (!property->ExportText_InContainer(0, text, parameterMemory, nullptr, actor,
                                                  PPF_None, actor)) {
                return UEC_RESULT_UNSUPPORTED;
            }
            outputTexts.Add(MoveTemp(text));
        }
        uec_result finalResult = UEC_RESULT_OK;
        for (uint32_t index = 0; index < *outCount; ++index)
        {
            outValues[index].kind = GetPropertyKind(outputProperties[static_cast<int32>(index)]);
            const uec_result result = CopyFStringToUtf8(
                outputTexts[static_cast<int32>(index)], outValues[index].buffer,
                outValues[index].buffer_size, &outValues[index].required_size);
            if (result != UEC_RESULT_OK) finalResult = result;
        }
        return finalResult;
    }

    uec_result UEC_CALL GetClassFunctionCount(uec_class* rawClass, uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* classHandle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(classHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = classHandle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        *outCount = 0;
        for (TFieldIterator<UFunction> iterator(klass, EFieldIteratorFlags::IncludeSuper); iterator; ++iterator)
        {
            if (*outCount == UINT32_MAX) return UEC_RESULT_INTERNAL_ERROR;
            ++(*outCount);
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetClassFunctionAt(uec_class* rawClass,
                                           uint32_t index,
                                           char* nameBuffer,
                                           size_t nameBufferSize,
                                           size_t* nameRequiredSize,
                                           uint32_t* outParameterCount,
                                           uec_bool* outHasReturnValue,
                                           uec_bool* outIsLatent)
    {
        if (nameRequiredSize == nullptr || outParameterCount == nullptr ||
            outHasReturnValue == nullptr || outIsLatent == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        *nameRequiredSize = 0; *outParameterCount = 0;
        *outHasReturnValue = UEC_FALSE; *outIsLatent = UEC_FALSE;
        auto* classHandle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(classHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = classHandle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        uint32_t current = 0;
        for (TFieldIterator<UFunction> iterator(klass, EFieldIteratorFlags::IncludeSuper); iterator; ++iterator)
        {
            if (current++ != index) continue;
            UFunction* function = *iterator;
            *outParameterCount = 0;
            *outHasReturnValue = UEC_FALSE;
            *outIsLatent = function->HasAnyFunctionFlags(FUNC_Latent) ? UEC_TRUE : UEC_FALSE;
            for (TFieldIterator<FProperty> propertyIterator(function); propertyIterator; ++propertyIterator)
            {
                FProperty* property = *propertyIterator;
                if (!property->HasAnyPropertyFlags(CPF_Parm)) continue;
                if (property->HasAnyPropertyFlags(CPF_ReturnParm)) {
                    *outHasReturnValue = UEC_TRUE;
                }
                else {
                    if (*outParameterCount == UINT32_MAX) return UEC_RESULT_INTERNAL_ERROR;
                    ++(*outParameterCount);
                }
            }
            return CopyFStringToUtf8(function->GetName(), nameBuffer, nameBufferSize, nameRequiredSize);
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    uec_result UEC_CALL GetClassFunctionParameterAt(uec_class* rawClass,
                                                    uint32_t functionIndex,
                                                    uint32_t parameterIndex,
                                                    char* nameBuffer,
                                                    size_t nameBufferSize,
                                                    size_t* nameRequiredSize,
                                                    uec_property_kind* outKind,
                                                    uint32_t* outFlags)
    {
        if (nameRequiredSize == nullptr || outKind == nullptr || outFlags == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        *nameRequiredSize = 0;
        *outKind = UEC_PROPERTY_UNKNOWN;
        *outFlags = 0;
        auto* classHandle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(classHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = classHandle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        uint32_t currentFunction = 0;
        for (TFieldIterator<UFunction> iterator(klass, EFieldIteratorFlags::IncludeSuper);
             iterator; ++iterator)
        {
            if (currentFunction++ != functionIndex) continue;
            UFunction* function = *iterator;
            uint32_t currentParameter = 0;
            for (TFieldIterator<FProperty> propertyIterator(function); propertyIterator;
                 ++propertyIterator)
            {
                FProperty* property = *propertyIterator;
                if (!property->HasAnyPropertyFlags(CPF_Parm)) continue;
                if (currentParameter++ != parameterIndex) continue;
                *outKind = GetPropertyKind(property);
                if (!property->HasAnyPropertyFlags(CPF_OutParm) &&
                    !property->HasAnyPropertyFlags(CPF_ReturnParm)) {
                    *outFlags |= UEC_FUNCTION_PARAMETER_INPUT;
                }
                if (property->HasAnyPropertyFlags(CPF_OutParm)) {
                    *outFlags |= UEC_FUNCTION_PARAMETER_OUT;
                }
                if (property->HasAnyPropertyFlags(CPF_ReturnParm)) {
                    *outFlags |= UEC_FUNCTION_PARAMETER_RETURN;
                }
                if (property->HasAnyPropertyFlags(CPF_ReferenceParm)) {
                    *outFlags |= UEC_FUNCTION_PARAMETER_REFERENCE;
                    *outFlags |= UEC_FUNCTION_PARAMETER_INPUT;
                }
                return CopyFStringToUtf8(property->GetName(), nameBuffer, nameBufferSize,
                                         nameRequiredSize);
            }
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    uec_result UEC_CALL GetActorPropertyObject(uec_actor* rawActor,
                                               uec_string_view propertyName,
                                               uec_object** outObject)
    {
        if (outObject == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outObject = nullptr;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(propertyName)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        FObjectPropertyBase* objectProperty = CastField<FObjectPropertyBase>(property);
        if (objectProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        UObject* value = objectProperty->GetObjectPropertyValue_InContainer(actor);
        if (value == nullptr) return UEC_RESULT_OK;
        FUECObject* handle = MakeObjectHandle(value);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outObject = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetActorPropertyObject(uec_actor* rawActor,
                                               uec_string_view propertyName,
                                               uec_object* rawObject)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(propertyName)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (!IsWritableProperty(property)) return UEC_RESULT_UNSUPPORTED;
        FObjectPropertyBase* objectProperty = CastField<FObjectPropertyBase>(property);
        if (objectProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        UObject* value = nullptr;
        if (rawObject != nullptr)
        {
            auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
            if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
            value = objectHandle->Value.Get();
            if (value == nullptr) return UEC_RESULT_INVALID_HANDLE;
            if (objectProperty->PropertyClass != nullptr && !value->IsA(objectProperty->PropertyClass)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
        }
        objectProperty->SetObjectPropertyValue_InContainer(actor, value);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetObjectPropertyObject(uec_object* rawOwner,
                                                uec_string_view propertyName,
                                                uec_object** outValue)
    {
        if (outValue == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outValue = nullptr;
        auto* ownerHandle = reinterpret_cast<FUECObject*>(rawOwner);
        if (!IsValidObject(ownerHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* owner = ownerHandle->Value.Get();
        if (owner == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(propertyName)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        FObjectPropertyBase* objectProperty = CastField<FObjectPropertyBase>(property);
        if (objectProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        UObject* value = objectProperty->GetObjectPropertyValue_InContainer(owner);
        if (value == nullptr) return UEC_RESULT_OK;
        FUECObject* handle = MakeObjectHandle(value);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outValue = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetObjectPropertyObject(uec_object* rawOwner,
                                                uec_string_view propertyName,
                                                uec_object* rawValue)
    {
        auto* ownerHandle = reinterpret_cast<FUECObject*>(rawOwner);
        if (!IsValidObject(ownerHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* owner = ownerHandle->Value.Get();
        if (owner == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(propertyName)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (!IsWritableProperty(property)) return UEC_RESULT_UNSUPPORTED;
        FObjectPropertyBase* objectProperty = CastField<FObjectPropertyBase>(property);
        if (objectProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        UObject* value = nullptr;
        if (rawValue != nullptr)
        {
            auto* valueHandle = reinterpret_cast<FUECObject*>(rawValue);
            if (!IsValidObject(valueHandle)) return UEC_RESULT_INVALID_HANDLE;
            value = valueHandle->Value.Get();
            if (value == nullptr) return UEC_RESULT_INVALID_HANDLE;
            if (objectProperty->PropertyClass != nullptr && !value->IsA(objectProperty->PropertyClass)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
        }
        objectProperty->SetObjectPropertyValue_InContainer(owner, value);
        return UEC_RESULT_OK;
    }
