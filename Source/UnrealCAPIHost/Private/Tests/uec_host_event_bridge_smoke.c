#include "uec_api.h"

#include <string.h>

typedef struct uec_event_bridge_smoke_state {
    uint64_t subscription_id;
    uint64_t actor_destroy_subscription_id;
    uint32_t callback_count;
    uint32_t actor_destroy_callback_count;
    uec_bool payload_valid;
    uec_bool callback_stats_valid;
    uec_bool actor_destroy_callback_valid;
    const uec_api* api;
    uec_context* context;
    uec_result self_unbind_result;
} uec_event_bridge_smoke_state;

static uec_bool EventCallbackCountIsAccurate(const uec_event_bridge_smoke_state* state)
{
    uec_runtime_stats stats = {0};
    if (state == NULL || state->api == NULL || state->context == NULL ||
        state->api->get_runtime_stats == NULL) return UEC_FALSE;
    stats.struct_size = sizeof(stats);
    if (state->api->get_runtime_stats(state->context, &stats) != UEC_RESULT_OK) {
        return UEC_FALSE;
    }
    return stats.active_callbacks == 1u ? UEC_TRUE : UEC_FALSE;
}

static void UEC_CALL VerifyActorDestroyedCallback(uint64_t subscriptionId, void* userData)
{
    uec_event_bridge_smoke_state* state = (uec_event_bridge_smoke_state*)userData;
    if (state == NULL) return;
    ++state->actor_destroy_callback_count;
    state->actor_destroy_callback_valid =
        subscriptionId == state->actor_destroy_subscription_id ? UEC_TRUE : UEC_FALSE;
    if (EventCallbackCountIsAccurate(state) != UEC_TRUE) {
        state->callback_stats_valid = UEC_FALSE;
    }
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
    if (EventCallbackCountIsAccurate(state) != UEC_TRUE) {
        state->callback_stats_valid = UEC_FALSE;
    }
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
    if (EventCallbackCountIsAccurate(state) != UEC_TRUE) {
        state->callback_stats_valid = UEC_FALSE;
    }
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
    const uec_string_view destroyFunction = {"K2_DestroyActor", sizeof("K2_DestroyActor") - 1};
    const uec_string_view classPath = {actorClassPath, sizeof(actorClassPath) - 1};
    const uec_string_view text = {eventText, sizeof(eventText) - 1};
    const uec_transform initialTransform = {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* world = NULL;
    uec_world* probeWorld = NULL;
    uec_actor* actor = NULL;
    uec_actor* observedDestroyedActor = NULL;
    uec_object* bridge = NULL;
    uint64_t subscriptionId = 0;
    uint64_t actorDestroySubscriptionId = 0;
    uec_runtime_stats baselineStats = {sizeof(uec_runtime_stats), 0u, 0u, 0u, 0u,
                                       0u, 0u, 0u, 0u, 0u};
    uec_runtime_stats observedStats = {sizeof(uec_runtime_stats), 0u, 0u, 0u, 0u,
                                       0u, 0u, 0u, 0u, 0u};
    uec_event_bridge_smoke_state state = {0};
    state.callback_stats_valid = UEC_TRUE;
    state.self_unbind_result = UEC_RESULT_INTERNAL_ERROR;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    state.api = api;
    state.context = context;
    if (api == NULL || context == NULL || api->get_or_create_actor_event_bridge == NULL ||
        api->destroy_actor_event_bridge == NULL || api->bind_actor_event_bridge == NULL ||
        api->unbind_actor_event_bridge == NULL || api->emit_actor_event_bridge == NULL ||
        api->get_runtime_stats == NULL || api->get_capabilities == NULL ||
        api->get_default_world == NULL || api->release_world == NULL ||
        api->spawn_actor == NULL || api->destroy_actor == NULL || api->release_actor == NULL ||
        api->invoke_actor_function == NULL || api->bind_actor_destroyed == NULL ||
        api->unbind_actor_destroyed == NULL ||
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
    if (baselineStats.live_worlds == UINT32_MAX ||
        baselineStats.live_actors == UINT32_MAX || baselineStats.live_objects == UINT32_MAX) {
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
    result = api->get_runtime_stats(context, &observedStats);
    if (result != UEC_RESULT_OK || observedStats.live_actors != baselineStats.live_actors + 1u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
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
    result = api->get_runtime_stats(context, &observedStats);
    if (result != UEC_RESULT_OK || observedStats.live_objects != baselineStats.live_objects + 1u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
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
    result = api->invoke_actor_function(actor, destroyFunction);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->release_actor(actor);
    if (result != UEC_RESULT_OK) goto cleanup;
    actor = NULL;
    if (api->destroy_actor(destroyedActor) != UEC_RESULT_INVALID_HANDLE) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->spawn_actor(world, classPath, &initialTransform, &observedDestroyedActor);
    if (result != UEC_RESULT_OK || observedDestroyedActor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->bind_actor_destroyed(observedDestroyedActor, &VerifyActorDestroyedCallback,
                                       &state, &actorDestroySubscriptionId);
    if (result != UEC_RESULT_OK) goto cleanup;
    state.actor_destroy_subscription_id = actorDestroySubscriptionId;
    result = api->invoke_actor_function(observedDestroyedActor, destroyFunction);
    if (result != UEC_RESULT_OK) goto cleanup;
    if (state.actor_destroy_callback_count != 1u ||
        state.actor_destroy_callback_valid != UEC_TRUE) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    observedDestroyedActor = NULL;
    actorDestroySubscriptionId = 0;
    result = api->get_runtime_stats(context, &observedStats);
    if (result != UEC_RESULT_OK ||
        observedStats.active_subscriptions != baselineStats.active_subscriptions ||
        observedStats.active_callbacks != baselineStats.active_callbacks ||
        observedStats.live_actors != baselineStats.live_actors ||
        observedStats.live_objects != baselineStats.live_objects ||
        state.callback_stats_valid != UEC_TRUE ||
        state.callback_count != 2 || state.actor_destroy_callback_count != 1u ||
        state.actor_destroy_callback_valid != UEC_TRUE) {
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
    if (api != NULL && actorDestroySubscriptionId != 0 && context != NULL) {
        const uec_result cleanupResult = api->unbind_actor_destroyed(
            context, actorDestroySubscriptionId);
        if (result == UEC_RESULT_OK) result = cleanupResult;
    }
    if (api != NULL && observedDestroyedActor != NULL) {
        const uec_result cleanupResult = api->destroy_actor(observedDestroyedActor);
        if (cleanupResult != UEC_RESULT_OK) (void)api->release_actor(observedDestroyedActor);
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
