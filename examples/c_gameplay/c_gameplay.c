#include "c_gameplay.h"

#include <stddef.h>
#include <stdint.h>

static void RecordExampleResult(uec_gameplay_example_state* state,
                                uec_result result)
{
    if (state != NULL && result != UEC_RESULT_OK && state->last_result == UEC_RESULT_OK)
    {
        state->last_result = result;
    }
}

static uec_bool ExampleTextEquals(uec_string_view value, const char* expected)
{
    if (expected == NULL || value.data == NULL) return UEC_FALSE;
    size_t expected_size = 0;
    while (expected[expected_size] != '\0') ++expected_size;
    if (value.size != expected_size) return UEC_FALSE;
    for (size_t index = 0; index < expected_size; ++index)
    {
        if (value.data[index] != expected[index]) return UEC_FALSE;
    }
    return UEC_TRUE;
}

/* Run only on the game thread. This also supports an error during setup, when
 * some of the objects in the state may not have been created yet. */
static void FinishGameplayExample(uec_gameplay_example_state* state)
{
    if (state == NULL || state->done == UEC_TRUE) return;

    if (state->timer_id != 0 && state->api != NULL && state->world != NULL)
    {
        const uec_result result = state->api->clear_timer(state->world, state->timer_id);
        RecordExampleResult(state, result);
        state->timer_id = 0;
    }
    if (state->event_subscription_id != 0 && state->api != NULL && state->context != NULL)
    {
        const uec_result result = state->api->unbind_actor_event_bridge(
            state->context, state->event_subscription_id);
        RecordExampleResult(state, result);
        state->event_subscription_id = 0;
    }
    if (state->event_bridge != NULL && state->api != NULL)
    {
        const uec_result result = state->api->destroy_actor_event_bridge(state->event_bridge);
        RecordExampleResult(state, result);
        if (result != UEC_RESULT_OK)
        {
            (void)state->api->release_object(state->event_bridge);
        }
        state->event_bridge = NULL;
    }
    if (state->actor != NULL && state->api != NULL)
    {
        const uec_result result = state->api->destroy_actor(state->actor);
        RecordExampleResult(state, result);
        if (result != UEC_RESULT_OK)
        {
            (void)state->api->release_actor(state->actor);
        }
        state->actor = NULL;
    }
    if (state->world != NULL && state->api != NULL)
    {
        const uec_result result = state->api->release_world(state->world);
        RecordExampleResult(state, result);
        state->world = NULL;
    }

    state->context = NULL;
    state->done = UEC_TRUE;
}

static void UEC_CALL ReceiveGameplayEvent(uint64_t subscription_id,
                                          int64_t event_id,
                                          int64_t integer_value,
                                          double real_value,
                                          uec_string_view text_value,
                                          void* raw_state)
{
    uec_gameplay_example_state* state = (uec_gameplay_example_state*)raw_state;
    if (state == NULL || state->done == UEC_TRUE) return;

    if (subscription_id != state->event_subscription_id || event_id != 1 ||
        integer_value != (int64_t)state->ticks || real_value != (double)state->ticks ||
        ExampleTextEquals(text_value, "timer-tick") != UEC_TRUE)
    {
        RecordExampleResult(state, UEC_RESULT_INTERNAL_ERROR);
        return;
    }
    ++state->events_received;
}

static void UEC_CALL MoveActorOnTimer(uint64_t timer_id, void* raw_state)
{
    uec_gameplay_example_state* state = (uec_gameplay_example_state*)raw_state;
    if (state == NULL || state->done == UEC_TRUE || state->api == NULL ||
        state->actor == NULL || state->event_bridge == NULL)
    {
        return;
    }

    uec_transform transform;
    uec_result result = state->api->get_actor_transform(state->actor, &transform);
    if (result != UEC_RESULT_OK)
    {
        RecordExampleResult(state, result);
        FinishGameplayExample(state);
        return;
    }
    transform.translation.x += 10.0;
    result = state->api->set_actor_transform(state->actor, &transform, UEC_TRUE);
    if (result != UEC_RESULT_OK)
    {
        RecordExampleResult(state, result);
        FinishGameplayExample(state);
        return;
    }

    ++state->ticks;
    static const char event_text[] = "timer-tick";
    const uec_string_view event_view = {event_text, sizeof(event_text) - 1};
    result = state->api->emit_actor_event_bridge(
        state->event_bridge, 1, (int64_t)state->ticks, (double)state->ticks, event_view);
    if (result != UEC_RESULT_OK || state->events_received != state->ticks ||
        state->last_result != UEC_RESULT_OK)
    {
        RecordExampleResult(state, result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result);
        FinishGameplayExample(state);
        return;
    }

    if (state->ticks < 3u) return;
    const uec_result clear_result = state->api->clear_timer(state->world, timer_id);
    RecordExampleResult(state, clear_result);
    state->timer_id = 0;
    FinishGameplayExample(state);
}

/* Starts a small game-thread example. The host keeps both state and context
 * alive until state->done becomes true. Each timer tick moves the actor, emits
 * an event through its bridge component, receives the callback synchronously,
 * and terminates after three validated event deliveries. */
uec_result UEC_CALL uec_gameplay_example_start(const uec_api* api,
                                               uec_context* context,
                                               uec_string_view actor_class_path,
                                               const uec_transform* initial_transform,
                                               uec_gameplay_example_state* state)
{
    if (api == NULL || context == NULL || initial_transform == NULL || state == NULL)
    {
        return UEC_RESULT_INVALID_ARGUMENT;
    }

    *state = (uec_gameplay_example_state){0};
    state->api = api;
    state->context = context;
    state->last_result = UEC_RESULT_OK;

    uec_capabilities capabilities = 0;
    if (api->get_capabilities == NULL)
    {
        state->last_result = UEC_RESULT_UNSUPPORTED;
        FinishGameplayExample(state);
        return state->last_result;
    }
    uec_result result = api->get_capabilities(context, &capabilities);
    if (result != UEC_RESULT_OK || (capabilities & UEC_CAPABILITY_EVENT_BRIDGE) == 0)
    {
        state->last_result = result == UEC_RESULT_OK ? UEC_RESULT_UNSUPPORTED : result;
        FinishGameplayExample(state);
        return state->last_result;
    }

    result = api->get_default_world(context, &state->world);
    if (result != UEC_RESULT_OK)
    {
        state->last_result = result;
        FinishGameplayExample(state);
        return state->last_result;
    }
    result = api->spawn_actor(state->world, actor_class_path, initial_transform, &state->actor);
    if (result != UEC_RESULT_OK)
    {
        state->last_result = result;
        FinishGameplayExample(state);
        return state->last_result;
    }
    result = api->get_or_create_actor_event_bridge(state->actor, &state->event_bridge);
    if (result != UEC_RESULT_OK)
    {
        state->last_result = result;
        FinishGameplayExample(state);
        return state->last_result;
    }
    result = api->bind_actor_event_bridge(state->event_bridge, &ReceiveGameplayEvent,
                                          state, &state->event_subscription_id);
    if (result != UEC_RESULT_OK)
    {
        state->last_result = result;
        FinishGameplayExample(state);
        return state->last_result;
    }

    result = api->set_timer(state->world, 0.1, UEC_TRUE, &MoveActorOnTimer,
                            state, &state->timer_id);
    if (result != UEC_RESULT_OK)
    {
        state->last_result = result;
        FinishGameplayExample(state);
        return state->last_result;
    }
    return UEC_RESULT_OK;
}

void UEC_CALL uec_gameplay_example_cancel(uec_gameplay_example_state* state)
{
    if (state == NULL || state->done == UEC_TRUE) return;
    FinishGameplayExample(state);
}
