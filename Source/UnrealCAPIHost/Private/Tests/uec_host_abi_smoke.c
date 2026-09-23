#include "uec_api.h"

#include <stddef.h>

static void UEC_CALL IgnoreInputActionCallback(uint64_t bindingId,
                                               uec_input_action_value value,
                                               void* userData)
{
    (void)bindingId; (void)value; (void)userData;
}

static uec_result CheckWidgetChildValidation(const uec_api* api, uec_context* context)
{
    const char childNameData[] = "Missing";
    const char embeddedNulData[] = {'A', '\0', 'B'};
    const uec_string_view childName = {childNameData, sizeof(childNameData) - 1u};
    const uec_string_view embeddedNulName = {embeddedNulData, sizeof(embeddedNulData)};
    const uec_string_view emptyName = {NULL, 0u};
    const uec_string_view nullDataName = {NULL, 1u};
    uec_object* child = (uec_object*)context;

    if (api->get_widget_child(NULL, childName, &child) != UEC_RESULT_INVALID_HANDLE ||
        child != NULL) return UEC_RESULT_INTERNAL_ERROR;
    child = (uec_object*)context;
    if (api->get_widget_child(NULL, embeddedNulName, &child) !=
            UEC_RESULT_INVALID_ARGUMENT || child != NULL) return UEC_RESULT_INTERNAL_ERROR;
    child = (uec_object*)context;
    if (api->get_widget_child(NULL, emptyName, &child) != UEC_RESULT_INVALID_ARGUMENT ||
        child != NULL) return UEC_RESULT_INTERNAL_ERROR;
    child = (uec_object*)context;
    if (api->get_widget_child(NULL, nullDataName, &child) != UEC_RESULT_INVALID_ARGUMENT ||
        child != NULL || api->get_widget_child(NULL, childName, NULL) !=
            UEC_RESULT_INVALID_ARGUMENT) return UEC_RESULT_INTERNAL_ERROR;
    return UEC_RESULT_OK;
}

uec_result UEC_CALL uec_host_smoke_bootstrap(void)
{
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL ||
        api->struct_size < offsetof(uec_api, release_context) + sizeof(api->release_context)) {
        return UEC_RESULT_INTERNAL_ERROR;
    }
    if (api->release_context == NULL) return UEC_RESULT_INTERNAL_ERROR;
    if (api->abi_major != UEC_ABI_MAJOR || api->abi_minor < UEC_ABI_MINOR ||
        api->get_capabilities == NULL || api->get_world_count_by_kind == NULL ||
        api->get_world_at_by_kind == NULL || api->log == NULL || api->set_widget_visibility == NULL ||
        api->get_checkbox_state == NULL || api->set_checkbox_state == NULL ||
        api->get_widget_child == NULL ||
        api->set_component_collision_enabled == NULL ||
        api->set_component_collision_channel_response == NULL || api->bind_input_action == NULL ||
        api->inject_input_action_value == NULL) {
        api->release_context(context);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    uec_capabilities capabilities = 0;
    result = api->get_capabilities(context, &capabilities);
    if (result == UEC_RESULT_OK && (capabilities & UEC_CAPABILITY_BOOTSTRAP) == 0)
        result = UEC_RESULT_INTERNAL_ERROR;

    uint32_t invalidWorldKindCount = 1u;
    uec_world* invalidWorldOutput = (uec_world*)context;
    uint64_t invalidInputBindingId = 1u;
    uec_input_action_value invalidInputValue = {0};
    invalidInputValue.struct_size = sizeof(invalidInputValue);
    invalidInputValue.kind = (uec_input_action_value_kind)99;
    uec_checkbox_state invalidCheckboxState = UEC_CHECKBOX_CHECKED;
    if (result == UEC_RESULT_OK &&
        (api->get_world_count_by_kind(context, (uec_world_kind)99, &invalidWorldKindCount) !=
             UEC_RESULT_INVALID_ARGUMENT || invalidWorldKindCount != 0u ||
         api->get_world_count_by_kind(context, UEC_WORLD_KIND_UNKNOWN, &invalidWorldKindCount) !=
             UEC_RESULT_INVALID_ARGUMENT || invalidWorldKindCount != 0u ||
         api->get_world_at_by_kind(context, (uec_world_kind)99, 0u, &invalidWorldOutput) !=
             UEC_RESULT_INVALID_ARGUMENT || invalidWorldOutput != NULL ||
         api->get_world_at_by_kind(context, UEC_WORLD_KIND_UNKNOWN, 0u, &invalidWorldOutput) !=
             UEC_RESULT_INVALID_ARGUMENT || invalidWorldOutput != NULL ||
         api->set_component_collision_channel_response(NULL, UEC_TRACE_VISIBILITY,
             (uec_collision_response)99) != UEC_RESULT_INVALID_ARGUMENT ||
         api->set_component_collision_channel_response(NULL, (uec_trace_channel)99,
             UEC_COLLISION_RESPONSE_IGNORE) != UEC_RESULT_INVALID_ARGUMENT ||
         api->set_widget_visibility(NULL, (uec_widget_visibility)99) != UEC_RESULT_INVALID_ARGUMENT ||
         api->get_checkbox_state(NULL, &invalidCheckboxState) != UEC_RESULT_INVALID_HANDLE ||
         invalidCheckboxState != UEC_CHECKBOX_UNCHECKED ||
         api->set_checkbox_state(NULL, (uec_checkbox_state)99) != UEC_RESULT_INVALID_ARGUMENT ||
         CheckWidgetChildValidation(api, context) != UEC_RESULT_OK ||
         api->set_component_collision_enabled(NULL, (uec_collision_enabled)99) !=
             UEC_RESULT_INVALID_ARGUMENT ||
         api->bind_input_action(NULL, NULL, (uec_input_trigger_event)99,
             &IgnoreInputActionCallback, NULL, &invalidInputBindingId) != UEC_RESULT_INVALID_ARGUMENT ||
         invalidInputBindingId != 0u ||
         api->inject_input_action_value(NULL, NULL, &invalidInputValue) !=
             UEC_RESULT_INVALID_ARGUMENT)) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK) {
        const char message[] = "UnrealCAPI C host bootstrap reached the bridge";
        const uec_string_view view = {message, sizeof(message) - 1};
        result = api->log(context, view);
    }
    const uec_result releaseResult = api->release_context(context);
    return result == UEC_RESULT_OK ? releaseResult : result;
}
