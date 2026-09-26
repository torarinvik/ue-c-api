#ifndef UEC_C_WIDGET_UI_H
#define UEC_C_WIDGET_UI_H

#include "uec_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Find a named UTextBlock child, set its contents, and release the weak handle. */
uec_result UEC_CALL uec_widget_set_text_child(const uec_api* api,
                                              uec_object* user_widget,
                                              uec_string_view child_name,
                                              uec_string_view text);

uec_result UEC_CALL uec_widget_set_editable_text_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    uec_string_view text);

uec_result UEC_CALL uec_widget_get_editable_text_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    char* buffer,
    size_t buffer_size,
    size_t* required_size);

uec_result UEC_CALL uec_widget_set_slider_child(const uec_api* api,
                                                uec_object* user_widget,
                                                uec_string_view child_name,
                                                double value);

uec_result UEC_CALL uec_widget_get_slider_child(const uec_api* api,
                                                uec_object* user_widget,
                                                uec_string_view child_name,
                                                double* out_value);

uec_result UEC_CALL uec_widget_set_combo_box_selected_option_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    uec_string_view option);

uec_result UEC_CALL uec_widget_get_combo_box_selected_option_child(
    const uec_api* api,
    uec_object* user_widget,
    uec_string_view child_name,
    char* buffer,
    size_t buffer_size,
    size_t* required_size);

#ifdef __cplusplus
}
#endif

#endif
