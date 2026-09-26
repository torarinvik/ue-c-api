#include "c_widget_ui.h"

#include <stddef.h>

uec_result UEC_CALL uec_widget_set_text_child(const uec_api* api,
                                              uec_object* user_widget,
                                              uec_string_view child_name,
                                              uec_string_view text)
{
    if (api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, get_widget_child) +
                               sizeof(api->get_widget_child)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->set_text_block_text == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }

    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;

    result = api->set_text_block_text(child, text);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

uec_result UEC_CALL uec_widget_set_editable_text_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    uec_string_view text)
{
    if (api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, set_editable_text_box_text) +
                               sizeof(api->set_editable_text_box_text)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->set_editable_text_box_text == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }

    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;

    result = api->set_editable_text_box_text(child, text);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

uec_result UEC_CALL uec_widget_get_editable_text_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    char* buffer,
    size_t buffer_size,
    size_t* required_size)
{
    if (required_size != NULL) *required_size = 0u;
    if (required_size == NULL || api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, get_editable_text_box_text) +
                               sizeof(api->get_editable_text_box_text)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->get_editable_text_box_text == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }

    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;

    result = api->get_editable_text_box_text(child, buffer, buffer_size, required_size);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}
