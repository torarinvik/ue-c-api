# Named UMG child example

This C helper demonstrates ABI 1.153 widget text, slider, and combo-box
access. It looks up a named child of an existing `UUserWidget`, uses the
type-specific `UTextBlock`, `UEditableTextBox`, `USlider`, or
`UComboBoxString` operation, and releases the returned weak object handle after
a successful lookup. The widget class must already be created, and calls must
run on Unreal's game thread.

Include `c_widget_ui.h`, compile `c_widget_ui.c`, and pass UTF-8 string views
whose lengths exclude any trailing NUL byte:

```c
const char child_name[] = "PlayerName";
const char slider_name[] = "Volume";
const char quality_name[] = "Quality";
const char quality_option[] = "High";
const char initial_name[] = "Ada";
char current_name[64];
char current_option[32];
char first_quality_option[32];
uint32_t quality_option_count = 0u;
size_t required_size = 0;
uec_string_view child_name_view = {child_name, sizeof(child_name) - 1u};
uec_string_view slider_name_view = {slider_name, sizeof(slider_name) - 1u};
uec_string_view quality_name_view = {quality_name, sizeof(quality_name) - 1u};
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
buffer convention. Slider values are normalized to `[0, 1]`. Missing children
and wrong widget types return an error. Start with an empty combo box for this
example. Options use zero-based indices; re-query the count after changing the
list. Add rejects duplicates, remove rejects absent options, and clear also
clears the selection. Each helper releases its child handle on every path after
successful lookup.
