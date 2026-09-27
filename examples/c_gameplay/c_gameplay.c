#include "c_gameplay.h"

#include <stddef.h>
#include <stdint.h>

#define GAMEPLAY_INPUT_MAX_STEP 1000000000.0

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

static uec_result ValidateGameplayExampleApi(const uec_api* api)
{
    const size_t required_size = offsetof(uec_api, emit_actor_event_bridge) +
                                 sizeof(api->emit_actor_event_bridge);
    if (api->struct_size < required_size) return UEC_RESULT_UNSUPPORTED;
    if (api->get_capabilities == NULL || api->get_default_world == NULL ||
        api->release_world == NULL || api->spawn_actor == NULL ||
        api->destroy_actor == NULL || api->release_actor == NULL ||
        api->get_actor_transform == NULL || api->set_actor_transform == NULL ||
        api->set_timer == NULL || api->clear_timer == NULL ||
        api->release_object == NULL || api->get_or_create_actor_event_bridge == NULL ||
        api->destroy_actor_event_bridge == NULL || api->bind_actor_event_bridge == NULL ||
        api->unbind_actor_event_bridge == NULL || api->emit_actor_event_bridge == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }
    return UEC_RESULT_OK;
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
    uec_result result = ValidateGameplayExampleApi(api);
    if (result != UEC_RESULT_OK) {
        state->last_result = result;
        FinishGameplayExample(state);
        return result;
    }

    uec_capabilities capabilities = 0;
    result = api->get_capabilities(context, &capabilities);
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

uec_result UEC_CALL uec_gameplay_get_player_state(
    const uec_api* api,
    uec_actor* controller,
    uec_object** out_player_state)
{
    if (out_player_state != NULL) *out_player_state = NULL;
    if (api == NULL || out_player_state == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, get_controller_player_state) +
                               sizeof(api->get_controller_player_state) ||
        api->get_controller_player_state == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }
    return api->get_controller_player_state(controller, out_player_state);
}

uec_result UEC_CALL uec_gameplay_apply_pawn_movement_input(
    const uec_api* api,
    uec_actor* pawn,
    uec_vector3 world_direction,
    double scale,
    uec_bool force)
{
    if (api == NULL || pawn == NULL ||
        (force != UEC_FALSE && force != UEC_TRUE)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    const size_t required_size = offsetof(uec_api, add_pawn_movement_input) +
                                 sizeof(api->add_pawn_movement_input);
    if (api->struct_size < required_size || api->add_pawn_movement_input == NULL)
        return UEC_RESULT_UNSUPPORTED;
    return api->add_pawn_movement_input(pawn, world_direction, scale, force);
}

uec_result UEC_CALL uec_gameplay_set_character_jump_pressed(
    const uec_api* api,
    uec_actor* character,
    uec_bool pressed)
{
    if (api == NULL || character == NULL ||
        (pressed != UEC_FALSE && pressed != UEC_TRUE)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    const size_t required_size = offsetof(uec_api, stop_character_jumping) +
                                 sizeof(api->stop_character_jumping);
    if (api->struct_size < required_size || api->jump_character == NULL ||
        api->stop_character_jumping == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }
    return pressed == UEC_TRUE ? api->jump_character(character) :
                                 api->stop_character_jumping(character);
}

uec_result UEC_CALL uec_gameplay_add_look_delta(
    const uec_api* api,
    uec_actor* controller,
    double yaw_delta_degrees,
    double pitch_delta_degrees)
{
    if (api == NULL || controller == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    const size_t required_size = offsetof(uec_api, set_controller_control_rotation) +
                                 sizeof(api->set_controller_control_rotation);
    if (api->struct_size < required_size ||
        api->get_controller_control_rotation == NULL ||
        api->set_controller_control_rotation == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }

    uec_vector3 rotation = {0};
    uec_result result = api->get_controller_control_rotation(controller, &rotation);
    if (result != UEC_RESULT_OK) return result;
    rotation.y += yaw_delta_degrees;
    rotation.x += pitch_delta_degrees;
    return api->set_controller_control_rotation(controller, rotation);
}

static void RememberInputMovementResult(uec_gameplay_input_movement_state* state,
                                        uec_result result)
{
    if (state != NULL && result != UEC_RESULT_OK && state->last_result == UEC_RESULT_OK)
    {
        state->last_result = result;
    }
}

static uec_result ValidateInputMovementApi(const uec_api* api)
{
    const size_t required_size = offsetof(uec_api, unbind_input_action) +
                                 sizeof(api->unbind_input_action);
    if (api->struct_size < required_size) return UEC_RESULT_UNSUPPORTED;
    if (api->set_actor_transform == NULL || api->get_actor_transform == NULL ||
        api->add_input_mapping_context == NULL ||
        api->remove_input_mapping_context == NULL ||
        api->bind_input_action == NULL || api->unbind_input_action == NULL)
    {
        return UEC_RESULT_UNSUPPORTED;
    }
    return UEC_RESULT_OK;
}

static void FinishInputMovement(uec_gameplay_input_movement_state* state)
{
    if (state == NULL || state->done == UEC_TRUE) return;

    if (state->api != NULL && state->context != NULL)
    {
        uint64_t* binding_ids[] = {
            &state->triggered_binding_id,
            &state->completed_binding_id,
            &state->canceled_binding_id};
        for (size_t index = 0; index < sizeof(binding_ids) / sizeof(binding_ids[0]); ++index)
        {
            if (*binding_ids[index] == 0) continue;
            RememberInputMovementResult(state, state->api->unbind_input_action(
                state->context, *binding_ids[index]));
            *binding_ids[index] = 0;
        }
    }
    if (state->mapping_installed == UEC_TRUE && state->api != NULL &&
        state->controller != NULL && state->mapping_context != NULL)
    {
        RememberInputMovementResult(state, state->api->remove_input_mapping_context(
            state->controller, state->mapping_context));
        state->mapping_installed = UEC_FALSE;
    }

    state->axis_x = 0.0;
    state->axis_y = 0.0;
    state->done = UEC_TRUE;
}

static uec_result MoveInputActor(uec_gameplay_input_movement_state* state)
{
    uec_transform transform;
    uec_result result = state->api->get_actor_transform(state->actor, &transform);
    if (result != UEC_RESULT_OK) return result;

    /* Rotate local forward/right input by the actor's world-space quaternion. */
    const double qx = transform.rotation.x;
    const double qy = transform.rotation.y;
    const double qz = transform.rotation.z;
    const double qw = transform.rotation.w;
    const double local_x = state->axis_x;
    const double local_y = state->axis_y;
    const double world_x = (1.0 - 2.0 * (qy * qy + qz * qz)) * local_x +
                           2.0 * (qx * qy - qw * qz) * local_y;
    const double world_y = 2.0 * (qx * qy + qw * qz) * local_x +
                           (1.0 - 2.0 * (qx * qx + qz * qz)) * local_y;
    transform.translation.x += world_x * state->movement_units_per_trigger_event;
    transform.translation.y += world_y * state->movement_units_per_trigger_event;
    result = state->api->set_actor_transform(state->actor, &transform, UEC_TRUE);
    if (result == UEC_RESULT_OK && state->movement_step_count != UINT64_MAX)
    {
        ++state->movement_step_count;
    }
    return result;
}

static void UEC_CALL UpdateInputMovement(uint64_t binding_id,
                                         uec_input_action_value value,
                                         void* raw_state)
{
    uec_gameplay_input_movement_state* state =
        (uec_gameplay_input_movement_state*)raw_state;
    if (state == NULL || state->done == UEC_TRUE) return;
    if (value.struct_size < sizeof(uec_input_action_value) ||
        value.kind != UEC_INPUT_ACTION_VALUE_AXIS_2D)
    {
        RememberInputMovementResult(state, UEC_RESULT_INVALID_ARGUMENT);
        return;
    }

    if (binding_id == state->triggered_binding_id)
    {
        if (!(value.axis.x >= -1.0 && value.axis.x <= 1.0) ||
            !(value.axis.y >= -1.0 && value.axis.y <= 1.0))
        {
            RememberInputMovementResult(state, UEC_RESULT_INVALID_ARGUMENT);
            return;
        }
        state->axis_x = value.axis.x;
        state->axis_y = value.axis.y;
        state->last_input_x = value.axis.x;
        state->last_input_y = value.axis.y;
        /* Apply one game-thread movement step for this active input event. */
        const uec_result result = MoveInputActor(state);
        if (result != UEC_RESULT_OK)
        {
            RememberInputMovementResult(state, result);
            FinishInputMovement(state);
            return;
        }
    }
    else if (binding_id == state->completed_binding_id ||
             binding_id == state->canceled_binding_id)
    {
        state->axis_x = 0.0;
        state->axis_y = 0.0;
    }
    else
    {
        RememberInputMovementResult(state, UEC_RESULT_INTERNAL_ERROR);
        return;
    }
    if (state->input_event_count != UINT64_MAX) ++state->input_event_count;
}

uec_result UEC_CALL uec_gameplay_input_movement_start(
    const uec_api* api,
    uec_context* context,
    uec_actor* controller,
    uec_actor* actor,
    uec_object* mapping_context,
    uec_object* axis2d_action,
    int32_t mapping_priority,
    double movement_units_per_trigger_event,
    uec_gameplay_input_movement_state* state)
{
    if (state == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *state = (uec_gameplay_input_movement_state){0};
    state->last_result = UEC_RESULT_OK;
    if (api == NULL || context == NULL || controller == NULL ||
        actor == NULL || mapping_context == NULL || axis2d_action == NULL ||
        !(movement_units_per_trigger_event > 0.0) ||
        movement_units_per_trigger_event > GAMEPLAY_INPUT_MAX_STEP)
    {
        state->last_result = UEC_RESULT_INVALID_ARGUMENT;
        state->done = UEC_TRUE;
        return state->last_result;
    }

    uec_result result = ValidateInputMovementApi(api);
    if (result != UEC_RESULT_OK)
    {
        state->last_result = result;
        state->done = UEC_TRUE;
        return result;
    }
    state->api = api;
    state->context = context;
    state->controller = controller;
    state->actor = actor;
    state->mapping_context = mapping_context;
    state->action = axis2d_action;
    state->movement_units_per_trigger_event = movement_units_per_trigger_event;
    state->started = UEC_TRUE;

    result = api->add_input_mapping_context(controller, mapping_context, mapping_priority);
    if (result == UEC_RESULT_OK) state->mapping_installed = UEC_TRUE;
    if (result == UEC_RESULT_OK)
    {
        result = api->bind_input_action(actor, axis2d_action, UEC_INPUT_TRIGGER_TRIGGERED,
            &UpdateInputMovement, state, &state->triggered_binding_id);
    }
    if (result == UEC_RESULT_OK)
    {
        result = api->bind_input_action(actor, axis2d_action, UEC_INPUT_TRIGGER_COMPLETED,
            &UpdateInputMovement, state, &state->completed_binding_id);
    }
    if (result == UEC_RESULT_OK)
    {
        result = api->bind_input_action(actor, axis2d_action, UEC_INPUT_TRIGGER_CANCELED,
            &UpdateInputMovement, state, &state->canceled_binding_id);
    }
    if (result != UEC_RESULT_OK)
    {
        RememberInputMovementResult(state, result);
        FinishInputMovement(state);
        return state->last_result;
    }
    return UEC_RESULT_OK;
}

void UEC_CALL uec_gameplay_input_movement_cancel(
    uec_gameplay_input_movement_state* state)
{
    if (state == NULL || state->done == UEC_TRUE) return;
    FinishInputMovement(state);
}
