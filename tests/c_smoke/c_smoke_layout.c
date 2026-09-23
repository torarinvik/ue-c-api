#include "uec_api.h"

#include <stddef.h>

#ifdef __cplusplus
#define UEC_TEST_ASSERT static_assert
#else
#define UEC_TEST_ASSERT _Static_assert
#endif

typedef struct uec_function_argument_abi_133 {
    uint32_t struct_size;
    uec_property_kind kind;
    uec_bool bool_value;
    uint8_t reserved[3];
    int64_t integer_value;
    double real_value;
    uec_object* object_value;
    uec_class* class_value;
    uec_world* world_value;
    uec_string_view text_value;
} uec_function_argument_abi_133;

typedef struct uec_function_output_abi_133 {
    uint32_t struct_size;
    uec_property_kind kind;
    uec_bool bool_value;
    uint8_t reserved[3];
    int64_t integer_value;
    double real_value;
    uec_object* object_value;
    uec_class* class_value;
    char* text_buffer;
    size_t text_buffer_size;
    size_t text_required_size;
} uec_function_output_abi_133;

UEC_TEST_ASSERT(sizeof(uec_vector3) == 24, "uec_vector3 ABI changed");
UEC_TEST_ASSERT(sizeof(uec_quaternion) == 32, "uec_quaternion ABI changed");
UEC_TEST_ASSERT(sizeof(uec_transform) == 80, "uec_transform ABI changed");
UEC_TEST_ASSERT(sizeof(uec_property_value) == 32, "uec_property_value ABI changed");
UEC_TEST_ASSERT(sizeof(uec_text_output) == 32, "uec_text_output ABI changed");
UEC_TEST_ASSERT(offsetof(uec_function_argument, integer_value) > offsetof(uec_function_argument, bool_value), "mixed invocation argument scalar layout changed");
UEC_TEST_ASSERT(offsetof(uec_function_argument, text_value) > offsetof(uec_function_argument, world_value), "mixed invocation argument handle layout changed");
UEC_TEST_ASSERT(offsetof(uec_function_output, text_buffer) > offsetof(uec_function_output, class_value), "mixed invocation output handle layout changed");
UEC_TEST_ASSERT(offsetof(uec_function_output, text_required_size) > offsetof(uec_function_output, text_buffer_size), "mixed invocation output buffer layout changed");
UEC_TEST_ASSERT(offsetof(uec_function_argument, struct_value) == sizeof(uec_function_argument_abi_133), "function argument 1.133 prefix changed");
UEC_TEST_ASSERT(offsetof(uec_function_output, struct_value) == sizeof(uec_function_output_abi_133), "function output 1.133 prefix changed");
UEC_TEST_ASSERT(UEC_FUNCTION_STRUCT_NONE == 0 &&
                   UEC_FUNCTION_STRUCT_TRANSFORM == 3,
               "typed function struct tags changed");
UEC_TEST_ASSERT(sizeof(uec_collision_shape) == 56, "uec_collision_shape ABI changed");
UEC_TEST_ASSERT(sizeof(uec_hit_result) == 72, "uec_hit_result ABI changed");
UEC_TEST_ASSERT(sizeof(uec_hit_result_details) == 200, "uec_hit_result_details ABI changed");
UEC_TEST_ASSERT(sizeof(uec_input_action_value) == 40, "uec_input_action_value ABI changed");
UEC_TEST_ASSERT(UEC_RESULT_OK == 0 && UEC_RESULT_INVALID_ARGUMENT == 1 &&
                   UEC_RESULT_INVALID_HANDLE == 2 && UEC_RESULT_BUFFER_TOO_SMALL == 3 &&
                   UEC_RESULT_NOT_INITIALIZED == 4 && UEC_RESULT_WRONG_THREAD == 5 &&
                   UEC_RESULT_UNSUPPORTED == 6 && UEC_RESULT_SHUTTING_DOWN == 7 &&
                   UEC_RESULT_INTERNAL_ERROR == 8 && UEC_RESULT_QUEUE_FULL == 9,
               "result code ABI values changed");
UEC_TEST_ASSERT(UEC_FALSE == 0u && UEC_TRUE == 1u, "boolean ABI values changed");
UEC_TEST_ASSERT(UEC_ABI_MINOR == 142u, "ABI minor must include widget child lookup");
UEC_TEST_ASSERT(UEC_CHECKBOX_UNCHECKED == 0 && UEC_CHECKBOX_CHECKED == 1 &&
                   UEC_CHECKBOX_UNDETERMINED == 2,
               "checkbox state enum values changed");
UEC_TEST_ASSERT(UEC_WIDGET_VISIBLE == 0 && UEC_WIDGET_COLLAPSED == 1 &&
                   UEC_WIDGET_HIDDEN == 2 && UEC_WIDGET_HIT_TEST_INVISIBLE == 3 &&
                   UEC_WIDGET_SELF_HIT_TEST_INVISIBLE == 4,
               "UMG visibility enum values changed");
UEC_TEST_ASSERT(UEC_PROPERTY_FLAG_EDIT_CONST == 1u && UEC_PROPERTY_FLAG_REFERENCE == (1u << 6),
               "property flag values changed");
UEC_TEST_ASSERT(UEC_PROPERTY_SOFT_OBJECT == 15 && UEC_PROPERTY_SOFT_CLASS == 16,
               "soft property kind values changed");
UEC_TEST_ASSERT(UEC_CAPABILITY_REFLECTION_CONTAINERS == (UINT64_C(1) << 26),
               "reflection container capability changed");
UEC_TEST_ASSERT(UEC_CAPABILITY_EVENT_BRIDGE == (UINT64_C(1) << 28),
               "event bridge capability changed");
UEC_TEST_ASSERT(UEC_CAPABILITY_ASYNC_LATENT_FUNCTIONS == (UINT64_C(1) << 29),
               "async latent invocation capability changed");
UEC_TEST_ASSERT(UEC_CAPABILITY_COLLISION_DETAILS == (UINT64_C(1) << 27),
               "collision details capability changed");
UEC_TEST_ASSERT(offsetof(uec_api, get_capabilities) > offsetof(uec_api, abi_minor),
               "uec_api function table ordering changed");
UEC_TEST_ASSERT(offsetof(uec_api, get_last_error) >
                   offsetof(uec_api, get_capabilities),
               "diagnostic retrieval must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, log) > offsetof(uec_api, get_last_error),
               "logging must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, release_context) > offsetof(uec_api, log),
               "context release must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_world_count) >
                   offsetof(uec_api, release_context),
               "world enumeration must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_world_at) >
                   offsetof(uec_api, get_world_count),
               "world lookup must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_world_kind) >
                   offsetof(uec_api, get_world_at),
               "world kind metadata must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_world_name) >
                   offsetof(uec_api, get_world_kind),
               "world names must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, travel_world) >
                   offsetof(uec_api, get_world_name),
               "world travel must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_first_player_controller) >
                   offsetof(uec_api, travel_world),
               "player lookup must append to uec_api");
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
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_property_struct_field_value) >
                   offsetof(uec_api, set_object_property_map_value),
               "typed struct writes must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_object_property_struct_field_value) >
                   offsetof(uec_api, set_actor_property_struct_field_value),
               "object typed struct writes must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_class_property_default_text) >
                   offsetof(uec_api, set_object_property_struct_field_value),
               "class defaults must append to uec_api");
    UEC_TEST_ASSERT(offsetof(uec_api, get_class_property_reference_class_path) >
                   offsetof(uec_api, get_class_property_default_text),
               "reference metadata must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_class_property_enum_value_count) >
                   offsetof(uec_api, get_class_property_reference_class_path),
               "enum metadata count must append to uec_api");
    UEC_TEST_ASSERT(offsetof(uec_api, get_class_property_enum_value_at) >
                   offsetof(uec_api, get_class_property_enum_value_count),
               "enum metadata values must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_class_property_struct_field_count) >
                   offsetof(uec_api, get_class_property_enum_value_at),
               "struct field counts must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_class_property_struct_field_at) >
                   offsetof(uec_api, get_class_property_struct_field_count),
               "struct field metadata must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_class) >
                   offsetof(uec_api, get_class_property_struct_field_at),
               "actor class references must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_property_class) >
                   offsetof(uec_api, get_actor_property_class),
               "actor class writes must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_class) >
                   offsetof(uec_api, set_actor_property_class),
               "object class references must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_object_property_class) >
                   offsetof(uec_api, get_object_property_class),
               "object class writes must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_class_property_struct_path) >
                   offsetof(uec_api, set_object_property_class),
               "struct type paths must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_class_property_container_kinds) >
                   offsetof(uec_api, get_class_property_struct_path),
               "container property kinds must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_property_set_element_text) >
                   offsetof(uec_api, get_class_property_container_kinds),
               "actor set text writes must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_object_property_set_element_text) >
                   offsetof(uec_api, set_actor_property_set_element_text),
               "object set text writes must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_property_set_element_value) >
                   offsetof(uec_api, set_object_property_set_element_text),
               "actor typed set writes must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_object_property_set_element_value) >
                   offsetof(uec_api, set_actor_property_set_element_value),
               "object typed set writes must append to uec_api");
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

UEC_TEST_ASSERT(offsetof(uec_api, trace_detailed) >
                   offsetof(uec_api, set_object_property_set_element_value),
               "detailed collision tracing must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, trace_detailed_filtered) >
                   offsetof(uec_api, trace_detailed),
               "filtered detailed collision tracing must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_component_physics_velocity) >
                   offsetof(uec_api, trace_detailed_filtered),
               "component physics velocity must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, apply_component_impulse) >
                   offsetof(uec_api, set_component_physics_velocity),
               "component physics impulse must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, apply_component_force) >
                   offsetof(uec_api, apply_component_impulse),
               "component physics force must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_component_physics_angular_velocity) >
                   offsetof(uec_api, apply_component_force),
               "component angular velocity read must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_component_physics_angular_velocity) >
                   offsetof(uec_api, get_component_physics_angular_velocity),
               "component angular velocity write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, apply_component_torque) >
                   offsetof(uec_api, set_component_physics_angular_velocity),
               "component torque must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, apply_component_angular_impulse) >
                   offsetof(uec_api, apply_component_torque),
               "component angular impulse must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_physics_angular_velocity) >
                   offsetof(uec_api, apply_component_angular_impulse),
               "actor angular velocity read must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_physics_angular_velocity) >
                   offsetof(uec_api, get_actor_physics_angular_velocity),
               "actor angular velocity write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, apply_actor_torque) >
                   offsetof(uec_api, set_actor_physics_angular_velocity),
               "actor torque must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, apply_actor_angular_impulse) >
                   offsetof(uec_api, apply_actor_torque),
               "actor angular impulse must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_soft_value) >
                   offsetof(uec_api, apply_actor_angular_impulse),
               "actor soft reference value output must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_soft_value) >
                   offsetof(uec_api, get_actor_property_soft_value),
               "object soft reference value output must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_actor_property_soft_value) >
                   offsetof(uec_api, get_object_property_soft_value),
               "actor soft reference value input must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_object_property_soft_value) >
                   offsetof(uec_api, set_actor_property_soft_value),
               "object soft reference value input must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_property_map_key) > offsetof(uec_api, set_object_property_soft_value), "actor typed map key must append to uec_api"); UEC_TEST_ASSERT(offsetof(uec_api, get_object_property_map_key) > offsetof(uec_api, get_actor_property_map_key), "object typed map key must append to uec_api"); UEC_TEST_ASSERT(offsetof(uec_api, invoke_actor_function_arguments) > offsetof(uec_api, get_object_property_map_key), "mixed invocation must append to uec_api"); UEC_TEST_ASSERT(offsetof(uec_api, get_or_create_actor_event_bridge) > offsetof(uec_api, invoke_actor_function_arguments), "event bridge lookup must append to uec_api"); UEC_TEST_ASSERT(offsetof(uec_api, destroy_actor_event_bridge) > offsetof(uec_api, get_or_create_actor_event_bridge), "event bridge destruction must append to uec_api"); UEC_TEST_ASSERT(offsetof(uec_api, bind_actor_event_bridge) > offsetof(uec_api, destroy_actor_event_bridge), "event subscription must append to uec_api"); UEC_TEST_ASSERT(offsetof(uec_api, unbind_actor_event_bridge) > offsetof(uec_api, bind_actor_event_bridge), "event unsubscription must append to uec_api"); UEC_TEST_ASSERT(offsetof(uec_api, emit_actor_event_bridge) > offsetof(uec_api, unbind_actor_event_bridge), "event dispatch must append to uec_api"); UEC_TEST_ASSERT(offsetof(uec_api, invoke_actor_function_latent) > offsetof(uec_api, emit_actor_event_bridge), "latent invocation must append to uec_api"); UEC_TEST_ASSERT(offsetof(uec_api, cancel_actor_function_latent) > offsetof(uec_api, invoke_actor_function_latent), "latent cancellation must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, save_versioned_application_data) >
                   offsetof(uec_api, cancel_actor_function_latent),
               "versioned application data save must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, load_versioned_application_data) >
                   offsetof(uec_api, save_versioned_application_data),
               "versioned application data load must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_controller_enhanced_input_subsystem) >
                   offsetof(uec_api, load_versioned_application_data),
               "Enhanced Input subsystem lookup must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_component_collision_channel_response) >
                   offsetof(uec_api, get_controller_enhanced_input_subsystem),
               "three-way collision response setter must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_progress_bar_percent) >
                   offsetof(uec_api, set_component_collision_channel_response),
               "progress-bar readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_progress_bar_percent) >
                   offsetof(uec_api, get_progress_bar_percent),
               "progress-bar write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_widget_enabled) >
                   offsetof(uec_api, set_progress_bar_percent),
               "widget enabled-state read must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_widget_enabled) >
                   offsetof(uec_api, get_widget_enabled),
               "widget enabled-state write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_checkbox_state) >
                   offsetof(uec_api, set_widget_enabled),
               "checkbox readback must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, set_checkbox_state) >
                   offsetof(uec_api, get_checkbox_state),
               "checkbox state write must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_widget_child) >
                   offsetof(uec_api, set_checkbox_state),
               "UMG child lookup must append to uec_api");
