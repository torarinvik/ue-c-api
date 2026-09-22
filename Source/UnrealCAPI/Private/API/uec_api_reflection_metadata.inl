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

    static uint32_t GetPropertyAccessFlags(const FProperty* property)
    {
        if (property == nullptr) return 0;
        uint32_t flags = 0;
        if (property->HasAnyPropertyFlags(CPF_EditConst)) flags |= UEC_PROPERTY_FLAG_EDIT_CONST;
        if (property->HasAnyPropertyFlags(CPF_BlueprintReadOnly)) flags |= UEC_PROPERTY_FLAG_BLUEPRINT_READ_ONLY;
        if (property->HasAnyPropertyFlags(CPF_ConstParm)) flags |= UEC_PROPERTY_FLAG_CONST_PARAMETER;
        if (property->HasAnyPropertyFlags(CPF_ReturnParm)) flags |= UEC_PROPERTY_FLAG_RETURN;
        if (property->HasAnyPropertyFlags(CPF_Parm)) flags |= UEC_PROPERTY_FLAG_PARAMETER;
        if (property->HasAnyPropertyFlags(CPF_OutParm)) flags |= UEC_PROPERTY_FLAG_OUT;
        if (property->HasAnyPropertyFlags(CPF_ReferenceParm)) flags |= UEC_PROPERTY_FLAG_REFERENCE;
        return flags;
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
            *outFlags = GetPropertyAccessFlags(property);
            return UEC_RESULT_OK;
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    uec_result UEC_CALL GetClassPropertyDefaultText(uec_class* rawClass,
                                                    uint32_t index,
                                                    char* buffer,
                                                    size_t bufferSize,
                                                    size_t* requiredSize,
                                                    uec_property_kind* outKind)
    {
        if (requiredSize == nullptr || outKind == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0;
        *outKind = UEC_PROPERTY_UNKNOWN;
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = handle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UObject* defaults = klass->GetDefaultObject();
        if (defaults == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        uint32_t current = 0;
        for (TFieldIterator<FProperty> iterator(klass, EFieldIteratorFlags::IncludeSuper);
             iterator; ++iterator)
        {
            if (current++ != index) continue;
            FProperty* property = *iterator;
            *outKind = GetPropertyKind(property);
            FString text;
            if (!property->ExportText_InContainer(0, text, defaults, nullptr, defaults,
                                                   PPF_None, defaults)) {
                return UEC_RESULT_UNSUPPORTED;
            }
            return CopyFStringToUtf8(text, buffer, bufferSize, requiredSize);
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    uec_result UEC_CALL GetClassPropertyReferenceClassPath(uec_class* rawClass,
                                                           uint32_t index,
                                                           char* buffer,
                                                           size_t bufferSize,
                                                           size_t* requiredSize,
                                                           uec_property_kind* outKind)
    {
        if (requiredSize == nullptr || outKind == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0;
        *outKind = UEC_PROPERTY_UNKNOWN;
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
            FProperty* property = *iterator;
            UClass* referencedClass = nullptr;
            if (const FClassProperty* classProperty = CastField<FClassProperty>(property)) {
                referencedClass = classProperty->MetaClass;
            } else if (const FSoftClassProperty* softClassProperty = CastField<FSoftClassProperty>(property)) {
                referencedClass = softClassProperty->MetaClass;
            } else if (const FSoftObjectProperty* softObjectProperty = CastField<FSoftObjectProperty>(property)) {
                referencedClass = softObjectProperty->PropertyClass;
            } else if (const FObjectPropertyBase* objectProperty = CastField<FObjectPropertyBase>(property)) {
                referencedClass = objectProperty->PropertyClass;
            } else {
                return UEC_RESULT_UNSUPPORTED;
            }
            if (referencedClass == nullptr) return UEC_RESULT_UNSUPPORTED;
            *outKind = GetPropertyKind(property);
            return CopyFStringToUtf8(referencedClass->GetPathName(), buffer, bufferSize, requiredSize);
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    uec_result UEC_CALL GetClassPropertyEnumValueCount(uec_class* rawClass,
                                                       uint32_t index,
                                                       uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
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
            const UEnum* enumeration = GetEnumForProperty(*iterator);
            if (enumeration == nullptr) return UEC_RESULT_UNSUPPORTED;
            const int32 count = enumeration->NumEnums();
            if (count < 0) return UEC_RESULT_INTERNAL_ERROR;
            *outCount = static_cast<uint32_t>(count);
            return UEC_RESULT_OK;
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    uec_result UEC_CALL GetClassPropertyEnumValueAt(uec_class* rawClass,
                                                     uint32_t index,
                                                     uint32_t valueIndex,
                                                     char* nameBuffer,
                                                     size_t nameBufferSize,
                                                     size_t* nameRequiredSize,
                                                     int64_t* outValue)
    {
        if (nameRequiredSize != nullptr) *nameRequiredSize = 0;
        if (outValue != nullptr) *outValue = 0;
        if (nameRequiredSize == nullptr || outValue == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
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
            const UEnum* enumeration = GetEnumForProperty(*iterator);
            if (enumeration == nullptr) return UEC_RESULT_UNSUPPORTED;
            const int32 count = enumeration->NumEnums();
            if (valueIndex >= static_cast<uint32_t>(FMath::Max(count, 0))) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            const int32 enumIndex = static_cast<int32>(valueIndex);
            *outValue = enumeration->GetValueByIndex(enumIndex);
            return CopyFStringToUtf8(enumeration->GetNameStringByIndex(enumIndex),
                                     nameBuffer, nameBufferSize, nameRequiredSize);
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

    /* Struct fields follow the same IncludeSuper ordering as class properties;
     * callers must re-query names and counts after reflected type changes. */
    static const FProperty* GetClassPropertyAtIndex(UClass* klass, uint32_t index)
    {
        uint32_t current = 0;
        for (TFieldIterator<FProperty> iterator(klass, EFieldIteratorFlags::IncludeSuper);
             iterator; ++iterator)
        {
            if (current++ != index) continue;
            return *iterator;
        }
        return nullptr;
    }

    uec_result UEC_CALL GetClassPropertyStructFieldCount(uec_class* rawClass,
                                                         uint32_t propertyIndex,
                                                         uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = handle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const FProperty* classProperty = GetClassPropertyAtIndex(klass, propertyIndex);
        if (classProperty == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        const FStructProperty* property = CastField<FStructProperty>(classProperty);
        if (property == nullptr || property->Struct == nullptr) return UEC_RESULT_UNSUPPORTED;
        for (TFieldIterator<FProperty> iterator(property->Struct, EFieldIteratorFlags::IncludeSuper);
             iterator; ++iterator)
        {
            if (*outCount == UINT32_MAX) return UEC_RESULT_INTERNAL_ERROR;
            ++(*outCount);
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetClassPropertyStructFieldAt(uec_class* rawClass,
                                                      uint32_t propertyIndex,
                                                      uint32_t fieldIndex,
                                                      char* nameBuffer,
                                                      size_t nameBufferSize,
                                                      size_t* nameRequiredSize,
                                                      uec_property_kind* outKind,
                                                      uint32_t* outFlags)
    {
        if (nameRequiredSize != nullptr) *nameRequiredSize = 0;
        if (outKind != nullptr) *outKind = UEC_PROPERTY_UNKNOWN;
        if (outFlags != nullptr) *outFlags = 0;
        if (nameRequiredSize == nullptr || outKind == nullptr || outFlags == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* handle = reinterpret_cast<FUECClass*>(rawClass);
        if (!IsValidClass(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UClass* klass = handle->Value.Get();
        if (klass == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const FProperty* classProperty = GetClassPropertyAtIndex(klass, propertyIndex);
        if (classProperty == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        const FStructProperty* property = CastField<FStructProperty>(classProperty);
        if (property == nullptr || property->Struct == nullptr) return UEC_RESULT_UNSUPPORTED;
        uint32_t current = 0;
        for (TFieldIterator<FProperty> iterator(property->Struct, EFieldIteratorFlags::IncludeSuper);
             iterator; ++iterator)
        {
            if (current++ != fieldIndex) continue;
            const FProperty* field = *iterator;
            *outKind = GetPropertyKind(field);
            *outFlags = GetPropertyAccessFlags(field);
            return CopyFStringToUtf8(field->GetName(), nameBuffer, nameBufferSize,
                                     nameRequiredSize);
        }
        return UEC_RESULT_INVALID_ARGUMENT;
    }
