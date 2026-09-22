#include "uec_api.h"

#include <string.h>
#include <stdlib.h>

struct uec_context {
    unsigned int marker;
};

static struct uec_context g_context = {0u};
static char g_last_error[64] = "No error";
static uint8_t* g_versioned_payload = NULL;
static size_t g_versioned_payload_size = 0u;
static uint32_t g_versioned_schema_version = 0u;
static int g_versioned_payload_saved = 0;

static void StubSetLastError(const char* message)
{
    const size_t length = strlen(message);
    const size_t copy_length = length < sizeof(g_last_error) - 1u
        ? length : sizeof(g_last_error) - 1u;
    memcpy(g_last_error, message, copy_length);
    g_last_error[copy_length] = '\0';
}

static uec_result UEC_CALL StubGetCapabilities(uec_context* context,
                                                uec_capabilities* outCapabilities)
{
    if (context != &g_context || outCapabilities == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *outCapabilities = UEC_CAPABILITY_BOOTSTRAP | UEC_CAPABILITY_ACTORS |
        UEC_CAPABILITY_REFLECTION | UEC_CAPABILITY_CLASS_METADATA |
        UEC_CAPABILITY_CONFIGURATION | UEC_CAPABILITY_STREAMING |
        UEC_CAPABILITY_SAVE_DATA |
        UEC_CAPABILITY_REFLECTION_CONTAINERS | UEC_CAPABILITY_COLLISION_DETAILS |
        UEC_CAPABILITY_PHYSICS | UEC_CAPABILITY_EVENT_BRIDGE |
        UEC_CAPABILITY_ASYNC_LATENT_FUNCTIONS;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL StubGetLastError(uec_context* context,
                                            char* buffer,
                                            size_t bufferSize,
                                            size_t* requiredSize)
{
    if (requiredSize != NULL) *requiredSize = 0;
    if (requiredSize == NULL) {
        StubSetLastError("Required-size output is null");
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    if (context != &g_context) {
        StubSetLastError("Invalid context handle");
        return UEC_RESULT_INVALID_HANDLE;
    }
    *requiredSize = strlen(g_last_error) + 1u;
    if (buffer == NULL || bufferSize < *requiredSize) {
        StubSetLastError("Output buffer is null or too small");
        return UEC_RESULT_BUFFER_TOO_SMALL;
    }
    memcpy(buffer, g_last_error, *requiredSize);
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
    if (context != &g_context) return UEC_RESULT_INVALID_HANDLE;
    free(g_versioned_payload);
    g_versioned_payload = NULL;
    g_versioned_payload_size = 0u;
    g_versioned_schema_version = 0u;
    g_versioned_payload_saved = 0;
    return UEC_RESULT_OK;
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

static uec_result UEC_CALL StubSetComponentPhysicsVelocity(uec_scene_component* component,
                                                           uec_vector3 velocity,
                                                           uec_bool addToCurrent)
{
    (void)velocity;
    (void)addToCurrent;
    return component == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubApplyComponentImpulse(uec_scene_component* component,
                                                     uec_vector3 impulse,
                                                     uec_bool velocityChange)
{
    (void)impulse;
    (void)velocityChange;
    return component == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubApplyComponentForce(uec_scene_component* component,
                                                    uec_vector3 force)
{
    (void)force;
    return component == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetComponentPhysicsAngularVelocity(
    uec_scene_component* component,
    uec_vector3* outVelocity)
{
    if (outVelocity != NULL) *outVelocity = (uec_vector3){0.0, 0.0, 0.0};
    if (outVelocity == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    return component == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetComponentPhysicsAngularVelocity(
    uec_scene_component* component,
    uec_vector3 velocity,
    uec_bool addToCurrent)
{
    (void)velocity;
    (void)addToCurrent;
    return component == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubApplyComponentTorque(uec_scene_component* component,
                                                    uec_vector3 torque,
                                                    uec_bool accelerationChange)
{
    (void)torque;
    (void)accelerationChange;
    return component == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubApplyComponentAngularImpulse(
    uec_scene_component* component,
    uec_vector3 impulse,
    uec_bool velocityChange)
{
    (void)impulse;
    (void)velocityChange;
    return component == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetActorPhysicsAngularVelocity(uec_actor* actor,
                                                              uec_vector3* outVelocity)
{
    if (outVelocity != NULL) *outVelocity = (uec_vector3){0.0, 0.0, 0.0};
    if (outVelocity == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    return actor == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetActorPhysicsAngularVelocity(uec_actor* actor,
                                                              uec_vector3 velocity,
                                                              uec_bool addToCurrent)
{
    (void)velocity;
    (void)addToCurrent;
    return actor == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubApplyActorTorque(uec_actor* actor,
                                                uec_vector3 torque,
                                                uec_bool accelerationChange)
{
    (void)torque;
    (void)accelerationChange;
    return actor == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubApplyActorAngularImpulse(uec_actor* actor,
                                                        uec_vector3 impulse,
                                                        uec_bool velocityChange)
{
    (void)impulse;
    (void)velocityChange;
    return actor == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetWorldCountByKind(uec_context* context,
                                                   uec_world_kind kind,
                                                   uint32_t* outCount)
{
    if (outCount != NULL) *outCount = 0u;
    if (outCount == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (kind < UEC_WORLD_KIND_GAME || kind > UEC_WORLD_KIND_INACTIVE) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    return context == &g_context ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubGetWorldAtByKind(uec_context* context,
                                                uec_world_kind kind,
                                                uint32_t index,
                                                uec_world** outWorld)
{
    (void)index;
    if (outWorld != NULL) *outWorld = NULL;
    if (outWorld == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (kind < UEC_WORLD_KIND_GAME || kind > UEC_WORLD_KIND_INACTIVE) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    return context == &g_context ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubInvokeActorFunctionArguments(
    uec_actor* actor,
    uec_string_view functionName,
    const uec_function_argument* arguments,
    uint32_t argumentCount,
    uec_function_output* outputs,
    uint32_t outputCapacity,
    uint32_t* outCount)
{
    (void)actor;
    (void)functionName;
    (void)arguments;
    (void)argumentCount;
    (void)outputs;
    (void)outputCapacity;
    if (outCount != NULL) *outCount = 0u;
    return outCount == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetOrCreateActorEventBridge(uec_actor* actor,
                                                            uec_object** outBridge)
{
    (void)actor;
    if (outBridge != NULL) *outBridge = NULL;
    return outBridge == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubDestroyActorEventBridge(uec_object* bridge)
{
    return bridge == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubBindActorEventBridge(uec_object* bridge,
                                                    uec_event_bridge_callback callback,
                                                    void* userData,
                                                    uint64_t* outSubscriptionId)
{
    (void)bridge;
    (void)userData;
    if (outSubscriptionId != NULL) *outSubscriptionId = 0u;
    if (callback == NULL || outSubscriptionId == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubUnbindActorEventBridge(uec_context* context,
                                                      uint64_t subscriptionId)
{
    (void)subscriptionId;
    return context == &g_context ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubEmitActorEventBridge(uec_object* bridge,
                                                    int64_t eventId,
                                                    int64_t integerValue,
                                                    double realValue,
                                                    uec_string_view textValue)
{
    (void)eventId;
    (void)integerValue;
    (void)realValue;
    (void)textValue;
    return bridge == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubInvokeActorFunctionLatent(
    uec_actor* actor,
    uec_string_view functionName,
    const uec_function_argument* arguments,
    uint32_t argumentCount,
    uec_latent_function_callback callback,
    void* userData,
    uint64_t* outRequestId)
{
    (void)actor;
    (void)userData;
    if (outRequestId != NULL) *outRequestId = 0u;
    if (outRequestId == NULL || callback == NULL ||
        (argumentCount != 0u && arguments == NULL) ||
        functionName.data == NULL || functionName.size == 0u) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubCancelActorFunctionLatent(uec_context* context,
                                                         uint64_t requestId)
{
    (void)requestId;
    return context == &g_context ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubSaveVersionedApplicationData(
    uec_context* context,
    uec_string_view slotName,
    int32_t userIndex,
    uint32_t schemaVersion,
    const uint8_t* data,
    size_t dataSize,
    uec_bool* outSaved)
{
    if (outSaved != NULL) *outSaved = UEC_FALSE;
    if (outSaved == NULL || slotName.data == NULL || slotName.size == 0u || userIndex < 0 ||
        schemaVersion == 0u || dataSize > 16u * 1024u * 1024u ||
        (data == NULL && dataSize != 0u)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    if (context != &g_context) return UEC_RESULT_INVALID_HANDLE;
    uint8_t* copy = dataSize == 0u ? NULL : (uint8_t*)malloc(dataSize);
    if (dataSize != 0u && copy == NULL) return UEC_RESULT_INTERNAL_ERROR;
    if (dataSize != 0u) memcpy(copy, data, dataSize);
    free(g_versioned_payload);
    g_versioned_payload = copy;
    g_versioned_payload_size = dataSize;
    g_versioned_schema_version = schemaVersion;
    g_versioned_payload_saved = 1;
    *outSaved = UEC_TRUE;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL StubLoadVersionedApplicationData(
    uec_context* context,
    uec_string_view slotName,
    int32_t userIndex,
    uint32_t* outSchemaVersion,
    uint8_t* buffer,
    size_t bufferCapacity,
    size_t* outRequiredSize)
{
    if (outSchemaVersion != NULL) *outSchemaVersion = 0u;
    if (outRequiredSize != NULL) *outRequiredSize = 0u;
    if (outSchemaVersion == NULL || outRequiredSize == NULL || slotName.data == NULL ||
        slotName.size == 0u || userIndex < 0 || (buffer == NULL && bufferCapacity != 0u)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    if (context != &g_context) return UEC_RESULT_INVALID_HANDLE;
    if (!g_versioned_payload_saved) return UEC_RESULT_NOT_INITIALIZED;
    *outSchemaVersion = g_versioned_schema_version;
    *outRequiredSize = g_versioned_payload_size;
    if (bufferCapacity < g_versioned_payload_size) return UEC_RESULT_BUFFER_TOO_SMALL;
    if (g_versioned_payload_size != 0u) {
        memcpy(buffer, g_versioned_payload, g_versioned_payload_size);
    }
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL StubGetControllerEnhancedInputSubsystem(
    uec_actor* controller,
    uec_object** outSubsystem)
{
    if (outSubsystem != NULL) *outSubsystem = NULL;
    if (outSubsystem == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    return controller == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
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

static uec_result UEC_CALL StubTraceDetailed(
    uec_world* world,
    uec_vector3 start,
    uec_vector3 end,
    const uec_collision_shape* shape,
    uec_trace_channel channel,
    uec_bool traceComplex,
    uec_hit_result_details* outHit)
{
    (void)world;
    (void)start;
    (void)end;
    (void)shape;
    (void)channel;
    (void)traceComplex;
    if (outHit == NULL || outHit->struct_size < sizeof(*outHit)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    outHit->struct_size = sizeof(*outHit);
    outHit->reserved = 0u;
    outHit->hit = (uec_hit_result){0};
    outHit->impact_point = (uec_vector3){0};
    outHit->impact_normal = (uec_vector3){0};
    outHit->trace_start = (uec_vector3){0};
    outHit->trace_end = (uec_vector3){0};
    outHit->penetration_depth = 0.0;
    outHit->item = -1;
    outHit->face_index = -1;
    outHit->component = NULL;
    return UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubTraceDetailedFiltered(
    uec_world* world,
    uec_vector3 start,
    uec_vector3 end,
    const uec_collision_shape* shape,
    uec_trace_channel channel,
    uec_bool traceComplex,
    const uec_actor* const* ignoredActors,
    uint32_t ignoredActorCount,
    uec_hit_result_details* outHit)
{
    (void)ignoredActors;
    (void)ignoredActorCount;
    return StubTraceDetailed(world, start, end, shape, channel, traceComplex, outHit);
}
