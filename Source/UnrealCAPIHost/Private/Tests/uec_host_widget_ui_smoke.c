#include "c_widget_ui.h"

#include <string.h>

extern int UEC_CALL uec_host_cleanup_widget_click_last_button(void);

typedef struct uec_host_widget_ui_callback_state {
    const uec_api* api;
    uec_context* context;
    uec_runtime_stats baseline;
    uint64_t subscription_id;
    uint32_t callback_count;
    uec_result callback_stats_result;
    uec_result callback_unbind_result;
    uec_bool token_active;
} uec_host_widget_ui_callback_state;

static uec_bool WidgetStatsMatch(const uec_runtime_stats* expected,
                                 const uec_runtime_stats* actual)
{
    return actual->active_subscriptions == expected->active_subscriptions &&
        actual->pending_requests == expected->pending_requests &&
        actual->active_callbacks == expected->active_callbacks &&
        actual->live_contexts == expected->live_contexts &&
        actual->live_worlds == expected->live_worlds &&
        actual->live_actors == expected->live_actors &&
        actual->live_components == expected->live_components &&
        actual->live_classes == expected->live_classes &&
        actual->live_objects == expected->live_objects ? UEC_TRUE : UEC_FALSE;
}

static void UEC_CALL CountAndUnbindButton(
    uint64_t subscription_id, void* user_data)
{
    uec_host_widget_ui_callback_state* state =
        (uec_host_widget_ui_callback_state*)user_data;
    if (state == NULL || subscription_id != state->subscription_id) return;
    ++state->callback_count;

    uec_runtime_stats observed = {0};
    observed.struct_size = sizeof(observed);
    state->callback_stats_result = state->api->get_runtime_stats(
        state->context, &observed);
    if (state->callback_stats_result == UEC_RESULT_OK &&
        (state->baseline.active_callbacks == UINT32_MAX ||
         observed.active_callbacks != state->baseline.active_callbacks + 1u)) {
        state->callback_stats_result = UEC_RESULT_INTERNAL_ERROR;
    }

    state->callback_unbind_result = uec_widget_unbind_button_clicked(
        state->api, state->context, subscription_id);
    if (state->callback_unbind_result == UEC_RESULT_OK) {
        state->token_active = UEC_FALSE;
    }
}

static uec_result CleanupWidgetSmoke(const uec_api* api,
                                     uec_context* context,
                                     uec_world* world,
                                     uec_object* widget,
                                     uec_bool widget_added,
                                     uec_host_widget_ui_callback_state* callback,
                                     const uec_runtime_stats* baseline,
                                     uec_result result)
{
    if (callback != NULL && callback->token_active == UEC_TRUE &&
        api != NULL && context != NULL) {
        const uec_result unbind_result = uec_widget_unbind_button_clicked(
            api, context, callback->subscription_id);
        if (result == UEC_RESULT_OK && unbind_result != UEC_RESULT_OK) {
            result = unbind_result;
        }
        callback->token_active = UEC_FALSE;
    }
    if (widget_added == UEC_TRUE && api != NULL && widget != NULL &&
        api->remove_widget_from_parent != NULL) {
        const uec_result remove_result = api->remove_widget_from_parent(widget);
        if (result == UEC_RESULT_OK && remove_result != UEC_RESULT_OK) {
            result = remove_result;
        }
    }
    if (widget != NULL && api != NULL && api->release_object != NULL) {
        const uec_result release_result = api->release_object(widget);
        if (result == UEC_RESULT_OK && release_result != UEC_RESULT_OK) {
            result = release_result;
        }
    }
    if (world != NULL && api != NULL && api->release_world != NULL) {
        const uec_result release_result = api->release_world(world);
        if (result == UEC_RESULT_OK && release_result != UEC_RESULT_OK) {
            result = release_result;
        }
    }
    if (baseline != NULL && api != NULL && context != NULL &&
        api->get_runtime_stats != NULL) {
        uec_runtime_stats observed = {0};
        observed.struct_size = sizeof(observed);
        const uec_result stats_result = api->get_runtime_stats(context, &observed);
        if (result == UEC_RESULT_OK &&
            (stats_result != UEC_RESULT_OK ||
             WidgetStatsMatch(baseline, &observed) != UEC_TRUE)) {
            result = stats_result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR :
                stats_result;
        }
    }
    if (context != NULL && api != NULL && api->release_context != NULL) {
        const uec_result release_result = api->release_context(context);
        if (result == UEC_RESULT_OK && release_result != UEC_RESULT_OK) {
            result = release_result;
        }
    }
    return result;
}

uec_result UEC_CALL uec_host_widget_ui_smoke(void)
{
    static const char widget_class_path_data[] =
        "/Script/UnrealCAPIHost.ECAPIHostCleanupWidget";
    static const char text_child_data[] = "CleanupTextBlock";
    static const char editable_text_child_data[] = "CleanupEditableTextBox";
    static const char slider_child_data[] = "CleanupSlider";
    static const char checkbox_child_data[] = "CleanupCheckBox";
    static const char progress_child_data[] = "CleanupProgressBar";
    static const char combo_child_data[] = "CleanupComboBox";
    static const char button_child_data[] = "CleanupButton";
    static const char first_option_data[] = "Low";
    static const char selected_option_data[] = "High";
    static const char added_option_data[] = "Ultra";
    static const char missing_option_data[] = "Missing";
    static const char missing_child_data[] = "NoSuchChild";
    static const char unicode_text_data[] = "Widget \xCE\xBB ready";
    const uec_string_view widget_class_path = {
        widget_class_path_data, sizeof(widget_class_path_data) - 1u};
    const uec_string_view text_child = {
        text_child_data, sizeof(text_child_data) - 1u};
    const uec_string_view editable_text_child = {
        editable_text_child_data, sizeof(editable_text_child_data) - 1u};
    const uec_string_view slider_child = {
        slider_child_data, sizeof(slider_child_data) - 1u};
    const uec_string_view checkbox_child = {
        checkbox_child_data, sizeof(checkbox_child_data) - 1u};
    const uec_string_view progress_child = {
        progress_child_data, sizeof(progress_child_data) - 1u};
    const uec_string_view combo_child = {
        combo_child_data, sizeof(combo_child_data) - 1u};
    const uec_string_view button_child = {
        button_child_data, sizeof(button_child_data) - 1u};
    const uec_string_view first_option = {
        first_option_data, sizeof(first_option_data) - 1u};
    const uec_string_view selected_option = {
        selected_option_data, sizeof(selected_option_data) - 1u};
    const uec_string_view added_option = {
        added_option_data, sizeof(added_option_data) - 1u};
    const uec_string_view missing_option = {
        missing_option_data, sizeof(missing_option_data) - 1u};
    const uec_string_view missing_child = {
        missing_child_data, sizeof(missing_child_data) - 1u};
    const uec_string_view unicode_text = {
        unicode_text_data, sizeof(unicode_text_data) - 1u};
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* world = NULL;
    uec_object* widget = NULL;
    uec_bool widget_added = UEC_FALSE;
    uec_runtime_stats baseline = {0};
    uec_bool baseline_valid = UEC_FALSE;
    uec_host_widget_ui_callback_state callback = {0};
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL || api->get_runtime_stats == NULL ||
        api->get_default_world == NULL || api->create_widget == NULL ||
        api->add_widget_to_viewport == NULL ||
        api->remove_widget_from_parent == NULL || api->release_object == NULL ||
        api->release_world == NULL || api->release_context == NULL) {
        return CleanupWidgetSmoke(api, context, NULL, NULL, UEC_FALSE, NULL,
                                  NULL, UEC_RESULT_UNSUPPORTED);
    }

    baseline.struct_size = sizeof(baseline);
    result = api->get_runtime_stats(context, &baseline);
    if (result != UEC_RESULT_OK) {
        return CleanupWidgetSmoke(api, context, NULL, NULL, UEC_FALSE, NULL,
                                  NULL, result);
    }
    baseline_valid = UEC_TRUE;
    callback.api = api;
    callback.context = context;
    callback.baseline = baseline;
    callback.callback_stats_result = UEC_RESULT_INTERNAL_ERROR;
    callback.callback_unbind_result = UEC_RESULT_INTERNAL_ERROR;

    result = api->get_default_world(context, &world);
    if (result == UEC_RESULT_OK && world == NULL) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) {
        result = api->create_widget(world, widget_class_path, &widget);
        if (result == UEC_RESULT_OK && widget == NULL) result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK) {
        result = api->add_widget_to_viewport(widget, 0);
        if (result == UEC_RESULT_OK) widget_added = UEC_TRUE;
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_set_text_child(api, widget, text_child, unicode_text);
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_set_editable_text_child(
            api, widget, editable_text_child, unicode_text);
    }
    if (result == UEC_RESULT_OK) {
        char text_buffer[64] = {0};
        size_t required_size = 0u;
        result = uec_widget_get_editable_text_child(
            api, widget, editable_text_child, text_buffer, sizeof(text_buffer),
            &required_size);
        if (result == UEC_RESULT_OK &&
            (strcmp(text_buffer, unicode_text_data) != 0 ||
             required_size != sizeof(unicode_text_data))) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        char short_buffer[4] = {'X', 'X', 'X', 'X'};
        size_t required_size = 0u;
        result = uec_widget_get_editable_text_child(
            api, widget, editable_text_child, short_buffer, sizeof(short_buffer),
            &required_size);
        if (result == UEC_RESULT_BUFFER_TOO_SMALL &&
            required_size == sizeof(unicode_text_data) && short_buffer[0] == 'X') {
            result = UEC_RESULT_OK;
        } else if (result == UEC_RESULT_OK) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_set_slider_child(api, widget, slider_child, 0.375);
    }
    if (result == UEC_RESULT_OK) {
        double slider_value = -1.0;
        result = uec_widget_get_slider_child(api, widget, slider_child, &slider_value);
        if (result == UEC_RESULT_OK &&
            (slider_value < 0.37499 || slider_value > 0.37501)) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        double slider_value = -1.0;
        result = uec_widget_set_slider_child(api, widget, slider_child, 1.5);
        if (result == UEC_RESULT_INVALID_ARGUMENT) {
            result = uec_widget_get_slider_child(api, widget, slider_child, &slider_value);
            if (result == UEC_RESULT_OK &&
                (slider_value < 0.37499 || slider_value > 0.37501)) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        } else if (result == UEC_RESULT_OK) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        double missing_value = -1.0;
        result = uec_widget_get_slider_child(api, widget, missing_child, &missing_value);
        if (result == UEC_RESULT_NOT_INITIALIZED && missing_value == 0.0) {
            result = UEC_RESULT_OK;
        } else if (result == UEC_RESULT_OK) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_set_checkbox_child(
            api, widget, checkbox_child, UEC_CHECKBOX_UNDETERMINED);
    }
    if (result == UEC_RESULT_OK) {
        uec_checkbox_state checkbox_state = UEC_CHECKBOX_UNCHECKED;
        result = uec_widget_get_checkbox_child(
            api, widget, checkbox_child, &checkbox_state);
        if (result == UEC_RESULT_OK &&
            checkbox_state != UEC_CHECKBOX_UNDETERMINED) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        uec_checkbox_state checkbox_state = UEC_CHECKBOX_UNCHECKED;
        result = uec_widget_set_checkbox_child(api, widget, checkbox_child,
                                               (uec_checkbox_state)99);
        if (result == UEC_RESULT_INVALID_ARGUMENT) {
            result = uec_widget_get_checkbox_child(
                api, widget, checkbox_child, &checkbox_state);
            if (result == UEC_RESULT_OK &&
                checkbox_state != UEC_CHECKBOX_UNDETERMINED) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        } else if (result == UEC_RESULT_OK) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_set_progress_bar_child(
            api, widget, progress_child, 0.625);
    }
    if (result == UEC_RESULT_OK) {
        double progress_percent = -1.0;
        result = uec_widget_get_progress_bar_child(
            api, widget, progress_child, &progress_percent);
        if (result == UEC_RESULT_OK &&
            (progress_percent < 0.62499 || progress_percent > 0.62501)) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        double progress_percent = -1.0;
        result = uec_widget_set_progress_bar_child(api, widget, progress_child, -0.1);
        if (result == UEC_RESULT_INVALID_ARGUMENT) {
            result = uec_widget_get_progress_bar_child(
                api, widget, progress_child, &progress_percent);
            if (result == UEC_RESULT_OK &&
                (progress_percent < 0.62499 || progress_percent > 0.62501)) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        } else if (result == UEC_RESULT_OK) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        uint32_t option_count = 0u;
        result = uec_widget_get_combo_box_option_count_child(
            api, widget, combo_child, &option_count);
        if (result == UEC_RESULT_OK && option_count != 2u) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        char option_buffer[32] = {0};
        size_t required_size = 0u;
        result = uec_widget_get_combo_box_option_child(
            api, widget, combo_child, 1u, option_buffer, sizeof(option_buffer),
            &required_size);
        if (result == UEC_RESULT_OK &&
            (strcmp(option_buffer, selected_option_data) != 0 ||
             required_size != sizeof(selected_option_data))) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_add_combo_box_option_child(
            api, widget, combo_child, selected_option);
        if (result == UEC_RESULT_INVALID_ARGUMENT) result = UEC_RESULT_OK;
        else if (result == UEC_RESULT_OK) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_add_combo_box_option_child(
            api, widget, combo_child, added_option);
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_remove_combo_box_option_child(
            api, widget, combo_child, missing_option);
        if (result == UEC_RESULT_INVALID_ARGUMENT) result = UEC_RESULT_OK;
        else if (result == UEC_RESULT_OK) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        uint32_t option_count = 0u;
        result = uec_widget_get_combo_box_option_count_child(
            api, widget, combo_child, &option_count);
        if (result == UEC_RESULT_OK && option_count != 3u) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_remove_combo_box_option_child(
            api, widget, combo_child, added_option);
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_clear_combo_box_options_child(api, widget, combo_child);
    }
    if (result == UEC_RESULT_OK) {
        uint32_t option_count = UINT32_MAX;
        result = uec_widget_get_combo_box_option_count_child(
            api, widget, combo_child, &option_count);
        if (result == UEC_RESULT_OK && option_count != 0u) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_add_combo_box_option_child(
            api, widget, combo_child, first_option);
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_add_combo_box_option_child(
            api, widget, combo_child, selected_option);
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_set_combo_box_selected_option_child(
            api, widget, combo_child, selected_option);
    }
    if (result == UEC_RESULT_OK) {
        char selected_buffer[32] = {0};
        size_t required_size = 0u;
        result = uec_widget_get_combo_box_selected_option_child(
            api, widget, combo_child, selected_buffer, sizeof(selected_buffer),
            &required_size);
        if (result == UEC_RESULT_OK &&
            (strcmp(selected_buffer, selected_option_data) != 0 ||
             required_size != sizeof(selected_option_data))) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK) {
        result = uec_widget_set_slider_child(api, widget, text_child, 0.5);
        if (result != UEC_RESULT_INVALID_ARGUMENT) result = UEC_RESULT_INTERNAL_ERROR;
        else result = UEC_RESULT_OK;
    }
    if (result == UEC_RESULT_OK) {
        callback.token_active = UEC_TRUE;
        result = uec_widget_bind_button_clicked_child(
            api, widget, button_child, &CountAndUnbindButton, &callback,
            &callback.subscription_id);
        if (result != UEC_RESULT_OK || callback.subscription_id == 0u) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            else callback.token_active = UEC_FALSE;
        }
    }
    if (result == UEC_RESULT_OK &&
        uec_host_cleanup_widget_click_last_button() == 0) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK &&
        (callback.callback_count != 1u ||
         callback.callback_stats_result != UEC_RESULT_OK ||
         callback.callback_unbind_result != UEC_RESULT_OK ||
         callback.token_active != UEC_FALSE)) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK &&
        uec_host_cleanup_widget_click_last_button() == 0) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK && callback.callback_count != 1u) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK) {
        callback.token_active = UEC_FALSE;
        result = uec_widget_bind_button_clicked_child(
            api, widget, button_child, &CountAndUnbindButton, &callback,
            &callback.subscription_id);
        if (result == UEC_RESULT_OK && callback.subscription_id != 0u) {
            callback.token_active = UEC_TRUE;
            result = uec_widget_unbind_button_clicked(
                api, context, callback.subscription_id);
            if (result == UEC_RESULT_OK) callback.token_active = UEC_FALSE;
        } else if (result == UEC_RESULT_OK) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result == UEC_RESULT_OK &&
        uec_host_cleanup_widget_click_last_button() == 0) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK && callback.callback_count != 1u) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }

    return CleanupWidgetSmoke(api, context, world, widget, widget_added,
                              &callback, baseline_valid == UEC_TRUE ? &baseline : NULL,
                              result);
}
