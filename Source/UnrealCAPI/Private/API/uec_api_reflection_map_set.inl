/* Reflection map/set adapters. Included inside the private implementation namespace. */
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

    static uec_result ImportMapValueText(UObject* owner,
                                         uec_string_view propertyName,
                                         uint32_t index,
                                         uec_string_view value)
    {
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0 ||
            !IsValidStringView(value)) return UEC_RESULT_INVALID_ARGUMENT;
        FMapProperty* mapProperty = CastField<FMapProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (mapProperty == nullptr) return UEC_RESULT_UNSUPPORTED;
        if (!IsWritableProperty(mapProperty) || mapProperty->ValueProp == nullptr) {
            return UEC_RESULT_UNSUPPORTED;
        }
        FScriptMapHelper helper(mapProperty, mapProperty->ContainerPtrToValuePtr<void>(owner));
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
        const FString text = ToFString(value);
        if (mapProperty->ValueProp->ImportText_InContainer(
                *text, helper.GetValuePtr(slot), owner, PPF_None, GWarn) == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetActorPropertyMapValueText(uec_actor* rawActor,
                                                     uec_string_view propertyName,
                                                     uint32_t index,
                                                     uec_string_view value)
    {
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return ImportMapValueText(actor, propertyName, index, value);
    }

    uec_result UEC_CALL SetObjectPropertyMapValueText(uec_object* rawObject,
                                                      uec_string_view propertyName,
                                                      uint32_t index,
                                                      uec_string_view value)
    {
        auto* objectHandle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(objectHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = objectHandle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return ImportMapValueText(object, propertyName, index, value);
    }

    static bool FindMapSlot(FScriptMapHelper& helper, uint32_t index, int32& outSlot)
    {
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX ||
            index >= static_cast<uint32_t>(helper.Num())) return false;
        uint32_t current = 0;
        for (int32 candidate = 0; candidate < helper.GetMaxIndex(); ++candidate)
        {
            if (!helper.IsValidIndex(candidate)) continue;
            if (current++ == index) { outSlot = candidate; return true; }
        }
        return false;
    }
    static bool FindSetSlot(FScriptSetHelper& helper, uint32_t index, int32& outSlot)
    {
        if (helper.Num() < 0 || static_cast<uint64>(helper.Num()) > UINT32_MAX ||
            index >= static_cast<uint32_t>(helper.Num())) return false;
        uint32_t current = 0;
        for (int32 candidate = 0; candidate < helper.GetMaxIndex(); ++candidate)
        {
            if (!helper.IsValidIndex(candidate)) continue;
            if (current++ == index) { outSlot = candidate; return true; }
        }
        return false;
    }
    static uec_result GetMapValue(UObject* owner,
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
        FMapProperty* mapProperty = CastField<FMapProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (mapProperty == nullptr || mapProperty->ValueProp == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptMapHelper helper(mapProperty, mapProperty->ContainerPtrToValuePtr<void>(owner));
        int32 slot = INDEX_NONE;
        if (!FindMapSlot(helper, index, slot)) return UEC_RESULT_INVALID_ARGUMENT;
        return ExportScalarPropertyValue(mapProperty->ValueProp, helper.GetValuePtr(slot), outValue);
    }
    static uec_result GetSetElement(UObject* owner,
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
        FSetProperty* setProperty = CastField<FSetProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (setProperty == nullptr || setProperty->ElementProp == nullptr) return UEC_RESULT_UNSUPPORTED;
        FScriptSetHelper helper(setProperty, setProperty->ContainerPtrToValuePtr<void>(owner));
        int32 slot = INDEX_NONE;
        if (!FindSetSlot(helper, index, slot)) return UEC_RESULT_INVALID_ARGUMENT;
        return ExportScalarPropertyValue(setProperty->ElementProp, helper.GetElementPtr(slot), outValue);
    }
    uec_result UEC_CALL GetActorPropertyMapValue(uec_actor* rawActor,
                                                 uec_string_view propertyName,
                                                 uint32_t index,
                                                 uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value)) return UEC_RESULT_INVALID_ARGUMENT;
        ResetPropertyValue(outValue);
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        return actor == nullptr ? UEC_RESULT_INVALID_HANDLE : GetMapValue(actor, propertyName, index, outValue);
    }
    uec_result UEC_CALL GetObjectPropertyMapValue(uec_object* rawObject,
                                                  uec_string_view propertyName,
                                                  uint32_t index,
                                                  uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value)) return UEC_RESULT_INVALID_ARGUMENT;
        ResetPropertyValue(outValue);
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        return object == nullptr ? UEC_RESULT_INVALID_HANDLE : GetMapValue(object, propertyName, index, outValue);
    }
    uec_result UEC_CALL GetActorPropertySetElementValue(uec_actor* rawActor,
                                                        uec_string_view propertyName,
                                                        uint32_t index,
                                                        uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value)) return UEC_RESULT_INVALID_ARGUMENT;
        ResetPropertyValue(outValue);
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        return actor == nullptr ? UEC_RESULT_INVALID_HANDLE : GetSetElement(actor, propertyName, index, outValue);
    }
    uec_result UEC_CALL GetObjectPropertySetElementValue(uec_object* rawObject,
                                                         uec_string_view propertyName,
                                                         uint32_t index,
                                                         uec_property_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(uec_property_value)) return UEC_RESULT_INVALID_ARGUMENT;
        ResetPropertyValue(outValue);
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        return object == nullptr ? UEC_RESULT_INVALID_HANDLE : GetSetElement(object, propertyName, index, outValue);
    }

    static uec_result SetMapValue(UObject* owner,
                                  uec_string_view propertyName,
                                  uint32_t index,
                                  const uec_property_value* value)
    {
        if (value == nullptr || value->struct_size < sizeof(uec_property_value)) return UEC_RESULT_INVALID_ARGUMENT;
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        FMapProperty* mapProperty = CastField<FMapProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (mapProperty == nullptr || mapProperty->ValueProp == nullptr) return UEC_RESULT_UNSUPPORTED;
        if (!IsWritableProperty(mapProperty)) return UEC_RESULT_UNSUPPORTED;
        FScriptMapHelper helper(mapProperty, mapProperty->ContainerPtrToValuePtr<void>(owner));
        int32 slot = INDEX_NONE;
        if (!FindMapSlot(helper, index, slot)) return UEC_RESULT_INVALID_ARGUMENT;
        return ImportScalarPropertyValue(mapProperty->ValueProp, helper.GetValuePtr(slot), value);
    }
    uec_result UEC_CALL SetActorPropertyMapValue(uec_actor* rawActor,
                                                 uec_string_view propertyName,
                                                 uint32_t index,
                                                 const uec_property_value* value)
    {
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        return actor == nullptr ? UEC_RESULT_INVALID_HANDLE : SetMapValue(actor, propertyName, index, value);
    }
    uec_result UEC_CALL SetObjectPropertyMapValue(uec_object* rawObject,
                                                  uec_string_view propertyName,
                                                  uint32_t index,
                                                  const uec_property_value* value)
    {
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        return object == nullptr ? UEC_RESULT_INVALID_HANDLE : SetMapValue(object, propertyName, index, value);
    }
