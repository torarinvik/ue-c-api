# Initial C API contract

The current runtime slice is intentionally small and versioned as ABI `1.40`.
Consumers call `uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, ...)` and use the
returned function table. The table and public structures contain only C types;
Unreal headers and C++ types stay inside the plugin.

`get_capabilities` reports the feature bits present in the loaded bridge. The
current implementation reports bootstrap, logging, world, actor, component,
timer, reflection, collision, asset loading, player flow, input, physics,
collision-query, audio, UI, camera, save-data, game-thread dispatch, and
movement, presentation, retained-object, and component-introspection adapters.

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
component order `(x, y, z, w)`. Null pointers paired with nonzero lengths,
lengths that cannot fit Unreal's `int32` string conversion, and non-finite
transform, physics, collision, audio, and movement values are rejected as
invalid arguments.

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
player-controller handle and run on the game thread. `get_input_action_value`
reads a loaded `UInputAction` through `UEnhancedPlayerInput` and returns its
current boolean, 1D, 2D, or 3D value. An action that is not currently
triggering returns zero in its configured value type; action events and
bindings remain outside this polling API. `inject_input_action_value` submits
a boolean or axis value through the same enhanced player-input path for
synthetic input and tests; it does not install persistent bindings.

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
enum, name, string, and text actor properties can be read through the typed
property functions; enum values use their underlying integer and expose their
reflected name through string reads. Arrays, maps, sets, structs, object
references, and reflected function invocation remain unsupported and return an explicit
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
before completion. At most 1024 object-load requests can be pending; callers
should cancel or await requests after `UEC_RESULT_QUEUE_FULL`. Outstanding
requests are cancelled during module shutdown.

`is_object_path_loaded` checks whether a valid soft object path currently
resolves in memory. It does not load or retain the object and is safe to use
before choosing between synchronous and asynchronous loading.

`spawn_sound_attached` creates a non-auto-destroying `UAudioComponent` attached
to a scene component and returns it as a weak object handle. The caller can
stop it with `stop_audio_component` and then release the handle. The sound and
attach-component handles are borrowed for the duration of the call.

`line_trace` maps a small stable C channel enum to Unreal collision channels and
returns a POD hit record. A hit actor, when present, is returned as an owned
weak actor handle and must be released with `release_actor`.

`line_trace_filtered` adds a caller-owned array of actor handles to ignore.
Every ignored handle must be valid for the duration of the call; the array is
borrowed and is never retained. It also rejects non-finite endpoints before
submitting the query.

`sweep_trace` applies a world-aligned sphere, box, or capsule shape between two
points and returns the first blocking hit using the same channel and hit-record
rules as `line_trace`. `overlap_shape` tests one of those shapes at a point and
returns the total number of unique actors found, copying at most `max_hits`
handles into the caller's array. Set `max_hits` to zero to query the count only;
overlap ordering is unspecified and every copied handle must be released.

`play_sound_at_location` is a game-thread, fire-and-forget adapter for a loaded
`USoundBase` object handle. It accepts volume and pitch multipliers, does not
retain the sound handle, and does not expose playback completion or replication.

`run_on_game_thread` accepts worker-thread callers and invokes the borrowed
callback on the game thread. At most 1024 callbacks can be queued at once;
submissions beyond that bound return `UEC_RESULT_QUEUE_FULL`. Cancellation
removes a pending callback before it runs, and module shutdown cancels all
remaining callbacks.

`create_widget` loads a `UUserWidget` class path and creates a weak object handle
owned by the caller. `add_widget_to_viewport` and `remove_widget_from_parent`
operate on that handle on the game thread. The widget must be kept alive by
being added to a viewport or another Unreal owner; releasing the bridge handle
does not destroy the widget.

`get_camera_field_of_view` and `set_camera_field_of_view` accept scene-component
handles that refer to `UCameraComponent` instances. Field of view is expressed
in degrees and writes are restricted to the open interval `(0, 360)`.

Object property accessors apply the same supported scalar, string, name, and text
reflection rules as actor property accessors, but accept any valid object handle.
Save-game helpers create a `USaveGame` subclass by class path, save or delete a
named slot synchronously, and load a slot only when its object is compatible
with the requested class. Save failures are returned through the `out_saved` or
`out_deleted` boolean; a missing load slot returns `UEC_RESULT_NOT_INITIALIZED`.

`run_on_game_thread` queues a borrowed callback and user pointer for execution
on Unreal's game thread and returns a request id. `cancel_game_thread_request`
can cancel a queued callback from any thread; cancellation wins if it races
with dispatch. The callback owns any handles it receives and must not retain
the borrowed user pointer after it returns. Module shutdown cancels queued
callbacks without invoking them.

`add_pawn_movement_input` forwards a world-space direction and scale to an
`APawn`; base pawns only accumulate input, while movement-capable subclasses
consume it. `jump_character` and `stop_character_jumping` require an
`ACharacter` handle and map to its built-in jump state. These calls are
game-thread-only.

`set_static_mesh` and `set_skeletal_mesh` assign already-loaded mesh object
handles to compatible scene components. Static mesh assignment reports engine
failure through `UEC_RESULT_INTERNAL_ERROR`; skeletal mesh assignment can
optionally reinitialize the animation pose. Both operations run on the game
thread and do not retain the asset handle.

`retain_object` creates a separate strong object handle backed by Unreal's
`TStrongObjectPtr`. Ordinary loaded and callback-returned object handles remain
weak; callers that need an asset or save object to survive garbage collection
must retain it and later release the retained handle with `release_object`.

`get_component_class_name` returns the full Unreal class path for a scene
component, and `component_is_a` checks it against another scene-component class
path. Both calls run on the game thread and let callers select camera, mesh,
primitive, or project-specific component adapters without guessing a type.

`attach_scene_component` attaches two components from the same world with
keep-world or keep-relative transform rules and an optional socket name.
`detach_scene_component` applies the same transform choice when removing a
parent. Both operations are game-thread-only.

`get_actor_class_name` returns an actor's full Unreal class path, while
`actor_is_a` checks inheritance against another actor class path. These queries
run on the game thread and return invalid-argument for non-actor class paths.

`add_input_mapping_context` and `remove_input_mapping_context` apply loaded
`UInputMappingContext` objects to a local player controller's Enhanced Input
subsystem. Adding accepts an integer priority; removing is idempotent at the
engine level. Action value polling, event callbacks, and binding tokens remain
outside this slice.

`get_class_function_count` and `get_class_function_at` enumerate reflected
functions, report non-return parameter counts, and identify return values and
latent functions. Function ordering follows Unreal's reflection iterator and
may change after hot reload or reinstancing; callers should re-enumerate before
invocation.

`set_component_collision_enabled` maps the stable C collision mode enum to a
primitive component's query/physics setting. `set_component_collision_response`
sets one supported trace channel to block or ignore. Both operations require a
primitive component and run on the game thread.

`play_skeletal_animation` and `stop_skeletal_animation` control the transient
animation state of skeletal mesh components using a loaded animation asset.
`set_component_material_scalar` and `set_component_material_vector` update all
matching material parameters on a mesh component; parameter names are UTF-8
views and vector values use the API's world-independent double-precision type.

## Verification

The standalone consumer in `tests/c_smoke/c_smoke.c` compiles without Unreal
headers and validates bootstrap, logging, diagnostics, ABI fields, and context
release at the source level, including the documented POD layout assertions.
Running the actor operations requires building the
plugin against a selected Unreal Engine version and executing it in a test
project, which is the next environment-dependent gate.
