#include "c_playable.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct mock_input_binding {
    uint64_t id;
    uec_input_trigger_event event;
    uec_input_action_callback callback;
    void* user_data;
    uec_bool active;
} mock_input_binding;

typedef struct mock_button_binding {
    uint64_t id;
    uec_widget_event_callback callback;
    void* user_data;
    uec_bool active;
} mock_button_binding;

static unsigned char g_opaque_objects[12];
static uec_transform g_actor_transform;
static double g_camera_fov;
static double g_progress;
static char g_status_text[128];
static uec_bool g_mapping_active;
static uec_bool g_widget_active;
static uec_bool g_sweep_was_requested;
static uec_bool g_trace_blocked;
static uec_bool g_saved;
static uint8_t g_saved_payload[sizeof(double) * 3u];
static size_t g_saved_size;
static uint32_t g_saved_version;
static uint64_t g_next_id;
static uint32_t g_input_unbind_count;
static uint32_t g_button_unbind_count;
static uint32_t g_actor_release_count;
static uint32_t g_component_release_count;
static uint32_t g_world_release_count;
static uec_result g_trace_result;
static mock_input_binding g_input_bindings[4];
static mock_button_binding g_button_bindings[2];

static uec_actor* MockPawn(void) { return (uec_actor*)&g_opaque_objects[0]; }
static uec_object* MockWidget(void) { return (uec_object*)&g_opaque_objects[1]; }
static uec_object* MockStatusChild(void) { return (uec_object*)&g_opaque_objects[2]; }
static uec_object* MockProgressChild(void) { return (uec_object*)&g_opaque_objects[3]; }
static uec_object* MockSaveChild(void) { return (uec_object*)&g_opaque_objects[4]; }
static uec_object* MockLoadChild(void) { return (uec_object*)&g_opaque_objects[5]; }
static uec_object* MockHitActor(void) { return (uec_object*)&g_opaque_objects[6]; }
static uec_scene_component* MockHitComponent(void)
{
    return (uec_scene_component*)&g_opaque_objects[7];
}

static uec_bool TextIs(uec_string_view text, const char* expected)
{
    const size_t length = strlen(expected);
    return text.data != NULL && text.size == length &&
        memcmp(text.data, expected, length) == 0 ? UEC_TRUE : UEC_FALSE;
}

static uec_result UEC_CALL MockCapabilities(uec_context* context,
                                           uec_capabilities* out_capabilities)
{
    (void)context;
    if (out_capabilities == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *out_capabilities = UEC_CAPABILITY_INPUT | UEC_CAPABILITY_CAMERA |
        UEC_CAPABILITY_COLLISION_DETAILS | UEC_CAPABILITY_UI | UEC_CAPABILITY_SAVE_DATA;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockGetWorld(uec_context* context, uec_world** out_world)
{
    (void)context;
    if (out_world == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *out_world = (uec_world*)&g_opaque_objects[8];
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockReleaseWorld(uec_world* world)
{
    if (world != (uec_world*)&g_opaque_objects[8]) return UEC_RESULT_INVALID_HANDLE;
    ++g_world_release_count;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockGetTransform(uec_actor* actor, uec_transform* out_transform)
{
    if (actor != MockPawn() || out_transform == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *out_transform = g_actor_transform;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockSetTransform(uec_actor* actor,
                                            const uec_transform* transform,
                                            uec_bool sweep)
{
    if (actor != MockPawn() || transform == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    g_actor_transform = *transform;
    g_sweep_was_requested = sweep;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockAddMapping(uec_actor* controller,
                                          uec_object* mapping,
                                          int32_t priority)
{
    if (controller == NULL || mapping == NULL || priority != 17 || g_mapping_active == UEC_TRUE)
        return UEC_RESULT_INVALID_ARGUMENT;
    g_mapping_active = UEC_TRUE;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockRemoveMapping(uec_actor* controller, uec_object* mapping)
{
    if (controller == NULL || mapping == NULL || g_mapping_active != UEC_TRUE)
        return UEC_RESULT_INVALID_ARGUMENT;
    g_mapping_active = UEC_FALSE;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockBindInput(uec_actor* actor,
                                         uec_object* action,
                                         uec_input_trigger_event event,
                                         uec_input_action_callback callback,
                                         void* user_data,
                                         uint64_t* out_id)
{
    if (actor != MockPawn() || action == NULL || callback == NULL || out_id == NULL)
        return UEC_RESULT_INVALID_ARGUMENT;
    for (size_t index = 0; index < sizeof(g_input_bindings) / sizeof(g_input_bindings[0]); ++index) {
        if (g_input_bindings[index].active == UEC_TRUE) continue;
        mock_input_binding* binding = &g_input_bindings[index];
        binding->id = ++g_next_id;
        binding->event = event;
        binding->callback = callback;
        binding->user_data = user_data;
        binding->active = UEC_TRUE;
        *out_id = binding->id;
        return UEC_RESULT_OK;
    }
    return UEC_RESULT_INTERNAL_ERROR;
}

static uec_result UEC_CALL MockUnbindInput(uec_context* context, uint64_t id)
{
    (void)context;
    for (size_t index = 0; index < sizeof(g_input_bindings) / sizeof(g_input_bindings[0]); ++index) {
        if (g_input_bindings[index].id != id || g_input_bindings[index].active != UEC_TRUE) continue;
        g_input_bindings[index].active = UEC_FALSE;
        ++g_input_unbind_count;
        return UEC_RESULT_OK;
    }
    return UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL MockGetFov(uec_scene_component* camera, double* out_fov)
{
    if (camera == NULL || out_fov == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *out_fov = g_camera_fov;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockSetFov(uec_scene_component* camera, double fov)
{
    if (camera == NULL || !isfinite(fov)) return UEC_RESULT_INVALID_ARGUMENT;
    g_camera_fov = fov;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockGetWidgetChild(uec_object* widget,
                                              uec_string_view name,
                                              uec_object** out_child)
{
    if (widget != MockWidget() || out_child == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    *out_child = NULL;
    if (TextIs(name, "StatusText") == UEC_TRUE) *out_child = MockStatusChild();
    else if (TextIs(name, "DistanceProgress") == UEC_TRUE) *out_child = MockProgressChild();
    else if (TextIs(name, "SaveButton") == UEC_TRUE) *out_child = MockSaveChild();
    else if (TextIs(name, "LoadButton") == UEC_TRUE) *out_child = MockLoadChild();
    return *out_child == NULL ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_OK;
}

static uec_result UEC_CALL MockSetText(uec_object* child, uec_string_view text)
{
    if (child != MockStatusChild() || text.data == NULL || text.size >= sizeof(g_status_text))
        return UEC_RESULT_INVALID_ARGUMENT;
    memcpy(g_status_text, text.data, text.size);
    g_status_text[text.size] = '\0';
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockSetProgress(uec_object* child, double percent)
{
    if (child != MockProgressChild() || !isfinite(percent) || percent < 0.0 || percent > 1.0)
        return UEC_RESULT_INVALID_ARGUMENT;
    g_progress = percent;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockBindButton(uec_object* child,
                                          uec_widget_event_callback callback,
                                          void* user_data,
                                          uint64_t* out_id)
{
    if (callback == NULL || out_id == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    size_t index = child == MockSaveChild() ? 0u : child == MockLoadChild() ? 1u : 2u;
    if (index >= 2u || g_button_bindings[index].active == UEC_TRUE)
        return UEC_RESULT_INVALID_ARGUMENT;
    mock_button_binding* binding = &g_button_bindings[index];
    binding->id = ++g_next_id;
    binding->callback = callback;
    binding->user_data = user_data;
    binding->active = UEC_TRUE;
    *out_id = binding->id;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockUnbindButton(uec_context* context, uint64_t id)
{
    (void)context;
    for (size_t index = 0; index < sizeof(g_button_bindings) / sizeof(g_button_bindings[0]); ++index) {
        if (g_button_bindings[index].id != id || g_button_bindings[index].active != UEC_TRUE) continue;
        g_button_bindings[index].active = UEC_FALSE;
        ++g_button_unbind_count;
        return UEC_RESULT_OK;
    }
    return UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL MockAddWidget(uec_object* widget, int32_t z_order)
{
    if (widget != MockWidget() || z_order != 0 || g_widget_active == UEC_TRUE)
        return UEC_RESULT_INVALID_ARGUMENT;
    g_widget_active = UEC_TRUE;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockRemoveWidget(uec_object* widget)
{
    if (widget != MockWidget() || g_widget_active != UEC_TRUE)
        return UEC_RESULT_INVALID_ARGUMENT;
    g_widget_active = UEC_FALSE;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockTrace(uec_world* world,
                                     uec_vector3 start,
                                     uec_vector3 end,
                                     const uec_collision_shape* shape,
                                     uec_trace_channel channel,
                                     uec_bool complex,
                                     const uec_actor* const* ignored,
                                     uint32_t ignored_count,
                                     uec_hit_result_details* out_hit)
{
    (void)start;
    if (world == NULL || shape != NULL || channel != UEC_TRACE_VISIBILITY ||
        complex != UEC_FALSE || ignored == NULL || ignored_count != 1u ||
        ignored[0] != MockPawn() || end.x == start.x || out_hit == NULL)
        return UEC_RESULT_INVALID_ARGUMENT;
    if (g_trace_result != UEC_RESULT_OK) return g_trace_result;
    out_hit->hit.blocking_hit = g_trace_blocked;
    if (g_trace_blocked == UEC_TRUE) {
        out_hit->hit.actor = (uec_actor*)MockHitActor();
        out_hit->component = MockHitComponent();
    }
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockReleaseActor(uec_actor* actor)
{
    if ((uec_object*)actor != MockHitActor()) return UEC_RESULT_INVALID_HANDLE;
    ++g_actor_release_count;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockReleaseComponent(uec_scene_component* component)
{
    if (component != MockHitComponent()) return UEC_RESULT_INVALID_HANDLE;
    ++g_component_release_count;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockReleaseObject(uec_object* object)
{
    (void)object;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockSave(uec_context* context,
                                    uec_string_view slot,
                                    int32_t user_index,
                                    uint32_t version,
                                    const uint8_t* data,
                                    size_t size,
                                    uec_bool* out_saved)
{
    (void)context;
    if (slot.size == 0u || user_index != 0 || version != 1u || data == NULL ||
        size != sizeof(g_saved_payload) || out_saved == NULL)
        return UEC_RESULT_INVALID_ARGUMENT;
    memcpy(g_saved_payload, data, size);
    g_saved_size = size;
    g_saved_version = version;
    g_saved = UEC_TRUE;
    *out_saved = UEC_TRUE;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockLoad(uec_context* context,
                                    uec_string_view slot,
                                    int32_t user_index,
                                    uint32_t* version,
                                    uint8_t* buffer,
                                    size_t capacity,
                                    size_t* required)
{
    (void)context;
    if (slot.size == 0u || user_index != 0 || version == NULL || required == NULL)
        return UEC_RESULT_INVALID_ARGUMENT;
    if (g_saved != UEC_TRUE) return UEC_RESULT_INVALID_HANDLE;
    *version = g_saved_version;
    *required = g_saved_size;
    if (buffer == NULL || capacity < g_saved_size) return UEC_RESULT_BUFFER_TOO_SMALL;
    memcpy(buffer, g_saved_payload, g_saved_size);
    return UEC_RESULT_OK;
}

static uec_api MakeApi(void)
{
    uec_api api = {0};
    api.struct_size = sizeof(api);
    api.get_capabilities = &MockCapabilities;
    api.get_default_world = &MockGetWorld;
    api.release_world = &MockReleaseWorld;
    api.get_actor_transform = &MockGetTransform;
    api.set_actor_transform = &MockSetTransform;
    api.add_input_mapping_context = &MockAddMapping;
    api.remove_input_mapping_context = &MockRemoveMapping;
    api.bind_input_action = &MockBindInput;
    api.unbind_input_action = &MockUnbindInput;
    api.get_camera_field_of_view = &MockGetFov;
    api.set_camera_field_of_view = &MockSetFov;
    api.add_widget_to_viewport = &MockAddWidget;
    api.remove_widget_from_parent = &MockRemoveWidget;
    api.unbind_button_clicked = &MockUnbindButton;
    api.bind_button_clicked = &MockBindButton;
    api.get_widget_child = &MockGetWidgetChild;
    api.set_text_block_text = &MockSetText;
    api.set_progress_bar_percent = &MockSetProgress;
    api.trace_detailed_filtered = &MockTrace;
    api.save_versioned_application_data = &MockSave;
    api.load_versioned_application_data = &MockLoad;
    api.release_actor = &MockReleaseActor;
    api.release_scene_component = &MockReleaseComponent;
    api.release_object = &MockReleaseObject;
    return api;
}

static void ResetMock(void)
{
    memset(g_opaque_objects, 0, sizeof(g_opaque_objects));
    memset(&g_actor_transform, 0, sizeof(g_actor_transform));
    g_actor_transform.rotation.w = 1.0;
    g_actor_transform.scale.x = 1.0;
    g_actor_transform.scale.y = 1.0;
    g_actor_transform.scale.z = 1.0;
    g_actor_transform.translation.x = 10.0;
    g_actor_transform.translation.y = 20.0;
    g_camera_fov = 60.0;
    g_progress = 0.0;
    g_status_text[0] = '\0';
    g_mapping_active = UEC_FALSE;
    g_widget_active = UEC_FALSE;
    g_sweep_was_requested = UEC_FALSE;
    g_trace_blocked = UEC_FALSE;
    g_saved = UEC_FALSE;
    g_saved_size = 0u;
    g_saved_version = 0u;
    g_next_id = 0u;
    g_input_unbind_count = 0u;
    g_button_unbind_count = 0u;
    g_actor_release_count = 0u;
    g_component_release_count = 0u;
    g_world_release_count = 0u;
    g_trace_result = UEC_RESULT_OK;
    memset(g_input_bindings, 0, sizeof(g_input_bindings));
    memset(g_button_bindings, 0, sizeof(g_button_bindings));
}

static mock_input_binding* InputForEvent(uec_input_trigger_event event)
{
    for (size_t index = 0; index < sizeof(g_input_bindings) / sizeof(g_input_bindings[0]); ++index)
        if (g_input_bindings[index].active == UEC_TRUE && g_input_bindings[index].event == event)
            return &g_input_bindings[index];
    return NULL;
}

static void ClickButton(size_t index)
{
    mock_button_binding* binding = &g_button_bindings[index];
    if (binding->active != UEC_TRUE || binding->callback == NULL) return;
    const uint64_t id = binding->id;
    uec_widget_event_callback callback = binding->callback;
    void* data = binding->user_data;
    binding->active = UEC_FALSE;
    callback(id, data);
}

static int Check(uec_bool condition, const char* description)
{
    if (condition == UEC_TRUE) return 0;
    fprintf(stderr, "playable sample smoke failed: %s\n", description);
    return 1;
}

int UEC_CALL uec_c_playable_smoke(void)
{
    ResetMock();
    uec_api api = MakeApi();
    uec_context* context = (uec_context*)&g_opaque_objects[9];
    uec_actor* controller = (uec_actor*)&g_opaque_objects[10];
    uec_object* mapping = (uec_object*)&g_opaque_objects[11];
    uec_object* action = (uec_object*)&g_opaque_objects[11];
    uec_scene_component* camera = (uec_scene_component*)&g_opaque_objects[10];
    const uec_string_view slot = {"test-playable-slot", 18u};
    uec_playable_sample_state state = {0};

    uec_result result = uec_playable_sample_start(
        &api, context, controller, MockPawn(), mapping, action, camera,
        MockWidget(), slot, 10.0, 17, &state);
    if (Check(result == UEC_RESULT_OK && state.started == UEC_TRUE &&
              state.done == UEC_FALSE, "starts consumer") != 0) return 1;
    if (Check(g_mapping_active == UEC_TRUE && g_widget_active == UEC_TRUE &&
              strcmp(g_status_text, "Ready 10, 20 | clear") == 0,
              "installs input, viewport UI, and initial feedback") != 0) return 1;

    g_trace_blocked = UEC_TRUE;
    mock_input_binding* triggered = InputForEvent(UEC_INPUT_TRIGGER_TRIGGERED);
    if (Check(triggered != NULL, "binds the movement action") != 0) return 1;
    uec_input_action_value value = {0};
    value.struct_size = sizeof(value);
    value.kind = UEC_INPUT_ACTION_VALUE_AXIS_2D;
    value.axis.x = 1.0;
    value.axis.y = -0.5;
    triggered->callback(triggered->id, value, triggered->user_data);
    if (Check(state.last_result == UEC_RESULT_OK && state.movement_step_count == 1u &&
              g_sweep_was_requested == UEC_TRUE &&
              fabs(g_actor_transform.translation.x - 20.0) < 0.0001 &&
              fabs(g_actor_transform.translation.y - 15.0) < 0.0001,
              "moves the pawn with a collision sweep") != 0) return 1;
    if (Check(state.blocking_trace_count == 1u && g_actor_release_count == 1u &&
              g_component_release_count == 1u && strstr(g_status_text, "blocked") != NULL &&
              g_progress > 0.01 && g_progress < 0.02 && g_camera_fov > 60.0,
              "filters and releases trace results and updates camera and UMG") != 0) return 1;

    mock_input_binding* completed = InputForEvent(UEC_INPUT_TRIGGER_COMPLETED);
    if (Check(completed != NULL, "binds completion reset") != 0) return 1;
    completed->callback(completed->id, value, completed->user_data);
    if (Check(g_camera_fov == 60.0 && state.axis_x == 0.0 && state.axis_y == 0.0,
              "resets input and camera response") != 0) return 1;

    ClickButton(0u);
    if (Check(state.save_count == 1u && g_saved == UEC_TRUE &&
              strcmp(g_status_text, "Saved current position") == 0,
              "saves the current position through a versioned slot") != 0) return 1;
    g_actor_transform.translation.x = 999.0;
    g_actor_transform.translation.y = 999.0;
    ClickButton(1u);
    if (Check(state.load_count == 1u &&
              fabs(g_actor_transform.translation.x - 20.0) < 0.0001 &&
              fabs(g_actor_transform.translation.y - 15.0) < 0.0001 &&
              strcmp(g_status_text, "Loaded saved position") == 0,
              "loads and collision-sweeps back to the saved position") != 0) return 1;

    uec_playable_sample_cancel(&state);
    if (Check(state.done == UEC_TRUE && state.last_result == UEC_RESULT_OK &&
              g_mapping_active == UEC_FALSE && g_widget_active == UEC_FALSE &&
              g_input_unbind_count == 3u && g_button_unbind_count == 0u &&
              g_world_release_count == 1u && g_camera_fov == 60.0,
              "cancels bindings and mapping, removes UI, restores FOV, and releases world") != 0)
        return 1;

    ResetMock();
    api = MakeApi();
    g_trace_result = UEC_RESULT_INTERNAL_ERROR;
    result = uec_playable_sample_start(&api, context, controller, MockPawn(), mapping,
        action, camera, MockWidget(), slot, 10.0, 17, &state);
    if (Check(result == UEC_RESULT_INTERNAL_ERROR && state.done == UEC_TRUE &&
              g_mapping_active == UEC_FALSE && g_widget_active == UEC_FALSE &&
              g_input_unbind_count == 3u && g_world_release_count == 1u,
              "rolls back partially initialized startup") != 0) return 1;

    ResetMock();
    api = MakeApi();
    result = uec_playable_sample_start(&api, context, controller, MockPawn(), mapping,
        action, camera, MockWidget(), slot, 0.0, 17, &state);
    return Check(result == UEC_RESULT_INVALID_ARGUMENT && state.done == UEC_TRUE &&
                 g_mapping_active == UEC_FALSE && g_widget_active == UEC_FALSE,
                 "rejects a zero movement step before side effects");
}
