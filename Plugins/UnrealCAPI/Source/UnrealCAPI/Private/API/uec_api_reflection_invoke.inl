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
                !IsValidEnumValue(property, value->integer_value)) {
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
                if ((value->kind != UEC_PROPERTY_INTEGER && value->kind != UEC_PROPERTY_ENUM) ||
                    (numericProperty->IsEnum() && !IsValidEnumValue(property, value->integer_value)) ||
                    !IsIntegerValueInRange(numericProperty, value->integer_value)) {
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
            *outIsLatent = IsLatentFunction(function) ? UEC_TRUE : UEC_FALSE;
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
