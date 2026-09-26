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

uec_result UEC_CALL uec_widget_set_slider_child(const uec_api* api,
                                                uec_object* user_widget,
                                                uec_string_view child_name,
                                                double value)
{
    if (api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, set_slider_value) +
                               sizeof(api->set_slider_value)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->set_slider_value == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->set_slider_value(child, value);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

uec_result UEC_CALL uec_widget_get_slider_child(const uec_api* api,
                                                uec_object* user_widget,
                                                uec_string_view child_name,
                                                double* out_value)
{
    if (out_value != NULL) *out_value = 0.0;
    if (out_value == NULL || api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, get_slider_value) +
                               sizeof(api->get_slider_value)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->get_slider_value == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->get_slider_value(child, out_value);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

uec_result UEC_CALL uec_widget_set_combo_box_selected_option_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    uec_string_view option)
{
    if (api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, set_combo_box_selected_option) +
                               sizeof(api->set_combo_box_selected_option)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->set_combo_box_selected_option == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->set_combo_box_selected_option(child, option);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

uec_result UEC_CALL uec_widget_get_combo_box_selected_option_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    char* buffer,
    size_t buffer_size,
    size_t* required_size)
{
    if (required_size != NULL) *required_size = 0u;
    if (required_size == NULL || api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, get_combo_box_selected_option) +
                               sizeof(api->get_combo_box_selected_option)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->get_combo_box_selected_option == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->get_combo_box_selected_option(
        child, buffer, buffer_size, required_size);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

uec_result UEC_CALL uec_widget_get_combo_box_option_count_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    uint32_t* out_count)
{
    if (out_count != NULL) *out_count = 0u;
    if (out_count == NULL || api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, get_combo_box_option_count) +
                               sizeof(api->get_combo_box_option_count)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->get_combo_box_option_count == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->get_combo_box_option_count(child, out_count);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

uec_result UEC_CALL uec_widget_get_combo_box_option_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    uint32_t index,
    char* buffer,
    size_t buffer_size,
    size_t* required_size)
{
    if (required_size != NULL) *required_size = 0u;
    if (required_size == NULL || api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, get_combo_box_option_at) +
                               sizeof(api->get_combo_box_option_at)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->get_combo_box_option_at == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->get_combo_box_option_at(child, index, buffer, buffer_size,
                                          required_size);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

uec_result UEC_CALL uec_widget_add_combo_box_option_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    uec_string_view option)
{
    if (api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, add_combo_box_option) +
                               sizeof(api->add_combo_box_option)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->add_combo_box_option == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->add_combo_box_option(child, option);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

uec_result UEC_CALL uec_widget_remove_combo_box_option_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    uec_string_view option)
{
    if (api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, remove_combo_box_option) +
                               sizeof(api->remove_combo_box_option)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->remove_combo_box_option == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->remove_combo_box_option(child, option);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

uec_result UEC_CALL uec_widget_clear_combo_box_options_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name)
{
    if (api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, clear_combo_box_options) +
                               sizeof(api->clear_combo_box_options)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->clear_combo_box_options == NULL ||
        api->release_object == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->clear_combo_box_options(child);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

/* Preserve Unreal's unchecked, checked, and undetermined states. */
uec_result UEC_CALL uec_widget_set_checkbox_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    uec_checkbox_state state)
{
    if (api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, set_checkbox_state) +
                               sizeof(api->set_checkbox_state)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->set_checkbox_state == NULL ||
        api->release_object == NULL) return UEC_RESULT_UNSUPPORTED;
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->set_checkbox_state(child, state);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

/* Getter output is set to unchecked before any lookup can fail. */
uec_result UEC_CALL uec_widget_get_checkbox_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    uec_checkbox_state* out_state)
{
    if (out_state != NULL) *out_state = UEC_CHECKBOX_UNCHECKED;
    if (out_state == NULL || api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, get_checkbox_state) +
                               sizeof(api->get_checkbox_state)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->get_checkbox_state == NULL ||
        api->release_object == NULL) return UEC_RESULT_UNSUPPORTED;
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->get_checkbox_state(child, out_state);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

/* Progress bars use the same normalized percentage range as sliders: [0, 1]. */
uec_result UEC_CALL uec_widget_set_progress_bar_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    double percent)
{
    if (api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, set_progress_bar_percent) +
                               sizeof(api->set_progress_bar_percent)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->set_progress_bar_percent == NULL ||
        api->release_object == NULL) return UEC_RESULT_UNSUPPORTED;
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->set_progress_bar_percent(child, percent);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}

/* A failed progress lookup leaves the caller's output at zero. */
uec_result UEC_CALL uec_widget_get_progress_bar_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    double* out_percent)
{
    if (out_percent != NULL) *out_percent = 0.0;
    if (out_percent == NULL || api == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (api->struct_size < offsetof(uec_api, get_progress_bar_percent) +
                               sizeof(api->get_progress_bar_percent)) {
        return UEC_RESULT_UNSUPPORTED;
    }
    if (api->get_widget_child == NULL || api->get_progress_bar_percent == NULL ||
        api->release_object == NULL) return UEC_RESULT_UNSUPPORTED;
    uec_object* child = NULL;
    uec_result result = api->get_widget_child(user_widget, child_name, &child);
    if (result != UEC_RESULT_OK) return result;
    if (child == NULL) return UEC_RESULT_INTERNAL_ERROR;
    result = api->get_progress_bar_percent(child, out_percent);
    const uec_result release_result = api->release_object(child);
    return result == UEC_RESULT_OK ? release_result : result;
}
