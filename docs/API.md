# Initial C API contract

The current runtime slice is intentionally small and versioned as ABI `1.114`.
Consumers call `uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, ...)` and use the
returned function table. The table and public structures contain only C types;
Unreal headers and C++ types stay inside the plugin.

`get_capabilities` reports the feature bits present in the loaded bridge. The
current implementation reports bootstrap, logging, world, actor, component,
timer, reflection, collision, asset loading, player flow, input, physics,
collision-query, audio, UI, camera, save-data, game-thread dispatch, movement,
presentation, retained-object, component-introspection, configuration, and
streaming adapters.

Contexts, worlds, and actors are opaque handles validated against typed
registries. Each handle receives a monotonic generation and kind tag; released
handles are tombstoned until module shutdown so a stale pointer cannot be
accepted after allocator address reuse. A world or actor handle is a
bridge-owned reference to an Unreal object that may become invalid when Unreal
destroys or unloads that object. Typed validation checks the underlying weak
object reference before every operation, including metadata-only queries, and
reports `UEC_RESULT_INVALID_HANDLE` when the referenced object is no longer
valid.
Releasing a handle releases the bridge handle; it does not destroy an Unreal
object. `destroy_actor` destroys the actor and tombstones its actor handle.

`get_last_error` requires a valid context and a non-null required-size output;
it returns the bounded diagnostic string using the same terminating-NUL buffer
contract as other text APIs.

Output pointers are cleared as soon as they are available on entry for
diagnostics, world, player, timer, streaming, actor, component, reflection,
collision, retained-object, identity, configuration, widget, audio, camera,
animation, object, save-game, path-query, and queued-request adapters.
A failed call with a non-null output pointer therefore leaves a null handle,
`UEC_FALSE`, zero, or an empty value instead of preserving stale caller data.

`get_runtime_stats` is a game-thread-only drain diagnostic. It reports the
number of registered subscriptions, pending asynchronous or game-thread
requests, consumer callbacks currently executing, and live bridge handles by
kind. Before unloading code that owns callback functions, stop submitting
work, cancel or unsubscribe everything, and wait for the first three counts to
reach zero; use the handle counts to find unreleased bridge ownership.
Each subscription category is bounded at 1024 active entries; a bind that
would exceed its category returns `UEC_RESULT_QUEUE_FULL`.

ABI minor 83 adds `get_world_count_by_kind` and `get_world_at_by_kind`. These
explicit context queries enumerate editor, PIE, game-preview, inactive, and
game world contexts without changing the active Game/PIE convenience lookup.

ABI minor 84 adds `invoke_actor_function_value`. It marshals scalar boolean,
integer, enum, float, and double arguments into reflected native or Blueprint
functions and returns the function return value or first out parameter in a
`uec_property_value`. Strings, objects, structs, containers, latent functions,
and network functions remain on the text or unsupported paths.

ABI minor 85 adds `invoke_actor_function_values`. It preserves the scalar
marshaling rules while returning the function result followed by every scalar
out parameter into a caller-sized `uec_property_value` array. The required
output count is reported before an undersized call returns
`UEC_RESULT_BUFFER_TOO_SMALL`; strings, objects, structs, containers, latent
functions, and network functions remain on the text or unsupported paths.

ABI minor 86 adds `get_class_function_parameter_at`. It reports the name,
property kind, and input/output/return/reference flags for one reflected
parameter in Unreal's function-property order, including the return property
when present. The metadata is descriptive; unsupported property kinds remain
unsupported by typed invocation.

ABI minor 87 extends `uec_runtime_stats` with live context, world, actor,
component, class, and object counts. The original four-field prefix remains
valid for older consumers; newer callers should set `struct_size` to the full
size before reading the appended fields.

ABI minor 88 adds `invoke_actor_function_text_values`. It imports positional
UTF-8 arguments through Unreal's text property conversion and returns the
function result followed by every out parameter as caller-owned UTF-8 buffers.
Each output reports its required size and reflected property kind, so strings,
names, localized text, structs, arrays, maps, and sets can use the same
bounded-buffer contract; unsupported signatures still return an explicit
unsupported result.

ABI minor 89 adds `find_object`. It resolves a full Unreal object path without
loading or retaining the object and returns `UEC_RESULT_NOT_INITIALIZED` when
the path is not currently loaded. A successful lookup returns a normal weak
object handle that the caller must release.

ABI minor 90 adds `travel_world_async` and `cancel_travel_request`. Travel
invalidates handles and world-owned subscriptions before submitting
`OpenLevel`; a one-shot callback receives the loaded world handle after
Unreal's post-load delegate fires. Travel callbacks run on the game thread,
borrow `user_data`, and are removed on cancellation or module shutdown.

ABI minor 91 adds `get_component_visible` and `get_component_active`, matching
the existing component setters with game-thread-only readback and deterministic
boolean outputs.

ABI minor 92 adds `get_class_function_flags`. It reports whether a reflected
function is Blueprint-callable, native, a Blueprint event, latent, networked,
or authority-only; consumers can reject unsupported or unsafe calls before
marshaling arguments.

ABI minor 93 adds `get_widget_visibility` and `get_text_block_text`. Both are
game-thread-only UMG readback helpers; text uses the same required-size and
NUL-terminated UTF-8 buffer contract as other string outputs.

ABI minor 94 adds `get_component_collision_enabled` and
`get_audio_component_playing`, which report the current primitive collision
mode and whether an attached audio component is playing.

ABI minor 95 adds `set_streaming_level_state_async` and
`cancel_streaming_level_request`. The request changes an existing streaming
level's target state and invokes its callback on the game thread when the
loaded and visible state reaches that target. Requests are bounded, cancellable,
and removed automatically when their world is cleaned up or the module shuts
down.

ABI minor 96 adds `get_component_collision_response`, which reports the
primitive component's response to a declared trace channel as ignore, overlap,
or block.

ABI minor 97 adds `get_config_integer` and `set_config_integer`. They use the
game INI, run on the game thread, and accept the signed 32-bit range exposed by
Unreal's integer configuration API.

ABI minor 98 adds `bind_actor_destroyed` and `unbind_actor_destroyed`. The
one-shot callback carries only its subscription id and borrowed user data, so
it cannot accidentally retain or use a destroyed actor handle.

ABI minor 99 adds `get_config_bool`, which reads a boolean from the game INI on
the game thread and clears its output before validation.

ABI minor 100 adds `get_actor_property_array_count` and
`get_actor_property_array_element_text`. They expose reflected dynamic arrays
through a count and caller-owned text values; the returned element text follows
Unreal's reflection serialization and remains valid only in the caller buffer.

ABI minor 101 adds `get_object_property_array_count` and
`get_object_property_array_element_text`. They apply the same count and
caller-owned text contract to reflected arrays on loaded or retained UObject
handles. Re-query the count after any mutation; element text is serialized by
Unreal and remains valid only in the caller buffer.

ABI minor 102 adds UObject map and set text readback. Map calls report a logical
entry count and export each key and value through separate `uec_text_output`
records; set calls report a logical element count and export one record per
element. Container iteration order is Unreal-defined and can change after any
mutation, so re-query the count and do not cache indices across changes.

ABI minor 103 adds `get_actor_property_soft_path` and
`get_object_property_soft_path`. They export reflected soft object and soft
class properties through the bounded UTF-8 path contract and report
`UEC_PROPERTY_SOFT_OBJECT` or `UEC_PROPERTY_SOFT_CLASS` respectively. The
readback does not load or retain the referenced asset.

ABI minor 104 adds actor map and set count/entry readers matching the UObject
container contract. Map keys and values and set elements use initialized
`uec_text_output` records and remain caller-owned.

ABI minor 105 adds `set_actor_property_soft_path` and
`set_object_property_soft_path`. They import Unreal soft object/class path text
only into writable reflected properties, reject read-only members, and never
retain or load the referenced asset.

ABI minor 106 adds nested struct field text readers for actor and UObject
properties. The outer property must be a reflected struct; dotted paths such as
`Transform.Location.X` may traverse nested structs, and the field kind and
serialized value are returned through the caller-owned UTF-8 buffer.

ABI minor 107 adds matching nested struct field text writers. Both the outer
struct and selected field must be writable reflected properties; dotted paths
may traverse nested structs, and Unreal imports the caller's text on the game
thread.

ABI minor 108 adds indexed array element text writers for actor and UObject
properties. The array property must be writable; callers must re-query the
count and avoid retaining indices after any mutation.

ABI minor 109 adds actor and UObject map value text writers. Keys are never
changed by these calls, and logical indices must be re-queried after mutation;
set element mutation remains unsupported until Unreal rehash behavior is
covered by the contract.

ABI minor 110 adds `get_class_property_flags`. It reports edit-const,
Blueprint-read-only, const-parameter, parameter, return, out, and reference
flags for the same reflected property order used by `get_class_property_at`.

ABI minor 111 adds typed scalar reads for reflected array elements on actors and
UObjects. Boolean, integer, enum, float, and double elements use
`uec_property_value`; compound elements remain available through the existing
text accessor.

ABI minor 112 adds the same typed scalar reads for reflected map values and set
elements. Map and set indices are logical enumeration positions and must be
re-queried after any mutation.

ABI minor 113 adds typed scalar reads for nested struct fields on actors and
UObjects. The field name accepts the same dotted path syntax as the text
accessor, and scalar leaves use `uec_property_value`.

ABI minor 114 adds typed scalar writes for reflected array elements and map
values. The same access checks, numeric range checks, and finite-value rules as
top-level property writes apply; set mutation remains separate.

World, object, class, actor, and component operations must run on Unreal's game
thread. The initial slice
returns `UEC_RESULT_WRONG_THREAD` for calls made from another thread. Queued
work is available through `run_on_game_thread`, which invokes a borrowed
callback on the game thread with cancellation and a bounded queue.

Module teardown enters a shutdown gate before canceling timers, queued work,
asset requests, save requests, and input bindings. New `uec_get_api` calls
return `UEC_RESULT_SHUTTING_DOWN`, and existing handles are rejected while the
gate is active. Consumers must stop submitting work and release their context
before unloading the plugin; callbacks already pending at teardown are
suppressed. Handle constructors also recheck the gate while registering world,
actor, component, class, and object handles, so late callbacks cannot publish
new live handles during teardown. Timer and native delegate registration also
rolls back its engine-side binding if shutdown begins before the bridge registry
entry is published. Asynchronous object-load and save-game requests perform the
same gated registry insertion before dispatch.

The module also listens for Unreal world cleanup. External teardown, PIE
restart, and non-bridge travel reuse the same timer, subscription, and
world-bound handle invalidation path; callers should reacquire handles after a
world is recreated.

Strings are UTF-8 views with an explicit byte length. The caller owns the bytes
for the duration of a call; the bridge does not retain them. Malformed UTF-8,
embedded NUL bytes, null pointers paired with nonzero lengths, and lengths that
cannot fit Unreal's `int32` conversion are rejected as invalid arguments. Transforms use
double-precision values in Unreal's world units and the Unreal quaternion
component order `(x, y, z, w)`. Null pointers paired with nonzero lengths,
non-finite transform, physics, collision, audio, and movement values are also
rejected as invalid arguments; values that will cross into Unreal float-only
parameters must also fit the engine's float range, including vector components,
quaternion components, and collision box extents. Transform writes also reject
zero-length quaternions. Boolean inputs must be
exactly `UEC_FALSE` or `UEC_TRUE`; invalid byte values are rejected rather than
silently treated as true.

World enumeration reports active Game and PIE worlds by index and labels each
handle with its world kind. `get_world_pie_instance` exposes Unreal's PIE
instance identifier (`-1` for the default/non-PIE context), allowing consumers
to distinguish simultaneous PIE worlds. `get_world_net_mode` reports whether a
world is standalone, a client, a listen server, or a dedicated server.
`get_world_has_authority` is a read-only guard for mutating workflows: it is
true for standalone, listen-server, and dedicated-server worlds, and false for
client worlds. Replication and RPC behavior remain outside this query.
Actor spawn, destruction, transform writes, possession, and server-side
physics writes return `UEC_RESULT_UNSUPPORTED` when their world is a client;
local view-target and input-prediction operations remain client-usable.
`get_world_game_mode` returns the authoritative game-mode object when one is
available; client worlds return `UEC_RESULT_UNSUPPORTED`.
`get_world_game_state` returns the active world game-state object when one is
available; worlds that have not initialized a game state return
`UEC_RESULT_NOT_INITIALIZED`.
`get_default_world` remains a convenience operation
that selects the first active world; consumers needing deterministic selection
should enumerate and retain the desired world handle.

`get_world_name` returns Unreal's current map name using the same bounded UTF-8
output convention as other names. `travel_world` submits a game-thread level
travel request through `UGameplayStatics::OpenLevel`; the call returning `OK`
means the request was submitted, not that loading has completed. The bridge
cancels timers, world-tick subscriptions, and actor-scoped collision/input
subscriptions owned by that world and immediately invalidates its world, actor,
component, and world-bound object handles; global asset handles remain valid.
Reacquire a world after travel and reacquire objects from the new world.

Collision and Enhanced Input subscriptions also install one actor-destruction
listener per owning world. If Unreal destroys an actor outside the bridge, its
actor/component handles are tombstoned and those subscriptions are removed
before the consumer can observe another callback.

Player-flow helpers use actor handles for controllers, pawns, and view targets;
the controller and target handles must belong to the same world, including the
same PIE instance.
`get_player_controller` selects an explicit local-player index, while
`get_first_player_controller` remains a convenience wrapper for index zero.
`get_world_game_instance` returns the world-scoped game-instance object as a
weak handle. Possession and view-target changes are submitted on the game
thread and require the supplied handles to reference the corresponding Unreal
types.

Input polling accepts Unreal key names such as `SpaceBar` or `Gamepad_LeftX`.
`get_input_key_down` returns the current digital state, while
`get_input_key_value` returns the controller's analog value. Both require a
player-controller handle and run on the game thread. `get_input_action_value`
reads a loaded `UInputAction` through `UEnhancedPlayerInput` and returns its
current boolean, 1D, 2D, or 3D value. Failed reads clear the value payload
after validating the caller's size tag. An action that is not currently
triggering returns zero in its configured value type; action events and
bindings remain outside this polling API. `inject_input_action_value` submits
a boolean or axis value through the same enhanced player-input path for
synthetic input and tests; it does not install persistent bindings.

Physics helpers read actor velocity and operate on a simulating primitive root
component. `get_component_velocity` also reads the current velocity of any
primitive scene component. Velocity replacement/addition, impulses, and forces return
`UEC_RESULT_UNSUPPORTED` when the actor has no simulating primitive root. Values
use Unreal world units and the API's double-precision vector type.

The actor class path passed to `spawn_actor` is an Unreal object/class path that
must be loadable in the current runtime build. A missing or non-actor class is
reported as `UEC_RESULT_INVALID_ARGUMENT`.

Actor names use UTF-8 output-buffer semantics, including a terminating NUL in
the required size. Tag checks accept a UTF-8 tag view and return an explicit
boolean result. `get_actor_tag_count` and `get_actor_tag_at` enumerate the
actor's tags with the same caller-owned output-buffer convention. The
authority-gated `set_actor_tag` entry adds or removes one non-empty tag on the
game thread; it returns `UEC_RESULT_UNSUPPORTED` for client worlds.

`get_actor_component_count_by_class` and `get_actor_component_at_by_class`
filter an actor's scene components by a loaded class path. The class must derive
from `USceneComponent`; results are returned in Unreal's component enumeration
order and each result is a weak scene-component handle.

`get_config_string` and `set_config_string` access the runtime's game INI on the
game thread. Sections and keys are non-empty UTF-8 strings; reads use the usual
caller-owned output buffer and return `UEC_RESULT_NOT_INITIALIZED` when the key
does not exist. Writes flush the game INI immediately and should be treated as
application configuration, not as a substitute for save-game data.

`get_streaming_level_count` and `get_streaming_level_at` expose the world's
current `ULevelStreaming` entries in engine order, including package name,
loaded state, and requested visibility. `set_streaming_level_state` submits
load and visibility flags for a matching package on the game thread; the call
changes streaming intent and does not wait for asynchronous loading to finish.

`is_class_path_loaded` checks whether a class object already exists in memory;
it never loads the class and therefore is safe for cook/dependency preflight.

`bind_component_hit` subscribes to the primitive component's one-shot hit event.
The callback receives a borrowed event token, an optional caller-owned weak actor
handle for the other actor, and the normal impulse. Unbind explicitly or the
subscription removes itself after the first hit; module shutdown removes all
remaining native delegates.

`get_actor_bounds` reports a caller-owned world-space origin and box extent for
an actor. `find_player_start` selects the start actor for an explicit local
player index; a missing start actor returns `UEC_RESULT_NOT_INITIALIZED`.

`get_actor_root_component` returns a separately releasable scene-component
handle. Component transforms are world transforms. Visibility and activation
changes affect the selected component and, when requested, its children.
`get_actor_component_count` and `get_actor_component_at` enumerate scene
components attached to an actor; each returned component handle must be
released independently.

Timers are owned by the selected world and run on the game thread. The bridge
does not copy `user_data`; callers must keep it valid until the timer callback
fires or `clear_timer` succeeds. One-shot timers are removed after their
callback. Looping timers remain active until cleared, their world is
invalidated, or module shutdown begins.

Class metadata is read through an opaque class handle obtained from a loadable
Unreal class path. The current metadata surface reports the class name,
inheritance checks, and reflected property names and broad property kinds. A
property index is only meaningful for the class state at the time of the call;
consumers should re-enumerate after hot reload or class reinstancing. Scalar,
enum, name, string, and text actor properties can be read through the typed
property functions; enum values use their underlying integer and expose their
reflected name through string reads. The string accessors also use Unreal's
reflected text import/export for supported structs, arrays, maps, sets, and
other property kinds that have a text representation. The serialized text is
the engine's property syntax, so callers should treat it as versioned Unreal
data rather than a stable cross-engine format. Soft object and soft class
properties have distinct kinds and explicit bounded path readers. Typed
`uec_property_value`
access remains limited to scalar and enum values. Property writes reject
reflected `EditConst`, `BlueprintReadOnly`, const-parameter, and return-value
flags. Text writes create
culture-neutral `FText` values for text properties; they do not create
localization tables. Integer and enum writes also reject values outside the
underlying property's representable range; enum writes additionally require a
declared value or valid bitfield combination. The ABI represents integers as
signed 64-bit values, so reads and writes reject unsigned values above
`INT64_MAX`. Float properties reject finite doubles outside Unreal's `float`
range before conversion.

`invoke_actor_function` supports only reflected actor functions with no
parameters, no return or out values, and no latent or network flag. The bridge
also rejects authority-only reflected functions when their actor belongs to a
client world. The appended
`invoke_actor_function_text` path accepts positional arguments in Unreal's
property text syntax and returns the function's return value, or its first out
parameter, through a bounded UTF-8 buffer. Pure out parameters are initialized
by Unreal before the call and do not consume an argument. It rejects latent and
network functions and applies the same authority-only client-world guard;
text syntax is engine-version-specific and the call remains game-thread-only.
Multiple out parameters beyond the first are executed but are not returned by
this convenience surface.

`subscribe_world_tick` registers a per-frame callback on the core Unreal
ticker and associates it with a world handle. The callback runs on the game
thread with the frame delta in seconds; it is suppressed automatically when
the world is destroyed or the module begins shutdown. `unsubscribe_world_tick`
is game-thread-only and accepts the returned subscription token. Callback
`user_data` is borrowed until the subscription is removed.

`bind_audio_finished` subscribes to a spawned audio component's native finished
delegate and returns a token. The callback fires when playback completes or is
stopped, then the one-shot subscription is removed. `unbind_audio_finished`
removes it early; both operations run on the game thread and borrow their
`user_data` until unbinding or callback delivery.

`load_object` synchronously loads an object from a runtime object path and
returns a weak opaque handle. The handle does not keep the UObject alive; calls
after Unreal unloads or destroys it return `UEC_RESULT_INVALID_HANDLE`. Object
names, full Unreal object paths, class paths, and `object_is_a` checks are
available on valid handles. Path and class-path reads are game-thread-only.

`get_actor_property_object` and `get_object_property_object` read hard reflected
object or class references and return releasable weak object handles. The
corresponding setters accept a valid compatible object handle or `NULL` to
clear the property. Soft references and container properties remain outside
this typed adapter; use the bounded string accessors when Unreal's text
serialization is an acceptable representation.

`request_object_load` uses Unreal's streamable asset manager and invokes the C
callback on the game thread. The callback owns any returned object handle and
must release it. `user_data` is borrowed until completion or cancellation;
`cancel_object_load` prevents the callback from being delivered when called
before completion. At most 1024 object-load requests can be pending; callers
should cancel or await requests after `UEC_RESULT_QUEUE_FULL`. Outstanding
requests are cancelled during module shutdown, and completion rechecks the
shutdown gate before calling consumer code.

`is_object_path_loaded` checks whether a valid soft object path currently
resolves in memory. It does not load or retain the object and is safe to use
before choosing between synchronous and asynchronous loading.

`spawn_sound_attached` creates a non-auto-destroying `UAudioComponent` attached
to a scene component and returns it as a weak object handle. The caller can
stop it with `stop_audio_component`, destroy it with
`destroy_audio_component`; destruction immediately invalidates the returned
handle, including a retained copy. The sound and
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
rules as `line_trace`. `sweep_trace_filtered` has the same behavior while
ignoring a borrowed array of valid actor handles. `overlap_shape` tests one of
those shapes at a point, deduplicates actors, and copies at most `max_hits`
handles into the caller's array. `out_count` is the number of handles written,
so it is always no greater than `max_hits`; passing zero leaves the count at
zero. `overlap_shape_filtered` applies the same bounded result contract while
ignoring a borrowed actor array. Overlap ordering is unspecified and every
copied handle must be released.

`play_sound_at_location` is a game-thread, fire-and-forget adapter for a loaded
`USoundBase` object handle. It accepts volume and pitch multipliers, does not
retain the sound handle, and does not expose playback completion or replication.

`run_on_game_thread` accepts worker-thread callers and invokes the borrowed
callback on the game thread. At most 1024 callbacks can be queued at once;
submissions beyond that bound return `UEC_RESULT_QUEUE_FULL`. Cancellation
removes a pending callback before it runs, and module shutdown cancels all
remaining callbacks. A callback that has just been dequeued is still
suppressed if shutdown begins before consumer code is entered.

`create_widget` loads a `UUserWidget` class path and creates a weak object handle
owned by the caller. `add_widget_to_viewport` and `remove_widget_from_parent`
operate on that handle on the game thread. The widget must be kept alive by
being added to a viewport or another Unreal owner; releasing the bridge handle
does not destroy the widget. `set_widget_visibility` supports visible,
collapsed, and hidden states for any `UWidget`; `set_text_block_text` updates
the text of a `UTextBlock` using a culture-neutral `FText`.
`bind_button_clicked` subscribes to a `UButton` click event and returns a
one-shot token; `unbind_button_clicked` removes it early. Click callbacks run
on the game thread and borrow their `user_data` until delivery or unbinding.

`get_camera_field_of_view` and `set_camera_field_of_view` accept scene-component
handles that refer to `UCameraComponent` instances. Field of view is expressed
in degrees and writes are restricted to the open interval `(0, 360)`.

Object property accessors apply the same supported scalar, enum, string, name,
text, and reflected text-serialization rules as actor property accessors, but
accept any valid object handle. Hard object-reference adapters remain the
preferred typed path for object references.
Save-game helpers create a `USaveGame` subclass by class path, save or delete a
named slot synchronously, and load a slot only when its object is compatible
with the requested class. Save failures are returned through the `out_saved` or
`out_deleted` boolean; a missing load slot returns `UEC_RESULT_NOT_INITIALIZED`.

`async_save_game_to_slot` and `async_load_game_from_slot` use Unreal's platform
save delegates and invoke `uec_save_game_callback` on the game thread. Pending
requests are bounded at 1024, can be cancelled by request id, and suppress the
callback when cancelled. A successful async load returns a weak save-game
object handle; retain it if it must survive beyond the callback.

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

`get_actor_count_by_class` and `get_actor_at_by_class` enumerate actors already
present in a world whose class derives from a supplied actor class path. The
enumeration order is unspecified; every returned actor handle is independently
owned and must be released.

`add_input_mapping_context` and `remove_input_mapping_context` apply loaded
`UInputMappingContext` objects to a local player controller's Enhanced Input
subsystem. Adding accepts an integer priority; removing is idempotent at the
engine level. `bind_input_action` binds a typed value callback to an actor's
`UEnhancedInputComponent` and returns a bridge binding id; unbind it with
`unbind_input_action`. Binding callbacks run on the game thread, borrow the
user pointer, and are suppressed after unbinding or module shutdown. Unbinding
from inside a callback defers native binding removal until that callback
returns. At most 1024 bindings can be active.

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
`bind_animation_finished` watches a currently playing single animation and
delivers one game-thread callback when it stops; it returns a token that can be
unbound before completion. Looping animations do not complete until they are
stopped. The adapter reports `UEC_RESULT_NOT_INITIALIZED` when no animation is
playing at bind time.
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
