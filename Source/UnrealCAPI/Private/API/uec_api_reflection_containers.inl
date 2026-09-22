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

    static uec_result PrepareTextOutput(uec_text_output* output)
    {
        if (output == nullptr || output->struct_size < sizeof(uec_text_output)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        output->kind = UEC_PROPERTY_UNKNOWN;
        output->required_size = 0;
        return UEC_RESULT_OK;
    }

    static uec_result ExportPropertyText(FProperty* property,
                                         void* value,
                                         UObject* owner,
                                         uec_text_output* output)
    {
        if (property == nullptr || value == nullptr || output == nullptr) return UEC_RESULT_UNSUPPORTED;
        output->kind = GetPropertyKind(property);
        FString text;
        property->ExportTextItem_Direct(text, value, nullptr, owner, PPF_None, owner);
        return CopyFStringToUtf8(text, output->buffer, output->buffer_size, &output->required_size);
    }

    uec_result UEC_CALL GetObjectPropertyMapCount(uec_object* rawObject,
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
        FMapProperty* mapProperty = CastField<FMapProperty>(
            object->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (mapProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptMapHelper helper(mapProperty, mapProperty->ContainerPtrToValuePtr<void>(object));
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outCount = static_cast<uint32_t>(helper.Num());
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetObjectPropertyMapEntryText(uec_object* rawObject,
                                                      uec_string_view propertyName,
                                                      uint32_t index,
                                                      uec_text_output* outKey,
                                                      uec_text_output* outValue)
    {
        const uec_result keyStatus = PrepareTextOutput(outKey);
        const uec_result valueStatus = PrepareTextOutput(outValue);
        if (keyStatus != UEC_RESULT_OK || valueStatus != UEC_RESULT_OK) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(propertyName) || propertyName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FMapProperty* mapProperty = CastField<FMapProperty>(
            object->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (mapProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptMapHelper helper(mapProperty, mapProperty->ContainerPtrToValuePtr<void>(object));
        if (index >= static_cast<uint32_t>(helper.Num())) return UEC_RESULT_INVALID_ARGUMENT;
        int32 slot = INDEX_NONE;
        uint32_t current = 0;
        for (int32 candidate = 0; candidate < helper.GetMaxIndex(); ++candidate)
        {
            if (!helper.IsValidIndex(candidate)) continue;
            if (current++ == index) { slot = candidate; break; }
        }
        if (slot == INDEX_NONE) return UEC_RESULT_INTERNAL_ERROR;
        const uec_result keyResult = ExportPropertyText(
            mapProperty->KeyProp, helper.GetKeyPtr(slot), object, outKey);
        const uec_result valueResult = ExportPropertyText(
            mapProperty->ValueProp, helper.GetValuePtr(slot), object, outValue);
        if (keyResult != UEC_RESULT_OK) return keyResult;
        return valueResult;
    }

    uec_result UEC_CALL GetObjectPropertySetCount(uec_object* rawObject,
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
        FSetProperty* setProperty = CastField<FSetProperty>(
            object->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (setProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptSetHelper helper(setProperty, setProperty->ContainerPtrToValuePtr<void>(object));
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outCount = static_cast<uint32_t>(helper.Num());
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetObjectPropertySetElementText(uec_object* rawObject,
                                                        uec_string_view propertyName,
                                                        uint32_t index,
                                                        uec_text_output* outElement)
    {
        if (PrepareTextOutput(outElement) != UEC_RESULT_OK) return UEC_RESULT_INVALID_ARGUMENT;
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(propertyName) || propertyName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FSetProperty* setProperty = CastField<FSetProperty>(
            object->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (setProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptSetHelper helper(setProperty, setProperty->ContainerPtrToValuePtr<void>(object));
        if (index >= static_cast<uint32_t>(helper.Num())) return UEC_RESULT_INVALID_ARGUMENT;
        int32 slot = INDEX_NONE;
        uint32_t current = 0;
        for (int32 candidate = 0; candidate < helper.GetMaxIndex(); ++candidate)
        {
            if (!helper.IsValidIndex(candidate)) continue;
            if (current++ == index) { slot = candidate; break; }
        }
        if (slot == INDEX_NONE) return UEC_RESULT_INTERNAL_ERROR;
        return ExportPropertyText(setProperty->ElementProp, helper.GetElementPtr(slot), object, outElement);
    }

    uec_result UEC_CALL GetActorPropertyMapCount(uec_actor* rawActor,
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
        FMapProperty* mapProperty = CastField<FMapProperty>(
            actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (mapProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptMapHelper helper(mapProperty, mapProperty->ContainerPtrToValuePtr<void>(actor));
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outCount = static_cast<uint32_t>(helper.Num());
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorPropertyMapEntryText(uec_actor* rawActor,
                                                     uec_string_view propertyName,
                                                     uint32_t index,
                                                     uec_text_output* outKey,
                                                     uec_text_output* outValue)
    {
        const uec_result keyStatus = PrepareTextOutput(outKey);
        const uec_result valueStatus = PrepareTextOutput(outValue);
        if (keyStatus != UEC_RESULT_OK || valueStatus != UEC_RESULT_OK) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(propertyName) || propertyName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FMapProperty* mapProperty = CastField<FMapProperty>(
            actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (mapProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptMapHelper helper(mapProperty, mapProperty->ContainerPtrToValuePtr<void>(actor));
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        if (index >= static_cast<uint32_t>(helper.Num())) return UEC_RESULT_INVALID_ARGUMENT;
        int32 slot = INDEX_NONE;
        uint32_t current = 0;
        for (int32 candidate = 0; candidate < helper.GetMaxIndex(); ++candidate)
        {
            if (!helper.IsValidIndex(candidate)) continue;
            if (current++ == index) { slot = candidate; break; }
        }
        if (slot == INDEX_NONE) return UEC_RESULT_INTERNAL_ERROR;
        const uec_result keyResult = ExportPropertyText(
            mapProperty->KeyProp, helper.GetKeyPtr(slot), actor, outKey);
        const uec_result valueResult = ExportPropertyText(
            mapProperty->ValueProp, helper.GetValuePtr(slot), actor, outValue);
        if (keyResult != UEC_RESULT_OK) return keyResult;
        return valueResult;
    }

    uec_result UEC_CALL GetActorPropertySetCount(uec_actor* rawActor,
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
        FSetProperty* setProperty = CastField<FSetProperty>(
            actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (setProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptSetHelper helper(setProperty, setProperty->ContainerPtrToValuePtr<void>(actor));
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outCount = static_cast<uint32_t>(helper.Num());
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetActorPropertySetElementText(uec_actor* rawActor,
                                                       uec_string_view propertyName,
                                                       uint32_t index,
                                                       uec_text_output* outElement)
    {
        if (PrepareTextOutput(outElement) != UEC_RESULT_OK) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(propertyName) || propertyName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        FSetProperty* setProperty = CastField<FSetProperty>(
            actor->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (setProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptSetHelper helper(setProperty, setProperty->ContainerPtrToValuePtr<void>(actor));
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        if (index >= static_cast<uint32_t>(helper.Num())) return UEC_RESULT_INVALID_ARGUMENT;
        int32 slot = INDEX_NONE;
        uint32_t current = 0;
        for (int32 candidate = 0; candidate < helper.GetMaxIndex(); ++candidate)
        {
            if (!helper.IsValidIndex(candidate)) continue;
            if (current++ == index) { slot = candidate; break; }
        }
        if (slot == INDEX_NONE) return UEC_RESULT_INTERNAL_ERROR;
        return ExportPropertyText(setProperty->ElementProp, helper.GetElementPtr(slot), actor, outElement);
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
        FProperty* field = structProperty->Struct->FindPropertyByName(FName(*ToFString(fieldName)));
        if (field == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        void* structValue = structProperty->ContainerPtrToValuePtr<void>(owner);
        void* fieldValue = field->ContainerPtrToValuePtr<void>(structValue);
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
        FProperty* field = structProperty->Struct->FindPropertyByName(FName(*ToFString(fieldName)));
        if (field == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsWritableProperty(field)) return UEC_RESULT_UNSUPPORTED;
        void* structValue = structProperty->ContainerPtrToValuePtr<void>(owner);
        if (structValue == nullptr) return UEC_RESULT_UNSUPPORTED;
        const FString text = ToFString(value);
        if (field->ImportText_InContainer(*text, structValue, owner, PPF_None, GWarn) == nullptr) {
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
