#include "uec_api.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

typedef struct uec_event_bridge_smoke_state {
    uint64_t subscription_id;
    uint32_t callback_count;
    uec_bool payload_valid;
    const uec_api* api;
    uec_context* context;
    uec_result self_unbind_result;
} uec_event_bridge_smoke_state;

typedef struct uec_latent_smoke_state {
    const uec_api* api;
    uec_context* context;
    uec_world* world;
    uec_actor* actor;
    uint64_t request_id, cancelled_request_id;
    uint32_t cancelled_callback_count, baseline_pending_requests;
    uec_result result;
    uec_bool callback_received, started, complete;
} uec_latent_smoke_state;

static uec_latent_smoke_state g_latent_smoke_state;
static void FinishLatentSmoke(uec_latent_smoke_state* state,
                              uec_result result,
                              uec_bool cancel_request)
{
    if (state == NULL || state->complete == UEC_TRUE) return;
    if (cancel_request == UEC_TRUE && state->request_id != 0 &&
        state->callback_received != UEC_TRUE && state->api != NULL &&
        state->context != NULL) {
        const uec_result cancel_result = state->api->cancel_actor_function_latent(
            state->context, state->request_id);
        if (result == UEC_RESULT_OK && cancel_result != UEC_RESULT_OK) result = cancel_result;
    }
    state->request_id = 0;
    if (state->actor != NULL && state->api != NULL) {
        const uec_result destroy_result = state->api->destroy_actor(state->actor);
        if (destroy_result != UEC_RESULT_OK) (void)state->api->release_actor(state->actor);
        if (result == UEC_RESULT_OK && destroy_result != UEC_RESULT_OK) result = destroy_result;
        state->actor = NULL;
    }
    if (state->world != NULL && state->api != NULL) {
        const uec_result release_result = state->api->release_world(state->world);
        if (result == UEC_RESULT_OK && release_result != UEC_RESULT_OK) result = release_result;
        state->world = NULL;
    }
    if (state->context != NULL && state->api != NULL) {
        const uec_result release_result = state->api->release_context(state->context);
        if (result == UEC_RESULT_OK && release_result != UEC_RESULT_OK) result = release_result;
        state->context = NULL;
    }
    state->result = result;
    state->complete = UEC_TRUE;
}

static uec_result FailLatentSmoke(uec_latent_smoke_state* state)
{
    FinishLatentSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
    return UEC_RESULT_INTERNAL_ERROR;
}

static void UEC_CALL CompleteLatentSmoke(uint64_t requestId,
                                         uec_result result,
                                         void* userData)
{
    uec_latent_smoke_state* state = (uec_latent_smoke_state*)userData;
    if (state == NULL) return;
    if (requestId == state->cancelled_request_id) {
        ++state->cancelled_callback_count;
        return;
    }
    if (state->complete == UEC_TRUE || state->callback_received == UEC_TRUE) return;
    if (requestId != state->request_id) result = UEC_RESULT_INTERNAL_ERROR;
    state->result = result;
    state->callback_received = UEC_TRUE;
}

static void UEC_CALL VerifyEventBridgeCallback(uint64_t subscriptionId,
                                               int64_t eventId,
                                               int64_t integerValue,
                                               double realValue,
                                               uec_string_view textValue,
                                               void* userData)
{
    uec_event_bridge_smoke_state* state = (uec_event_bridge_smoke_state*)userData;
    const char expectedText[] = "bridge-smoke";
    if (state == NULL) return;
    ++state->callback_count;
    state->payload_valid = UEC_FALSE;
    if (subscriptionId != state->subscription_id || eventId != 731 || integerValue != -42 ||
        realValue != 3.25 || textValue.data == NULL || textValue.size != sizeof(expectedText) - 1) {
        return;
    }
    for (size_t index = 0; index < textValue.size; ++index) {
        if (textValue.data[index] != expectedText[index]) return;
    }
    state->payload_valid = UEC_TRUE;
}

static void UEC_CALL SelfUnbindEventBridgeCallback(uint64_t subscriptionId,
                                                   int64_t eventId,
                                                   int64_t integerValue,
                                                   double realValue,
                                                   uec_string_view textValue,
                                                   void* userData)
{
    uec_event_bridge_smoke_state* state = (uec_event_bridge_smoke_state*)userData;
    const char expectedText[] = "bridge-smoke";
    if (state == NULL) return;
    ++state->callback_count;
    state->payload_valid = UEC_FALSE;
    if (subscriptionId != state->subscription_id || eventId != 732 || integerValue != 7 ||
        realValue != 4.5 || textValue.data == NULL || textValue.size != sizeof(expectedText) - 1) {
        return;
    }
    for (size_t index = 0; index < textValue.size; ++index) {
        if (textValue.data[index] != expectedText[index]) return;
    }
    state->payload_valid = UEC_TRUE;
    state->self_unbind_result = state->api->unbind_actor_event_bridge(state->context,
                                                                      subscriptionId);
}

uec_result UEC_CALL uec_host_event_bridge_smoke(void)
{
    static const char actorClassPath[] = "/Script/Engine.Actor";
    static const char eventText[] = "bridge-smoke";
    const uec_string_view classPath = {actorClassPath, sizeof(actorClassPath) - 1};
    const uec_string_view text = {eventText, sizeof(eventText) - 1};
    const uec_transform initialTransform = {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* world = NULL;
    uec_world* probeWorld = NULL;
    uec_actor* actor = NULL;
    uec_object* bridge = NULL;
    uint64_t subscriptionId = 0;
    uec_runtime_stats baselineStats = {sizeof(uec_runtime_stats), 0u, 0u, 0u, 0u,
                                       0u, 0u, 0u, 0u, 0u};
    uec_runtime_stats observedStats = {sizeof(uec_runtime_stats), 0u, 0u, 0u, 0u,
                                       0u, 0u, 0u, 0u, 0u};
    uec_event_bridge_smoke_state state = {0, 0, UEC_FALSE, NULL, NULL,
                                          UEC_RESULT_INTERNAL_ERROR};
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    state.api = api;
    state.context = context;
    if (api == NULL || context == NULL || api->get_or_create_actor_event_bridge == NULL ||
        api->destroy_actor_event_bridge == NULL || api->bind_actor_event_bridge == NULL ||
        api->unbind_actor_event_bridge == NULL || api->emit_actor_event_bridge == NULL ||
        api->get_runtime_stats == NULL || api->get_capabilities == NULL ||
        api->get_default_world == NULL || api->release_world == NULL ||
        api->line_trace == NULL || api->sweep_trace == NULL ||
        api->get_component_transform == NULL || api->get_widget_enabled == NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    uec_capabilities capabilities = 0;
    result = api->get_capabilities(context, &capabilities);
    if (result != UEC_RESULT_OK || (capabilities & UEC_CAPABILITY_EVENT_BRIDGE) == 0) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_UNSUPPORTED;
        goto cleanup;
    }
    result = api->get_runtime_stats(context, &baselineStats);
    if (result != UEC_RESULT_OK) goto cleanup;
    if (baselineStats.live_worlds == UINT32_MAX) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_default_world(context, &probeWorld);
    if (result != UEC_RESULT_OK || probeWorld == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_runtime_stats(context, &observedStats);
    if (result != UEC_RESULT_OK || observedStats.live_worlds != baselineStats.live_worlds + 1u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->release_world(probeWorld);
    if (result != UEC_RESULT_OK) goto cleanup;
    probeWorld = NULL;
    result = api->get_runtime_stats(context, &observedStats);
    if (result != UEC_RESULT_OK || observedStats.live_worlds != baselineStats.live_worlds) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_default_world(context, &world);
    if (result != UEC_RESULT_OK) goto cleanup;
    uec_hit_result invalidHit = {0};
    invalidHit.blocking_hit = UEC_TRUE;
    invalidHit.distance = 1.0;
    if (api->line_trace(world, (uec_vector3){0}, (uec_vector3){0},
            (uec_trace_channel)99, UEC_FALSE, &invalidHit) != UEC_RESULT_INVALID_ARGUMENT ||
        invalidHit.blocking_hit != UEC_FALSE || invalidHit.distance != 0.0 ||
        invalidHit.actor != NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    const uec_collision_shape invalidShape = {
        sizeof(uec_collision_shape), (uec_collision_shape_kind)99, 0u, 0.0, {0}, 0.0};
    invalidHit.blocking_hit = UEC_TRUE;
    invalidHit.distance = 1.0;
    if (api->sweep_trace(world, (uec_vector3){0}, (uec_vector3){0}, &invalidShape,
            UEC_TRACE_VISIBILITY, UEC_FALSE, &invalidHit) != UEC_RESULT_INVALID_ARGUMENT ||
        invalidHit.blocking_hit != UEC_FALSE || invalidHit.distance != 0.0 ||
        invalidHit.actor != NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->spawn_actor(world, classPath, &initialTransform, &actor);
    if (result != UEC_RESULT_OK) goto cleanup;
    uec_transform zeroTransform = {0};
    uec_bool wrongKindEnabled = UEC_TRUE;
    uec_transform wrongKindTransform = {
        {41.0, 42.0, 43.0}, {44.0, 45.0, 46.0, 47.0}, {48.0, 49.0, 50.0}};
    if (api->get_component_transform((uec_scene_component*)actor, &wrongKindTransform) !=
            UEC_RESULT_INVALID_HANDLE ||
        memcmp(&wrongKindTransform, &zeroTransform, sizeof(zeroTransform)) != 0 ||
        api->get_widget_enabled((uec_object*)actor, &wrongKindEnabled) !=
            UEC_RESULT_INVALID_HANDLE || wrongKindEnabled != UEC_FALSE) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_or_create_actor_event_bridge(actor, &bridge);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->bind_actor_event_bridge(bridge, &VerifyEventBridgeCallback,
                                          &state, &subscriptionId);
    if (result != UEC_RESULT_OK) goto cleanup;
    state.subscription_id = subscriptionId;
    result = api->emit_actor_event_bridge(bridge, 731, -42, 3.25, text);
    if (result != UEC_RESULT_OK || state.callback_count != 1 ||
        state.payload_valid != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->unbind_actor_event_bridge(context, subscriptionId);
    if (result != UEC_RESULT_OK) goto cleanup;
    subscriptionId = 0;
    result = api->emit_actor_event_bridge(bridge, 732, 0, 0.0, text);
    if (result != UEC_RESULT_OK || state.callback_count != 1) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->bind_actor_event_bridge(bridge, &SelfUnbindEventBridgeCallback,
                                          &state, &subscriptionId);
    if (result != UEC_RESULT_OK) goto cleanup;
    state.subscription_id = subscriptionId;
    result = api->emit_actor_event_bridge(bridge, 732, 7, 4.5, text);
    if (result != UEC_RESULT_OK || state.callback_count != 2 ||
        state.payload_valid != UEC_TRUE || state.self_unbind_result != UEC_RESULT_OK) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    subscriptionId = 0;
    result = api->emit_actor_event_bridge(bridge, 733, 0, 0.0, text);
    if (result != UEC_RESULT_OK || state.callback_count != 2) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->bind_actor_event_bridge(bridge, &VerifyEventBridgeCallback,
                                          &state, &subscriptionId);
    if (result != UEC_RESULT_OK) goto cleanup;
    state.subscription_id = subscriptionId;
    result = api->get_runtime_stats(context, &observedStats);
    if (result != UEC_RESULT_OK ||
        observedStats.active_subscriptions != baselineStats.active_subscriptions + 1u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    uec_object* destroyedBridge = bridge;
    result = api->destroy_actor_event_bridge(bridge);
    if (result != UEC_RESULT_OK) goto cleanup;
    bridge = NULL;
    if (api->emit_actor_event_bridge(destroyedBridge, 734, 0, 0.0, text) !=
        UEC_RESULT_INVALID_HANDLE) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    uec_actor* destroyedActor = actor;
    result = api->destroy_actor(actor);
    if (result != UEC_RESULT_OK) goto cleanup;
    actor = NULL;
    if (api->destroy_actor(destroyedActor) != UEC_RESULT_INVALID_HANDLE) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_runtime_stats(context, &observedStats);
    if (result != UEC_RESULT_OK ||
        observedStats.active_subscriptions != baselineStats.active_subscriptions ||
        state.callback_count != 2) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = UEC_RESULT_OK;

cleanup:
    if (api != NULL && subscriptionId != 0 && context != NULL) {
        const uec_result cleanupResult = api->unbind_actor_event_bridge(context, subscriptionId);
        if (result == UEC_RESULT_OK) result = cleanupResult;
    }
    if (api != NULL && bridge != NULL) {
        const uec_result cleanupResult = api->destroy_actor_event_bridge(bridge);
        if (cleanupResult != UEC_RESULT_OK) (void)api->release_object(bridge);
        if (result == UEC_RESULT_OK) result = cleanupResult;
    }
    if (api != NULL && actor != NULL) {
        const uec_result cleanupResult = api->destroy_actor(actor);
        if (cleanupResult != UEC_RESULT_OK) (void)api->release_actor(actor);
        if (result == UEC_RESULT_OK) result = cleanupResult;
    }
    if (api != NULL && probeWorld != NULL) {
        const uec_result cleanupResult = api->release_world(probeWorld);
        if (result == UEC_RESULT_OK) result = cleanupResult;
    }
    if (api != NULL && world != NULL) {
        const uec_result cleanupResult = api->release_world(world);
        if (result == UEC_RESULT_OK) result = cleanupResult;
    }
    if (api != NULL && context != NULL) {
        const uec_result cleanupResult = api->release_context(context);
        if (result == UEC_RESULT_OK) result = cleanupResult;
    }
    return result;
}

uec_result UEC_CALL uec_host_latent_smoke_start(void)
{
    static const char actorClassPath[] =
        "/Script/UnrealCAPIHost.UECAPIHostLatentSmokeActor";
    static const char functionName[] = "WaitForSmokeDuration";
    static const char nonLatentFunctionName[] = "NoOpSmokeCall";
    static const char scalarFunctionName[] = "ScalarSmokeCall";
    static const char textFunctionName[] = "ValidateSmokeText";
    static const char echoTextFunctionName[] = "EchoSmokeText";
    static const char vectorFunctionName[] = "VectorSmokeCall";
    static const char quaternionFunctionName[] = "QuaternionSmokeCall";
    static const char transformFunctionName[] = "TransformSmokeCall";
    static const char outputFunctionName[] = "BuildSmokeOutputs";
    static const char worldContextFunctionName[] = "WorldContextSmokeCall";
    static const char missingFunctionName[] = "MissingLatentSmokeFunction";
    uec_latent_smoke_state* state = &g_latent_smoke_state;
    if (state->started == UEC_TRUE) return UEC_RESULT_INVALID_ARGUMENT;
    *state = (uec_latent_smoke_state){0};
    state->started = UEC_TRUE;
    uec_runtime_stats baselineStats = {0};
    baselineStats.struct_size = sizeof(baselineStats);

    const uec_string_view classPath = {actorClassPath, sizeof(actorClassPath) - 1};
    const uec_string_view latentName = {functionName, sizeof(functionName) - 1};
    const uec_transform initialTransform = {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR,
                                    &state->api, &state->context);
    if (result != UEC_RESULT_OK) {
        FinishLatentSmoke(state, result, UEC_FALSE);
        return result;
    }
    if (state->api == NULL || state->context == NULL ||
        state->api->get_capabilities == NULL ||
        state->api->get_default_world == NULL || state->api->release_world == NULL ||
        state->api->get_runtime_stats == NULL ||
        state->api->spawn_actor == NULL || state->api->destroy_actor == NULL ||
        state->api->release_actor == NULL ||
        state->api->invoke_actor_function_latent == NULL ||
        state->api->cancel_actor_function_latent == NULL ||
        state->api->invoke_actor_function_arguments == NULL ||
        state->api->invoke_actor_function_value == NULL ||
        state->api->invoke_actor_function_text == NULL ||
        state->api->get_world_count_by_kind == NULL ||
        state->api->get_world_at_by_kind == NULL ||
        state->api->release_context == NULL) {
        return FailLatentSmoke(state);
    }
    uec_capabilities capabilities = 0;
    result = state->api->get_capabilities(state->context, &capabilities);
    if (result != UEC_RESULT_OK ||
        (capabilities & UEC_CAPABILITY_ASYNC_LATENT_FUNCTIONS) == 0) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_UNSUPPORTED;
        FinishLatentSmoke(state, result, UEC_FALSE);
        return result;
    }
    result = state->api->get_runtime_stats(state->context, &baselineStats);
    if (result != UEC_RESULT_OK) {
        FinishLatentSmoke(state, result, UEC_FALSE);
        return result;
    }
    state->baseline_pending_requests = baselineStats.pending_requests;
    result = state->api->get_default_world(state->context, &state->world);
    if (result == UEC_RESULT_OK) {
        result = state->api->spawn_actor(state->world, classPath,
                                         &initialTransform, &state->actor);
    }
    if (result != UEC_RESULT_OK) {
        FinishLatentSmoke(state, result, UEC_FALSE);
        return result;
    }

    uec_function_argument duration = {0};
    duration.struct_size = sizeof(duration);
    duration.kind = UEC_PROPERTY_FLOAT;
    duration.real_value = 0.05;
    uec_function_argument latentArguments[2] = {0};
    latentArguments[0].struct_size = sizeof(latentArguments[0]);
    latentArguments[0].kind = UEC_PROPERTY_OBJECT;
    latentArguments[0].world_value = state->world;
    latentArguments[1] = duration;
    uint64_t rejectedRequestId = UINT64_MAX;
    uec_string_view nonLatentName = {
        nonLatentFunctionName, sizeof(nonLatentFunctionName) - 1};
    result = state->api->invoke_actor_function_latent(
        state->actor, nonLatentName, NULL, 0u, &CompleteLatentSmoke,
        state, &rejectedRequestId);
    if (result != UEC_RESULT_UNSUPPORTED || rejectedRequestId != 0)
        return FailLatentSmoke(state);
    uec_string_view worldContextName = {
        worldContextFunctionName, sizeof(worldContextFunctionName) - 1};
    uint32_t noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, worldContextName, &latentArguments[0], 1u,
        NULL, 0u, &noOutputs);
    if (result != UEC_RESULT_OK || noOutputs != 0) return FailLatentSmoke(state);
    uec_function_argument malformedScalarArgument = duration;
    malformedScalarArgument.world_value = state->world;
    uec_string_view scalarName = {
        scalarFunctionName, sizeof(scalarFunctionName) - 1};
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, scalarName, &malformedScalarArgument, 1u,
        NULL, 0u, &noOutputs);
    if (result != UEC_RESULT_INVALID_ARGUMENT || noOutputs != 0)
        return FailLatentSmoke(state);
    uec_function_argument scalarWithText = duration;
    scalarWithText.text_value.data = scalarFunctionName;
    scalarWithText.text_value.size = sizeof(scalarFunctionName) - 1;
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, scalarName, &scalarWithText, 1u,
        NULL, 0u, &noOutputs);
    if (result != UEC_RESULT_INVALID_ARGUMENT || noOutputs != 0)
        return FailLatentSmoke(state);
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, scalarName, NULL, 0u, NULL, 0u, &noOutputs);
    if (result != UEC_RESULT_INVALID_ARGUMENT || noOutputs != 0)
        return FailLatentSmoke(state);
    uec_function_argument wrongScalarKind = duration;
    wrongScalarKind.kind = (uec_property_kind)99;
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, scalarName, &wrongScalarKind, 1u,
        NULL, 0u, &noOutputs);
    if (result != UEC_RESULT_INVALID_ARGUMENT || noOutputs != 0) {
        return FailLatentSmoke(state);
    }
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, scalarName, &duration, 1u, NULL, 0u, &noOutputs);
    if (result != UEC_RESULT_OK || noOutputs != 0) return FailLatentSmoke(state);
    uec_function_argument structArgument = {0};
    structArgument.struct_size = sizeof(structArgument);
    structArgument.kind = UEC_PROPERTY_STRUCT;
    structArgument.struct_value.kind = UEC_FUNCTION_STRUCT_VECTOR3;
    structArgument.struct_value.value.vector3 = (uec_vector3){1.25, -2.5, 9.0};
    uec_function_output structOutput = {0};
    structOutput.struct_size = sizeof(structOutput);
    structOutput.struct_value.kind = UEC_FUNCTION_STRUCT_VECTOR3;
    uec_string_view vectorFunction = {
        vectorFunctionName, sizeof(vectorFunctionName) - 1};
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, vectorFunction, &structArgument, 1u,
        &structOutput, 1u, &noOutputs);
    if (result != UEC_RESULT_OK || noOutputs != 1u ||
        structOutput.kind != UEC_PROPERTY_STRUCT ||
        structOutput.struct_value.kind != UEC_FUNCTION_STRUCT_VECTOR3 ||
        structOutput.struct_value.value.vector3.x != 1.25 ||
        structOutput.struct_value.value.vector3.y != -2.5 ||
        structOutput.struct_value.value.vector3.z != 9.0) {
        return FailLatentSmoke(state);
    }
    structArgument.struct_value.value.vector3.x = NAN;
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, vectorFunction, &structArgument, 1u,
        &structOutput, 1u, &noOutputs);
    if (result != UEC_RESULT_INVALID_ARGUMENT) return FailLatentSmoke(state);
    structArgument.struct_value.kind = (uec_function_struct_kind)99;
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, vectorFunction, &structArgument, 1u, &structOutput, 1u, &noOutputs);
    if (result != UEC_RESULT_INVALID_ARGUMENT || noOutputs != 0u)
        return FailLatentSmoke(state);
    structArgument.struct_value.kind = UEC_FUNCTION_STRUCT_QUATERNION;
    structArgument.struct_value.value.quaternion = (uec_quaternion){
        0.0, 0.0, 0.7071067811865476, 0.7071067811865476};
    structOutput.struct_value.kind = UEC_FUNCTION_STRUCT_QUATERNION;
    uec_string_view quaternionFunction = {
        quaternionFunctionName, sizeof(quaternionFunctionName) - 1};
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, quaternionFunction, &structArgument, 1u,
        &structOutput, 1u, &noOutputs);
    if (result != UEC_RESULT_OK || noOutputs != 1u ||
        structOutput.kind != UEC_PROPERTY_STRUCT ||
        structOutput.struct_value.kind != UEC_FUNCTION_STRUCT_QUATERNION ||
        structOutput.struct_value.value.quaternion.z != 0.7071067811865476 ||
        structOutput.struct_value.value.quaternion.w != 0.7071067811865476) {
        return FailLatentSmoke(state);
    }
    structArgument.struct_value.kind = UEC_FUNCTION_STRUCT_TRANSFORM;
    structArgument.struct_value.value.transform = (uec_transform){
        {1.0, 2.0, 3.0}, {0.0, 0.0, 0.0, 1.0}, {2.0, 3.0, 4.0}};
    structOutput.struct_value.kind = UEC_FUNCTION_STRUCT_TRANSFORM;
    uec_string_view transformFunction = {
        transformFunctionName, sizeof(transformFunctionName) - 1};
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, transformFunction, &structArgument, 1u,
        &structOutput, 1u, &noOutputs);
    if (result != UEC_RESULT_OK || noOutputs != 1u ||
        structOutput.kind != UEC_PROPERTY_STRUCT ||
        structOutput.struct_value.kind != UEC_FUNCTION_STRUCT_TRANSFORM ||
        structOutput.struct_value.value.transform.translation.x != 1.0 ||
        structOutput.struct_value.value.transform.translation.y != 2.0 ||
        structOutput.struct_value.value.transform.translation.z != 3.0 ||
        structOutput.struct_value.value.transform.scale.x != 2.0 ||
        structOutput.struct_value.value.transform.scale.y != 3.0 ||
        structOutput.struct_value.value.transform.scale.z != 4.0) {
        return FailLatentSmoke(state);
    }
    structArgument.struct_value.kind = UEC_FUNCTION_STRUCT_VECTOR3;
    structArgument.struct_value.value.vector3 = (uec_vector3){0.0, 0.0, 0.0};
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, transformFunction, &structArgument, 1u,
        &structOutput, 1u, &noOutputs);
    if (result != UEC_RESULT_INVALID_ARGUMENT) return FailLatentSmoke(state);
    structArgument.struct_value.kind = UEC_FUNCTION_STRUCT_QUATERNION;
    structArgument.struct_value.value.quaternion = (uec_quaternion){0.0, 0.0, 0.0, 0.0};
    structOutput.struct_value.kind = UEC_FUNCTION_STRUCT_QUATERNION;
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, quaternionFunction, &structArgument, 1u,
        &structOutput, 1u, &noOutputs);
    if (result != UEC_RESULT_INVALID_ARGUMENT) return FailLatentSmoke(state);
    static const char quotedSmokeText[] = "\"mixed-smoke\"";
    uec_function_argument textArgument = {0};
    textArgument.struct_size = (uint32_t)offsetof(uec_function_argument, struct_value);
    textArgument.kind = UEC_PROPERTY_STRING;
    textArgument.text_value.data = quotedSmokeText;
    textArgument.text_value.size = sizeof(quotedSmokeText) - 1;
    uec_function_output textOutput = {0};
    textOutput.struct_size = sizeof(textOutput);
    uec_string_view textFunction = {
        textFunctionName, sizeof(textFunctionName) - 1};
    noOutputs = 0;
    result = state->api->invoke_actor_function_arguments(
        state->actor, textFunction, &textArgument, 1u,
        NULL, 0u, &noOutputs);
    if (result != UEC_RESULT_BUFFER_TOO_SMALL || noOutputs != 1u) {
        return FailLatentSmoke(state);
    }
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, textFunction, &textArgument, 1u,
        &textOutput, 1u, &noOutputs);
    if (result != UEC_RESULT_OK || noOutputs != 1u ||
        textOutput.kind != UEC_PROPERTY_BOOL || textOutput.bool_value != UEC_TRUE) {
        return FailLatentSmoke(state);
    }
    char echoedText[64] = {0};
    uec_function_output echoOutput = {0};
    echoOutput.struct_size = (uint32_t)offsetof(uec_function_output, struct_value);
    echoOutput.text_buffer = echoedText;
    echoOutput.text_buffer_size = 4u;
    uec_string_view echoTextFunction = {
        echoTextFunctionName, sizeof(echoTextFunctionName) - 1};
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, echoTextFunction, &textArgument, 1u,
        &echoOutput, 1u, &noOutputs);
    if (result != UEC_RESULT_BUFFER_TOO_SMALL || noOutputs != 1u ||
        echoOutput.kind != UEC_PROPERTY_STRING ||
        echoOutput.text_required_size <= echoOutput.text_buffer_size) {
        return FailLatentSmoke(state);
    }
    echoOutput.text_buffer_size = sizeof(echoedText);
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, echoTextFunction, &textArgument, 1u,
        &echoOutput, 1u, &noOutputs);
    if (result != UEC_RESULT_OK || noOutputs != 1u ||
        echoOutput.kind != UEC_PROPERTY_STRING ||
        echoOutput.text_required_size == 0 ||
        echoOutput.text_required_size > sizeof(echoedText) ||
        echoedText[echoOutput.text_required_size - 1] != '\0' ||
        strstr(echoedText, "mixed-smoke") == NULL) {
        return FailLatentSmoke(state);
    }
    uec_string_view outputFunction = {
        outputFunctionName, sizeof(outputFunctionName) - 1};
    noOutputs = 0;
    result = state->api->invoke_actor_function_arguments(
        state->actor, outputFunction, NULL, 0u, NULL, 0u, &noOutputs);
    if (result != UEC_RESULT_BUFFER_TOO_SMALL || noOutputs != 3u) {
        return FailLatentSmoke(state);
    }
    char outputText[64] = {0};
    uec_function_output mixedOutputs[3] = {0};
    for (uint32_t index = 0; index < 3u; ++index) {
        mixedOutputs[index].struct_size = sizeof(mixedOutputs[index]);
    }
    mixedOutputs[2].text_buffer = outputText;
    mixedOutputs[2].text_buffer_size = sizeof(outputText);
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, outputFunction, NULL, 0u, mixedOutputs, 3u, &noOutputs);
    if (result != UEC_RESULT_OK || noOutputs != 3u ||
        mixedOutputs[0].kind != UEC_PROPERTY_BOOL ||
        mixedOutputs[0].bool_value != UEC_TRUE ||
        mixedOutputs[1].kind != UEC_PROPERTY_INTEGER ||
        mixedOutputs[1].integer_value != 42 ||
        mixedOutputs[2].kind != UEC_PROPERTY_STRING ||
        mixedOutputs[2].text_required_size == 0 ||
        mixedOutputs[2].text_required_size > sizeof(outputText) ||
        outputText[mixedOutputs[2].text_required_size - 1] != '\0' ||
        strstr(outputText, "output-smoke") == NULL) {
        return FailLatentSmoke(state);
    }
    uint32_t invalidWorldCount = UINT32_MAX;
    result = state->api->get_world_count_by_kind(
        state->context, (uec_world_kind)99, &invalidWorldCount);
    if (result != UEC_RESULT_INVALID_ARGUMENT || invalidWorldCount != 0)
        return FailLatentSmoke(state);
    uint32_t editorWorldCount = 0;
    result = state->api->get_world_count_by_kind(
        state->context, UEC_WORLD_KIND_EDITOR, &editorWorldCount);
    if (result != UEC_RESULT_OK) {
        FinishLatentSmoke(state, result, UEC_FALSE);
        return result;
    }
    if (editorWorldCount != 0) {
        uec_world* otherWorld = NULL;
        result = state->api->get_world_at_by_kind(
            state->context, UEC_WORLD_KIND_EDITOR, 0u, &otherWorld);
        if (result != UEC_RESULT_OK || otherWorld == NULL) {
            FinishLatentSmoke(state, result == UEC_RESULT_OK
                ? UEC_RESULT_INTERNAL_ERROR : result, UEC_FALSE);
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }
        if (otherWorld != state->world) {
            uec_function_argument crossWorldContext = latentArguments[0];
            crossWorldContext.world_value = otherWorld;
            noOutputs = UINT32_MAX;
            const uec_result crossWorldResult = state->api->invoke_actor_function_arguments(
                state->actor, worldContextName, &crossWorldContext, 1u,
                NULL, 0u, &noOutputs);
            if (crossWorldResult != UEC_RESULT_INVALID_ARGUMENT || noOutputs != 0) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }
        const uec_result releaseOtherWorldResult = state->api->release_world(otherWorld);
        if (result == UEC_RESULT_OK && releaseOtherWorldResult != UEC_RESULT_OK) {
            result = releaseOtherWorldResult;
        }
        if (result != UEC_RESULT_OK) {
            FinishLatentSmoke(state, result, UEC_FALSE);
            return result;
        }
    }
    uec_property_value scalarOutput = {0};
    scalarOutput.struct_size = sizeof(scalarOutput);
    result = state->api->invoke_actor_function_value(
        state->actor, latentName, NULL, 0u, &scalarOutput);
    if (result != UEC_RESULT_UNSUPPORTED) return FailLatentSmoke(state);
    size_t requiredSize = 99u;
    uec_property_kind returnKind = UEC_PROPERTY_INTEGER;
    result = state->api->invoke_actor_function_text(
        state->actor, latentName, NULL, 0u, NULL, 0u,
        &requiredSize, &returnKind);
    if (result != UEC_RESULT_UNSUPPORTED || requiredSize != 0u ||
        returnKind != UEC_PROPERTY_UNKNOWN) {
        return FailLatentSmoke(state);
    }
    uec_string_view missingName = {
        missingFunctionName, sizeof(missingFunctionName) - 1};
    rejectedRequestId = UINT64_MAX;
    result = state->api->invoke_actor_function_latent(
        state->actor, missingName, NULL, 0u, &CompleteLatentSmoke,
        state, &rejectedRequestId);
    if (result != UEC_RESULT_INVALID_ARGUMENT || rejectedRequestId != 0) {
        return FailLatentSmoke(state);
    }
    rejectedRequestId = UINT64_MAX;
    result = state->api->invoke_actor_function_latent(
        state->actor, latentName, NULL, 0u, &CompleteLatentSmoke,
        state, &rejectedRequestId);
    if (result != UEC_RESULT_UNSUPPORTED || rejectedRequestId != 0) {
        return FailLatentSmoke(state);
    }
    uec_function_argument missingWorldContext[2] = {
        latentArguments[0], latentArguments[1]};
    missingWorldContext[0].world_value = NULL;
    rejectedRequestId = UINT64_MAX;
    result = state->api->invoke_actor_function_latent(
        state->actor, latentName, missingWorldContext, 2u,
        &CompleteLatentSmoke, state, &rejectedRequestId);
    if (result != UEC_RESULT_UNSUPPORTED || rejectedRequestId != 0) {
        return FailLatentSmoke(state);
    }
    uec_function_argument wrongDurationKind = duration;
    wrongDurationKind.kind = UEC_PROPERTY_INTEGER;
    uec_function_argument wrongLatentArguments[2] = {
        latentArguments[0], wrongDurationKind};
    rejectedRequestId = UINT64_MAX;
    result = state->api->invoke_actor_function_latent(
        state->actor, latentName, wrongLatentArguments, 2u,
        &CompleteLatentSmoke, state, &rejectedRequestId);
    if (result != UEC_RESULT_INVALID_ARGUMENT || rejectedRequestId != 0) {
        return FailLatentSmoke(state);
    }
    result = state->api->invoke_actor_function_latent(
        state->actor, latentName, latentArguments, 2u, &CompleteLatentSmoke,
        state, &state->request_id);
    if (result != UEC_RESULT_OK) {
        FinishLatentSmoke(state, result, UEC_TRUE);
        return result;
    }
    result = state->api->invoke_actor_function_latent(
        state->actor, latentName, latentArguments, 2u, &CompleteLatentSmoke,
        state, &state->cancelled_request_id);
    if (result == UEC_RESULT_OK) {
        result = state->api->cancel_actor_function_latent(
            state->context, state->cancelled_request_id);
    }
    if (result == UEC_RESULT_OK) {
        uec_runtime_stats observedStats = {0};
        observedStats.struct_size = sizeof(observedStats);
        result = state->api->get_runtime_stats(state->context, &observedStats);
        if (result == UEC_RESULT_OK &&
            (state->baseline_pending_requests == UINT32_MAX ||
             observedStats.pending_requests != state->baseline_pending_requests + 1u)) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result != UEC_RESULT_OK) FinishLatentSmoke(state, result, UEC_TRUE);
    return result;
}

uec_bool UEC_CALL uec_host_latent_smoke_poll(uec_result* outResult)
{
    if (outResult == NULL) return UEC_FALSE;
    uec_latent_smoke_state* state = &g_latent_smoke_state;
    if (state->complete != UEC_TRUE && state->callback_received == UEC_TRUE) {
        if (state->cancelled_callback_count != 0) state->result = UEC_RESULT_INTERNAL_ERROR;
        if (state->api != NULL && state->context != NULL &&
            state->api->get_runtime_stats != NULL) {
            uec_runtime_stats observedStats = {0};
            observedStats.struct_size = sizeof(observedStats);
            const uec_result statsResult = state->api->get_runtime_stats(
                state->context, &observedStats);
            if (statsResult != UEC_RESULT_OK ||
                observedStats.pending_requests != state->baseline_pending_requests) {
                state->result = UEC_RESULT_INTERNAL_ERROR;
            }
        }
        FinishLatentSmoke(state, state->result, UEC_FALSE);
    }
    *outResult = state->complete == UEC_TRUE ? state->result : UEC_RESULT_NOT_INITIALIZED;
    return state->complete;
}

void UEC_CALL uec_host_latent_smoke_cancel(void)
{
    FinishLatentSmoke(&g_latent_smoke_state, UEC_RESULT_OK, UEC_TRUE);
}
