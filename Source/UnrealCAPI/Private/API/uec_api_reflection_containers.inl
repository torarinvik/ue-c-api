/* Reflection container adapters. Included inside the private implementation namespace. */
    uec_result UEC_CALL GetActorPropertyArrayCount(uec_actor* rawActor,
                                                   uec_string_view propertyName,
                                                   uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FArrayProperty* arrayProperty = CastField<FArrayProperty>(
            actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (arrayProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptArrayHelper helper(arrayProperty, arrayProperty->ContainerPtrToValuePtr<void>(actor));
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outCount = static_cast<uint32_t>(helper.Num());
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorPropertyArrayElementText(uec_actor* rawActor,
                                                         uec_string_view propertyName,
                                                         uint32_t index,
                                                         char* buffer,
                                                         size_t bufferSize,
                                                         size_t* requiredSize,
                                                         uec_property_kind* outKind)
    {
        if (requiredSize != nullptr) *requiredSize = 0;
        if (outKind != nullptr) *outKind = UEC_PROPERTY_UNKNOWN;
        if (requiredSize == nullptr || outKind == nullptr || !IsValidStringView(propertyName) ||
            propertyName.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FArrayProperty* arrayProperty = CastField<FArrayProperty>(
            actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (arrayProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptArrayHelper helper(arrayProperty, arrayProperty->ContainerPtrToValuePtr<void>(actor));
        if (index >= static_cast<uint32_t>(helper.Num())) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* innerProperty = arrayProperty->Inner;
        if (innerProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        *outKind = GetPropertyKind(innerProperty);
        FString value;
        innerProperty->ExportTextItem_Direct(value, helper.GetRawPtr(static_cast<int32>(index)),
                                              nullptr, actor, PPF_None, actor);
        return CopyFStringToUtf8(value, buffer, bufferSize, requiredSize);
    }

    static uec_result ExportSoftPropertyPath(UObject* owner,
                                             uec_string_view propertyName,
                                             char* buffer,
                                             size_t bufferSize,
                                             size_t* requiredSize,
                                             uec_property_kind* outKind)
    {
        if (requiredSize == nullptr || outKind == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0; *outKind = UEC_PROPERTY_UNKNOWN;
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        FProperty* property = owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!CastField<FSoftObjectProperty>(property) && !CastField<FSoftClassProperty>(property)) {
            return UEC_RESULT_UNSUPPORTED;
        }
        *outKind = GetPropertyKind(property);
        FString path;
        if (!property->ExportTextItem_Direct(path, property->ContainerPtrToValuePtr<void>(owner),
                                             nullptr, owner, PPF_None, owner)) {
            return UEC_RESULT_UNSUPPORTED;
        }
        return CopyFStringToUtf8(path, buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL GetActorPropertySoftPath(uec_actor* rawActor,
                                                 uec_string_view propertyName,
                                                 char* buffer,
                                                 size_t bufferSize,
                                                 size_t* requiredSize,
                                                 uec_property_kind* outKind)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return ExportSoftPropertyPath(actor, propertyName, buffer, bufferSize, requiredSize, outKind);
    }

    uec_result UEC_CALL GetObjectPropertySoftPath(uec_object* rawObject,
                                                  uec_string_view propertyName,
                                                  char* buffer,
                                                  size_t bufferSize,
                                                  size_t* requiredSize,
                                                  uec_property_kind* outKind)
    {
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return ExportSoftPropertyPath(object, propertyName, buffer, bufferSize, requiredSize, outKind);
    }

    uec_result UEC_CALL GetObjectPropertyArrayCount(uec_object* rawObject,
                                                    uec_string_view propertyName,
                                                    uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FArrayProperty* arrayProperty = CastField<FArrayProperty>(
            object->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (arrayProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptArrayHelper helper(arrayProperty, arrayProperty->ContainerPtrToValuePtr<void>(object));
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outCount = static_cast<uint32_t>(helper.Num());
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetObjectPropertyArrayElementText(uec_object* rawObject,
                                                          uec_string_view propertyName,
                                                          uint32_t index,
                                                          char* buffer,
                                                          size_t bufferSize,
                                                          size_t* requiredSize,
                                                          uec_property_kind* outKind)
    {
        if (requiredSize != nullptr) *requiredSize = 0;
        if (outKind != nullptr) *outKind = UEC_PROPERTY_UNKNOWN;
        if (requiredSize == nullptr || outKind == nullptr || !IsValidStringView(propertyName) ||
            propertyName.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FArrayProperty* arrayProperty = CastField<FArrayProperty>(
            object->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (arrayProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptArrayHelper helper(arrayProperty, arrayProperty->ContainerPtrToValuePtr<void>(object));
        if (index >= static_cast<uint32_t>(helper.Num())) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* innerProperty = arrayProperty->Inner;
        if (innerProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        *outKind = GetPropertyKind(innerProperty);
        FString value;
        innerProperty->ExportTextItem_Direct(value, helper.GetRawPtr(static_cast<int32>(index)),
                                              nullptr, object, PPF_None, object);
        return CopyFStringToUtf8(value, buffer, bufferSize, requiredSize);
    }

    static uec_result ImportSoftPropertyPath(UObject* owner,
                                             uec_string_view propertyName,
                                             uec_string_view path)
    {
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0 ||
            !IsValidStringView(path)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* property = owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName)));
        if (property == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!CastField<FSoftObjectProperty>(property) && !CastField<FSoftClassProperty>(property)) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (!IsWritableProperty(property)) return UEC_RESULT_UNSUPPORTED;
        const FString text = ToFString(path);
        if (property->ImportText_InContainer(*text, owner, owner, PPF_None, GWarn) == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetActorPropertySoftPath(uec_actor* rawActor,
                                                 uec_string_view propertyName,
                                                 uec_string_view path)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return ImportSoftPropertyPath(actor, propertyName, path);
    }

    uec_result UEC_CALL SetObjectPropertySoftPath(uec_object* rawObject,
                                                  uec_string_view propertyName,
                                                  uec_string_view path)
    {
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return ImportSoftPropertyPath(object, propertyName, path);
    }

    static FProperty* ResolveStructFieldPath(FStructProperty* outerProperty,
                                             UObject* owner,
                                             const FString& fieldPath,
                                             void*& outContainer)
    {
        outContainer = nullptr;
        if (outerProperty == nullptr || owner == nullptr || fieldPath.IsEmpty()) return nullptr;
        TArray<FString> segments;
        fieldPath.ParseIntoArray(segments, TEXT("."), false);
        if (segments.Num() == 0) return nullptr;
        FProperty* currentProperty = outerProperty;
        void* container = owner;
        for (const FString& segment : segments)
        {
            if (segment.IsEmpty()) return nullptr;
            const FStructProperty* currentStruct = CastField<FStructProperty>(currentProperty);
            if (currentStruct == nullptr || currentStruct->Struct == nullptr) return nullptr;
            void* structValue = currentStruct->ContainerPtrToValuePtr<void>(container);
            currentProperty = currentStruct->Struct->FindPropertyByName(FName(*segment));
            if (currentProperty == nullptr) return nullptr;
            container = structValue;
        }
        outContainer = container;
        return currentProperty;
    }

    static uec_result ExportStructFieldText(UObject* owner,
                                            uec_string_view propertyName,
                                            uec_string_view fieldName,
                                            char* buffer,
                                            size_t bufferSize,
                                            size_t* requiredSize,
                                            uec_property_kind* outKind)
    {
        if (requiredSize == nullptr || outKind == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0; *outKind = UEC_PROPERTY_UNKNOWN;
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0 ||
            !IsValidStringView(fieldName) || fieldName.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        FStructProperty* structProperty = CastField<FStructProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (structProperty == nullptr || structProperty->Struct == nullptr) {
            return UEC_RESULT_UNSUPPORTED;
        }
        void* fieldContainer = nullptr;
        FProperty* field = ResolveStructFieldPath(structProperty, owner, ToFString(fieldName), fieldContainer);
        if (field == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        void* fieldValue = field->ContainerPtrToValuePtr<void>(fieldContainer);
        if (fieldValue == nullptr) return UEC_RESULT_UNSUPPORTED;
        *outKind = GetPropertyKind(field);
        FString text;
        if (!field->ExportTextItem_Direct(text, fieldValue, nullptr, owner, PPF_None, owner)) {
            return UEC_RESULT_UNSUPPORTED;
        }
        return CopyFStringToUtf8(text, buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL GetActorPropertyStructFieldText(uec_actor* rawActor,
                                                        uec_string_view propertyName,
                                                        uec_string_view fieldName,
                                                        char* buffer,
                                                        size_t bufferSize,
                                                        size_t* requiredSize,
                                                        uec_property_kind* outKind)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return ExportStructFieldText(actor, propertyName, fieldName, buffer, bufferSize,
                                     requiredSize, outKind);
    }

    uec_result UEC_CALL GetObjectPropertyStructFieldText(uec_object* rawObject,
                                                         uec_string_view propertyName,
                                                         uec_string_view fieldName,
                                                         char* buffer,
                                                         size_t bufferSize,
                                                         size_t* requiredSize,
                                                         uec_property_kind* outKind)
    {
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return ExportStructFieldText(object, propertyName, fieldName, buffer, bufferSize,
                                     requiredSize, outKind);
    }

    static uec_result ImportStructFieldText(UObject* owner,
                                            uec_string_view propertyName,
                                            uec_string_view fieldName,
                                            uec_string_view value)
    {
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0 ||
            !IsValidStringView(fieldName) || fieldName.size == 0 || !IsValidStringView(value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        FStructProperty* structProperty = CastField<FStructProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (structProperty == nullptr || structProperty->Struct == nullptr) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (!IsWritableProperty(structProperty)) return UEC_RESULT_UNSUPPORTED;
        void* fieldContainer = nullptr;
        FProperty* field = ResolveStructFieldPath(structProperty, owner, ToFString(fieldName), fieldContainer);
        if (field == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsWritableProperty(field)) return UEC_RESULT_UNSUPPORTED;
        if (fieldContainer == nullptr) return UEC_RESULT_UNSUPPORTED;
        const FString text = ToFString(value);
        if (field->ImportText_InContainer(*text, fieldContainer, owner, PPF_None, GWarn) == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetActorPropertyStructFieldText(uec_actor* rawActor,
                                                        uec_string_view propertyName,
                                                        uec_string_view fieldName,
                                                        uec_string_view value)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return ImportStructFieldText(actor, propertyName, fieldName, value);
    }

    uec_result UEC_CALL SetObjectPropertyStructFieldText(uec_object* rawObject,
                                                         uec_string_view propertyName,
                                                         uec_string_view fieldName,
                                                         uec_string_view value)
    {
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return ImportStructFieldText(object, propertyName, fieldName, value);
    }

    static uec_result ImportArrayElementText(UObject* owner,
                                             uec_string_view propertyName,
                                             uint32_t index,
                                             uec_string_view value)
    {
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0 ||
            !IsValidStringView(value)) return UEC_RESULT_INVALID_ARGUMENT;
        FArrayProperty* arrayProperty = CastField<FArrayProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (arrayProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        if (!IsWritableProperty(arrayProperty) || arrayProperty->Inner == nullptr) {
            return UEC_RESULT_UNSUPPORTED;
        }
        FScriptArrayHelper helper(arrayProperty, arrayProperty->ContainerPtrToValuePtr<void>(owner));
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        if (index >= static_cast<uint32_t>(helper.Num())) return UEC_RESULT_INVALID_ARGUMENT;
        const FString text = ToFString(value);
        if (arrayProperty->Inner->ImportText_InContainer(
                *text, helper.GetRawPtr(static_cast<int32>(index)), owner, PPF_None, GWarn) == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetActorPropertyArrayElementText(uec_actor* rawActor,
                                                         uec_string_view propertyName,
                                                         uint32_t index,
                                                         uec_string_view value)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return ImportArrayElementText(actor, propertyName, index, value);
    }

    uec_result UEC_CALL SetObjectPropertyArrayElementText(uec_object* rawObject,
                                                          uec_string_view propertyName,
                                                          uint32_t index,
                                                          uec_string_view value)
    {
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return ImportArrayElementText(object, propertyName, index, value);
    }

    static uec_result GetArrayElementValue(UObject* owner,
                                           uec_string_view propertyName,
                                           uint32_t index,
                                           uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        ResetPropertyValue(outValue);
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        FArrayProperty* arrayProperty = CastField<FArrayProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (arrayProperty == nullptr || arrayProperty->Inner == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptArrayHelper helper(arrayProperty, arrayProperty->ContainerPtrToValuePtr<void>(owner));
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        if (index >= static_cast<uint32_t>(helper.Num())) return UEC_RESULT_INVALID_ARGUMENT;
        return ExportScalarPropertyValue(arrayProperty->Inner,
                                         helper.GetRawPtr(static_cast<int32>(index)), outValue);
    }
    uec_result UEC_CALL GetActorPropertyArrayElementValue(uec_actor* rawActor,
                                                         uec_string_view propertyName,
                                                         uint32_t index,
                                                         uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        ResetPropertyValue(outValue);
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return GetArrayElementValue(actor, propertyName, index, outValue);
    }
    uec_result UEC_CALL GetObjectPropertyArrayElementValue(uec_object* rawObject,
                                                          uec_string_view propertyName,
                                                          uint32_t index,
                                                          uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        ResetPropertyValue(outValue);
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return GetArrayElementValue(object, propertyName, index, outValue);
    }

    static uec_result GetStructFieldValue(UObject* owner,
                                          uec_string_view propertyName,
                                          uec_string_view fieldName,
                                          uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        ResetPropertyValue(outValue);
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0 ||
            !IsValidStringView(fieldName) || fieldName.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        FStructProperty* structProperty = CastField<FStructProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (structProperty == nullptr || structProperty->Struct == nullptr) return UEC_RESULT_UNSUPPORTED;
        void* fieldContainer = nullptr;
        FProperty* field = ResolveStructFieldPath(structProperty, owner, ToFString(fieldName), fieldContainer);
        if (field == nullptr || fieldContainer == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        return ExportScalarPropertyValue(field, field->ContainerPtrToValuePtr<void>(fieldContainer), outValue);
    }
    uec_result UEC_CALL GetActorPropertyStructFieldValue(uec_actor* rawActor,
                                                         uec_string_view propertyName,
                                                         uec_string_view fieldName,
                                                         uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value)) return UEC_RESULT_INVALID_ARGUMENT;
        ResetPropertyValue(outValue);
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        return actor == nullptr ? UEC_RESULT_INVALID_HANDLE : GetStructFieldValue(actor, propertyName, fieldName, outValue);
    }
    uec_result UEC_CALL GetObjectPropertyStructFieldValue(uec_object* rawObject,
                                                          uec_string_view propertyName,
                                                          uec_string_view fieldName,
                                                          uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value)) return UEC_RESULT_INVALID_ARGUMENT;
        ResetPropertyValue(outValue);
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        return object == nullptr ? UEC_RESULT_INVALID_HANDLE : GetStructFieldValue(object, propertyName, fieldName, outValue);
    }

    static uec_result SetArrayElementValue(UObject* owner,
                                           uec_string_view propertyName,
                                           uint32_t index,
                                           const uec_property_value* value)
    {
        if (value == nullptr || value->struct_size < sizeof(uec_property_value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        FArrayProperty* arrayProperty = CastField<FArrayProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (arrayProperty == nullptr || arrayProperty->Inner == nullptr) return UEC_RESULT_UNSUPPORTED;
        if (!IsWritableProperty(arrayProperty)) return UEC_RESULT_UNSUPPORTED;
        FScriptArrayHelper helper(arrayProperty, arrayProperty->ContainerPtrToValuePtr<void>(owner));
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        if (index >= static_cast<uint32_t>(helper.Num())) return UEC_RESULT_INVALID_ARGUMENT;
        return ImportScalarPropertyValue(arrayProperty->Inner,
                                         helper.GetRawPtr(static_cast<int32>(index)), value);
    }
    uec_result UEC_CALL SetActorPropertyArrayElementValue(uec_actor* rawActor,
                                                         uec_string_view propertyName,
                                                         uint32_t index,
                                                         const uec_property_value* value)
    {
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        return actor == nullptr ? UEC_RESULT_INVALID_HANDLE : SetArrayElementValue(actor, propertyName, index, value);
    }
    uec_result UEC_CALL SetObjectPropertyArrayElementValue(uec_object* rawObject,
                                                          uec_string_view propertyName,
                                                          uint32_t index,
                                                          const uec_property_value* value)
    {
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        return object == nullptr ? UEC_RESULT_INVALID_HANDLE : SetArrayElementValue(object, propertyName, index, value);
    }
