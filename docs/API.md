# Initial C API contract

The current runtime slice is intentionally small and versioned as ABI `1.21`.
Consumers call `uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, ...)` and use the
returned function table. The table and public structures contain only C types;
Unreal headers and C++ types stay inside the plugin.

`get_capabilities` reports the feature bits present in the loaded bridge. The
current implementation reports bootstrap, logging, world, actor, component,
timer, reflection, collision, asset loading, player flow, input, physics,
collision-query, audio, and UI adapters.

Contexts, worlds, and actors are opaque handles validated against typed active
handle registries. A world or actor handle is a bridge-owned reference to an
Unreal object that may become invalid when Unreal destroys or unloads that
object. Every operation reports
`UEC_RESULT_INVALID_HANDLE` when the referenced object is no longer valid.
Releasing a handle releases the bridge handle; it does not destroy an Unreal
object. `destroy_actor` destroys the actor and consumes its actor handle.

World and actor operations must run on Unreal's game thread. The initial slice
returns `UEC_RESULT_WRONG_THREAD` for calls made from another thread. Queued
work and completion callbacks are deliberately deferred until the threading
phase of the implementation plan.

Strings are UTF-8 views with an explicit byte length. The caller owns the bytes
for the duration of a call; the bridge does not retain them. Transforms use
double-precision values in Unreal's world units and the Unreal quaternion
component order `(x, y, z, w)`.

World enumeration reports active Game and PIE worlds by index and labels each
handle with its world kind. `get_default_world` remains a convenience operation
that selects the first active world; consumers needing deterministic selection
should enumerate and retain the desired world handle.

`get_world_name` returns Unreal's current map name using the same bounded UTF-8
output convention as other names. `travel_world` submits a game-thread level
travel request through `UGameplayStatics::OpenLevel`; the call returning `OK`
means the request was submitted, not that loading has completed. Existing world
and object handles may become invalid during travel.

Player-flow helpers use actor handles for controllers, pawns, and view targets.
The controller lookup selects local player index zero. Possession and view-target
changes are submitted on the game thread and require the supplied handles to
reference the corresponding Unreal types.

Input polling accepts Unreal key names such as `SpaceBar` or `Gamepad_LeftX`.
`get_input_key_down` returns the current digital state, while
`get_input_key_value` returns the controller's analog value. Both require a
player-controller handle and run on the game thread.

Physics helpers read actor velocity and operate on a simulating primitive root
component. Velocity replacement/addition, impulses, and forces return
`UEC_RESULT_UNSUPPORTED` when the actor has no simulating primitive root. Values
use Unreal world units and the API's double-precision vector type.

The actor class path passed to `spawn_actor` is an Unreal object/class path that
must be loadable in the current runtime build. A missing or non-actor class is
reported as `UEC_RESULT_INVALID_ARGUMENT`.

Actor names use UTF-8 output-buffer semantics, including a terminating NUL in
the required size. Tag checks accept a UTF-8 tag view and return an explicit
boolean result.

`get_actor_root_component` returns a separately releasable scene-component
handle. Component transforms are world transforms. Visibility and activation
changes affect the selected component and, when requested, its children.
`get_actor_component_count` and `get_actor_component_at` enumerate scene
components attached to an actor; each returned component handle must be
released independently.

Timers are owned by the selected world and run on the game thread. The bridge
does not copy `user_data`; callers must keep it valid until the timer callback
fires or `clear_timer` succeeds. One-shot timers are removed after their
callback. Looping timers remain active until cleared or module shutdown.

Class metadata is read through an opaque class handle obtained from a loadable
Unreal class path. The current metadata surface reports the class name,
inheritance checks, and reflected property names and broad property kinds. A
property index is only meaningful for the class state at the time of the call;
consumers should re-enumerate after hot reload or class reinstancing. Scalar,
name, string, and text actor properties can be read through the typed
property functions. Arrays, maps, sets, structs, object references, and
reflected function invocation remain unsupported and return an explicit
unsupported result. Text writes create culture-neutral `FText` values from the
provided UTF-8 text; they do not create localization tables.

`invoke_actor_function` supports only reflected actor functions with no
parameters, no return or out values, and no latent flag. Functions with any
parameters or latent behavior return `UEC_RESULT_UNSUPPORTED` until a typed
argument and async completion ABI is available.

`load_object` synchronously loads an object from a runtime object path and
returns a weak opaque handle. The handle does not keep the UObject alive; calls
after Unreal unloads or destroys it return `UEC_RESULT_INVALID_HANDLE`. Object
names and `object_is_a` checks are available on valid handles.

`request_object_load` uses Unreal's streamable asset manager and invokes the C
callback on the game thread. The callback owns any returned object handle and
must release it. `user_data` is borrowed until completion or cancellation;
`cancel_object_load` prevents the callback from being delivered when called
before completion. Outstanding requests are cancelled during module shutdown.

`line_trace` maps a small stable C channel enum to Unreal collision channels and
returns a POD hit record. A hit actor, when present, is returned as an owned
weak actor handle and must be released with `release_actor`.

`sweep_trace` applies a world-aligned sphere, box, or capsule shape between two
points and returns the first blocking hit using the same channel and hit-record
rules as `line_trace`. `overlap_shape` tests one of those shapes at a point and
returns the total number of unique actors found, copying at most `max_hits`
handles into the caller's array. Set `max_hits` to zero to query the count only;
overlap ordering is unspecified and every copied handle must be released.

`play_sound_at_location` is a game-thread, fire-and-forget adapter for a loaded
`USoundBase` object handle. It accepts volume and pitch multipliers, does not
retain the sound handle, and does not expose playback completion or replication.

`create_widget` loads a `UUserWidget` class path and creates a weak object handle
owned by the caller. `add_widget_to_viewport` and `remove_widget_from_parent`
operate on that handle on the game thread. The widget must be kept alive by
being added to a viewport or another Unreal owner; releasing the bridge handle
does not destroy the widget.

## Verification

The standalone consumer in `tests/c_smoke/c_smoke.c` compiles without Unreal
headers and validates bootstrap, logging, diagnostics, ABI fields, and context
release at the source level, including the documented POD layout assertions.
Running the actor operations requires building the
plugin against a selected Unreal Engine version and executing it in a test
project, which is the next environment-dependent gate.
