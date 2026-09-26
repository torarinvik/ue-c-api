# C playable sample

`c_playable.c` is a consumer-side example that composes Enhanced Input,
collision-swept actor movement, a filtered forward trace, camera response, UMG
feedback, and versioned position save/load. The implementation is C11 and only
depends on the public C ABI plus the small named-widget helpers in
`examples/c_widget_ui`.

The host supplies a local player controller, its input-enabled pawn, a mapping
context containing an Axis2D action, a camera component, and an already-created
UMG widget. The widget tree must contain a `TextBlock` named `StatusText`, a
`ProgressBar` named `DistanceProgress`, and buttons named `SaveButton` and
`LoadButton`. The sample adds the widget to the viewport and installs/removes
the input context. Passed Unreal handles are borrowed; the consumer retains
them, the API context, the save-slot bytes, and `uec_playable_sample_state`
until `uec_playable_sample_cancel` has returned.

For example, after the host has loaded and validated its assets:

```c
uec_playable_sample_state play = {0};
uec_string_view slot = {"MyGame-Position", 16};
uec_result result = uec_playable_sample_start(
    api, context, controller, pawn, mapping, move_action, camera, widget,
    slot, 12.0, 0, &play);
/* Keep state, context, slot bytes, and all borrowed handles alive while playing. */
if (result == UEC_RESULT_OK) {
    /* ... */
    uec_playable_sample_cancel(&play); /* game thread */
}
```

The named controls show the pawn's position, whether the filtered visibility
trace ahead is blocked, and distance from the starting point. Movement is
applied once for each Enhanced Input `Triggered` callback; tune the supplied
distance for the action's trigger frequency. The camera widens in response to
the Axis2D magnitude and returns to its original FOV on `Completed`, `Canceled`,
or cleanup. Save/load buttons persist only the three position coordinates in
schema version 1 under the supplied user slot. Extend the schema version when
changing that payload; opaque application data is owned and interpreted by the
consumer.

The module does not load assets, possess the pawn, create the widget, or manage
the local player's lifetime. Those choices stay with the host project, which
can use its own Blueprint classes while consuming this C orchestration layer.
`tests/c_smoke/c_playable_smoke.c` drives the full movement, trace, camera,
widget, persistence, and cancellation flow against a deterministic C ABI mock.
