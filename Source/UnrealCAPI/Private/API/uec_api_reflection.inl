    static bool IsValidEnumValue(const FEnumProperty* property, int64 value)
    {
        const UEnum* enumeration = property == nullptr ? nullptr : property->GetEnum();
        return enumeration != nullptr && enumeration->IsValidEnumValueOrBitfield(value);
    }
    static const UEnum* GetEnumForProperty(const FProperty* property)
    {
        if (const FEnumProperty* enumProperty = CastField<FEnumProperty>(property)) {
            return enumProperty->GetEnum();
        }
        if (const FByteProperty* byteProperty = CastField<FByteProperty>(property)) {
            return byteProperty->GetIntPropertyEnum();
        }
        return nullptr;
    }
    static void ResetPropertyValue(uec_property_value* value)
    {
        value->kind = UEC_PROPERTY_UNKNOWN;
        value->bool_value = UEC_FALSE;
        value->integer_value = 0;
        value->real_value = 0.0;
    }
    static uec_result ExportScalarPropertyValue(const FProperty* property,
                                                const void* value,
                                                uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        ResetPropertyValue(outValue);
        if (property == nullptr || value == nullptr) return UEC_RESULT_UNSUPPORTED;
        outValue->kind = GetPropertyKind(property);
        if (const FBoolProperty* boolProperty = CastField<FBoolProperty>(property)) {
            outValue->bool_value = boolProperty->GetPropertyValue(value) ? UEC_TRUE : UEC_FALSE;
            return UEC_RESULT_OK;
        }
        const FNumericProperty* numeric = CastField<FNumericProperty>(property);
        const FNumericProperty* integer = numeric;
        if (const FEnumProperty* enumProperty = CastField<FEnumProperty>(property)) {
            integer = enumProperty->GetUnderlyingProperty();
        }
        if (integer != nullptr && integer->IsInteger()) {
            if (IsUnsignedIntegerProperty(integer)) {
                const uint64 raw = integer->GetUnsignedIntPropertyValue(value);
                if (raw > static_cast<uint64>(TNumericLimits<int64>::Max())) {
                    return UEC_RESULT_UNSUPPORTED;
                }
                outValue->integer_value = static_cast<int64>(raw);
            } else {
                outValue->integer_value = integer->GetSignedIntPropertyValue(value);
            }
            return UEC_RESULT_OK;
        }
        if (numeric != nullptr && numeric->IsFloatingPoint()) {
            outValue->real_value = numeric->GetFloatingPointPropertyValue(value);
            return UEC_RESULT_OK;
        }
        return UEC_RESULT_UNSUPPORTED;
    }
    static uec_result ImportScalarPropertyValue(const FProperty* property,
                                                void* data,
                                                const uec_property_value* value)
    {
        if (value == nullptr || value->struct_size < sizeof(uec_property_value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (property == nullptr || data == nullptr || !IsWritableProperty(property)) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (const FBoolProperty* boolProperty = CastField<FBoolProperty>(property)) {
            if (value->kind != UEC_PROPERTY_BOOL || !IsValidBool(value->bool_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            boolProperty->SetPropertyValue(data, value->bool_value != UEC_FALSE);
            return UEC_RESULT_OK;
        }
        if (const FEnumProperty* enumProperty = CastField<FEnumProperty>(property)) {
            FNumericProperty* underlying = enumProperty->GetUnderlyingProperty();
            if ((value->kind != UEC_PROPERTY_INTEGER && value->kind != UEC_PROPERTY_ENUM) ||
                !IsIntegerValueInRange(underlying, value->integer_value) ||
                !IsValidEnumValue(enumProperty, value->integer_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            underlying->SetIntPropertyValue(data, value->integer_value);
            return UEC_RESULT_OK;
        }
        if (FNumericProperty* numeric = CastField<FNumericProperty>(property)) {
            if (numeric->IsFloatingPoint()) {
                if ((value->kind != UEC_PROPERTY_FLOAT && value->kind != UEC_PROPERTY_DOUBLE) ||
                    !FMath::IsFinite(value->real_value) ||
                    (CastField<FFloatProperty>(property) && !IsRepresentableFloat(value->real_value))) {
                    return UEC_RESULT_INVALID_ARGUMENT;
                }
                numeric->SetFloatingPointPropertyValue(data, value->real_value);
                return UEC_RESULT_OK;
            }
            if (numeric->IsInteger()) {
                if ((value->kind != UEC_PROPERTY_INTEGER && value->kind != UEC_PROPERTY_ENUM) ||
                    !IsIntegerValueInRange(numeric, value->integer_value)) {
                    return UEC_RESULT_INVALID_ARGUMENT;
                }
                if (IsUnsignedIntegerProperty(numeric)) {
                    numeric->SetIntPropertyValue(data, static_cast<uint64>(value->integer_value));
                } else {
                    numeric->SetIntPropertyValue(data, value->integer_value);
                }
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_UNSUPPORTED;
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

    #include "uec_api_reflection_metadata.inl"
