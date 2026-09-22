#include "uec_api.h"

#include <string.h>

struct uec_context {
    unsigned int marker;
};

static struct uec_context g_context = {0u};

static uec_result UEC_CALL StubGetCapabilities(uec_context* context,
                                                uec_capabilities* outCapabilities)
{
    if (context != &g_context || outCapabilities == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *outCapabilities = UEC_CAPABILITY_BOOTSTRAP | UEC_CAPABILITY_ACTORS |
        UEC_CAPABILITY_REFLECTION | UEC_CAPABILITY_CLASS_METADATA |
        UEC_CAPABILITY_CONFIGURATION | UEC_CAPABILITY_STREAMING;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL StubGetLastError(uec_context* context,
                                            char* buffer,
                                            size_t bufferSize,
                                            size_t* requiredSize)
{
    static const char message[] = "No error";
    if (requiredSize != NULL) *requiredSize = 0;
    if (requiredSize == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (context != &g_context) return UEC_RESULT_INVALID_HANDLE;
    *requiredSize = sizeof(message);
    if (buffer == NULL || bufferSize < sizeof(message)) return UEC_RESULT_BUFFER_TOO_SMALL;
    memcpy(buffer, message, sizeof(message));
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL StubLog(uec_context* context, uec_string_view message)
{
    if (context != &g_context || (message.data == NULL && message.size != 0u)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL StubReleaseContext(uec_context* context)
{
    return context == &g_context ? UEC_RESULT_OK : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubGetRuntimeStats(uec_context* context,
                                               uec_runtime_stats* outStats)
{
    if (outStats == NULL || outStats->struct_size < offsetof(uec_runtime_stats, live_contexts)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    outStats->active_subscriptions = 0u;
    outStats->pending_requests = 0u;
    outStats->active_callbacks = 0u;
    if (outStats->struct_size >= sizeof(*outStats)) {
        outStats->live_contexts = 1u;
        outStats->live_worlds = 0u;
        outStats->live_actors = 0u;
        outStats->live_components = 0u;
        outStats->live_classes = 0u;
        outStats->live_objects = 0u;
    }
    return context == &g_context ? UEC_RESULT_OK : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubGetWorldCountByKind(uec_context* context,
                                                   uec_world_kind kind,
                                                   uint32_t* outCount)
{
    (void)kind;
    if (outCount != NULL) *outCount = 0u;
    if (outCount == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    return context == &g_context ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubGetWorldAtByKind(uec_context* context,
                                                uec_world_kind kind,
                                                uint32_t index,
                                                uec_world** outWorld)
{
    (void)kind;
    (void)index;
    if (outWorld != NULL) *outWorld = NULL;
    if (outWorld == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    return context == &g_context ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubInvokeActorFunctionValue(
    uec_actor* actor,
    uec_string_view functionName,
    const uec_property_value* argumentValues,
    uint32_t argumentCount,
    uec_property_value* outReturnValue)
{
    (void)actor;
    (void)functionName;
    (void)argumentValues;
    (void)argumentCount;
    if (outReturnValue != NULL && outReturnValue->struct_size >= sizeof(*outReturnValue)) {
        outReturnValue->kind = UEC_PROPERTY_UNKNOWN;
        outReturnValue->bool_value = UEC_FALSE;
        outReturnValue->integer_value = 0;
        outReturnValue->real_value = 0.0;
    }
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubInvokeActorFunctionValues(
    uec_actor* actor,
    uec_string_view functionName,
    const uec_property_value* argumentValues,
    uint32_t argumentCount,
    uec_property_value* outValues,
    uint32_t outCapacity,
    uint32_t* outCount)
{
    (void)actor;
    (void)functionName;
    (void)argumentValues;
    (void)argumentCount;
    (void)outValues;
    (void)outCapacity;
    if (outCount != NULL) *outCount = 0u;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetClassFunctionParameterAt(
    uec_class* klass,
    uint32_t functionIndex,
    uint32_t parameterIndex,
    char* nameBuffer,
    size_t nameBufferSize,
    size_t* nameRequiredSize,
    uec_property_kind* outKind,
    uint32_t* outFlags)
{
    (void)klass;
    (void)functionIndex;
    (void)parameterIndex;
    (void)nameBuffer;
    (void)nameBufferSize;
    if (nameRequiredSize != NULL) *nameRequiredSize = 0u;
    if (outKind != NULL) *outKind = UEC_PROPERTY_UNKNOWN;
    if (outFlags != NULL) *outFlags = 0u;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubInvokeActorFunctionTextValues(
    uec_actor* actor,
    uec_string_view functionName,
    const uec_string_view* argumentValues,
    uint32_t argumentCount,
    uec_text_output* outValues,
    uint32_t outCapacity,
    uint32_t* outCount)
{
    (void)actor;
    (void)functionName;
    (void)argumentValues;
    (void)argumentCount;
    (void)outValues;
    (void)outCapacity;
    if (outCount != NULL) *outCount = 0u;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubFindObject(uec_context* context,
                                          uec_string_view objectPath,
                                          uec_object** outObject)
{
    (void)objectPath;
    if (outObject != NULL) *outObject = NULL;
    if (outObject == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    return context == &g_context ? UEC_RESULT_NOT_INITIALIZED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubTravelWorldAsync(uec_world* world,
                                                uec_string_view levelPath,
                                                uec_travel_callback callback,
                                                void* userData,
                                                uint64_t* outRequestId)
{
    (void)world;
    (void)levelPath;
    (void)callback;
    (void)userData;
    if (outRequestId != NULL) *outRequestId = 0u;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubCancelTravelRequest(uec_context* context, uint64_t requestId)
{
    (void)requestId;
    return context == &g_context ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubGetComponentVisible(uec_scene_component* component,
                                                   uec_bool* outVisible)
{
    (void)component;
    if (outVisible != NULL) *outVisible = UEC_FALSE;
    return outVisible == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetComponentActive(uec_scene_component* component,
                                                  uec_bool* outActive)
{
    (void)component;
    if (outActive != NULL) *outActive = UEC_FALSE;
    return outActive == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetClassFunctionFlags(uec_class* klass,
                                                    uint32_t index,
                                                    uint32_t* outFlags)
{
    (void)klass;
    (void)index;
    if (outFlags != NULL) *outFlags = 0u;
    return outFlags == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetWidgetVisibility(uec_object* widget,
                                                   uec_widget_visibility* outVisibility)
{
    (void)widget;
    if (outVisibility != NULL) *outVisibility = UEC_WIDGET_VISIBLE;
    return outVisibility == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetTextBlockText(uec_object* widget,
                                                char* buffer,
                                                size_t bufferSize,
                                                size_t* requiredSize)
{
    (void)widget;
    (void)buffer;
    (void)bufferSize;
    if (requiredSize != NULL) *requiredSize = 0u;
    return requiredSize == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetComponentCollisionEnabled(uec_scene_component* component,
                                                            uec_collision_enabled* outEnabled)
{
    (void)component;
    if (outEnabled != NULL) *outEnabled = UEC_COLLISION_DISABLED;
    return outEnabled == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetAudioComponentPlaying(uec_object* audioComponent,
                                                        uec_bool* outPlaying)
{
    (void)audioComponent;
    if (outPlaying != NULL) *outPlaying = UEC_FALSE;
    return outPlaying == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetStreamingLevelStateAsync(
    uec_world* world,
    uec_string_view packagePath,
    uec_bool shouldBeLoaded,
    uec_bool shouldBeVisible,
    uec_streaming_callback callback,
    void* userData,
    uint64_t* outRequestId)
{
    (void)world;
    (void)packagePath;
    (void)shouldBeLoaded;
    (void)shouldBeVisible;
    (void)callback;
    (void)userData;
    if (outRequestId != NULL) *outRequestId = 0u;
    return outRequestId == NULL || callback == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubCancelStreamingLevelRequest(uec_context* context,
                                                            uint64_t requestId)
{
    (void)requestId;
    return context == &g_context ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubGetComponentCollisionResponse(
    uec_scene_component* component,
    uec_trace_channel channel,
    uec_collision_response* outResponse)
{
    (void)component;
    (void)channel;
    if (outResponse != NULL) *outResponse = UEC_COLLISION_RESPONSE_IGNORE;
    return outResponse == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetConfigInteger(uec_context* context,
                                                uec_string_view section,
                                                uec_string_view key,
                                                int64_t* outValue)
{
    (void)section;
    (void)key;
    if (outValue != NULL) *outValue = 0;
    return context == &g_context && outValue != NULL ? UEC_RESULT_UNSUPPORTED :
        (context != &g_context ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_INVALID_ARGUMENT);
}

static uec_result UEC_CALL StubSetConfigInteger(uec_context* context,
                                                uec_string_view section,
                                                uec_string_view key,
                                                int64_t value)
{
    (void)section;
    (void)key;
    (void)value;
    return context == &g_context ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubBindActorDestroyed(uec_actor* actor,
                                                  uec_actor_destroyed_callback callback,
                                                  void* userData,
                                                  uint64_t* outSubscriptionId)
{
    (void)actor;
    (void)userData;
    if (outSubscriptionId != NULL) *outSubscriptionId = 0u;
    return callback == NULL || outSubscriptionId == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubUnbindActorDestroyed(uec_context* context,
                                                    uint64_t subscriptionId)
{
    (void)subscriptionId;
    return context == &g_context ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubGetConfigBool(uec_context* context,
                                             uec_string_view section,
                                             uec_string_view key,
                                             uec_bool* outValue)
{
    (void)section;
    (void)key;
    if (outValue != NULL) *outValue = UEC_FALSE;
    return context == &g_context && outValue != NULL ? UEC_RESULT_UNSUPPORTED :
        (context != &g_context ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_INVALID_ARGUMENT);
}

static uec_result UEC_CALL StubGetActorPropertyArrayCount(uec_actor* actor,
                                                          uec_string_view propertyName,
                                                          uint32_t* outCount)
{
    (void)actor;
    (void)propertyName;
    if (outCount != NULL) *outCount = 0u;
    return outCount == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetActorPropertyArrayElementText(
    uec_actor* actor,
    uec_string_view propertyName,
    uint32_t index,
    char* buffer,
    size_t bufferSize,
    size_t* requiredSize,
    uec_property_kind* outKind)
{
    (void)actor;
    (void)propertyName;
    (void)index;
    (void)buffer;
    (void)bufferSize;
    if (requiredSize != NULL) *requiredSize = 0u;
    if (outKind != NULL) *outKind = UEC_PROPERTY_UNKNOWN;
    return requiredSize == NULL || outKind == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetObjectPropertyArrayCount(uec_object* object,
                                                           uec_string_view propertyName,
                                                           uint32_t* outCount)
{
    (void)object;
    (void)propertyName;
    if (outCount != NULL) *outCount = 0u;
    return outCount == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetObjectPropertyArrayElementText(
    uec_object* object,
    uec_string_view propertyName,
    uint32_t index,
    char* buffer,
    size_t bufferSize,
    size_t* requiredSize,
    uec_property_kind* outKind)
{
    (void)object;
    (void)propertyName;
    (void)index;
    (void)buffer;
    (void)bufferSize;
    if (requiredSize != NULL) *requiredSize = 0u;
    if (outKind != NULL) *outKind = UEC_PROPERTY_UNKNOWN;
    return requiredSize == NULL || outKind == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubPrepareTextOutput(uec_text_output* output)
{
    if (output == NULL || output->struct_size < sizeof(uec_text_output)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    output->kind = UEC_PROPERTY_UNKNOWN;
    output->required_size = 0u;
    return UEC_RESULT_OK;
}

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
    .invoke_actor_function_text_values = &StubInvokeActorFunctionTextValues,
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
    .run_on_game_thread = &StubRunOnGameThread
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
