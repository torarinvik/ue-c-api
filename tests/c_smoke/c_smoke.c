#include "uec_api.h"

#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
#define UEC_TEST_ASSERT static_assert
#else
#define UEC_TEST_ASSERT _Static_assert
#endif

UEC_TEST_ASSERT(sizeof(uec_vector3) == 24, "uec_vector3 ABI changed");
UEC_TEST_ASSERT(sizeof(uec_quaternion) == 32, "uec_quaternion ABI changed");
UEC_TEST_ASSERT(sizeof(uec_transform) == 80, "uec_transform ABI changed");
UEC_TEST_ASSERT(sizeof(uec_property_value) == 32, "uec_property_value ABI changed");
UEC_TEST_ASSERT(sizeof(uec_text_output) == 32, "uec_text_output ABI changed");
UEC_TEST_ASSERT(sizeof(uec_collision_shape) == 56, "uec_collision_shape ABI changed");
UEC_TEST_ASSERT(sizeof(uec_hit_result) == 72, "uec_hit_result ABI changed");
UEC_TEST_ASSERT(sizeof(uec_input_action_value) == 40, "uec_input_action_value ABI changed");
UEC_TEST_ASSERT(UEC_RESULT_QUEUE_FULL == 9, "queue-full result code changed");
UEC_TEST_ASSERT(UEC_FALSE == 0u && UEC_TRUE == 1u, "boolean ABI values changed");
UEC_TEST_ASSERT(UEC_ABI_MINOR == 114u, "ABI minor must include typed container writes");
UEC_TEST_ASSERT(UEC_PROPERTY_FLAG_EDIT_CONST == 1u && UEC_PROPERTY_FLAG_REFERENCE == (1u << 6),
               "property flag values changed");
UEC_TEST_ASSERT(UEC_PROPERTY_SOFT_OBJECT == 15 && UEC_PROPERTY_SOFT_CLASS == 16,
               "soft property kind values changed");
UEC_TEST_ASSERT(offsetof(uec_api, get_capabilities) > offsetof(uec_api, abi_minor),
               "uec_api function table ordering changed");
UEC_TEST_ASSERT(offsetof(uec_api, sweep_trace) > offsetof(uec_api, cancel_object_load),
               "collision query functions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, run_on_game_thread) > offsetof(uec_api, delete_game_slot),
               "thread dispatch functions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, async_save_game_to_slot) >
                   offsetof(uec_api, set_object_property_object),
               "async save functions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_count_by_class) >
                   offsetof(uec_api, cancel_save_game_request),
               "actor query functions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, bind_input_action) >
                   offsetof(uec_api, destroy_audio_component),
               "input binding functions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_player_controller) >
                   offsetof(uec_api, unbind_input_action),
               "context access functions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, invoke_actor_function_text) >
                   offsetof(uec_api, get_world_game_instance),
               "text-marshaled invocation must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, subscribe_world_tick) >
                   offsetof(uec_api, invoke_actor_function_text),
               "tick subscriptions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, bind_audio_finished) >
                   offsetof(uec_api, unsubscribe_world_tick),
               "audio subscriptions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_path) >
                   offsetof(uec_api, unbind_audio_finished),
               "object identity queries must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_widget_visibility) >
                   offsetof(uec_api, get_object_class_name),
               "widget adapters must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, bind_button_clicked) >
                   offsetof(uec_api, set_text_block_text),
               "widget subscriptions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_component_velocity) >
                   offsetof(uec_api, unbind_button_clicked),
               "component physics queries must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_world_pie_instance) >
                   offsetof(uec_api, get_component_velocity),
               "world context queries must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_world_net_mode) >
                   offsetof(uec_api, get_world_pie_instance),
               "network context queries must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_tag_count) >
                   offsetof(uec_api, get_world_net_mode),
               "actor tag queries must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_tag_at) >
                   offsetof(uec_api, get_actor_tag_count),
               "actor tag output must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_bounds) >
                   offsetof(uec_api, get_actor_tag_at),
               "actor bounds must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, find_player_start) >
                   offsetof(uec_api, get_actor_bounds),
               "player-start lookup must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, bind_animation_finished) >
                   offsetof(uec_api, find_player_start),
               "animation callbacks must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, unbind_animation_finished) >
                   offsetof(uec_api, bind_animation_finished),
               "animation unsubscribe must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_world_has_authority) >
                   offsetof(uec_api, unbind_animation_finished),
               "authority query must append to uec_api");
    UEC_TEST_ASSERT(offsetof(uec_api, get_world_game_mode) >
                   offsetof(uec_api, get_world_has_authority),
               "game-mode query must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_world_game_state) >
                   offsetof(uec_api, get_world_game_mode),
               "game-state query must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_component_count_by_class) >
                   offsetof(uec_api, get_world_game_state),
               "component class count must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_component_at_by_class) >
                   offsetof(uec_api, get_actor_component_count_by_class),
               "component class lookup must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_array_element_value) >
                   offsetof(uec_api, get_class_property_flags),
               "typed array values must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_array_element_value) >
                   offsetof(uec_api, get_actor_property_array_element_value),
               "object typed array values must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_map_value) >
                   offsetof(uec_api, get_object_property_array_element_value),
               "typed map values must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_map_value) >
                   offsetof(uec_api, get_actor_property_map_value),
               "object typed map values must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_set_element_value) >
                   offsetof(uec_api, get_object_property_map_value),
               "typed set values must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_set_element_value) >
                   offsetof(uec_api, get_actor_property_set_element_value),
               "object typed set values must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_struct_field_value) >
                   offsetof(uec_api, get_object_property_set_element_value),
               "typed struct fields must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_struct_field_value) >
                   offsetof(uec_api, get_actor_property_struct_field_value),
               "object typed struct fields must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_property_array_element_value) >
                   offsetof(uec_api, get_object_property_struct_field_value),
               "typed array writes must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_object_property_array_element_value) >
                   offsetof(uec_api, set_actor_property_array_element_value),
               "object typed array writes must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_property_map_value) >
                   offsetof(uec_api, set_object_property_array_element_value),
               "typed map writes must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_object_property_map_value) >
                   offsetof(uec_api, set_actor_property_map_value),
               "object typed map writes must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_config_string) >
                   offsetof(uec_api, get_actor_component_at_by_class),
               "configuration reads must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_config_string) >
                   offsetof(uec_api, get_config_string),
               "configuration writes must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_streaming_level_count) >
                   offsetof(uec_api, set_config_string),
               "streaming count must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_streaming_level_at) >
                   offsetof(uec_api, get_streaming_level_count),
               "streaming lookup must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_streaming_level_state) >
                   offsetof(uec_api, get_streaming_level_at),
               "streaming state must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, is_class_path_loaded) >
                   offsetof(uec_api, set_streaming_level_state),
               "class load queries must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, bind_component_hit) >
                   offsetof(uec_api, is_class_path_loaded),
               "collision callbacks must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, unbind_component_hit) >
                   offsetof(uec_api, bind_component_hit),
               "collision unbinding must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, sweep_trace_filtered) >
                   offsetof(uec_api, unbind_component_hit),
               "filtered sweeps must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, overlap_shape_filtered) >
                   offsetof(uec_api, sweep_trace_filtered),
               "filtered overlaps must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_tag) >
                   offsetof(uec_api, overlap_shape_filtered),
               "actor tag mutation must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_runtime_stats) >
                   offsetof(uec_api, set_actor_tag),
               "runtime stats must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_world_count_by_kind) >
                   offsetof(uec_api, get_runtime_stats),
               "world-kind count must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_world_at_by_kind) >
                   offsetof(uec_api, get_world_count_by_kind),
               "world-kind lookup must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, invoke_actor_function_value) >
                   offsetof(uec_api, get_world_at_by_kind),
               "typed invocation must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, invoke_actor_function_values) >
                   offsetof(uec_api, invoke_actor_function_value),
               "multi-output invocation must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_class_function_parameter_at) >
                   offsetof(uec_api, invoke_actor_function_values),
               "parameter metadata must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, invoke_actor_function_text_values) >
                   offsetof(uec_api, get_class_function_parameter_at),
               "text outputs must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, find_object) >
                   offsetof(uec_api, invoke_actor_function_text_values),
               "loaded-object lookup must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, travel_world_async) > offsetof(uec_api, find_object),
               "travel completion must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, cancel_travel_request) >
                   offsetof(uec_api, travel_world_async),
               "travel cancellation must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_component_visible) >
                   offsetof(uec_api, cancel_travel_request),
               "component visibility readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_component_active) >
                   offsetof(uec_api, get_component_visible),
               "component activation readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_class_function_flags) >
                   offsetof(uec_api, get_component_active),
               "function flags must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_widget_visibility) >
                   offsetof(uec_api, get_class_function_flags),
               "widget visibility must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_text_block_text) >
                   offsetof(uec_api, get_widget_visibility),
               "text block readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_component_collision_enabled) >
                   offsetof(uec_api, get_text_block_text),
               "collision readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_audio_component_playing) >
                   offsetof(uec_api, get_component_collision_enabled),
               "audio readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_streaming_level_state_async) >
                   offsetof(uec_api, get_audio_component_playing),
               "streaming completion must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, cancel_streaming_level_request) >
                   offsetof(uec_api, set_streaming_level_state_async),
               "streaming cancellation must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_component_collision_response) >
                   offsetof(uec_api, cancel_streaming_level_request),
               "collision response must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_config_integer) >
                   offsetof(uec_api, get_component_collision_response),
               "config integer readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_config_integer) >
                   offsetof(uec_api, get_config_integer),
               "config integer write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, bind_actor_destroyed) >
                   offsetof(uec_api, set_config_integer),
               "actor destruction binding must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, unbind_actor_destroyed) >
                   offsetof(uec_api, bind_actor_destroyed),
               "actor destruction unbinding must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_config_bool) >
                   offsetof(uec_api, unbind_actor_destroyed),
               "config boolean readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_array_count) >
                   offsetof(uec_api, get_config_bool),
               "array count must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_array_element_text) >
                   offsetof(uec_api, get_actor_property_array_count),
               "array element readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_array_count) >
                   offsetof(uec_api, get_actor_property_array_element_text),
               "object array count must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_array_element_text) >
                   offsetof(uec_api, get_object_property_array_count),
               "object array element readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_map_count) >
                   offsetof(uec_api, get_object_property_array_element_text),
               "object map count must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_map_entry_text) >
                   offsetof(uec_api, get_object_property_map_count),
               "object map entry readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_set_count) >
                   offsetof(uec_api, get_object_property_map_entry_text),
               "object set count must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_set_element_text) >
                   offsetof(uec_api, get_object_property_set_count),
               "object set element readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_soft_path) >
                   offsetof(uec_api, get_object_property_set_element_text),
               "actor soft path readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_soft_path) >
                   offsetof(uec_api, get_actor_property_soft_path),
               "object soft path readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_map_count) >
                   offsetof(uec_api, get_object_property_soft_path),
               "actor map count must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_map_entry_text) >
                   offsetof(uec_api, get_actor_property_map_count),
               "actor map entry readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_set_count) >
                   offsetof(uec_api, get_actor_property_map_entry_text),
               "actor set count must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_set_element_text) >
                   offsetof(uec_api, get_actor_property_set_count),
               "actor set element readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_property_soft_path) >
                   offsetof(uec_api, get_actor_property_set_element_text),
               "actor soft path write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_object_property_soft_path) >
                   offsetof(uec_api, set_actor_property_soft_path),
               "object soft path write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_struct_field_text) >
                   offsetof(uec_api, set_object_property_soft_path),
               "actor struct field readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_struct_field_text) >
                   offsetof(uec_api, get_actor_property_struct_field_text),
               "object struct field readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_property_struct_field_text) >
                   offsetof(uec_api, get_object_property_struct_field_text),
               "actor struct field write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_object_property_struct_field_text) >
                   offsetof(uec_api, set_actor_property_struct_field_text),
               "object struct field write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_property_array_element_text) >
                   offsetof(uec_api, set_object_property_struct_field_text),
               "actor array element write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_object_property_array_element_text) >
                   offsetof(uec_api, set_actor_property_array_element_text),
               "object array element write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_property_map_value_text) >
                   offsetof(uec_api, set_object_property_array_element_text),
               "actor map value write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_object_property_map_value_text) >
                   offsetof(uec_api, set_actor_property_map_value_text),
               "object map value write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_class_property_flags) >
                   offsetof(uec_api, set_object_property_map_value_text),
               "property flags must append to uec_api");

static void UEC_CALL NoopGameThreadCallback(void* user_data)
{
    (void)user_data;
}

int main(void)
{
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK || api == NULL || context == NULL)
    {
        return 1;
    }

    const uec_api* rejected_api = api;
    uec_context* rejected_context = context;
    result = uec_get_api(UEC_ABI_MAJOR + 1u, UEC_ABI_MINOR + 1u,
                         &rejected_api, &rejected_context);
    if (result != UEC_RESULT_UNSUPPORTED || rejected_api != NULL || rejected_context != NULL)
    {
        api->release_context(context);
        return 7;
    }

    size_t invalid_required = 42;
    result = api->get_last_error(NULL, NULL, 0, &invalid_required);
    if (result != UEC_RESULT_INVALID_HANDLE || invalid_required != 0)
    {
        api->release_context(context);
        return 8;
    }

    uint64_t invalid_request_id = UINT64_C(42);
    result = api->run_on_game_thread(NULL, &NoopGameThreadCallback, NULL,
                                     &invalid_request_id);
    if (result != UEC_RESULT_INVALID_HANDLE || invalid_request_id != 0)
    {
        api->release_context(context);
        return 9;
    }

    uec_capabilities capabilities = 0;
    result = api->get_capabilities(context, &capabilities);
    if (result != UEC_RESULT_OK || (capabilities & UEC_CAPABILITY_BOOTSTRAP) == 0 ||
        (capabilities & UEC_CAPABILITY_ACTORS) == 0 ||
        (capabilities & UEC_CAPABILITY_REFLECTION) == 0 ||
        (capabilities & UEC_CAPABILITY_CLASS_METADATA) == 0 ||
        (capabilities & UEC_CAPABILITY_CONFIGURATION) == 0 ||
        (capabilities & UEC_CAPABILITY_STREAMING) == 0)
    {
        api->release_context(context);
        return 5;
    }

    uec_runtime_stats stats = {sizeof(stats), 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
    result = api->get_runtime_stats(context, &stats);
    if (result != UEC_RESULT_OK || stats.active_subscriptions != 0u ||
        stats.pending_requests != 0u || stats.active_callbacks != 0u ||
        stats.live_contexts != 1u || stats.live_worlds != 0u || stats.live_actors != 0u ||
        stats.live_components != 0u || stats.live_classes != 0u || stats.live_objects != 0u)
    {
        api->release_context(context);
        return 10;
    }

    uint32_t world_count = 42u;
    result = api->get_world_count_by_kind(context, UEC_WORLD_KIND_GAME, &world_count);
    if (result != UEC_RESULT_UNSUPPORTED || world_count != 0u)
    {
        api->release_context(context);
        return 11;
    }

    uec_property_value invocation_result = {sizeof(invocation_result), UEC_PROPERTY_UNKNOWN,
                                            UEC_FALSE, {0u, 0u, 0u}, 0, 0.0};
    const uec_string_view empty_function_name = {NULL, 0u};
    result = api->invoke_actor_function_value(NULL, empty_function_name, NULL, 0,
                                              &invocation_result);
    if (result != UEC_RESULT_UNSUPPORTED || invocation_result.kind != UEC_PROPERTY_UNKNOWN)
    {
        api->release_context(context);
        return 12;
    }

    uint32_t invocation_count = 42u;
    result = api->invoke_actor_function_values(NULL, empty_function_name, NULL, 0, NULL, 0,
                                               &invocation_count);
    if (result != UEC_RESULT_UNSUPPORTED || invocation_count != 0u)
    {
        api->release_context(context);
        return 13;
    }

    size_t parameter_name_size = 99u;
    uec_property_kind parameter_kind = UEC_PROPERTY_BOOL;
    uint32_t parameter_flags = 99u;
    result = api->get_class_function_parameter_at(NULL, 0u, 0u, NULL, 0u,
                                                   &parameter_name_size, &parameter_kind,
                                                   &parameter_flags);
    if (result != UEC_RESULT_UNSUPPORTED || parameter_name_size != 0u ||
        parameter_kind != UEC_PROPERTY_UNKNOWN || parameter_flags != 0u)
    {
        api->release_context(context);
        return 14;
    }

    uint32_t text_output_count = 42u;
    result = api->invoke_actor_function_text_values(NULL, empty_function_name, NULL, 0u,
                                                    NULL, 0u, &text_output_count);
    if (result != UEC_RESULT_UNSUPPORTED || text_output_count != 0u)
    {
        api->release_context(context);
        return 15;
    }

    uec_object* found_object = (uec_object*)1;
    result = api->find_object(context, empty_function_name, &found_object);
    if (result != UEC_RESULT_NOT_INITIALIZED || found_object != NULL)
    {
        api->release_context(context);
        return 16;
    }

    uint64_t travel_request_id = 42u;
    result = api->travel_world_async(NULL, empty_function_name, NULL, NULL, &travel_request_id);
    if (result != UEC_RESULT_UNSUPPORTED || travel_request_id != 0u ||
        api->cancel_travel_request(context, 1u) != UEC_RESULT_UNSUPPORTED)
    {
        api->release_context(context);
        return 17;
    }

    uec_bool component_visible = UEC_TRUE;
    uec_bool component_active = UEC_TRUE;
    if (api->get_component_visible(NULL, &component_visible) != UEC_RESULT_UNSUPPORTED ||
        component_visible != UEC_FALSE ||
        api->get_component_active(NULL, &component_active) != UEC_RESULT_UNSUPPORTED ||
        component_active != UEC_FALSE)
    {
        api->release_context(context);
        return 18;
    }

    uint32_t function_flags = 42u;
    if (api->get_class_function_flags(NULL, 0u, &function_flags) != UEC_RESULT_UNSUPPORTED ||
        function_flags != 0u)
    {
        api->release_context(context);
        return 19;
    }

    uec_widget_visibility widget_visibility = UEC_WIDGET_HIDDEN;
    size_t widget_text_required = 42u;
    if (api->get_widget_visibility(NULL, &widget_visibility) != UEC_RESULT_UNSUPPORTED ||
        widget_visibility != UEC_WIDGET_VISIBLE ||
        api->get_text_block_text(NULL, NULL, 0u, &widget_text_required) != UEC_RESULT_UNSUPPORTED ||
        widget_text_required != 0u)
    {
        api->release_context(context);
        return 20;
    }

    uec_collision_enabled collision_enabled = UEC_COLLISION_QUERY_AND_PHYSICS;
    uec_bool audio_playing = UEC_TRUE;
    if (api->get_component_collision_enabled(NULL, &collision_enabled) != UEC_RESULT_UNSUPPORTED ||
        collision_enabled != UEC_COLLISION_DISABLED ||
        api->get_audio_component_playing(NULL, &audio_playing) != UEC_RESULT_UNSUPPORTED ||
        audio_playing != UEC_FALSE)
    {
        api->release_context(context);
        return 21;
    }

    uint64_t streaming_request_id = 42u;
    const uec_string_view streaming_package = {"/Game/Test", 10u};
    if (api->set_streaming_level_state_async(NULL, streaming_package,
                                             UEC_TRUE, UEC_TRUE, NULL, NULL,
                                             &streaming_request_id) != UEC_RESULT_INVALID_ARGUMENT ||
        streaming_request_id != 0u ||
        api->cancel_streaming_level_request(context, 42u) != UEC_RESULT_UNSUPPORTED)
    {
        api->release_context(context);
        return 22;
    }

    uec_collision_response collision_response = UEC_COLLISION_RESPONSE_BLOCK;
    if (api->get_component_collision_response(NULL, UEC_TRACE_VISIBILITY,
                                              &collision_response) != UEC_RESULT_UNSUPPORTED ||
        collision_response != UEC_COLLISION_RESPONSE_IGNORE)
    {
        api->release_context(context);
        return 23;
    }

    int64_t config_value = 42;
    if (api->get_config_integer(context, streaming_package, streaming_package,
                                &config_value) != UEC_RESULT_UNSUPPORTED ||
        config_value != 0 ||
        api->set_config_integer(context, streaming_package, streaming_package, 1) !=
            UEC_RESULT_UNSUPPORTED)
    {
        api->release_context(context);
        return 24;
    }

    uint64_t destroyed_subscription_id = 42u;
    if (api->bind_actor_destroyed(NULL, NULL, NULL, &destroyed_subscription_id) != UEC_RESULT_INVALID_ARGUMENT ||
        destroyed_subscription_id != 0u ||
        api->unbind_actor_destroyed(context, 42u) != UEC_RESULT_UNSUPPORTED)
    {
        api->release_context(context);
        return 25;
    }

    uec_bool config_bool = UEC_TRUE;
    if (api->get_config_bool(context, streaming_package, streaming_package, &config_bool) !=
            UEC_RESULT_UNSUPPORTED || config_bool != UEC_FALSE)
    {
        api->release_context(context);
        return 26;
    }

    uint32_t array_count = 42u;
    size_t array_required = 42u;
    uec_property_kind array_kind = UEC_PROPERTY_STRING;
    if (api->get_actor_property_array_count(NULL, streaming_package, &array_count) != UEC_RESULT_UNSUPPORTED ||
        array_count != 0u ||
        api->get_actor_property_array_element_text(NULL, streaming_package, 0u, NULL, 0u,
                                                   &array_required, &array_kind) != UEC_RESULT_UNSUPPORTED ||
        array_required != 0u || array_kind != UEC_PROPERTY_UNKNOWN)
    {
        api->release_context(context);
        return 27;
    }

    array_count = 42u;
    array_required = 42u;
    array_kind = UEC_PROPERTY_STRING;
    if (api->get_object_property_array_count(NULL, streaming_package, &array_count) != UEC_RESULT_UNSUPPORTED ||
        array_count != 0u ||
        api->get_object_property_array_element_text(NULL, streaming_package, 0u, NULL, 0u,
                                                    &array_required, &array_kind) != UEC_RESULT_UNSUPPORTED ||
        array_required != 0u || array_kind != UEC_PROPERTY_UNKNOWN)
    {
        api->release_context(context);
        return 28;
    }

    uint32_t map_count = 42u;
    uec_text_output map_key = {sizeof(map_key), UEC_PROPERTY_STRING, NULL, 0u, 42u};
    uec_text_output map_value = {sizeof(map_value), UEC_PROPERTY_STRING, NULL, 0u, 42u};
    uec_text_output set_element = {sizeof(set_element), UEC_PROPERTY_STRING, NULL, 0u, 42u};
    if (api->get_object_property_map_count(NULL, streaming_package, &map_count) != UEC_RESULT_UNSUPPORTED ||
        map_count != 0u ||
        api->get_object_property_map_entry_text(NULL, streaming_package, 0u, &map_key, &map_value) !=
            UEC_RESULT_UNSUPPORTED || map_key.kind != UEC_PROPERTY_UNKNOWN ||
        map_key.required_size != 0u || map_value.kind != UEC_PROPERTY_UNKNOWN ||
        map_value.required_size != 0u ||
        api->get_object_property_set_count(NULL, streaming_package, &map_count) != UEC_RESULT_UNSUPPORTED ||
        map_count != 0u ||
        api->get_object_property_set_element_text(NULL, streaming_package, 0u, &set_element) !=
            UEC_RESULT_UNSUPPORTED || set_element.kind != UEC_PROPERTY_UNKNOWN ||
        set_element.required_size != 0u)
    {
        api->release_context(context);
        return 29;
    }

    size_t soft_required = 42u;
    uec_property_kind soft_kind = UEC_PROPERTY_STRING;
    if (api->get_actor_property_soft_path(NULL, streaming_package, NULL, 0u,
                                           &soft_required, &soft_kind) != UEC_RESULT_UNSUPPORTED ||
        soft_required != 0u || soft_kind != UEC_PROPERTY_UNKNOWN ||
        api->get_object_property_soft_path(NULL, streaming_package, NULL, 0u,
                                           &soft_required, &soft_kind) != UEC_RESULT_UNSUPPORTED ||
        soft_required != 0u || soft_kind != UEC_PROPERTY_UNKNOWN)
    {
        api->release_context(context);
        return 30;
    }

    map_count = 42u;
    map_key.kind = UEC_PROPERTY_STRING;
    map_key.required_size = 42u;
    map_value.kind = UEC_PROPERTY_STRING;
    map_value.required_size = 42u;
    set_element.kind = UEC_PROPERTY_STRING;
    set_element.required_size = 42u;
    if (api->get_actor_property_map_count(NULL, streaming_package, &map_count) != UEC_RESULT_UNSUPPORTED ||
        map_count != 0u ||
        api->get_actor_property_map_entry_text(NULL, streaming_package, 0u, &map_key, &map_value) !=
            UEC_RESULT_UNSUPPORTED || map_key.kind != UEC_PROPERTY_UNKNOWN ||
        map_key.required_size != 0u || map_value.kind != UEC_PROPERTY_UNKNOWN ||
        map_value.required_size != 0u ||
        api->get_actor_property_set_count(NULL, streaming_package, &map_count) != UEC_RESULT_UNSUPPORTED ||
        map_count != 0u ||
        api->get_actor_property_set_element_text(NULL, streaming_package, 0u, &set_element) !=
            UEC_RESULT_UNSUPPORTED || set_element.kind != UEC_PROPERTY_UNKNOWN ||
        set_element.required_size != 0u)
    {
        api->release_context(context);
        return 31;
    }

    if (api->set_actor_property_soft_path(NULL, streaming_package, streaming_package) !=
            UEC_RESULT_UNSUPPORTED ||
        api->set_object_property_soft_path(NULL, streaming_package, streaming_package) !=
            UEC_RESULT_UNSUPPORTED)
    {
        api->release_context(context);
        return 32;
    }

    if (api->get_actor_property_struct_field_text(NULL, streaming_package, streaming_package,
                                                  NULL, 0u, &soft_required, &soft_kind) !=
            UEC_RESULT_UNSUPPORTED || soft_required != 0u ||
        soft_kind != UEC_PROPERTY_UNKNOWN ||
        api->get_object_property_struct_field_text(NULL, streaming_package, streaming_package,
                                                   NULL, 0u, &soft_required, &soft_kind) !=
            UEC_RESULT_UNSUPPORTED || soft_required != 0u ||
        soft_kind != UEC_PROPERTY_UNKNOWN)
    {
        api->release_context(context);
        return 33;
    }

    if (api->set_actor_property_struct_field_text(NULL, streaming_package, streaming_package,
                                                   streaming_package) != UEC_RESULT_UNSUPPORTED ||
        api->set_object_property_struct_field_text(NULL, streaming_package, streaming_package,
                                                    streaming_package) != UEC_RESULT_UNSUPPORTED)
    {
        api->release_context(context);
        return 34;
    }

    if (api->set_actor_property_array_element_text(NULL, streaming_package, 0u,
                                                   streaming_package) != UEC_RESULT_UNSUPPORTED ||
        api->set_object_property_array_element_text(NULL, streaming_package, 0u,
                                                    streaming_package) != UEC_RESULT_UNSUPPORTED)
    {
        api->release_context(context);
        return 35;
    }

    if (api->set_actor_property_map_value_text(NULL, streaming_package, 0u,
                                               streaming_package) != UEC_RESULT_UNSUPPORTED ||
        api->set_object_property_map_value_text(NULL, streaming_package, 0u,
                                                streaming_package) != UEC_RESULT_UNSUPPORTED)
    {
        api->release_context(context);
        return 36;
    }

    uint32_t property_flags = 42u;
    if (api->get_class_property_flags(NULL, 0u, &property_flags) != UEC_RESULT_UNSUPPORTED ||
        property_flags != 0u)
    {
        api->release_context(context);
        return 37;
    }

    uec_property_value array_value = {sizeof(array_value), UEC_PROPERTY_STRING,
                                      UEC_TRUE, {0u, 0u, 0u}, 42, 42.0};
    if (api->get_actor_property_array_element_value(NULL, streaming_package, 0u,
                                                    &array_value) != UEC_RESULT_UNSUPPORTED ||
        array_value.kind != UEC_PROPERTY_UNKNOWN || array_value.bool_value != UEC_FALSE ||
        array_value.integer_value != 0 || array_value.real_value != 0.0 ||
        api->get_object_property_array_element_value(NULL, streaming_package, 0u,
                                                     &array_value) != UEC_RESULT_UNSUPPORTED ||
        array_value.kind != UEC_PROPERTY_UNKNOWN || array_value.bool_value != UEC_FALSE ||
        array_value.integer_value != 0 || array_value.real_value != 0.0)
    {
        api->release_context(context);
        return 38;
    }

    if (api->get_actor_property_map_value(NULL, streaming_package, 0u, &array_value) != UEC_RESULT_UNSUPPORTED ||
        array_value.kind != UEC_PROPERTY_UNKNOWN || array_value.bool_value != UEC_FALSE ||
        array_value.integer_value != 0 || array_value.real_value != 0.0 ||
        api->get_object_property_map_value(NULL, streaming_package, 0u, &array_value) != UEC_RESULT_UNSUPPORTED ||
        array_value.kind != UEC_PROPERTY_UNKNOWN || array_value.bool_value != UEC_FALSE ||
        array_value.integer_value != 0 || array_value.real_value != 0.0 ||
        api->get_actor_property_set_element_value(NULL, streaming_package, 0u, &array_value) != UEC_RESULT_UNSUPPORTED ||
        array_value.kind != UEC_PROPERTY_UNKNOWN || array_value.bool_value != UEC_FALSE ||
        array_value.integer_value != 0 || array_value.real_value != 0.0 ||
        api->get_object_property_set_element_value(NULL, streaming_package, 0u, &array_value) != UEC_RESULT_UNSUPPORTED ||
        array_value.kind != UEC_PROPERTY_UNKNOWN || array_value.bool_value != UEC_FALSE ||
        array_value.integer_value != 0 || array_value.real_value != 0.0)
    {
        api->release_context(context);
        return 39;
    }

    if (api->get_actor_property_struct_field_value(NULL, streaming_package, streaming_package,
                                                   &array_value) != UEC_RESULT_UNSUPPORTED ||
        array_value.kind != UEC_PROPERTY_UNKNOWN || array_value.bool_value != UEC_FALSE ||
        array_value.integer_value != 0 || array_value.real_value != 0.0 ||
        api->get_object_property_struct_field_value(NULL, streaming_package, streaming_package,
                                                    &array_value) != UEC_RESULT_UNSUPPORTED ||
        array_value.kind != UEC_PROPERTY_UNKNOWN || array_value.bool_value != UEC_FALSE ||
        array_value.integer_value != 0 || array_value.real_value != 0.0)
    {
        api->release_context(context);
        return 40;
    }

    if (api->set_actor_property_array_element_value(NULL, streaming_package, 0u, &array_value) != UEC_RESULT_UNSUPPORTED ||
        api->set_object_property_array_element_value(NULL, streaming_package, 0u, &array_value) != UEC_RESULT_UNSUPPORTED ||
        api->set_actor_property_map_value(NULL, streaming_package, 0u, &array_value) != UEC_RESULT_UNSUPPORTED ||
        api->set_object_property_map_value(NULL, streaming_package, 0u, &array_value) != UEC_RESULT_UNSUPPORTED)
    {
        api->release_context(context);
        return 41;
    }

    const char message[] = "C ABI smoke test";
    const uec_string_view message_view = {message, sizeof(message) - 1u};
    result = api->log(context, message_view);
    if (result != UEC_RESULT_OK)
    {
        api->release_context(context);
        return 2;
    }

    char error[32];
    size_t required = 0;
    result = api->get_last_error(context, error, sizeof(error), &required);
    if (result != UEC_RESULT_OK || required == 0)
    {
        api->release_context(context);
        return 3;
    }

    if (api->struct_size < sizeof(uec_api) || api->abi_major != UEC_ABI_MAJOR)
    {
        api->release_context(context);
        return 6;
    }

    api->release_context(context);
    puts("uec C ABI smoke test passed");
    return 0;
}
