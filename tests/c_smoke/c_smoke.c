#include "uec_api.h"

#include <stdio.h>
#include <string.h>

uec_result UEC_CALL uec_host_smoke_bootstrap(void);

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
        (capabilities & UEC_CAPABILITY_STREAMING) == 0 ||
        (capabilities & UEC_CAPABILITY_REFLECTION_CONTAINERS) == 0 ||
        (capabilities & UEC_CAPABILITY_COLLISION_DETAILS) == 0 ||
        (capabilities & UEC_CAPABILITY_PHYSICS) == 0 ||
        api->trace_detailed == NULL || api->trace_detailed_filtered == NULL ||
        api->get_actor_property_soft_value == NULL || api->get_object_property_soft_value == NULL ||
        api->set_actor_property_soft_value == NULL || api->set_object_property_soft_value == NULL ||
        api->get_actor_property_map_key == NULL || api->get_object_property_map_key == NULL ||
        api->invoke_actor_function_arguments == NULL ||
        api->get_or_create_actor_event_bridge == NULL || api->destroy_actor_event_bridge == NULL ||
        api->bind_actor_event_bridge == NULL || api->unbind_actor_event_bridge == NULL ||
        api->emit_actor_event_bridge == NULL ||
        api->set_component_physics_velocity == NULL || api->apply_component_impulse == NULL ||
        api->apply_component_force == NULL || api->get_component_physics_angular_velocity == NULL ||
        api->set_component_physics_angular_velocity == NULL || api->apply_component_torque == NULL ||
        api->apply_component_angular_impulse == NULL || api->get_actor_physics_angular_velocity == NULL ||
        api->set_actor_physics_angular_velocity == NULL || api->apply_actor_torque == NULL ||
        api->apply_actor_angular_impulse == NULL)
    {
        api->release_context(context);
        return 5;
    }

    uec_hit_result_details details;
    memset(&details, 0, sizeof(details));
    details.struct_size = sizeof(details);
    details.hit.blocking_hit = UEC_TRUE;
    details.item = 42;
    details.face_index = 42;
    const uec_vector3 trace_start = {0.0, 0.0, 0.0};
    const uec_vector3 trace_end = {1.0, 1.0, 1.0};
    if (api->trace_detailed(NULL, trace_start, trace_end, NULL,
                            UEC_TRACE_VISIBILITY, UEC_FALSE, &details) !=
            UEC_RESULT_UNSUPPORTED ||
        api->trace_detailed_filtered(NULL, trace_start, trace_end, NULL,
                                     UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &details) !=
            UEC_RESULT_UNSUPPORTED || details.hit.blocking_hit != UEC_FALSE ||
        details.item != -1 || details.face_index != -1 || details.component != NULL)
    {
        api->release_context(context);
        return 54;
    }

    const uec_vector3 physics_value = {0.0, 0.0, 0.0};
    if (api->set_component_physics_velocity(NULL, physics_value, UEC_FALSE) !=
            UEC_RESULT_INVALID_HANDLE ||
        api->apply_component_impulse(NULL, physics_value, UEC_FALSE) !=
            UEC_RESULT_INVALID_HANDLE ||
        api->apply_component_force(NULL, physics_value) != UEC_RESULT_INVALID_HANDLE)
    {
        api->release_context(context);
        return 55;
    }

    uec_vector3 angular_velocity = {42.0, 42.0, 42.0};
    if (api->get_component_physics_angular_velocity(NULL, &angular_velocity) !=
            UEC_RESULT_INVALID_HANDLE ||
        api->set_component_physics_angular_velocity(NULL, physics_value, UEC_FALSE) !=
            UEC_RESULT_INVALID_HANDLE ||
        api->apply_component_torque(NULL, physics_value, UEC_FALSE) !=
            UEC_RESULT_INVALID_HANDLE ||
        api->apply_component_angular_impulse(NULL, physics_value, UEC_FALSE) !=
            UEC_RESULT_INVALID_HANDLE || angular_velocity.x != 0.0 ||
        angular_velocity.y != 0.0 || angular_velocity.z != 0.0)
    {
        api->release_context(context);
        return 56;
    }

    uec_vector3 actor_angular_velocity = {42.0, 42.0, 42.0};
    if (api->get_actor_physics_angular_velocity(NULL, &actor_angular_velocity) !=
            UEC_RESULT_INVALID_HANDLE ||
        api->set_actor_physics_angular_velocity(NULL, physics_value, UEC_FALSE) !=
            UEC_RESULT_INVALID_HANDLE ||
        api->apply_actor_torque(NULL, physics_value, UEC_FALSE) != UEC_RESULT_INVALID_HANDLE ||
        api->apply_actor_angular_impulse(NULL, physics_value, UEC_FALSE) !=
            UEC_RESULT_INVALID_HANDLE || actor_angular_velocity.x != 0.0 ||
        actor_angular_velocity.y != 0.0 || actor_angular_velocity.z != 0.0)
    {
        api->release_context(context);
        return 57;
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

    uint32_t mixed_output_count = 42u;
    result = api->invoke_actor_function_arguments(NULL, empty_function_name, NULL, 0u,
                                                  NULL, 0u, &mixed_output_count);
    if (result != UEC_RESULT_UNSUPPORTED || mixed_output_count != 0u ||
        api->invoke_actor_function_arguments(NULL, empty_function_name, NULL, 0u,
                                             NULL, 0u, NULL) != UEC_RESULT_INVALID_ARGUMENT)
    {
        api->release_context(context);
        return 55;
    }

    uec_object* event_bridge = (uec_object*)1;
    uint64_t event_subscription_id = 42u;
    if (api->get_or_create_actor_event_bridge(NULL, &event_bridge) != UEC_RESULT_UNSUPPORTED ||
        event_bridge != NULL || api->destroy_actor_event_bridge(NULL) != UEC_RESULT_INVALID_HANDLE ||
        api->bind_actor_event_bridge(NULL, NULL, NULL, &event_subscription_id) != UEC_RESULT_INVALID_ARGUMENT ||
        event_subscription_id != 0u ||
        api->unbind_actor_event_bridge(context, 1u) != UEC_RESULT_UNSUPPORTED ||
        api->emit_actor_event_bridge(NULL, 1, 0, 0.0, empty_function_name) != UEC_RESULT_INVALID_HANDLE)
    {
        api->release_context(context);
        return 56;
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
    uec_property_value map_key_value = {sizeof(map_key_value), UEC_PROPERTY_STRING,
                                        UEC_FALSE, {0u, 0u, 0u}, 42, 42.0};
    uec_text_output map_key = {sizeof(map_key), UEC_PROPERTY_STRING, NULL, 0u, 42u};
    uec_text_output map_value = {sizeof(map_value), UEC_PROPERTY_STRING, NULL, 0u, 42u};
    uec_text_output set_element = {sizeof(set_element), UEC_PROPERTY_STRING, NULL, 0u, 42u};
    if (api->get_object_property_map_count(NULL, streaming_package, &map_count) != UEC_RESULT_UNSUPPORTED ||
        map_count != 0u ||
        api->get_actor_property_map_key(NULL, streaming_package, 0u, &map_key_value) !=
            UEC_RESULT_UNSUPPORTED || map_key_value.kind != UEC_PROPERTY_UNKNOWN ||
        map_key_value.integer_value != 0 || map_key_value.real_value != 0.0 ||
        api->get_object_property_map_key(NULL, streaming_package, 0u, &map_key_value) !=
            UEC_RESULT_UNSUPPORTED || map_key_value.kind != UEC_PROPERTY_UNKNOWN ||
        map_key_value.integer_value != 0 || map_key_value.real_value != 0.0 ||
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

    uec_text_output soft_value = {sizeof(soft_value), UEC_PROPERTY_STRING,
                                  NULL, 0u, 42u};
    if (api->get_actor_property_soft_value(NULL, streaming_package, &soft_value) !=
            UEC_RESULT_UNSUPPORTED || soft_value.kind != UEC_PROPERTY_UNKNOWN ||
        soft_value.required_size != 0u ||
        api->get_object_property_soft_value(NULL, streaming_package, &soft_value) !=
            UEC_RESULT_UNSUPPORTED || soft_value.kind != UEC_PROPERTY_UNKNOWN ||
        soft_value.required_size != 0u ||
        api->set_actor_property_soft_value(NULL, streaming_package,
                                           UEC_PROPERTY_SOFT_OBJECT, streaming_package) !=
            UEC_RESULT_INVALID_HANDLE ||
        api->set_object_property_soft_value(NULL, streaming_package,
                                            UEC_PROPERTY_SOFT_CLASS, streaming_package) !=
            UEC_RESULT_INVALID_HANDLE)
    {
        api->release_context(context);
        return 58;
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

    if (api->set_actor_property_struct_field_value(NULL, streaming_package, streaming_package,
                                                   &array_value) != UEC_RESULT_UNSUPPORTED ||
        api->set_object_property_struct_field_value(NULL, streaming_package, streaming_package,
                                                    &array_value) != UEC_RESULT_UNSUPPORTED)
    {
        api->release_context(context);
        return 42;
    }

    size_t default_required = 42u;
    uec_property_kind default_kind = UEC_PROPERTY_STRING;
    if (api->get_class_property_default_text(NULL, 0u, NULL, 0u, &default_required,
                                             &default_kind) != UEC_RESULT_UNSUPPORTED ||
        default_required != 0u || default_kind != UEC_PROPERTY_UNKNOWN)
    {
        api->release_context(context);
        return 43;
    }

    if (api->get_class_property_reference_class_path(NULL, 0u, NULL, 0u,
                                                     &default_required, &default_kind) !=
            UEC_RESULT_UNSUPPORTED || default_required != 0u ||
        default_kind != UEC_PROPERTY_UNKNOWN)
    {
        api->release_context(context);
        return 44;
    }

    uint32_t enum_count = 99u;
    if (api->get_class_property_enum_value_count(NULL, 0u, &enum_count) !=
            UEC_RESULT_UNSUPPORTED || enum_count != 0u)
    {
        api->release_context(context);
        return 45;
    }
    int64_t enum_value = 99;
    size_t enum_required = 99u;
    if (api->get_class_property_enum_value_at(NULL, 0u, 0u, NULL, 0u,
                                              &enum_required, &enum_value) !=
            UEC_RESULT_UNSUPPORTED || enum_required != 0u || enum_value != 0)
    {
        api->release_context(context);
        return 46;
    }

    uint32_t struct_field_count = 99u;
    if (api->get_class_property_struct_field_count(NULL, 0u, &struct_field_count) !=
            UEC_RESULT_UNSUPPORTED || struct_field_count != 0u)
    {
        api->release_context(context);
        return 47;
    }
    uec_property_kind struct_field_kind = UEC_PROPERTY_STRING;
    uint32_t struct_field_flags = 99u;
    if (api->get_class_property_struct_field_at(
            NULL, 0u, 0u, NULL, 0u, &enum_required, &struct_field_kind,
            &struct_field_flags) != UEC_RESULT_UNSUPPORTED || enum_required != 0u ||
        struct_field_kind != UEC_PROPERTY_UNKNOWN || struct_field_flags != 0u)
    {
        api->release_context(context);
        return 48;
    }

    uec_class* property_class = (uec_class*)1;
    if (api->get_actor_property_class(NULL, streaming_package, &property_class) !=
            UEC_RESULT_UNSUPPORTED || property_class != NULL ||
        api->get_object_property_class(NULL, streaming_package, &property_class) !=
            UEC_RESULT_UNSUPPORTED || property_class != NULL)
    {
        api->release_context(context);
        return 49;
    }
    if (api->set_actor_property_class(NULL, streaming_package, NULL) != UEC_RESULT_UNSUPPORTED ||
        api->set_object_property_class(NULL, streaming_package, NULL) != UEC_RESULT_UNSUPPORTED)
    {
        api->release_context(context);
        return 50;
    }

    size_t struct_path_required = 99u;
    uec_property_kind struct_path_kind = UEC_PROPERTY_STRING;
    if (api->get_class_property_struct_path(NULL, 0u, NULL, 0u,
                                            &struct_path_required, &struct_path_kind) !=
            UEC_RESULT_UNSUPPORTED || struct_path_required != 0u ||
        struct_path_kind != UEC_PROPERTY_UNKNOWN)
    {
        api->release_context(context);
        return 51;
    }

    uec_property_kind container_key_kind = UEC_PROPERTY_STRING;
    uec_property_kind container_value_kind = UEC_PROPERTY_STRING;
    if (api->get_class_property_container_kinds(
            NULL, 0u, &container_key_kind, &container_value_kind) !=
            UEC_RESULT_UNSUPPORTED || container_key_kind != UEC_PROPERTY_UNKNOWN ||
        container_value_kind != UEC_PROPERTY_UNKNOWN)
    {
        api->release_context(context);
        return 52;
    }

    if (api->set_actor_property_set_element_text(NULL, streaming_package, 0u, streaming_package) !=
            UEC_RESULT_UNSUPPORTED ||
        api->set_object_property_set_element_text(NULL, streaming_package, 0u, streaming_package) !=
            UEC_RESULT_UNSUPPORTED ||
        api->set_actor_property_set_element_value(NULL, streaming_package, 0u, NULL) !=
            UEC_RESULT_INVALID_ARGUMENT ||
        api->set_object_property_set_element_value(NULL, streaming_package, 0u,
                                                   &invocation_result) != UEC_RESULT_UNSUPPORTED)
    {
        api->release_context(context);
        return 53;
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
    if (result != UEC_RESULT_OK || required == 0 || strcmp(error, "Invalid context handle") != 0)
    {
        api->release_context(context);
        return 3;
    }

    if (api->struct_size < sizeof(uec_api) || api->abi_major != UEC_ABI_MAJOR)
    {
        api->release_context(context);
        return 6;
    }

    if (uec_host_smoke_bootstrap() != UEC_RESULT_OK)
    {
        api->release_context(context);
        return 54;
    }

    api->release_context(context);
    puts("uec C ABI smoke test passed");
    return 0;
}
