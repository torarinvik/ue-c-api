#include "c_gameplay.h"

typedef struct uec_host_gameplay_smoke_state {
    const uec_api* api;
    uec_context* context;
    uec_gameplay_example_state example;
    uec_runtime_stats baseline;
    uec_result result;
    uec_bool baseline_valid;
    uec_bool example_started;
    uec_bool started;
    uec_bool complete;
} uec_host_gameplay_smoke_state;

static uec_host_gameplay_smoke_state g_gameplay_smoke_state;

static uec_bool GameplayStatsMatch(const uec_runtime_stats* expected,
                                   const uec_runtime_stats* observed,
                                   uec_bool example_active)
{
    const uint32_t expectedSubscriptions = expected->active_subscriptions +
        (example_active == UEC_TRUE ? 2u : 0u);
    const uint32_t expectedWorlds = expected->live_worlds +
        (example_active == UEC_TRUE ? 1u : 0u);
    const uint32_t expectedActors = expected->live_actors +
        (example_active == UEC_TRUE ? 1u : 0u);
    const uint32_t expectedObjects = expected->live_objects +
        (example_active == UEC_TRUE ? 1u : 0u);
    return observed->active_subscriptions == expectedSubscriptions &&
        observed->pending_requests == expected->pending_requests &&
        observed->active_callbacks == expected->active_callbacks &&
        observed->live_contexts == expected->live_contexts &&
        observed->live_worlds == expectedWorlds &&
        observed->live_actors == expectedActors &&
        observed->live_components == expected->live_components &&
        observed->live_classes == expected->live_classes &&
        observed->live_objects == expectedObjects ? UEC_TRUE : UEC_FALSE;
}

static void FinishGameplaySmoke(uec_host_gameplay_smoke_state* state,
                                uec_result result,
                                uec_bool cancel_example)
{
    if (state == NULL || state->complete == UEC_TRUE) return;
    if (cancel_example == UEC_TRUE && state->example_started == UEC_TRUE &&
        state->example.done != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        state->example.last_result = result;
        uec_gameplay_example_cancel(&state->example);
    }
    if (state->baseline_valid == UEC_TRUE && state->api != NULL &&
        state->context != NULL && state->api->get_runtime_stats != NULL) {
        uec_runtime_stats observed = {0};
        observed.struct_size = sizeof(observed);
        const uec_result statsResult = state->api->get_runtime_stats(
            state->context, &observed);
        if (statsResult != UEC_RESULT_OK ||
            GameplayStatsMatch(&state->baseline, &observed, UEC_FALSE) != UEC_TRUE) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (state->context != NULL && state->api != NULL &&
        state->api->release_context != NULL) {
        const uec_result releaseResult = state->api->release_context(state->context);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    state->context = NULL;
    state->result = result;
    state->complete = UEC_TRUE;
}

uec_result UEC_CALL uec_host_gameplay_example_smoke_start(void)
{
    static const char actorClassPath[] = "/Script/Engine.DefaultPawn";
    uec_host_gameplay_smoke_state* state = &g_gameplay_smoke_state;
    if (state->started == UEC_TRUE) return UEC_RESULT_INVALID_ARGUMENT;
    *state = (uec_host_gameplay_smoke_state){0};
    state->started = UEC_TRUE;
    state->result = UEC_RESULT_INTERNAL_ERROR;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR,
                                    &state->api, &state->context);
    if (result != UEC_RESULT_OK) {
        FinishGameplaySmoke(state, result, UEC_FALSE);
        return result;
    }
    if (state->api == NULL || state->context == NULL ||
        state->api->get_runtime_stats == NULL ||
        state->api->release_context == NULL) {
        FinishGameplaySmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    state->baseline.struct_size = sizeof(state->baseline);
    result = state->api->get_runtime_stats(state->context, &state->baseline);
    if (result != UEC_RESULT_OK ||
        state->baseline.active_subscriptions > UINT32_MAX - 2u ||
        state->baseline.live_worlds == UINT32_MAX ||
        state->baseline.live_actors == UINT32_MAX ||
        state->baseline.live_objects == UINT32_MAX) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        FinishGameplaySmoke(state, result, UEC_FALSE);
        return result;
    }
    state->baseline_valid = UEC_TRUE;
    state->example_started = UEC_TRUE;
    const uec_string_view classPath = {actorClassPath, sizeof(actorClassPath) - 1u};
    const uec_transform initialTransform = {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    result = uec_gameplay_example_start(state->api, state->context, classPath,
                                        &initialTransform, &state->example);
    if (result == UEC_RESULT_OK) {
        uec_runtime_stats observed = {0};
        observed.struct_size = sizeof(observed);
        result = state->api->get_runtime_stats(state->context, &observed);
        if (result == UEC_RESULT_OK &&
            GameplayStatsMatch(&state->baseline, &observed, UEC_TRUE) != UEC_TRUE) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result != UEC_RESULT_OK) {
        FinishGameplaySmoke(state, result, UEC_TRUE);
        return result;
    }
    return UEC_RESULT_OK;
}

uec_bool UEC_CALL uec_host_gameplay_example_smoke_poll(uec_result* outResult)
{
    if (outResult == NULL) return UEC_FALSE;
    uec_host_gameplay_smoke_state* state = &g_gameplay_smoke_state;
    if (state->complete != UEC_TRUE && state->example.done == UEC_TRUE) {
        uec_result result = state->example.last_result;
        if (state->example.ticks != 3u || state->example.events_received != 3u) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        FinishGameplaySmoke(state, result, UEC_FALSE);
    }
    *outResult = state->complete == UEC_TRUE ? state->result : UEC_RESULT_NOT_INITIALIZED;
    return state->complete;
}

void UEC_CALL uec_host_gameplay_example_smoke_cancel(void)
{
    FinishGameplaySmoke(&g_gameplay_smoke_state, UEC_RESULT_INTERNAL_ERROR, UEC_TRUE);
}
