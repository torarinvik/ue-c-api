/* Reflection set adapters. Included inside the private implementation namespace. */
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

    static uec_result GetSetElementStructValue(
        UObject* owner,
        uec_string_view propertyName,
        uint32_t index,
        uec_property_struct_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(*outValue)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        ResetPropertyStructValue(outValue);
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        FSetProperty* setProperty = CastField<FSetProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (setProperty == nullptr || setProperty->ElementProp == nullptr) return UEC_RESULT_UNSUPPORTED;
        const uec_function_struct_kind kind = GetInvocationStructKind(setProperty->ElementProp);
        if (!IsSupportedInvocationStructKind(kind) || kind == UEC_FUNCTION_STRUCT_NONE) {
            return UEC_RESULT_UNSUPPORTED;
        }
        FScriptSetHelper helper(setProperty, setProperty->ContainerPtrToValuePtr<void>(owner));
        int32 slot = INDEX_NONE;
        if (!FindSetSlot(helper, index, slot)) return UEC_RESULT_INVALID_ARGUMENT;
        return ExportTypedStructValue(setProperty->ElementProp,
                                      helper.GetElementPtr(slot), outValue);
    }

    uec_result UEC_CALL GetActorPropertySetElementStructValue(
        uec_actor* rawActor,
        uec_string_view propertyName,
        uint32_t index,
        uec_property_struct_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(*outValue)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        ResetPropertyStructValue(outValue);
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        return actor == nullptr ? UEC_RESULT_INVALID_HANDLE :
            GetSetElementStructValue(actor, propertyName, index, outValue);
    }

    uec_result UEC_CALL GetObjectPropertySetElementStructValue(
        uec_object* rawObject,
        uec_string_view propertyName,
        uint32_t index,
        uec_property_struct_value* outValue)
    {
        if (outValue == nullptr || outValue->struct_size < sizeof(*outValue)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        ResetPropertyStructValue(outValue);
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        return object == nullptr ? UEC_RESULT_INVALID_HANDLE :
            GetSetElementStructValue(object, propertyName, index, outValue);
    }
    static uec_result CommitSetElement(FSetProperty* setProperty,
                                       FScriptSetHelper& helper,
                                       int32 slot,
                                       void* parsedValue)
    {
        if (setProperty == nullptr || setProperty->ElementProp == nullptr ||
            parsedValue == nullptr || slot == INDEX_NONE) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        FProperty* elementProperty = setProperty->ElementProp;
        for (int32 candidate = 0; candidate < helper.GetMaxIndex(); ++candidate)
        {
            if (candidate == slot || !helper.IsValidIndex(candidate)) continue;
            if (elementProperty->Identical(parsedValue, helper.GetElementPtr(candidate), PPF_None))
            {
                elementProperty->DestroyAndFreeValue(parsedValue);
                return UEC_RESULT_INVALID_ARGUMENT;
            }
        }
        elementProperty->CopySingleValue(helper.GetElementPtr(slot), parsedValue);
        elementProperty->DestroyAndFreeValue(parsedValue);
        helper.Rehash();
        return UEC_RESULT_OK;
    }

    static uec_result SetSetElementText(UObject* owner,
                                        uec_string_view propertyName,
                                        uint32_t index,
                                        uec_string_view value)
    {
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0 ||
            !IsValidStringView(value)) return UEC_RESULT_INVALID_ARGUMENT;
        FSetProperty* setProperty = CastField<FSetProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (setProperty == nullptr || setProperty->ElementProp == nullptr) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (!IsWritablePropertyForObject(owner, setProperty)) return UEC_RESULT_UNSUPPORTED;
        FScriptSetHelper helper(setProperty, setProperty->ContainerPtrToValuePtr<void>(owner));
        int32 slot = INDEX_NONE;
        if (!FindSetSlot(helper, index, slot)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* elementProperty = setProperty->ElementProp;
        void* parsedValue = elementProperty->AllocateAndInitializeValue();
        if (parsedValue == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        const FString text = ToFString(value);
        if (elementProperty->ImportText_Direct(*text, parsedValue, owner, PPF_None, GWarn) == nullptr)
        {
            elementProperty->DestroyAndFreeValue(parsedValue);
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        return CommitSetElement(setProperty, helper, slot, parsedValue);
    }

    static uec_result SetSetElementValue(UObject* owner,
                                         uec_string_view propertyName,
                                         uint32_t index,
                                         const uec_property_value* value)
    {
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0 ||
            value == nullptr || value->struct_size < sizeof(uec_property_value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        FSetProperty* setProperty = CastField<FSetProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (setProperty == nullptr || setProperty->ElementProp == nullptr) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (!IsWritablePropertyForObject(owner, setProperty)) return UEC_RESULT_UNSUPPORTED;
        FScriptSetHelper helper(setProperty, setProperty->ContainerPtrToValuePtr<void>(owner));
        int32 slot = INDEX_NONE;
        if (!FindSetSlot(helper, index, slot)) return UEC_RESULT_INVALID_ARGUMENT;
        FProperty* elementProperty = setProperty->ElementProp;
        void* parsedValue = elementProperty->AllocateAndInitializeValue();
        if (parsedValue == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        const uec_result importResult = ImportScalarPropertyValue(elementProperty, parsedValue, value);
        if (importResult != UEC_RESULT_OK)
        {
            elementProperty->DestroyAndFreeValue(parsedValue);
            return importResult;
        }
        return CommitSetElement(setProperty, helper, slot, parsedValue);
    }

    uec_result UEC_CALL SetActorPropertySetElementText(uec_actor* rawActor,
                                                       uec_string_view propertyName,
                                                       uint32_t index,
                                                       uec_string_view value)
    {
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        return actor == nullptr ? UEC_RESULT_INVALID_HANDLE
                                : SetSetElementText(actor, propertyName, index, value);
    }

    uec_result UEC_CALL SetObjectPropertySetElementText(uec_object* rawObject,
                                                        uec_string_view propertyName,
                                                        uint32_t index,
                                                        uec_string_view value)
    {
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        return object == nullptr ? UEC_RESULT_INVALID_HANDLE
                                 : SetSetElementText(object, propertyName, index, value);
    }

    uec_result UEC_CALL SetActorPropertySetElementValue(
        uec_actor* rawActor, uec_string_view propertyName, uint32_t index,
        const uec_property_value* value)
    {
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        return actor == nullptr ? UEC_RESULT_INVALID_HANDLE
                                : SetSetElementValue(actor, propertyName, index, value);
    }

    uec_result UEC_CALL SetObjectPropertySetElementValue(
        uec_object* rawObject, uec_string_view propertyName, uint32_t index,
        const uec_property_value* value)
    {
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        return object == nullptr ? UEC_RESULT_INVALID_HANDLE
                                 : SetSetElementValue(object, propertyName, index, value);
    }

    static uec_result SetSetElementStructValue(
        UObject* owner,
        uec_string_view propertyName,
        uint32_t index,
        const uec_property_struct_value* value)
    {
        if (owner == nullptr || !IsValidStringView(propertyName) || propertyName.size == 0 ||
            value == nullptr || value->struct_size < sizeof(*value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        FSetProperty* setProperty = CastField<FSetProperty>(
            owner->GetClass()->FindPropertyByName(FName(*ToFString(propertyName))));
        if (setProperty == nullptr || setProperty->ElementProp == nullptr) {
            return UEC_RESULT_UNSUPPORTED;
        }
        FProperty* elementProperty = setProperty->ElementProp;
        const uec_function_struct_kind kind = GetInvocationStructKind(elementProperty);
        if (!IsSupportedInvocationStructKind(kind) || kind == UEC_FUNCTION_STRUCT_NONE) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (!IsWritablePropertyForObject(owner, setProperty)) return UEC_RESULT_UNSUPPORTED;
        FScriptSetHelper helper(setProperty, setProperty->ContainerPtrToValuePtr<void>(owner));
        int32 slot = INDEX_NONE;
        if (!FindSetSlot(helper, index, slot)) return UEC_RESULT_INVALID_ARGUMENT;

        void* parsedValue = elementProperty->AllocateAndInitializeValue();
        if (parsedValue == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        const uec_result importResult = ImportTypedStructValue(elementProperty, parsedValue, value);
        if (importResult != UEC_RESULT_OK) {
            elementProperty->DestroyAndFreeValue(parsedValue);
            return importResult;
        }
        return CommitSetElement(setProperty, helper, slot, parsedValue);
    }

    uec_result UEC_CALL SetActorPropertySetElementStructValue(
        uec_actor* rawActor,
        uec_string_view propertyName,
        uint32_t index,
        const uec_property_struct_value* value)
    {
        auto* handle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        AActor* actor = handle->Value.Get();
        return actor == nullptr ? UEC_RESULT_INVALID_HANDLE :
            SetSetElementStructValue(actor, propertyName, index, value);
    }

    uec_result UEC_CALL SetObjectPropertySetElementStructValue(
        uec_object* rawObject,
        uec_string_view propertyName,
        uint32_t index,
        const uec_property_struct_value* value)
    {
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        return object == nullptr ? UEC_RESULT_INVALID_HANDLE :
            SetSetElementStructValue(object, propertyName, index, value);
    }
