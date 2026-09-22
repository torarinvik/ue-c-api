    uec_result UEC_CALL FindClass(uec_context* rawContext,
                                  uec_string_view classPath,
                                  uec_class** outClass)
    {
        if (outClass != nullptr) *outClass = nullptr;
        if (outClass == nullptr || !IsValidStringView(classPath) || classPath.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = LoadClass<UObject>(nullptr, *ToFString(classPath));
        if (klass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = new FUECClass();
        if (!InitializeHandle(handle->Header, EUECHandleKind::Class))
        {
            delete handle;
            return UEC_RESULT_INTERNAL_ERROR;
        }
        handle->Value = klass;
        {
            FScopeLock lock(&GHandleMutex);
            if (GShuttingDown) { delete handle; return UEC_RESULT_SHUTTING_DOWN; }
            GClasses.Add(handle);
        }
        *outClass = reinterpret_cast<uec_class*>(handle);
        return UEC_RESULT_OK;
    }
    static bool IsValidEnumValue(const FEnumProperty* property, int64 value)
    {
        const UEnum* enumeration = property == nullptr ? nullptr : property->GetEnum();
        return enumeration != nullptr && enumeration->IsValidEnumValueOrBitfield(value);
    }
    uec_result UEC_CALL IsClassPathLoaded(uec_context* rawContext,
                                          uec_string_view classPath,
                                          uec_bool* outLoaded)
    {
        if (outLoaded != nullptr) *outLoaded = UEC_FALSE;
        if (outLoaded == nullptr || !IsValidStringView(classPath) || classPath.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outLoaded = FindObject<UClass>(nullptr, *ToFString(classPath)) != nullptr
            ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }
    uec_result UEC_CALL ReleaseClass(uec_class* rawClass)
    {
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TombstoneHandle(handle->Header);
        handle->Value.Reset();
        return UEC_RESULT_OK;
    }
    uec_result UEC_CALL GetClassName(uec_class* rawClass,
                                     char* buffer,
                                     size_t bufferSize,
                                     size_t* requiredSize)
    {
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0;
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = handle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return CopyFStringToUtf8(klass->GetName(), buffer, bufferSize, requiredSize);
    }
    uec_result UEC_CALL ClassIsA(uec_class* rawClass,
                                 uec_string_view parentClassPath,
                                 uec_bool* outIsA)
    {
        if (outIsA != nullptr) *outIsA = UEC_FALSE;
        if (outIsA == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidStringView(parentClassPath) || parentClassPath.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = handle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UClass* parent = LoadClass<UObject>(nullptr, *ToFString(parentClassPath));
        if (parent == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outIsA = klass->IsChildOf(parent) ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }
    uec_result UEC_CALL GetClassPropertyCount(uec_class* rawClass, uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = handle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        *outCount = 0;
        for (TFieldIterator<FProperty> iterator(klass, EFieldIteratorFlags::IncludeSuper); iterator; ++iterator)
        {
            if (*outCount == UINT32_MAX) return UEC_RESULT_INTERNAL_ERROR;
            ++(*outCount);
        }
        return UEC_RESULT_OK;
    }
    uec_result UEC_CALL GetClassPropertyAt(uec_class* rawClass,
                                            uint32_t index,
                                            char* nameBuffer,
                                            size_t nameBufferSize,
                                            size_t* nameRequiredSize,
                                            uec_property_kind* outKind)
    {
        if (nameRequiredSize == nullptr || outKind == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *nameRequiredSize = 0; *outKind = UEC_PROPERTY_UNKNOWN;
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = handle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        uint32_t current = 0;
        for (TFieldIterator<FProperty> iterator(klass, EFieldIteratorFlags::IncludeSuper); iterator; ++iterator)
        {
            if (current++ != index) continue;
            FProperty* property = *iterator;
            *outKind = GetPropertyKind(property);
            return CopyFStringToUtf8(property->GetName(), nameBuffer, nameBufferSize, nameRequiredSize);
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    uec_result UEC_CALL GetClassPropertyFlags(uec_class* rawClass,
                                              uint32_t index,
                                              uint32_t* outFlags)
    {
        if (outFlags != nullptr) *outFlags = 0;
        if (outFlags == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = handle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        uint32_t current = 0;
        for (TFieldIterator<FProperty> iterator(klass, EFieldIteratorFlags::IncludeSuper);
             iterator; ++iterator)
        {
            if (current++ != index) continue;
            const FProperty* property = *iterator;
            if (property->HasAnyPropertyFlags(CPF_EditConst)) *outFlags |= UEC_PROPERTY_FLAG_EDIT_CONST;
            if (property->HasAnyPropertyFlags(CPF_BlueprintReadOnly)) {
                *outFlags |= UEC_PROPERTY_FLAG_BLUEPRINT_READ_ONLY;
            }
            if (property->HasAnyPropertyFlags(CPF_ConstParm)) *outFlags |= UEC_PROPERTY_FLAG_CONST_PARAMETER;
            if (property->HasAnyPropertyFlags(CPF_ReturnParm)) *outFlags |= UEC_PROPERTY_FLAG_RETURN;
            if (property->HasAnyPropertyFlags(CPF_Parm)) *outFlags |= UEC_PROPERTY_FLAG_PARAMETER;
            if (property->HasAnyPropertyFlags(CPF_OutParm)) *outFlags |= UEC_PROPERTY_FLAG_OUT;
            if (property->HasAnyPropertyFlags(CPF_ReferenceParm)) *outFlags |= UEC_PROPERTY_FLAG_REFERENCE;
            return UEC_RESULT_OK;
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    uec_result UEC_CALL GetClassFunctionFlags(uec_class* rawClass,
                                              uint32_t index,
                                              uint32_t* outFlags)
    {
        if (outFlags != nullptr) *outFlags = 0;
        if (outFlags == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* classHandle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(classHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = classHandle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        uint32_t current = 0;
        for (TFieldIterator<UFunction> iterator(klass, EFieldIteratorFlags::IncludeSuper);
             iterator; ++iterator)
        {
            if (current++ != index) continue;
            const UFunction* function = *iterator;
            if (function->HasAnyFunctionFlags(FUNC_BlueprintCallable)) {
                *outFlags |= UEC_FUNCTION_FLAG_BLUEPRINT_CALLABLE;
            }
            if (function->HasAnyFunctionFlags(FUNC_Native)) *outFlags |= UEC_FUNCTION_FLAG_NATIVE;
            if (function->HasAnyFunctionFlags(FUNC_BlueprintEvent)) *outFlags |= UEC_FUNCTION_FLAG_EVENT;
            if (function->HasAnyFunctionFlags(FUNC_Latent)) *outFlags |= UEC_FUNCTION_FLAG_LATENT;
            if (function->HasAnyFunctionFlags(FUNC_Net)) *outFlags |= UEC_FUNCTION_FLAG_NETWORK;
            if (function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly)) {
                *outFlags |= UEC_FUNCTION_FLAG_AUTHORITY_ONLY;
            }
            return UEC_RESULT_OK;
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    uec_result UEC_CALL GetActorPropertyValue(uec_actor* rawActor,
                                              uec_string_view propertyName,
                                              uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value))
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        outValue->kind = UEC_PROPERTY_UNKNOWN;
        outValue->bool_value = UEC_FALSE;
        outValue->integer_value = 0;
        outValue->real_value = 0.0;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(propertyName)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        outValue->kind = GetPropertyKind(property);
        if (FBoolProperty* boolProperty = CastField<FBoolProperty>(property))
        {
            outValue->bool_value = boolProperty->GetPropertyValue_InContainer(actor) ? UEC_TRUE : UEC_FALSE;
            return UEC_RESULT_OK;
        }
        if (FEnumProperty* enumProperty = CastField<FEnumProperty>(property))
        {
            FNumericProperty* underlying = enumProperty->GetUnderlyingProperty();
            if (!TryReadIntegerProperty(underlying, actor, outValue->integer_value)) {
                return UEC_RESULT_UNSUPPORTED;
            }
            return UEC_RESULT_OK;
        }
        if (FNumericProperty* numericProperty = CastField<FNumericProperty>(property))
        {
            if (numericProperty->IsFloatingPoint())
            {
                outValue->real_value = numericProperty->GetFloatingPointPropertyValue_InContainer(actor);
                return UEC_RESULT_OK;
            }
            if (numericProperty->IsInteger())
            {
                if (!TryReadIntegerProperty(numericProperty, actor, outValue->integer_value)) {
                    return UEC_RESULT_UNSUPPORTED;
                }
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_UNSUPPORTED;
    }
    uec_result UEC_CALL GetActorPropertyString(uec_actor* rawActor,
                                               uec_string_view propertyName,
                                               char* buffer,
                                               size_t bufferSize,
                                               size_t* requiredSize,
                                               uec_property_kind* outKind)
    {
        if (requiredSize == nullptr || outKind == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0; *outKind = UEC_PROPERTY_UNKNOWN;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(propertyName)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        FString value;
        *outKind = GetPropertyKind(property);
        if (const FStrProperty* stringProperty = CastField<FStrProperty>(property))
        {
            value = stringProperty->GetPropertyValue_InContainer(actor);
        }
        else if (const FNameProperty* nameProperty = CastField<FNameProperty>(property))
        {
            value = nameProperty->GetPropertyValue_InContainer(actor).ToString();
        }
        else if (const FTextProperty* textProperty = CastField<FTextProperty>(property))
        {
            value = textProperty->GetPropertyValue_InContainer(actor).ToString();
        }
        else if (const FEnumProperty* enumProperty = CastField<FEnumProperty>(property))
        {
            const FNumericProperty* underlying = enumProperty->GetUnderlyingProperty();
            int64 enumValue = 0;
            if (!TryReadIntegerProperty(underlying, actor, enumValue)) return UEC_RESULT_UNSUPPORTED;
            value = enumProperty->GetEnum()->GetNameStringByValue(enumValue);
        }
        else
        {
            if (!property->ExportText_InContainer(0, value, actor, nullptr, actor, PPF_None, actor))
            {
                return UEC_RESULT_UNSUPPORTED;
            }
        }
        return CopyFStringToUtf8(value, buffer, bufferSize, requiredSize);
    }
    uec_result UEC_CALL SetActorPropertyValue(uec_actor* rawActor,
                                              uec_string_view propertyName,
                                              const uec_property_value* value)
    {
        if (value == nullptr || value->struct_size < sizeof(uec_property_value)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(propertyName)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsWritableProperty(property)) return UEC_RESULT_UNSUPPORTED;
        if (FBoolProperty* boolProperty = CastField<FBoolProperty>(property))
        {
            if (value->kind != UEC_PROPERTY_BOOL || !IsValidBool(value->bool_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            boolProperty->SetPropertyValue_InContainer(actor, value->bool_value != UEC_FALSE);
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
            underlying->SetNumericPropertyValueFromString_InContainer(actor, *text);
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
                    numericProperty->ContainerPtrToValuePtr<void>(actor), value->real_value);
                return UEC_RESULT_OK;
            }
            if (numericProperty->IsInteger())
            {
                if (value->kind != UEC_PROPERTY_INTEGER && value->kind != UEC_PROPERTY_ENUM) return UEC_RESULT_INVALID_ARGUMENT;
                if (!IsIntegerValueInRange(numericProperty, value->integer_value)) {
                    return UEC_RESULT_INVALID_ARGUMENT;
                }
                const FString text = LexToString(value->integer_value);
                numericProperty->SetNumericPropertyValueFromString_InContainer(actor, *text);
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_UNSUPPORTED;
    }
    uec_result UEC_CALL SetActorPropertyString(uec_actor* rawActor,
                                               uec_string_view propertyName,
                                               uec_string_view value)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(propertyName) || !IsValidStringView(value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        FProperty* property = actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsWritableProperty(property)) return UEC_RESULT_UNSUPPORTED;
        const FString text = ToFString(value);
        if (const FStrProperty* stringProperty = CastField<FStrProperty>(property))
        {
            stringProperty->SetPropertyValue_InContainer(actor, text);
            return UEC_RESULT_OK;
        }
        if (const FNameProperty* nameProperty = CastField<FNameProperty>(property))
        {
            nameProperty->SetPropertyValue_InContainer(actor, FName(*text));
            return UEC_RESULT_OK;
        }
        if (const FTextProperty* textProperty = CastField<FTextProperty>(property))
        {
            textProperty->SetPropertyValue_InContainer(actor, FText::FromString(text));
            return UEC_RESULT_OK;
        }
        if (property->ImportText_InContainer(*text, actor, actor, PPF_None, GWarn) == nullptr)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        return UEC_RESULT_OK;
    }
    uec_result UEC_CALL GetObjectPropertyValue(uec_object* rawObject,
                                               uec_string_view propertyName,
                                               uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        outValue->kind = UEC_PROPERTY_UNKNOWN;
        outValue->bool_value = UEC_FALSE;
        outValue->integer_value = 0;
        outValue->real_value = 0.0;
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(propertyName)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = object->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        outValue->kind = GetPropertyKind(property);
        if (FBoolProperty* boolProperty = CastField<FBoolProperty>(property))
        {
            outValue->bool_value = boolProperty->GetPropertyValue_InContainer(object) ? UEC_TRUE : UEC_FALSE;
            return UEC_RESULT_OK;
        }
        if (FEnumProperty* enumProperty = CastField<FEnumProperty>(property))
        {
            FNumericProperty* underlying = enumProperty->GetUnderlyingProperty();
            if (!TryReadIntegerProperty(underlying, object, outValue->integer_value)) {
                return UEC_RESULT_UNSUPPORTED;
            }
            return UEC_RESULT_OK;
        }
        if (FNumericProperty* numericProperty = CastField<FNumericProperty>(property))
        {
            if (numericProperty->IsFloatingPoint())
            {
                outValue->real_value = numericProperty->GetFloatingPointPropertyValue_InContainer(object);
                return UEC_RESULT_OK;
            }
            if (numericProperty->IsInteger())
            {
                if (!TryReadIntegerProperty(numericProperty, object, outValue->integer_value)) {
                    return UEC_RESULT_UNSUPPORTED;
                }
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_UNSUPPORTED;
    }

    uec_result UEC_CALL GetObjectPropertyString(uec_object* rawObject,
                                                uec_string_view propertyName,
                                                char* buffer,
                                                size_t bufferSize,
                                                size_t* requiredSize,
                                                uec_property_kind* outKind)
    {
        if (requiredSize == nullptr || outKind == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0; *outKind = UEC_PROPERTY_UNKNOWN;
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(propertyName)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = object->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        FString value;
        *outKind = GetPropertyKind(property);
        if (const FStrProperty* stringProperty = CastField<FStrProperty>(property))
        {
            value = stringProperty->GetPropertyValue_InContainer(object);
        }
        else if (const FNameProperty* nameProperty = CastField<FNameProperty>(property))
        {
            value = nameProperty->GetPropertyValue_InContainer(object).ToString();
        }
        else if (const FTextProperty* textProperty = CastField<FTextProperty>(property))
        {
            value = textProperty->GetPropertyValue_InContainer(object).ToString();
        }
        else if (const FEnumProperty* enumProperty = CastField<FEnumProperty>(property))
        {
            const FNumericProperty* underlying = enumProperty->GetUnderlyingProperty();
            int64 enumValue = 0;
            if (!TryReadIntegerProperty(underlying, object, enumValue)) return UEC_RESULT_UNSUPPORTED;
            value = enumProperty->GetEnum()->GetNameStringByValue(enumValue);
        }
        else
        {
            if (!property->ExportText_InContainer(0, value, object, nullptr, object, PPF_None, object))
            {
                return UEC_RESULT_UNSUPPORTED;
            }
        }
        return CopyFStringToUtf8(value, buffer, bufferSize, requiredSize);
    }
