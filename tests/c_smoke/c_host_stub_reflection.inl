
static uec_result UEC_CALL StubGetObjectPropertyMapCount(uec_object* object,
                                                         uec_string_view propertyName,
                                                         uint32_t* outCount)
{
    (void)object;
    (void)propertyName;
    if (outCount != NULL) *outCount = 0u;
    return outCount == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetObjectPropertyMapEntryText(uec_object* object,
                                                             uec_string_view propertyName,
                                                             uint32_t index,
                                                             uec_text_output* outKey,
                                                             uec_text_output* outValue)
{
    (void)object;
    (void)propertyName;
    (void)index;
    const uec_result keyResult = StubPrepareTextOutput(outKey);
    const uec_result valueResult = StubPrepareTextOutput(outValue);
    return keyResult != UEC_RESULT_OK || valueResult != UEC_RESULT_OK
        ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetObjectPropertySetCount(uec_object* object,
                                                         uec_string_view propertyName,
                                                         uint32_t* outCount)
{
    (void)object;
    (void)propertyName;
    if (outCount != NULL) *outCount = 0u;
    return outCount == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetObjectPropertySetElementText(uec_object* object,
                                                               uec_string_view propertyName,
                                                               uint32_t index,
                                                               uec_text_output* outElement)
{
    (void)object;
    (void)propertyName;
    (void)index;
    return StubPrepareTextOutput(outElement) == UEC_RESULT_OK
        ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_ARGUMENT;
}

static uec_result UEC_CALL StubGetActorPropertySoftPath(uec_actor* actor,
                                                        uec_string_view propertyName,
                                                        char* buffer,
                                                        size_t bufferSize,
                                                        size_t* requiredSize,
                                                        uec_property_kind* outKind)
{
    (void)actor;
    (void)propertyName;
    (void)buffer;
    (void)bufferSize;
    if (requiredSize != NULL) *requiredSize = 0u;
    if (outKind != NULL) *outKind = UEC_PROPERTY_UNKNOWN;
    return requiredSize == NULL || outKind == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetObjectPropertySoftPath(uec_object* object,
                                                         uec_string_view propertyName,
                                                         char* buffer,
                                                         size_t bufferSize,
                                                         size_t* requiredSize,
                                                         uec_property_kind* outKind)
{
    (void)object;
    (void)propertyName;
    (void)buffer;
    (void)bufferSize;
    if (requiredSize != NULL) *requiredSize = 0u;
    if (outKind != NULL) *outKind = UEC_PROPERTY_UNKNOWN;
    return requiredSize == NULL || outKind == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetActorPropertySoftValue(uec_actor* actor,
                                                         uec_string_view propertyName,
                                                         uec_text_output* outValue)
{
    (void)actor;
    (void)propertyName;
    const uec_result result = StubPrepareTextOutput(outValue);
    return result == UEC_RESULT_OK ? UEC_RESULT_UNSUPPORTED : result;
}

static uec_result UEC_CALL StubGetObjectPropertySoftValue(uec_object* object,
                                                          uec_string_view propertyName,
                                                          uec_text_output* outValue)
{
    (void)object;
    (void)propertyName;
    const uec_result result = StubPrepareTextOutput(outValue);
    return result == UEC_RESULT_OK ? UEC_RESULT_UNSUPPORTED : result;
}

static uec_result UEC_CALL StubSetActorPropertySoftValue(uec_actor* actor,
                                                         uec_string_view propertyName,
                                                         uec_property_kind kind,
                                                         uec_string_view path)
{
    (void)propertyName;
    (void)kind;
    (void)path;
    return actor == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetObjectPropertySoftValue(uec_object* object,
                                                          uec_string_view propertyName,
                                                          uec_property_kind kind,
                                                          uec_string_view path)
{
    (void)propertyName;
    (void)kind;
    (void)path;
    return object == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetActorPropertyMapCount(uec_actor* actor,
                                                        uec_string_view propertyName,
                                                        uint32_t* outCount)
{
    (void)actor;
    (void)propertyName;
    if (outCount != NULL) *outCount = 0u;
    return outCount == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetActorPropertyMapEntryText(uec_actor* actor,
                                                            uec_string_view propertyName,
                                                            uint32_t index,
                                                            uec_text_output* outKey,
                                                            uec_text_output* outValue)
{
    (void)actor;
    (void)propertyName;
    (void)index;
    const uec_result keyResult = StubPrepareTextOutput(outKey);
    const uec_result valueResult = StubPrepareTextOutput(outValue);
    return keyResult != UEC_RESULT_OK || valueResult != UEC_RESULT_OK
        ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetActorPropertySetCount(uec_actor* actor,
                                                        uec_string_view propertyName,
                                                        uint32_t* outCount)
{
    (void)actor;
    (void)propertyName;
    if (outCount != NULL) *outCount = 0u;
    return outCount == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetActorPropertySetElementText(uec_actor* actor,
                                                               uec_string_view propertyName,
                                                               uint32_t index,
                                                               uec_text_output* outElement)
{
    (void)actor;
    (void)propertyName;
    (void)index;
    return StubPrepareTextOutput(outElement) == UEC_RESULT_OK
        ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_ARGUMENT;
}

static uec_result UEC_CALL StubSetActorPropertySetElementText(
    uec_actor* actor, uec_string_view propertyName, uint32_t index, uec_string_view value)
{
    (void)actor;
    (void)propertyName;
    (void)index;
    (void)value;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetObjectPropertySetElementText(
    uec_object* object, uec_string_view propertyName, uint32_t index, uec_string_view value)
{
    (void)object;
    (void)propertyName;
    (void)index;
    (void)value;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetActorPropertySoftPath(uec_actor* actor,
                                                        uec_string_view propertyName,
                                                        uec_string_view path)
{
    (void)actor;
    (void)propertyName;
    (void)path;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetObjectPropertySoftPath(uec_object* object,
                                                         uec_string_view propertyName,
                                                         uec_string_view path)
{
    (void)object;
    (void)propertyName;
    (void)path;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetActorPropertyStructFieldText(
    uec_actor* actor,
    uec_string_view propertyName,
    uec_string_view fieldName,
    char* buffer,
    size_t bufferSize,
    size_t* requiredSize,
    uec_property_kind* outKind)
{
    (void)actor;
    (void)propertyName;
    (void)fieldName;
    (void)buffer;
    (void)bufferSize;
    if (requiredSize != NULL) *requiredSize = 0u;
    if (outKind != NULL) *outKind = UEC_PROPERTY_UNKNOWN;
    return requiredSize == NULL || outKind == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetObjectPropertyStructFieldText(
    uec_object* object,
    uec_string_view propertyName,
    uec_string_view fieldName,
    char* buffer,
    size_t bufferSize,
    size_t* requiredSize,
    uec_property_kind* outKind)
{
    (void)object;
    (void)propertyName;
    (void)fieldName;
    (void)buffer;
    (void)bufferSize;
    if (requiredSize != NULL) *requiredSize = 0u;
    if (outKind != NULL) *outKind = UEC_PROPERTY_UNKNOWN;
    return requiredSize == NULL || outKind == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetActorPropertyStructFieldText(uec_actor* actor,
                                                               uec_string_view propertyName,
                                                               uec_string_view fieldName,
                                                               uec_string_view value)
{
    (void)actor;
    (void)propertyName;
    (void)fieldName;
    (void)value;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetObjectPropertyStructFieldText(uec_object* object,
                                                                uec_string_view propertyName,
                                                                uec_string_view fieldName,
                                                                uec_string_view value)
{
    (void)object;
    (void)propertyName;
    (void)fieldName;
    (void)value;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetActorPropertyArrayElementText(uec_actor* actor,
                                                                uec_string_view propertyName,
                                                                uint32_t index,
                                                                uec_string_view value)
{
    (void)actor;
    (void)propertyName;
    (void)index;
    (void)value;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetObjectPropertyArrayElementText(uec_object* object,
                                                                 uec_string_view propertyName,
                                                                 uint32_t index,
                                                                 uec_string_view value)
{
    (void)object;
    (void)propertyName;
    (void)index;
    (void)value;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetActorPropertyMapValueText(uec_actor* actor,
                                                            uec_string_view propertyName,
                                                            uint32_t index,
                                                            uec_string_view value)
{
    (void)actor;
    (void)propertyName;
    (void)index;
    (void)value;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetObjectPropertyMapValueText(uec_object* object,
                                                             uec_string_view propertyName,
                                                             uint32_t index,
                                                             uec_string_view value)
{
    (void)object;
    (void)propertyName;
    (void)index;
    (void)value;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result StubGetArrayElementValue(uec_property_value* outValue)
{
    if (outValue == NULL || outValue->struct_size < sizeof(*outValue)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    outValue->kind = UEC_PROPERTY_UNKNOWN;
    outValue->bool_value = UEC_FALSE;
    outValue->integer_value = 0;
    outValue->real_value = 0.0;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetActorPropertyArrayElementValue(
    uec_actor* actor, uec_string_view propertyName, uint32_t index,
    uec_property_value* outValue)
{
    (void)actor;
    (void)propertyName;
    (void)index;
    return StubGetArrayElementValue(outValue);
}

static uec_result UEC_CALL StubGetObjectPropertyArrayElementValue(
    uec_object* object, uec_string_view propertyName, uint32_t index,
    uec_property_value* outValue)
{
    (void)object;
    (void)propertyName;
    (void)index;
    return StubGetArrayElementValue(outValue);
}

static uec_result UEC_CALL StubGetActorPropertyMapValue(
    uec_actor* actor, uec_string_view propertyName, uint32_t index,
    uec_property_value* outValue)
{
    (void)actor;
    (void)propertyName;
    (void)index;
    return StubGetArrayElementValue(outValue);
}

static uec_result UEC_CALL StubGetObjectPropertyMapValue(
    uec_object* object, uec_string_view propertyName, uint32_t index,
    uec_property_value* outValue)
{
    (void)object;
    (void)propertyName;
    (void)index;
    return StubGetArrayElementValue(outValue);
}

static uec_result UEC_CALL StubGetActorPropertyMapKey(
    uec_actor* actor, uec_string_view propertyName, uint32_t index,
    uec_property_value* outKey)
{
    (void)actor; (void)propertyName; (void)index;
    return StubGetArrayElementValue(outKey);
}

static uec_result UEC_CALL StubGetObjectPropertyMapKey(
    uec_object* object, uec_string_view propertyName, uint32_t index,
    uec_property_value* outKey)
{
    (void)object; (void)propertyName; (void)index;
    return StubGetArrayElementValue(outKey);
}

static uec_result UEC_CALL StubGetActorPropertySetElementValue(
    uec_actor* actor, uec_string_view propertyName, uint32_t index,
    uec_property_value* outValue)
{
    (void)actor;
    (void)propertyName;
    (void)index;
    return StubGetArrayElementValue(outValue);
}

static uec_result UEC_CALL StubGetObjectPropertySetElementValue(
    uec_object* object, uec_string_view propertyName, uint32_t index,
    uec_property_value* outValue)
{
    (void)object;
    (void)propertyName;
    (void)index;
    return StubGetArrayElementValue(outValue);
}

static uec_result UEC_CALL StubGetActorPropertyStructFieldValue(
    uec_actor* actor, uec_string_view propertyName, uec_string_view fieldName,
    uec_property_value* outValue)
{
    (void)actor;
    (void)propertyName;
    (void)fieldName;
    return StubGetArrayElementValue(outValue);
}

static uec_result UEC_CALL StubGetObjectPropertyStructFieldValue(
    uec_object* object, uec_string_view propertyName, uec_string_view fieldName,
    uec_property_value* outValue)
{
    (void)object;
    (void)propertyName;
    (void)fieldName;
    return StubGetArrayElementValue(outValue);
}

static uec_result StubSetContainerValue(const uec_property_value* value)
{
    if (value == NULL || value->struct_size < sizeof(*value)) return UEC_RESULT_INVALID_ARGUMENT;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetActorPropertyArrayElementValue(
    uec_actor* actor, uec_string_view propertyName, uint32_t index,
    const uec_property_value* value)
{
    (void)actor;
    (void)propertyName;
    (void)index;
    return StubSetContainerValue(value);
}

static uec_result UEC_CALL StubSetObjectPropertyArrayElementValue(
    uec_object* object, uec_string_view propertyName, uint32_t index,
    const uec_property_value* value)
{
    (void)object;
    (void)propertyName;
    (void)index;
    return StubSetContainerValue(value);
}

static uec_result UEC_CALL StubSetActorPropertyMapValue(
    uec_actor* actor, uec_string_view propertyName, uint32_t index,
    const uec_property_value* value)
{
    (void)actor;
    (void)propertyName;
    (void)index;
    return StubSetContainerValue(value);
}

static uec_result UEC_CALL StubSetObjectPropertyMapValue(
    uec_object* object, uec_string_view propertyName, uint32_t index,
    const uec_property_value* value)
{
    (void)object;
    (void)propertyName;
    (void)index;
    return StubSetContainerValue(value);
}

static uec_result UEC_CALL StubSetActorPropertySetElementValue(
    uec_actor* actor, uec_string_view propertyName, uint32_t index,
    const uec_property_value* value)
{
    (void)actor;
    (void)propertyName;
    (void)index;
    return StubSetContainerValue(value);
}

static uec_result UEC_CALL StubSetObjectPropertySetElementValue(
    uec_object* object, uec_string_view propertyName, uint32_t index,
    const uec_property_value* value)
{
    (void)object;
    (void)propertyName;
    (void)index;
    return StubSetContainerValue(value);
}

static uec_result UEC_CALL StubSetActorPropertyStructFieldValue(
    uec_actor* actor, uec_string_view propertyName, uec_string_view fieldName,
    const uec_property_value* value)
{
    (void)actor;
    (void)propertyName;
    (void)fieldName;
    return StubSetContainerValue(value);
}

static uec_result UEC_CALL StubSetObjectPropertyStructFieldValue(
    uec_object* object, uec_string_view propertyName, uec_string_view fieldName,
    const uec_property_value* value)
{
    (void)object;
    (void)propertyName;
    (void)fieldName;
    return StubSetContainerValue(value);
}

static uec_result UEC_CALL StubGetClassPropertyDefaultText(
    uec_class* klass, uint32_t index, char* buffer, size_t bufferSize,
    size_t* requiredSize, uec_property_kind* outKind)
{
    (void)klass;
    (void)index;
    (void)buffer;
    (void)bufferSize;
    if (requiredSize != NULL) *requiredSize = 0u;
    if (outKind != NULL) *outKind = UEC_PROPERTY_UNKNOWN;
    return requiredSize == NULL || outKind == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetClassPropertyReferenceClassPath(
    uec_class* klass, uint32_t index, char* buffer, size_t bufferSize,
    size_t* requiredSize, uec_property_kind* outKind)
{
    (void)klass;
    (void)index;
    (void)buffer;
    (void)bufferSize;
    if (requiredSize != NULL) *requiredSize = 0u;
    if (outKind != NULL) *outKind = UEC_PROPERTY_UNKNOWN;
    return requiredSize == NULL || outKind == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetClassPropertyEnumValueCount(
    uec_class* klass, uint32_t index, uint32_t* outCount)
{
    (void)klass;
    (void)index;
    if (outCount != NULL) *outCount = 0u;
    return outCount == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetClassPropertyEnumValueAt(
    uec_class* klass, uint32_t index, uint32_t valueIndex, char* nameBuffer,
    size_t nameBufferSize, size_t* nameRequiredSize, int64_t* outValue)
{
    (void)klass;
    (void)index;
    (void)valueIndex;
    (void)nameBuffer;
    (void)nameBufferSize;
    if (nameRequiredSize != NULL) *nameRequiredSize = 0u;
    if (outValue != NULL) *outValue = 0;
    return nameRequiredSize == NULL || outValue == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetClassPropertyStructFieldCount(
    uec_class* klass, uint32_t propertyIndex, uint32_t* outCount)
{
    (void)klass;
    (void)propertyIndex;
    if (outCount != NULL) *outCount = 0u;
    return outCount == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetClassPropertyStructFieldAt(
    uec_class* klass, uint32_t propertyIndex, uint32_t fieldIndex,
    char* nameBuffer, size_t nameBufferSize, size_t* nameRequiredSize,
    uec_property_kind* outKind, uint32_t* outFlags)
{
    (void)klass;
    (void)propertyIndex;
    (void)fieldIndex;
    (void)nameBuffer;
    (void)nameBufferSize;
    if (nameRequiredSize != NULL) *nameRequiredSize = 0u;
    if (outKind != NULL) *outKind = UEC_PROPERTY_UNKNOWN;
    if (outFlags != NULL) *outFlags = 0u;
    return nameRequiredSize == NULL || outKind == NULL || outFlags == NULL
        ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetActorPropertyClass(
    uec_actor* actor, uec_string_view propertyName, uec_class** outClass)
{
    (void)actor;
    (void)propertyName;
    if (outClass != NULL) *outClass = NULL;
    return outClass == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetActorPropertyClass(
    uec_actor* actor, uec_string_view propertyName, uec_class* klass)
{
    (void)actor;
    (void)propertyName;
    (void)klass;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetObjectPropertyClass(
    uec_object* object, uec_string_view propertyName, uec_class** outClass)
{
    (void)object;
    (void)propertyName;
    if (outClass != NULL) *outClass = NULL;
    return outClass == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetObjectPropertyClass(
    uec_object* object, uec_string_view propertyName, uec_class* klass)
{
    (void)object;
    (void)propertyName;
    (void)klass;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetClassPropertyStructPath(
    uec_class* klass, uint32_t propertyIndex, char* buffer, size_t bufferSize,
    size_t* requiredSize, uec_property_kind* outKind)
{
    (void)klass;
    (void)propertyIndex;
    (void)buffer;
    (void)bufferSize;
    if (requiredSize != NULL) *requiredSize = 0u;
    if (outKind != NULL) *outKind = UEC_PROPERTY_UNKNOWN;
    return requiredSize == NULL || outKind == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetClassPropertyContainerKinds(
    uec_class* klass, uint32_t propertyIndex, uec_property_kind* outKeyKind,
    uec_property_kind* outValueKind)
{
    (void)klass;
    (void)propertyIndex;
    if (outKeyKind != NULL) *outKeyKind = UEC_PROPERTY_UNKNOWN;
    if (outValueKind != NULL) *outValueKind = UEC_PROPERTY_UNKNOWN;
    return outKeyKind == NULL || outValueKind == NULL
        ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetClassPropertyFlags(uec_class* klass,
                                                     uint32_t index,
                                                     uint32_t* outFlags)
{
    (void)klass;
    (void)index;
    if (outFlags != NULL) *outFlags = 0u;
    return outFlags == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubRunOnGameThread(uec_context* context,
                                               uec_game_thread_callback callback,
                                               void* userData,
                                               uint64_t* outRequestId)
{
    (void)userData;
    if (outRequestId != NULL) *outRequestId = 0;
    if (callback == NULL || outRequestId == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (context != &g_context) return UEC_RESULT_INVALID_HANDLE;
    *outRequestId = 1;
    return UEC_RESULT_OK;
}

static const uec_api g_api = {
    .struct_size = sizeof(uec_api),
    .abi_major = UEC_ABI_MAJOR,
    .abi_minor = UEC_ABI_MINOR,
    .get_capabilities = &StubGetCapabilities,
    .get_last_error = &StubGetLastError,
    .log = &StubLog,
    .release_context = &StubReleaseContext,
    .get_runtime_stats = &StubGetRuntimeStats,
    .get_world_count_by_kind = &StubGetWorldCountByKind,
    .get_world_at_by_kind = &StubGetWorldAtByKind,
    .get_class_function_parameter_at = &StubGetClassFunctionParameterAt,
    .invoke_actor_function_value = &StubInvokeActorFunctionValue,
    .invoke_actor_function_values = &StubInvokeActorFunctionValues,
    .invoke_actor_function_text_values = &StubInvokeActorFunctionTextValues, .invoke_actor_function_arguments = &StubInvokeActorFunctionArguments, .get_or_create_actor_event_bridge = &StubGetOrCreateActorEventBridge, .destroy_actor_event_bridge = &StubDestroyActorEventBridge, .bind_actor_event_bridge = &StubBindActorEventBridge, .unbind_actor_event_bridge = &StubUnbindActorEventBridge, .emit_actor_event_bridge = &StubEmitActorEventBridge, .invoke_actor_function_latent = &StubInvokeActorFunctionLatent, .cancel_actor_function_latent = &StubCancelActorFunctionLatent, .save_versioned_application_data = &StubSaveVersionedApplicationData, .load_versioned_application_data = &StubLoadVersionedApplicationData, .get_controller_enhanced_input_subsystem = &StubGetControllerEnhancedInputSubsystem,
    .find_object = &StubFindObject,
    .travel_world_async = &StubTravelWorldAsync,
    .cancel_travel_request = &StubCancelTravelRequest,
    .get_component_visible = &StubGetComponentVisible,
    .get_component_active = &StubGetComponentActive,
    .get_class_function_flags = &StubGetClassFunctionFlags,
    .get_widget_visibility = &StubGetWidgetVisibility,
    .get_text_block_text = &StubGetTextBlockText,
    .get_component_collision_enabled = &StubGetComponentCollisionEnabled,
    .get_audio_component_playing = &StubGetAudioComponentPlaying,
    .set_streaming_level_state_async = &StubSetStreamingLevelStateAsync,
    .cancel_streaming_level_request = &StubCancelStreamingLevelRequest,
    .get_component_collision_response = &StubGetComponentCollisionResponse,
    .get_config_integer = &StubGetConfigInteger,
    .set_config_integer = &StubSetConfigInteger,
    .bind_actor_destroyed = &StubBindActorDestroyed,
    .unbind_actor_destroyed = &StubUnbindActorDestroyed,
    .get_config_bool = &StubGetConfigBool,
    .get_actor_property_array_count = &StubGetActorPropertyArrayCount,
    .get_actor_property_array_element_text = &StubGetActorPropertyArrayElementText,
    .get_object_property_array_count = &StubGetObjectPropertyArrayCount,
    .get_object_property_array_element_text = &StubGetObjectPropertyArrayElementText,
    .get_object_property_map_count = &StubGetObjectPropertyMapCount,
    .get_object_property_map_entry_text = &StubGetObjectPropertyMapEntryText,
    .get_object_property_set_count = &StubGetObjectPropertySetCount,
    .get_object_property_set_element_text = &StubGetObjectPropertySetElementText,
    .get_actor_property_soft_path = &StubGetActorPropertySoftPath,
    .get_object_property_soft_path = &StubGetObjectPropertySoftPath,
    .get_actor_property_soft_value = &StubGetActorPropertySoftValue,
    .get_object_property_soft_value = &StubGetObjectPropertySoftValue,
    .set_actor_property_soft_value = &StubSetActorPropertySoftValue,
    .set_object_property_soft_value = &StubSetObjectPropertySoftValue,
    .get_actor_property_map_count = &StubGetActorPropertyMapCount,
    .get_actor_property_map_entry_text = &StubGetActorPropertyMapEntryText,
    .get_actor_property_set_count = &StubGetActorPropertySetCount,
    .get_actor_property_set_element_text = &StubGetActorPropertySetElementText,
    .set_actor_property_soft_path = &StubSetActorPropertySoftPath,
    .set_object_property_soft_path = &StubSetObjectPropertySoftPath,
    .get_actor_property_struct_field_text = &StubGetActorPropertyStructFieldText,
    .get_object_property_struct_field_text = &StubGetObjectPropertyStructFieldText,
    .set_actor_property_struct_field_text = &StubSetActorPropertyStructFieldText,
    .set_object_property_struct_field_text = &StubSetObjectPropertyStructFieldText,
    .set_actor_property_array_element_text = &StubSetActorPropertyArrayElementText,
    .set_object_property_array_element_text = &StubSetObjectPropertyArrayElementText,
    .set_actor_property_map_value_text = &StubSetActorPropertyMapValueText,
    .set_object_property_map_value_text = &StubSetObjectPropertyMapValueText,
    .get_class_property_flags = &StubGetClassPropertyFlags,
    .get_actor_property_array_element_value = &StubGetActorPropertyArrayElementValue,
    .get_object_property_array_element_value = &StubGetObjectPropertyArrayElementValue,
    .get_actor_property_map_value = &StubGetActorPropertyMapValue,
    .get_object_property_map_value = &StubGetObjectPropertyMapValue,
    .get_actor_property_set_element_value = &StubGetActorPropertySetElementValue,
    .get_object_property_set_element_value = &StubGetObjectPropertySetElementValue,
    .get_actor_property_struct_field_value = &StubGetActorPropertyStructFieldValue,
    .get_object_property_struct_field_value = &StubGetObjectPropertyStructFieldValue,
    .set_actor_property_array_element_value = &StubSetActorPropertyArrayElementValue,
    .set_object_property_array_element_value = &StubSetObjectPropertyArrayElementValue,
    .set_actor_property_map_value = &StubSetActorPropertyMapValue,
    .set_object_property_map_value = &StubSetObjectPropertyMapValue,
    .set_actor_property_struct_field_value = &StubSetActorPropertyStructFieldValue,
    .set_object_property_struct_field_value = &StubSetObjectPropertyStructFieldValue,
    .get_class_property_default_text = &StubGetClassPropertyDefaultText,
    .get_class_property_reference_class_path = &StubGetClassPropertyReferenceClassPath,
    .get_class_property_enum_value_count = &StubGetClassPropertyEnumValueCount,
    .get_class_property_enum_value_at = &StubGetClassPropertyEnumValueAt,
    .get_class_property_struct_field_count = &StubGetClassPropertyStructFieldCount,
    .get_class_property_struct_field_at = &StubGetClassPropertyStructFieldAt,
    .get_actor_property_class = &StubGetActorPropertyClass,
    .set_actor_property_class = &StubSetActorPropertyClass,
    .get_object_property_class = &StubGetObjectPropertyClass,
    .set_object_property_class = &StubSetObjectPropertyClass,
    .get_class_property_struct_path = &StubGetClassPropertyStructPath,
    .get_class_property_container_kinds = &StubGetClassPropertyContainerKinds,
    .set_actor_property_set_element_text = &StubSetActorPropertySetElementText,
    .set_object_property_set_element_text = &StubSetObjectPropertySetElementText,
    .set_actor_property_set_element_value = &StubSetActorPropertySetElementValue,
    .set_object_property_set_element_value = &StubSetObjectPropertySetElementValue,
    .trace_detailed = &StubTraceDetailed,
    .trace_detailed_filtered = &StubTraceDetailedFiltered,
    .set_component_physics_velocity = &StubSetComponentPhysicsVelocity,
    .apply_component_impulse = &StubApplyComponentImpulse,
    .apply_component_force = &StubApplyComponentForce,
    .get_component_physics_angular_velocity = &StubGetComponentPhysicsAngularVelocity,
    .set_component_physics_angular_velocity = &StubSetComponentPhysicsAngularVelocity,
    .apply_component_torque = &StubApplyComponentTorque,
    .apply_component_angular_impulse = &StubApplyComponentAngularImpulse,
    .get_actor_physics_angular_velocity = &StubGetActorPhysicsAngularVelocity,
    .set_actor_physics_angular_velocity = &StubSetActorPhysicsAngularVelocity,
    .apply_actor_torque = &StubApplyActorTorque,
    .apply_actor_angular_impulse = &StubApplyActorAngularImpulse,
    .get_actor_property_map_key = &StubGetActorPropertyMapKey,
    .get_object_property_map_key = &StubGetObjectPropertyMapKey,
    .run_on_game_thread = &StubRunOnGameThread,
    .set_component_collision_channel_response = &StubSetComponentCollisionChannelResponse,
    .get_progress_bar_percent = &StubGetProgressBarPercent,
    .set_progress_bar_percent = &StubSetProgressBarPercent, .get_widget_enabled = &StubGetWidgetEnabled, .set_widget_enabled = &StubSetWidgetEnabled
};

UEC_API uec_result UEC_CALL uec_get_api(uint32_t requestedMajor,
                                        uint32_t requestedMinor,
                                        const uec_api** outApi,
                                        uec_context** outContext)
{
    if (outApi == NULL || outContext == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *outApi = NULL;
    *outContext = NULL;
    if (requestedMajor != UEC_ABI_MAJOR || requestedMinor > UEC_ABI_MINOR) {
        return UEC_RESULT_UNSUPPORTED;
    }
    *outApi = &g_api;
    *outContext = &g_context;
    return UEC_RESULT_OK;
}
