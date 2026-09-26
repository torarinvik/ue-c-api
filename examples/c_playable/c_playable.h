#ifndef UEC_C_PLAYABLE_H
#define UEC_C_PLAYABLE_H

#include "uec_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A small top-down playable consumer that composes Enhanced Input, swept
 * movement, collision feedback, camera response, UMG controls, and versioned
 * application save data. The caller supplies all Unreal objects and retains
 * them, the API context, state, and save-slot bytes until cancel completes.
 * Start/cancel and all callbacks must run on Unreal's game thread.
 */
typedef struct uec_playable_sample_state {
    const uec_api* api;
    uec_context* context;
    uec_world* world;
    uec_actor* controller;
    uec_actor* pawn;
    uec_object* mapping_context;
    uec_object* move_action;
    uec_scene_component* camera;
    uec_object* widget;
    uec_string_view save_slot;
    uec_transform starting_transform;
    double default_camera_fov;
    double movement_units_per_trigger_event;
    double axis_x;
    double axis_y;
    uint64_t triggered_binding_id;
    uint64_t completed_binding_id;
    uint64_t canceled_binding_id;
    uint64_t save_button_subscription_id;
    uint64_t load_button_subscription_id;
    uint64_t input_event_count;
    uint64_t movement_step_count;
    uint64_t blocking_trace_count;
    uint64_t save_count;
    uint64_t load_count;
    uec_result last_result;
    uec_bool mapping_installed;
    uec_bool widget_added;
    uec_bool started;
    uec_bool done;
} uec_playable_sample_state;

uec_result UEC_CALL uec_playable_sample_start(
    const uec_api* api,
    uec_context* context,
    uec_actor* controller,
    uec_actor* pawn,
    uec_object* mapping_context,
    uec_object* axis2d_move_action,
    uec_scene_component* camera,
    uec_object* widget,
    uec_string_view save_slot,
    double movement_units_per_trigger_event,
    int32_t mapping_priority,
    uec_playable_sample_state* state);

void UEC_CALL uec_playable_sample_cancel(uec_playable_sample_state* state);

#ifdef __cplusplus
}
#endif

#endif
