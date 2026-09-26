# Named UMG child example

This C helper demonstrates ABI 1.148 widget text access. It looks up a named
child of an existing `UUserWidget`, uses the type-specific `UTextBlock` or
`UEditableTextBox` operation, and releases the returned weak object handle
after a successful lookup. The widget class must already be created, and calls
must run on Unreal's game thread.

Include `c_widget_ui.h`, compile `c_widget_ui.c`, and pass UTF-8 string views
whose lengths exclude any trailing NUL byte:

```c
const char child_name[] = "PlayerName";
const char initial_name[] = "Ada";
char current_name[64];
size_t required_size = 0;
uec_string_view child_name_view = {child_name, sizeof(child_name) - 1u};
uec_string_view initial_name_view = {initial_name, sizeof(initial_name) - 1u};
uec_result result = uec_widget_set_editable_text_child(
    api, widget, child_name_view, initial_name_view);
if (result == UEC_RESULT_OK) {
    result = uec_widget_get_editable_text_child(
        api, widget, child_name_view, current_name, sizeof(current_name),
        &required_size);
}
```

The display-text child must be a `UTextBlock`; editable-text children must be a
`UEditableTextBox`. The read helper follows the API's required-size and UTF-8
buffer convention. Missing children and wrong widget types return an error,
and the helper releases its child handle on every path after successful lookup.
