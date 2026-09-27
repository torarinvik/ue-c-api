#ifndef UEC_GAMEPLAY_EXAMPLE_H
#define UEC_GAMEPLAY_EXAMPLE_H

#include "uec_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The host owns this state and its context until done becomes UEC_TRUE. */
typedef struct uec_gameplay_example_state {
    const uec_api* api;
    uec_context* context;
    uec_world* world;
    uec_actor* actor;
    uec_object* event_bridge;
    uint64_t timer_id;
    uint64_t event_subscription_id;
    uint32_t ticks;
    uint32_t events_received;
    uec_result last_result;
    uec_bool done;
} uec_gameplay_example_state;

/* Start and cancel must run on the game thread. The caller retains context. */
uec_result UEC_CALL uec_gameplay_example_start(
    const uec_api* api,
    uec_context* context,
    uec_string_view actor_class_path,
    const uec_transform* initial_transform,
    uec_gameplay_example_state* state);
void UEC_CALL uec_gameplay_example_cancel(uec_gameplay_example_state* state);

/*
 * The caller supplies an Enhanced Input controller, an input-enabled actor,
 * a mapping context, and an Axis2D action configured in that context. Handles
 * and context are borrowed and must remain valid until cancel returns. Start,
 * cancel, and input callbacks run on the game thread. The
 * sample moves along the actor's local X/Y plane once per Triggered callback
 * and sweeps the transform so blocking collision can stop motion.
 */
typedef struct uec_gameplay_input_movement_state {
    const uec_api* api;
    uec_context* context;
    uec_actor* controller;
    uec_actor* actor;
    uec_object* mapping_context;
    uec_object* action;
    uint64_t triggered_binding_id;
    uint64_t completed_binding_id;
    uint64_t canceled_binding_id;
    double axis_x;
    double axis_y;
    double last_input_x;
    double last_input_y;
    double movement_units_per_trigger_event;
    uint64_t input_event_count;
    uint64_t movement_step_count;
    uec_result last_result;
    uec_bool mapping_installed;
    uec_bool started;
    uec_bool done;
} uec_gameplay_input_movement_state;

/*
 * The mapping context is installed by this sample and removed by cancel.
 * It owns no passed handle; the consumer keeps those handles and state alive
 * until done is UEC_TRUE. Movement distance is Unreal units per Triggered
 * event; Enhanced Input normally emits those events each active input tick.
 */
uec_result UEC_CALL uec_gameplay_input_movement_start(
    const uec_api* api,
    uec_context* context,
    uec_actor* controller,
    uec_actor* actor,
    uec_object* mapping_context,
    uec_object* axis2d_action,
    int32_t mapping_priority,
    double movement_units_per_trigger_event,
    uec_gameplay_input_movement_state* state);
void UEC_CALL uec_gameplay_input_movement_cancel(
    uec_gameplay_input_movement_state* state);

/* The returned weak handle is owned by the caller and must be released. */
uec_result UEC_CALL uec_gameplay_get_player_state(
    const uec_api* api,
    uec_actor* controller,
    uec_object** out_player_state);

/* Apply game-thread look deltas in degrees and preserve the current roll. */
uec_result UEC_CALL uec_gameplay_add_look_delta(
    const uec_api* api,
    uec_actor* controller,
    double yaw_delta_degrees,
    double pitch_delta_degrees);

#ifdef __cplusplus
}
#endif

#endif
