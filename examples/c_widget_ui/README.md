# Named UMG child example

This C helper demonstrates ABI 1.142: it looks up a named `UTextBlock` inside
an existing `UUserWidget`, updates the text, and releases the returned weak
object handle on every path after a successful lookup. The widget class must
already be created and this call must run on Unreal's game thread.

Include `c_widget_ui.h`, compile `c_widget_ui.c`, and pass UTF-8 string views
whose lengths exclude any trailing NUL byte:

```c
const char child_name[] = "StatusText";
const char status[] = "Ready";
uec_string_view child_name_view = {child_name, sizeof(child_name) - 1u};
uec_string_view status_view = {status, sizeof(status) - 1u};
uec_result result = uec_widget_set_text_child(api, widget,
                                               child_name_view, status_view);
```

The named widget must be a `UTextBlock`. If the child does not exist or has a
different type, the bridge returns its lookup or type error without retaining
the child handle.
