#include "uec_api.h"

#include <stddef.h>

typedef struct uec_event_bridge_smoke_state {
    uint64_t subscription_id;
    uint32_t callback_count;
    uec_bool payload_valid;
    const uec_api* api;
    uec_context* context;
    uec_result self_unbind_result;
} uec_event_bridge_smoke_state;

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

uec_result UEC_CALL uec_host_smoke_bootstrap(void)
{
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL ||
        api->struct_size < offsetof(uec_api, release_context) + sizeof(api->release_context)) {
        return UEC_RESULT_INTERNAL_ERROR;
    }
    if (api->release_context == NULL) return UEC_RESULT_INTERNAL_ERROR;
    if (api->abi_major != UEC_ABI_MAJOR || api->abi_minor < UEC_ABI_MINOR ||
        api->get_capabilities == NULL || api->log == NULL) {
        api->release_context(context);
        return UEC_RESULT_INTERNAL_ERROR;
    }

    uec_capabilities capabilities = 0;
    result = api->get_capabilities(context, &capabilities);
    if (result == UEC_RESULT_OK && (capabilities & UEC_CAPABILITY_BOOTSTRAP) == 0) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK) {
        const char message[] = "UnrealCAPI C host bootstrap reached the bridge";
        const uec_string_view view = {message, sizeof(message) - 1};
        result = api->log(context, view);
    }

    const uec_result release_result = api->release_context(context);
    return result == UEC_RESULT_OK ? release_result : result;
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
        api->get_runtime_stats == NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_runtime_stats(context, &baselineStats);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_default_world(context, &world);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->spawn_actor(world, classPath, &initialTransform, &actor);
    if (result != UEC_RESULT_OK) goto cleanup;
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
