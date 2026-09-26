#include "c_playable.h"

#include "c_widget_ui.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define PLAYABLE_PAYLOAD_VERSION 1u
#define PLAYABLE_STATUS_CAPACITY 128u
#define PLAYABLE_PROGRESS_DISTANCE 1000.0
#define PLAYABLE_CAMERA_FOV_BOOST 8.0
#define PLAYABLE_CAMERA_MAX_FOV 120.0
#define PLAYABLE_TRACE_DISTANCE 250.0
#define PLAYABLE_MAX_COORDINATE 1000000000.0

static uec_string_view PlayableText(const char* text)
{
    uec_string_view value = {text, 0u};
    if (text != NULL) while (text[value.size] != '\0') ++value.size;
    return value;
}

static uec_bool PlayableTextValid(uec_string_view text)
{
    return text.data != NULL && text.size != 0u ? UEC_TRUE : UEC_FALSE;
}

static void PlayableRemember(uec_playable_sample_state* state, uec_result result)
{
    if (state != NULL && result != UEC_RESULT_OK && state->last_result == UEC_RESULT_OK)
        state->last_result = result;
}

static uec_result PlayableValidateApi(const uec_api* api)
{
    const size_t required_size = offsetof(uec_api, get_widget_child) +
                                sizeof(api->get_widget_child);
    if (api == NULL || api->struct_size < required_size) return UEC_RESULT_UNSUPPORTED;
    if (api->get_capabilities == NULL || api->get_default_world == NULL ||
        api->release_world == NULL || api->get_actor_transform == NULL ||
        api->set_actor_transform == NULL || api->add_input_mapping_context == NULL ||
        api->remove_input_mapping_context == NULL || api->bind_input_action == NULL ||
        api->unbind_input_action == NULL || api->get_camera_field_of_view == NULL ||
        api->set_camera_field_of_view == NULL || api->add_widget_to_viewport == NULL ||
        api->remove_widget_from_parent == NULL || api->unbind_button_clicked == NULL ||
        api->bind_button_clicked == NULL || api->get_widget_child == NULL ||
        api->set_text_block_text == NULL || api->set_progress_bar_percent == NULL ||
        api->trace_detailed_filtered == NULL || api->save_versioned_application_data == NULL ||
        api->load_versioned_application_data == NULL || api->release_actor == NULL ||
        api->release_scene_component == NULL || api->release_object == NULL)
        return UEC_RESULT_UNSUPPORTED;
    return UEC_RESULT_OK;
}

static uec_result PlayableSetStatus(uec_playable_sample_state* state,
                                    const char* message)
{
    static const uec_string_view status_child = {"StatusText", 10u};
    if (state == NULL || state->api == NULL || state->widget == NULL || message == NULL)
        return UEC_RESULT_INVALID_ARGUMENT;
    return uec_widget_set_text_child(state->api, state->widget, status_child,
                                     PlayableText(message));
}

static uec_result PlayableSetProgress(uec_playable_sample_state* state,
                                      double percent)
{
    static const uec_string_view progress_child = {"DistanceProgress", 16u};
    if (state == NULL || state->api == NULL || state->widget == NULL)
        return UEC_RESULT_INVALID_ARGUMENT;
    return uec_widget_set_progress_bar_child(state->api, state->widget,
                                              progress_child, percent);
}

static void PlayableReleaseHit(uec_playable_sample_state* state,
                               uec_hit_result_details* hit)
{
    if (state == NULL || state->api == NULL || hit == NULL) return;
    if (hit->hit.actor != NULL) {
        PlayableRemember(state, state->api->release_actor(hit->hit.actor));
        hit->hit.actor = NULL;
    }
    if (hit->component != NULL) {
        PlayableRemember(state, state->api->release_scene_component(hit->component));
        hit->component = NULL;
    }
}

static uec_result PlayableTraceAhead(uec_playable_sample_state* state,
                                     const uec_transform* transform,
                                     uec_bool* out_blocked)
{
    if (out_blocked != NULL) *out_blocked = UEC_FALSE;
    if (state == NULL || transform == NULL || out_blocked == NULL)
        return UEC_RESULT_INVALID_ARGUMENT;

    const double qx = transform->rotation.x;
    const double qy = transform->rotation.y;
    const double qz = transform->rotation.z;
    const double qw = transform->rotation.w;
    const uec_vector3 forward = {
        1.0 - 2.0 * (qy * qy + qz * qz),
        2.0 * (qx * qy + qw * qz),
        2.0 * (qx * qz - qw * qy)};
    const uec_vector3 start = {
        transform->translation.x, transform->translation.y,
        transform->translation.z + 50.0};
    const uec_vector3 end = {
        start.x + forward.x * PLAYABLE_TRACE_DISTANCE,
        start.y + forward.y * PLAYABLE_TRACE_DISTANCE,
        start.z + forward.z * PLAYABLE_TRACE_DISTANCE};
    uec_hit_result_details hit = {0};
    hit.struct_size = sizeof(hit);
    const uec_actor* ignored_actors[] = {state->pawn};
    const uec_result result = state->api->trace_detailed_filtered(
        state->world, start, end, NULL, UEC_TRACE_VISIBILITY, UEC_FALSE,
        ignored_actors, 1u, &hit);
    if (result != UEC_RESULT_OK) {
        PlayableReleaseHit(state, &hit);
        return result;
    }
    *out_blocked = hit.hit.blocking_hit;
    if (*out_blocked == UEC_TRUE && state->blocking_trace_count != UINT64_MAX)
        ++state->blocking_trace_count;
    PlayableReleaseHit(state, &hit);
    return state->last_result;
}

static double PlayableClamp(double value, double low, double high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static uec_result PlayableRefreshFeedback(uec_playable_sample_state* state,
                                          const char* prefix)
{
    if (state == NULL || state->api == NULL || state->pawn == NULL)
        return UEC_RESULT_INVALID_ARGUMENT;
    uec_transform transform = {0};
    uec_result result = state->api->get_actor_transform(state->pawn, &transform);
    if (result != UEC_RESULT_OK) return result;

    uec_bool blocked = UEC_FALSE;
    result = PlayableTraceAhead(state, &transform, &blocked);
    if (result != UEC_RESULT_OK) return result;

    const double dx = transform.translation.x - state->starting_transform.translation.x;
    const double dy = transform.translation.y - state->starting_transform.translation.y;
    const double distance = sqrt(dx * dx + dy * dy);
    const double progress = PlayableClamp(distance / PLAYABLE_PROGRESS_DISTANCE, 0.0, 1.0);
    result = PlayableSetProgress(state, progress);
    if (result != UEC_RESULT_OK) return result;

    char status[PLAYABLE_STATUS_CAPACITY] = {0};
    const int written = snprintf(status, sizeof(status), "%s %.0f, %.0f | %s",
        prefix == NULL ? "Ready" : prefix, transform.translation.x,
        transform.translation.y, blocked == UEC_TRUE ? "blocked" : "clear");
    if (written < 0 || (size_t)written >= sizeof(status)) return UEC_RESULT_INTERNAL_ERROR;
    return PlayableSetStatus(state, status);
}

static uec_result PlayableApplyCameraResponse(uec_playable_sample_state* state,
                                             double axis_x,
                                             double axis_y)
{
    const double magnitude = sqrt(axis_x * axis_x + axis_y * axis_y);
    const double target_fov = PlayableClamp(
        state->default_camera_fov + magnitude * PLAYABLE_CAMERA_FOV_BOOST,
        state->default_camera_fov, PLAYABLE_CAMERA_MAX_FOV);
    return state->api->set_camera_field_of_view(state->camera, target_fov);
}

static uec_result PlayableMove(uec_playable_sample_state* state,
                               double axis_x,
                               double axis_y)
{
    uec_transform transform = {0};
    uec_result result = state->api->get_actor_transform(state->pawn, &transform);
    if (result != UEC_RESULT_OK) return result;

    const double qx = transform.rotation.x;
    const double qy = transform.rotation.y;
    const double qz = transform.rotation.z;
    const double qw = transform.rotation.w;
    const double world_x = (1.0 - 2.0 * (qy * qy + qz * qz)) * axis_x +
                           2.0 * (qx * qy - qw * qz) * axis_y;
    const double world_y = 2.0 * (qx * qy + qw * qz) * axis_x +
                           (1.0 - 2.0 * (qx * qx + qz * qz)) * axis_y;
    transform.translation.x += world_x * state->movement_units_per_trigger_event;
    transform.translation.y += world_y * state->movement_units_per_trigger_event;
    result = state->api->set_actor_transform(state->pawn, &transform, UEC_TRUE);
    if (result != UEC_RESULT_OK) return result;
    if (state->movement_step_count != UINT64_MAX) ++state->movement_step_count;

    result = PlayableApplyCameraResponse(state, axis_x, axis_y);
    if (result != UEC_RESULT_OK) return result;
    return PlayableRefreshFeedback(state, "Move");
}

static void UEC_CALL PlayableInput(uint64_t binding_id,
                                   uec_input_action_value value,
                                   void* user_data)
{
    uec_playable_sample_state* state = (uec_playable_sample_state*)user_data;
    if (state == NULL || state->done == UEC_TRUE) return;
    if (value.struct_size < sizeof(value) || value.kind != UEC_INPUT_ACTION_VALUE_AXIS_2D) {
        PlayableRemember(state, UEC_RESULT_INVALID_ARGUMENT);
        return;
    }
    if (binding_id == state->triggered_binding_id) {
        if (!isfinite(value.axis.x) || !isfinite(value.axis.y) ||
            value.axis.x < -1.0 || value.axis.x > 1.0 ||
            value.axis.y < -1.0 || value.axis.y > 1.0) {
            PlayableRemember(state, UEC_RESULT_INVALID_ARGUMENT);
            return;
        }
        state->axis_x = value.axis.x;
        state->axis_y = value.axis.y;
        PlayableRemember(state, PlayableMove(state, state->axis_x, state->axis_y));
    } else if (binding_id == state->completed_binding_id ||
               binding_id == state->canceled_binding_id) {
        state->axis_x = 0.0;
        state->axis_y = 0.0;
        PlayableRemember(state, state->api->set_camera_field_of_view(
            state->camera, state->default_camera_fov));
        PlayableRemember(state, PlayableRefreshFeedback(state, "Ready"));
    } else {
        PlayableRemember(state, UEC_RESULT_INTERNAL_ERROR);
    }
    if (state->input_event_count != UINT64_MAX) ++state->input_event_count;
}

static void UEC_CALL PlayableSaveClicked(uint64_t subscription_id, void* user_data)
{
    uec_playable_sample_state* state = (uec_playable_sample_state*)user_data;
    if (state == NULL || state->done == UEC_TRUE ||
        subscription_id != state->save_button_subscription_id) return;
    state->save_button_subscription_id = 0u;

    uec_transform transform = {0};
    uec_result result = state->api->get_actor_transform(state->pawn, &transform);
    const double position[3] = {transform.translation.x,
                                 transform.translation.y,
                                 transform.translation.z};
    uec_bool saved = UEC_FALSE;
    if (result == UEC_RESULT_OK) {
        result = state->api->save_versioned_application_data(
            state->context, state->save_slot, 0, PLAYABLE_PAYLOAD_VERSION,
            (const uint8_t*)position, sizeof(position), &saved);
    }
    if (result == UEC_RESULT_OK && saved != UEC_TRUE) result = UEC_RESULT_INTERNAL_ERROR;
    PlayableRemember(state, result);
    if (result == UEC_RESULT_OK && state->save_count != UINT64_MAX) ++state->save_count;
    PlayableRemember(state, PlayableSetStatus(state,
        result == UEC_RESULT_OK ? "Saved current position" : "Save failed"));
}

static uec_bool PlayablePositionValid(const double position[3])
{
    for (size_t index = 0; index < 3u; ++index) {
        if (!isfinite(position[index]) || fabs(position[index]) > PLAYABLE_MAX_COORDINATE)
            return UEC_FALSE;
    }
    return UEC_TRUE;
}

static void UEC_CALL PlayableLoadClicked(uint64_t subscription_id, void* user_data)
{
    uec_playable_sample_state* state = (uec_playable_sample_state*)user_data;
    if (state == NULL || state->done == UEC_TRUE ||
        subscription_id != state->load_button_subscription_id) return;
    state->load_button_subscription_id = 0u;

    uint8_t payload[sizeof(double) * 3u] = {0u};
    size_t required_size = 0u;
    uint32_t version = 0u;
    uec_result result = state->api->load_versioned_application_data(
        state->context, state->save_slot, 0, &version, payload,
        sizeof(payload), &required_size);
    double position[3] = {0.0, 0.0, 0.0};
    if (result == UEC_RESULT_OK &&
        (version != PLAYABLE_PAYLOAD_VERSION || required_size != sizeof(position)))
        result = UEC_RESULT_UNSUPPORTED;
    if (result == UEC_RESULT_OK) {
        memcpy(position, payload, sizeof(position));
        if (PlayablePositionValid(position) != UEC_TRUE)
            result = UEC_RESULT_INVALID_ARGUMENT;
    }
    uec_transform transform = {0};
    if (result == UEC_RESULT_OK) result = state->api->get_actor_transform(state->pawn, &transform);
    if (result == UEC_RESULT_OK) {
        transform.translation.x = position[0];
        transform.translation.y = position[1];
        transform.translation.z = position[2];
        result = state->api->set_actor_transform(state->pawn, &transform, UEC_TRUE);
    }
    PlayableRemember(state, result);
    if (result == UEC_RESULT_OK && state->load_count != UINT64_MAX) ++state->load_count;
    if (result == UEC_RESULT_OK) result = PlayableRefreshFeedback(state, "Loaded");
    PlayableRemember(state, result);
    PlayableRemember(state, PlayableSetStatus(state,
        result == UEC_RESULT_OK ? "Loaded saved position" : "Load failed"));
}

static void PlayableUnbind(uec_playable_sample_state* state, uint64_t* subscription_id)
{
    if (state == NULL || subscription_id == NULL || *subscription_id == 0u) return;
    if (state->context != NULL && state->api != NULL && state->api->unbind_button_clicked != NULL)
        PlayableRemember(state, state->api->unbind_button_clicked(state->context, *subscription_id));
    *subscription_id = 0u;
}

static void PlayableFinish(uec_playable_sample_state* state)
{
    if (state == NULL || state->done == UEC_TRUE) return;
    if (state->api != NULL && state->context != NULL) {
        uint64_t* binding_ids[] = {&state->triggered_binding_id,
                                   &state->completed_binding_id,
                                   &state->canceled_binding_id};
        for (size_t index = 0u; index < sizeof(binding_ids) / sizeof(binding_ids[0]); ++index) {
            if (*binding_ids[index] == 0u) continue;
            PlayableRemember(state, state->api->unbind_input_action(
                state->context, *binding_ids[index]));
            *binding_ids[index] = 0u;
        }
    }
    PlayableUnbind(state, &state->save_button_subscription_id);
    PlayableUnbind(state, &state->load_button_subscription_id);
    if (state->mapping_installed == UEC_TRUE && state->api != NULL) {
        PlayableRemember(state, state->api->remove_input_mapping_context(
            state->controller, state->mapping_context));
        state->mapping_installed = UEC_FALSE;
    }
    if (state->widget_added == UEC_TRUE && state->api != NULL) {
        PlayableRemember(state, state->api->remove_widget_from_parent(state->widget));
        state->widget_added = UEC_FALSE;
    }
    if (state->camera != NULL && state->api != NULL && state->started == UEC_TRUE)
        PlayableRemember(state, state->api->set_camera_field_of_view(
            state->camera, state->default_camera_fov));
    if (state->world != NULL && state->api != NULL) {
        PlayableRemember(state, state->api->release_world(state->world));
        state->world = NULL;
    }
    state->axis_x = 0.0;
    state->axis_y = 0.0;
    state->done = UEC_TRUE;
}

static uec_result PlayableCheckCapabilities(uec_playable_sample_state* state)
{
    uec_capabilities capabilities = 0;
    uec_result result = state->api->get_capabilities(state->context, &capabilities);
    const uec_capabilities required = UEC_CAPABILITY_INPUT | UEC_CAPABILITY_CAMERA |
        UEC_CAPABILITY_COLLISION_DETAILS | UEC_CAPABILITY_UI | UEC_CAPABILITY_SAVE_DATA;
    if (result == UEC_RESULT_OK && (capabilities & required) != required)
        result = UEC_RESULT_UNSUPPORTED;
    return result;
}

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
    uec_playable_sample_state* state)
{
    if (state == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *state = (uec_playable_sample_state){0};
    state->last_result = UEC_RESULT_OK;
    if (api == NULL || context == NULL || controller == NULL || pawn == NULL ||
        mapping_context == NULL || axis2d_move_action == NULL || camera == NULL ||
        widget == NULL || PlayableTextValid(save_slot) != UEC_TRUE ||
        !isfinite(movement_units_per_trigger_event) ||
        movement_units_per_trigger_event <= 0.0 || movement_units_per_trigger_event > 1000000.0) {
        state->last_result = UEC_RESULT_INVALID_ARGUMENT;
        state->done = UEC_TRUE;
        return state->last_result;
    }
    uec_result result = PlayableValidateApi(api);
    if (result != UEC_RESULT_OK) {
        state->last_result = result;
        state->done = UEC_TRUE;
        return result;
    }

    state->api = api;
    state->context = context;
    state->controller = controller;
    state->pawn = pawn;
    state->mapping_context = mapping_context;
    state->move_action = axis2d_move_action;
    state->camera = camera;
    state->widget = widget;
    state->save_slot = save_slot;
    state->movement_units_per_trigger_event = movement_units_per_trigger_event;
    result = PlayableCheckCapabilities(state);
    if (result == UEC_RESULT_OK) result = api->get_default_world(context, &state->world);
    if (result == UEC_RESULT_OK && state->world == NULL) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) result = api->get_actor_transform(pawn, &state->starting_transform);
    if (result == UEC_RESULT_OK) result = api->get_camera_field_of_view(camera,
                                                                         &state->default_camera_fov);
    if (result == UEC_RESULT_OK && (!isfinite(state->default_camera_fov) ||
                                    state->default_camera_fov <= 0.0 ||
                                    state->default_camera_fov > PLAYABLE_CAMERA_MAX_FOV))
        result = UEC_RESULT_INVALID_ARGUMENT;
    if (result == UEC_RESULT_OK) {
        result = api->add_input_mapping_context(controller, mapping_context, mapping_priority);
        if (result == UEC_RESULT_OK) state->mapping_installed = UEC_TRUE;
    }
    if (result == UEC_RESULT_OK) {
        result = api->bind_input_action(pawn, axis2d_move_action, UEC_INPUT_TRIGGER_TRIGGERED,
            &PlayableInput, state, &state->triggered_binding_id);
    }
    if (result == UEC_RESULT_OK) {
        result = api->bind_input_action(pawn, axis2d_move_action, UEC_INPUT_TRIGGER_COMPLETED,
            &PlayableInput, state, &state->completed_binding_id);
    }
    if (result == UEC_RESULT_OK) {
        result = api->bind_input_action(pawn, axis2d_move_action, UEC_INPUT_TRIGGER_CANCELED,
            &PlayableInput, state, &state->canceled_binding_id);
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_bind_button_clicked_child(api, widget,
            PlayableText("SaveButton"), &PlayableSaveClicked, state,
            &state->save_button_subscription_id);
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_bind_button_clicked_child(api, widget,
            PlayableText("LoadButton"), &PlayableLoadClicked, state,
            &state->load_button_subscription_id);
    }
    if (result == UEC_RESULT_OK) {
        result = api->add_widget_to_viewport(widget, 0);
        if (result == UEC_RESULT_OK) state->widget_added = UEC_TRUE;
    }
    if (result == UEC_RESULT_OK) {
        state->started = UEC_TRUE;
        result = PlayableRefreshFeedback(state, "Ready");
    }
    if (result != UEC_RESULT_OK) {
        state->last_result = result;
        PlayableFinish(state);
        return result;
    }
    return UEC_RESULT_OK;
}

void UEC_CALL uec_playable_sample_cancel(uec_playable_sample_state* state)
{
    PlayableFinish(state);
}
