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

typedef struct uec_latent_smoke_state {
    const uec_api* api;
    uec_context* context;
    uec_world* world;
    uec_actor* actor;
    uint64_t request_id;
    uint64_t cancelled_request_id;
    uint32_t cancelled_callback_count;
    uint32_t baseline_pending_requests;
    uec_result result;
    uec_bool callback_received;
    uec_bool started;
    uec_bool complete;
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
        api->get_runtime_stats == NULL || api->get_capabilities == NULL) {
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

uec_result UEC_CALL uec_host_latent_smoke_start(void)
{
    static const char actorClassPath[] =
        "/Script/UnrealCAPIHost.UECAPIHostLatentSmokeActor";
    static const char functionName[] = "WaitForSmokeDuration";
    static const char nonLatentFunctionName[] = "NoOpSmokeCall";
    static const char scalarFunctionName[] = "ScalarSmokeCall";
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
        FinishLatentSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
        return UEC_RESULT_INTERNAL_ERROR;
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
    if (result != UEC_RESULT_UNSUPPORTED || rejectedRequestId != 0) {
        FinishLatentSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    uec_string_view worldContextName = {
        worldContextFunctionName, sizeof(worldContextFunctionName) - 1};
    uint32_t noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, worldContextName, &latentArguments[0], 1u,
        NULL, 0u, &noOutputs);
    if (result != UEC_RESULT_OK || noOutputs != 0) {
        FinishLatentSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    uec_function_argument malformedScalarArgument = duration;
    malformedScalarArgument.world_value = state->world;
    uec_string_view scalarName = {
        scalarFunctionName, sizeof(scalarFunctionName) - 1};
    noOutputs = UINT32_MAX;
    result = state->api->invoke_actor_function_arguments(
        state->actor, scalarName, &malformedScalarArgument, 1u,
        NULL, 0u, &noOutputs);
    if (result != UEC_RESULT_INVALID_ARGUMENT || noOutputs != 0) {
        FinishLatentSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
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
    if (result != UEC_RESULT_UNSUPPORTED) {
        FinishLatentSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    size_t requiredSize = 99u;
    uec_property_kind returnKind = UEC_PROPERTY_INTEGER;
    result = state->api->invoke_actor_function_text(
        state->actor, latentName, NULL, 0u, NULL, 0u,
        &requiredSize, &returnKind);
    if (result != UEC_RESULT_UNSUPPORTED || requiredSize != 0u ||
        returnKind != UEC_PROPERTY_UNKNOWN) {
        FinishLatentSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    uec_string_view missingName = {
        missingFunctionName, sizeof(missingFunctionName) - 1};
    rejectedRequestId = UINT64_MAX;
    result = state->api->invoke_actor_function_latent(
        state->actor, missingName, NULL, 0u, &CompleteLatentSmoke,
        state, &rejectedRequestId);
    if (result != UEC_RESULT_INVALID_ARGUMENT || rejectedRequestId != 0) {
        FinishLatentSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    rejectedRequestId = UINT64_MAX;
    result = state->api->invoke_actor_function_latent(
        state->actor, latentName, NULL, 0u, &CompleteLatentSmoke,
        state, &rejectedRequestId);
    if (result != UEC_RESULT_UNSUPPORTED || rejectedRequestId != 0) {
        FinishLatentSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    uec_function_argument missingWorldContext[2] = {
        latentArguments[0], latentArguments[1]};
    missingWorldContext[0].world_value = NULL;
    rejectedRequestId = UINT64_MAX;
    result = state->api->invoke_actor_function_latent(
        state->actor, latentName, missingWorldContext, 2u,
        &CompleteLatentSmoke, state, &rejectedRequestId);
    if (result != UEC_RESULT_UNSUPPORTED || rejectedRequestId != 0) {
        FinishLatentSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
        return UEC_RESULT_INTERNAL_ERROR;
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
        FinishLatentSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
        return UEC_RESULT_INTERNAL_ERROR;
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
