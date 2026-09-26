# Named UMG child example

This C helper demonstrates ABI 1.153 named-child access for text, slider,
checkbox, progress-bar, and combo-box widgets, plus one-shot button clicks. It
uses each widget's typed API and releases the returned weak object handle after
a successful lookup. The widget class must already be created, and calls must
run on Unreal's game thread.

The click helper binds a one-shot `UButton` callback. Keep its user data alive
until the click arrives or the token is explicitly unbound:

```c
static void UEC_CALL on_confirm_clicked(uint64_t subscription_id, void* user_data)
{
    int* confirmed = (int*)user_data;
    (void)subscription_id;
    *confirmed = 1;
}
```

Include `c_widget_ui.h`, compile `c_widget_ui.c`, and pass UTF-8 string views
whose lengths exclude any trailing NUL byte:

```c
const char child_name[] = "PlayerName";
const char slider_name[] = "Volume";
const char quality_name[] = "Quality";
const char ready_check_name[] = "ReadyCheck";
const char loading_progress_name[] = "LoadingProgress";
const char confirm_button_name[] = "ConfirmButton";
const char quality_option[] = "High";
const char initial_name[] = "Ada";
char current_name[64];
char current_option[32];
char first_quality_option[32];
uint32_t quality_option_count = 0u;
uec_checkbox_state ready_state = UEC_CHECKBOX_UNCHECKED;
double loading_progress = 0.0;
uint64_t confirm_subscription_id = 0u;
int confirmed = 0;
size_t required_size = 0;
uec_string_view child_name_view = {child_name, sizeof(child_name) - 1u};
uec_string_view slider_name_view = {slider_name, sizeof(slider_name) - 1u};
uec_string_view quality_name_view = {quality_name, sizeof(quality_name) - 1u};
uec_string_view ready_check_name_view = {
    ready_check_name, sizeof(ready_check_name) - 1u};
uec_string_view loading_progress_name_view = {
    loading_progress_name, sizeof(loading_progress_name) - 1u};
uec_string_view confirm_button_name_view = {
    confirm_button_name, sizeof(confirm_button_name) - 1u};
uec_string_view quality_option_view = {quality_option, sizeof(quality_option) - 1u};
uec_string_view initial_name_view = {initial_name, sizeof(initial_name) - 1u};
uec_result result = uec_widget_set_editable_text_child(
    api, widget, child_name_view, initial_name_view);
double volume = 0.0;
if (result == UEC_RESULT_OK) {
    result = uec_widget_set_slider_child(api, widget, slider_name_view, 0.75);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_get_editable_text_child(
        api, widget, child_name_view, current_name, sizeof(current_name),
        &required_size);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_get_slider_child(api, widget, slider_name_view, &volume);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_set_checkbox_child(
        api, widget, ready_check_name_view, UEC_CHECKBOX_CHECKED);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_get_checkbox_child(
        api, widget, ready_check_name_view, &ready_state);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_set_progress_bar_child(
        api, widget, loading_progress_name_view, 0.6);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_get_progress_bar_child(
        api, widget, loading_progress_name_view, &loading_progress);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_bind_button_clicked_child(
        api, widget, confirm_button_name_view, &on_confirm_clicked, &confirmed,
        &confirm_subscription_id);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_add_combo_box_option_child(
        api, widget, quality_name_view, quality_option_view);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_set_combo_box_selected_option_child(
        api, widget, quality_name_view, quality_option_view);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_get_combo_box_option_count_child(
        api, widget, quality_name_view, &quality_option_count);
}
if (result == UEC_RESULT_OK && quality_option_count > 0u) {
    result = uec_widget_get_combo_box_option_child(
        api, widget, quality_name_view, 0u, first_quality_option,
        sizeof(first_quality_option), &required_size);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_get_combo_box_selected_option_child(
        api, widget, quality_name_view, current_option, sizeof(current_option),
        &required_size);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_remove_combo_box_option_child(
        api, widget, quality_name_view, quality_option_view);
}
if (result == UEC_RESULT_OK) {
    result = uec_widget_clear_combo_box_options_child(
        api, widget, quality_name_view);
}
```

The display-text child must be a `UTextBlock`; editable-text children must be a
`UEditableTextBox`. The read helper follows the API's required-size and UTF-8
buffer convention. Slider and progress-bar values are normalized to `[0, 1]`;
checkbox helpers preserve unchecked, checked, and undetermined states. Missing
children and wrong widget types return an error. Start with an empty combo box
for this example. Options use zero-based indices; re-query the count after
changing the list. Add rejects duplicates, remove rejects absent options, and
clear also clears the selection. Each helper releases its child handle after
successful lookup. Click callbacks run on the game thread; the one-shot token
is retired after delivery. Call `uec_widget_unbind_button_clicked` with the
live context and token to cancel before delivery.
