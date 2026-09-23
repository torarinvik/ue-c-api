# C consumer integration

Unreal loads `UnrealCAPI` as an in-process runtime module. A consumer includes
`Plugins/UnrealCAPI/Source/UnrealCAPI/Public/uec_api.h` and receives the function table from the
exported bootstrap entry point after the host has initialized the engine:

```c
const uec_api* api = NULL;
uec_context* context = NULL;
uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
if (result != UEC_RESULT_OK) {
    /* The host can report the failure or retry after module startup. */
}
```

The minimal host project also contains a tracked C translation unit at
`Source/UnrealCAPIHost/Private/uec_host_smoke.c`. It performs bootstrap,
capability, logging, and context-release calls from C, then runs event-bridge
and latent completion, cancellation, signature-validation, scalar and
text-backed mixed invocation, mixed-output capacity preflight and ordering,
pure out parameters, mixed-call argument-count and scalar-kind rejection,
short text-output sizing and retry, invalid world-kind handling, explicit
world-context, cross-world rejection when an editor world is available,
stale actor and bridge handle rejection, and pending-request drain probes after
a Game or PIE world becomes available. A
packaged or PIE run is still required to verify those paths on an installed
target engine.

The returned table is owned by the plugin and remains valid until the module
is unloaded. The context is a bridge handle and must be released through
`api->release_context`. Consumers call through the table rather than linking
against private C++ symbols or Unreal headers.
The runtime module is built with C++ exceptions disabled, so no C++ exception
may cross the C ABI. Unreal assertions and fatal errors remain process-level
failures.

## Compatibility

The requested major version must match the bridge. A consumer may request an
older minor version to use an earlier prefix of the append-only `uec_api`
table. Before calling an optional entry, compare its byte offset with
`api->struct_size`; an older bridge can return a smaller table. The
`abi_major`, `abi_minor`, and `struct_size` fields identify the table that was
actually returned.

The appended `get_runtime_stats` entry is available in ABI minor 82. It is a
game-thread drain check for consumers that may unload callback code: after
stopping new work and canceling requests or subscriptions, poll its
`active_subscriptions`, `pending_requests`, and `active_callbacks` fields until
all are zero. ABI 87 appends `live_contexts`, `live_worlds`, `live_actors`,
`live_components`, `live_classes`, and `live_objects`; initialize the full
`struct_size` to read them and use the original prefix when targeting older
bridges.

The bridge checks null/count consistency, size-tagged structures, and opaque
handle membership. It cannot determine whether an arbitrary non-null pointer
from the caller is readable or whether its allocation is as large as the
declared buffer capacity or element count. Keep every caller-owned buffer and
array valid for the full call and large enough for the sizes you provide;
invalid memory can crash the Unreal process.

The ABI 83 `get_world_count_by_kind` and `get_world_at_by_kind` entries expose
explicit editor, PIE, game-preview, inactive, and game-world selection. The
original Game/PIE lookup remains the convenience path for active gameplay.
ABI 84 adds scalar `invoke_actor_function_value`; initialize every argument and
return `uec_property_value` with its `struct_size` before calling it.
ABI 85 adds `invoke_actor_function_values`; initialize every argument and each
caller-provided output slot with its `struct_size`, call once with a capacity,
and retry with the reported count when the result is
`UEC_RESULT_BUFFER_TOO_SMALL`. Outputs are ordered as the return value first,
then reflected scalar out parameters.
ABI 86 adds `get_class_function_parameter_at`; its parameter index follows
Unreal's reflected property order, including a return property when present.
Inspect its flags before selecting the scalar or text invocation path.
ABI 88 adds `invoke_actor_function_text_values`; initialize each
`uec_text_output` with its full `struct_size`, provide caller-owned buffers (or
zero capacity for a required-size probe), and inspect `required_size` and
`kind` after the call. Outputs are ordered as the return value first, followed
by reflected out parameters.
ABI 89 adds `find_object`; it performs a game-thread-only lookup against loaded
full object paths and never loads or retains an object. Treat
`UEC_RESULT_NOT_INITIALIZED` as a cache miss, then choose `load_object` or an
async request explicitly when loading is intended.
ABI 90 adds `travel_world_async`; the callback runs on the game thread after
the post-load map delegate, receives a newly created world handle, and borrows
`user_data`. Release the world handle received by the callback. Calling
`cancel_travel_request` removes the completion callback and request tracking,
but does not stop an `OpenLevel` submission that has already started or restore
the old world's invalidated handles. Module shutdown removes all remaining
travel completion callbacks.
ABI 91 adds component visibility and activation readback; both functions clear
their boolean output before validating the component and require the game
thread.
ABI 92 adds `get_class_function_flags`; inspect its bit flags together with
parameter metadata before invoking a reflected function. Latent, network, and
authority-only flags identify paths that the scalar and text adapters reject.
ABI 93 adds UMG visibility and `UTextBlock` text readback; query the required
text size first, then provide a caller-owned UTF-8 buffer.
ABI 94 adds collision-mode and audio-playing readback to pair with the common
component and audio mutators.
ABI 95 adds streaming-level state completion requests. Keep the request id
until the callback or explicit cancellation; callbacks report the observed
loaded and visible state and run on the game thread.
ABI 96 adds per-channel collision response readback, including overlap values
that the boolean setter cannot express.
ABI 97 adds game-INI integer get/set helpers; values outside Unreal's signed
32-bit configuration range are rejected before writing.
ABI 98 adds one-shot actor-destruction callbacks. Unbind while the actor is
alive when possible; world cleanup and module shutdown remove remaining
subscriptions without invoking consumer code.
ABI 99 adds boolean game-INI readback alongside the existing string and
integer configuration helpers.
ABI 100 adds reflected actor-array count and text-element readback. Treat the
array as invalidated after any mutation and query the count again before using
later indices.
ABI 101 adds the same count and text-element readback for reflected arrays on
loaded or retained UObject handles. Re-query after mutation and keep returned
text only in the caller-owned buffer.
ABI 102 adds UObject map and set counts with text export through initialized
`uec_text_output` records. Map key/value and set element text is caller-owned;
iteration order is Unreal-defined and indices must be re-queried after mutation.
ABI 103 adds bounded soft object/class path readback for actor and UObject
properties. These calls report distinct soft-reference kinds and never load or
retain the referenced asset.
ABI 104 adds actor map/set counts and text entries. The container adapters are
append-only and invalidate indices after mutation; re-query before later reads.
ABI 105 adds soft object/class path writes for writable reflected properties;
the path is imported on the game thread and the referenced asset is not loaded
or retained by the bridge.
ABI 106 adds nested-struct field text reads, including dotted paths such as
`Transform.Location.X`. Re-enumerate or retry after hot reload and treat the
serialized field text as Unreal-versioned data.
ABI 107 adds matching nested-struct field writes, which reject read-only outer
structs and fields before importing text.
ABI 108 adds indexed reflected array element writes. Re-query counts after each
mutation and treat imported text as Unreal-versioned property syntax.
ABI 109 adds map value writes without changing keys. Set mutation remains
unsupported; re-query map counts after writes before using later indices.
ABI 110 adds class-property access flags so consumers can inspect editability
and parameter semantics before issuing reflected writes.
ABI 111 adds typed scalar reads for reflected array elements. Boolean, integer,
enum, float, and double elements use `uec_property_value`; compound elements
remain on the text accessor, and array indices must still be re-queried after
mutation.
ABI 112 adds typed scalar reads for reflected map values and set elements. Map
and set indices are logical enumeration positions; re-query counts after any
mutation before using a later index.
ABI 113 adds typed scalar reads for nested struct fields, including dotted paths
such as `Transform.Location.X`; compound leaves remain on the text accessor.
ABI 114 adds typed scalar writes for reflected array elements and map values.
Writes reject read-only containers and invalid scalar ranges before mutation.
ABI 115 adds typed scalar writes for nested struct fields, including dotted
paths such as `Transform.Location.X`.
ABI 116 adds class-default property text readback using the reflected property
order, so consumers can inspect defaults without constructing an instance.
ABI 117 adds referenced-class paths for object, class, and soft-reference
properties, allowing consumers to validate target classes before writes.
ABI 118 adds enumeration of reflected enum names and signed values, including
byte-backed enums, so consumers can build validated selection controls.
ABI 119 adds reflected struct-field enumeration with field kinds and access
flags, using the same IncludeSuper ordering as the property metadata calls.
ABI 120 adds typed hard class-property reads and writes through `uec_class*`
handles; the reflected `MetaClass` constraint is checked before mutation.
ABI 121 adds the reflected `UStruct` path for struct properties, allowing a
consumer to identify the field schema it is about to enumerate.
ABI 122 adds reflected container kind metadata. Arrays and sets report their
element kind in `out_value_kind`; maps report both key and value kinds. The
unused output for arrays and sets is `UEC_PROPERTY_UNKNOWN`, and the call
returns `UEC_RESULT_UNSUPPORTED` for non-container properties.
ABI 123 adds set element replacement through text and typed scalar writers.
The bridge rejects a replacement that duplicates another set element, rehashes
a successful replacement, and invalidates all cached logical indices.

ABI 124 adds `trace_detailed` and `trace_detailed_filtered`, append-only
collision query entries. Pass a null shape for a line trace or a size-initialized
sphere, box, or capsule descriptor for a sweep. The returned `uec_hit_result_details` owns any actor and component
handles until the consumer releases them; both calls run on the game thread, and
the filtered variant rejects invalid ignored-actor handles. They return
`UEC_RESULT_UNSUPPORTED` when the bridge does not advertise
`UEC_CAPABILITY_COLLISION_DETAILS`.
ABI 125 adds append-only scene-component physics operations for setting linear
velocity, applying impulses, and applying forces. They require a simulating
primitive component, game-thread execution, world authority, and finite vector
arguments.
ABI 126 adds angular-velocity readback plus angular-velocity and torque writes;
the readback is game-thread-only, while writes also require world authority.
ABI 127 adds angular impulse application in radians to the same authority-gated
simulating-component physics boundary.
ABI 128 adds the same angular readback and mutation operations through the
actor's simulating primitive root.
ABI 129 adds typed output records for actor and UObject soft object/class
properties. Initialize `uec_text_output.struct_size`; the `kind` distinguishes
soft object from soft class, and the path is copied to the caller's buffer.
Typed write entries require a matching kind and reject mismatched soft-object
or soft-class properties.
ABI 130 adds typed scalar map-key reads for actor and UObject properties. Use
the existing text-entry calls for map keys whose kinds do not fit
`uec_property_value`, and re-query map indices after mutations.
ABI 131 adds `invoke_actor_function_arguments` for mixed signatures. Zero-
initialize each argument and output record, then set `struct_size`; use scalar fields, a hard
object/world handle, a class handle, or Unreal property text according to the
reflected kind. Non-null handle or text fields that do not match the selected
kind are rejected. A null object, world, or class handle passes a null
reference.
Text input follows Unreal's property syntax, including quoted strings. Output
handles are caller-owned and must be released with `release_object` or
`release_class`. A short output array is rejected before invocation, while a
short per-value text buffer is reported after the function has run; do not
retry a side-effecting call solely to grow those text buffers.
For a world-context parameter, pass a non-null world handle positionally.
Mixed and latent calls reject world handles and world-bound object handles
from a different world than the target actor. The bridge does not inspect
editor-only UFunction metadata, so callers must follow each function's
reflected parameter contract.
ABI minor 132 adds a local actor event component. Retrieve or create it with
`get_or_create_actor_event_bridge`, bind a synchronous game-thread C callback,
and emit through C or its Blueprint-callable `EmitEvent`; Blueprint graphs may
bind `OnEvent`. Callback text and `user_data` are borrowed for the callback
only. Self-unbind is supported, subscriptions are bounded at 1024, and actor,
component, world, travel, and module teardown cancel them. Component creation
and explicit destruction require authority. The component does not replicate;
an already-present component can be retrieved without authority.
Check `UEC_CAPABILITY_EVENT_BRIDGE` before depending on these entries.
ABI minor 133 adds `invoke_actor_function_latent` and
`cancel_actor_function_latent`; check `UEC_CAPABILITY_ASYNC_LATENT_FUNCTIONS`.
Pass a size-initialized mixed argument array for every non-latent input
parameter. The function must expose one `FLatentActionInfo`; return, out, and
reference parameters are unsupported. Pass a world handle for world-context
parameters; any world-bound input handle must resolve to the target actor's
world. Completion runs on the game thread and borrows `user_data`. Cancellation
suppresses the callback and requests removal
from the world's latent-action manager, but Unreal may finish an action already
being processed. Actor/world teardown, travel, and plugin shutdown cancel
pending requests.
ABI minor 134 adds typed `FVector`, `FQuat`, and `FTransform` values to the
size-tagged mixed-call records. Set `kind` to `UEC_PROPERTY_STRUCT` and select
`struct_value.kind` on arguments; write the matching
`struct_value.value.vector3`, `.quaternion`, or `.transform` member. Set the
desired kind on an output to request a typed result. `UEC_FUNCTION_STRUCT_NONE`
keeps the existing text-backed behavior. The new fields extend the ABI 1.133
prefix, so old record sizes remain supported. Vector/transform components
must be finite and quaternion values nonzero.
ABI minor 135 adds `save_versioned_application_data` and
`load_versioned_application_data`. Supply a nonzero schema version and at most
16 MiB of opaque bytes; the bridge stores and returns the version but leaves
schema interpretation and migration to the consumer. Both calls run on the
game thread. A load with a short or null zero-capacity buffer returns
`UEC_RESULT_BUFFER_TOO_SMALL`, sets the required size and stored version, and
copies no partial payload. Retry with a sufficiently sized buffer.
ABI minor 136 adds `get_controller_enhanced_input_subsystem`, which returns a
weak object handle to the controller's `UEnhancedInputLocalPlayerSubsystem`.
Check `UEC_CAPABILITY_INPUT`; release the handle with `release_object` and call
it on the game thread. Controllers without an associated local player return
`UEC_RESULT_NOT_INITIALIZED`.
ABI minor 137 adds `set_component_collision_channel_response` so consumers can
set a primitive component's channel response to Ignore, Overlap, or Block. It
uses the existing `uec_collision_response` values and rejects undeclared enum
values.
ABI minor 138 adds `get_progress_bar_percent` and `set_progress_bar_percent`
for normalized UMG progress values. Setter inputs must be finite and in
`[0, 1]`; both calls require the game thread and a valid `UProgressBar` handle.
ABI minor 139 adds `get_widget_enabled` and `set_widget_enabled` for a `UWidget`;
the getter clears its output on failure, and the setter accepts only `UEC_FALSE`
or `UEC_TRUE`. Both calls require the game thread.
ABI minor 140 adds the `UEC_WIDGET_HIT_TEST_INVISIBLE` and
`UEC_WIDGET_SELF_HIT_TEST_INVISIBLE` visibility modes. Both remain visible;
the first also blocks hit testing for child widgets, while the second leaves
child hit testing enabled.
Subscription categories are bounded at 1024 active entries and return
`UEC_RESULT_QUEUE_FULL` when full; unsubscribe before creating replacement
bindings during bursts.

The public header is C11-compatible and the table contains only fixed-width
integers, opaque handles, callbacks, and POD values. Keep the header in the
consumer's build without adding Unreal include paths.

## Cooked assets

Object and class paths are lookup keys; passing a path to `load_object` or
`request_object_load` does not add its package to a cooked build. Every asset
that a packaged consumer may request must already be reachable from a cooked
reference or be included by the project's Asset Manager rules. For assets that
are selected by path at runtime, register a Primary Asset or a runtime
`PrimaryAssetLabel` with the explicit asset list or directory rule, and keep
the label and its targets in the target platform's cook configuration. Unreal's
[Asset Management](https://dev.epicgames.com/documentation/unreal-engine/asset-management-in-unreal-engine)
and [cooking and chunking](https://dev.epicgames.com/documentation/unreal-engine/cooking-content-and-creating-chunks-in-unreal-engine)
documentation describe those project-level rules.

Keep the exact `/Game/...` object or class path in the consumer's data rather
than constructing it from editor-only names. Before requesting a path, use
`is_object_path_loaded` or `is_class_path_loaded` as a fast availability check;
these calls do not load content. Treat a failed load callback or an
`UEC_RESULT_NOT_INITIALIZED` result as an absent cooked dependency and report
the path to the host. `retain_object` is still required when the consumer must
keep a loaded object alive after the callback or beyond the current gameplay
operation.

## Threads and callbacks

World, object, actor, component, reflection, input, UI, audio, and save-game
operations run on the Unreal game thread unless their API entry explicitly
queues work. A worker thread should submit a borrowed callback with
`run_on_game_thread` or use an asynchronous request API. The callback's
`user_data` pointer is not copied or retained; keep its storage alive until the
request completes or is canceled.

Timer, tick, input, audio, widget, and primitive-component hit subscriptions
return tokens. Unsubscribe with the matching context before releasing consumer
state. A callback may unsubscribe itself; the bridge suppresses later calls
after cancellation and during module shutdown. Input binding removal is
deferred until an in-flight input callback returns, and a binding that cannot
obtain a native Unreal handle is removed before the call reports failure.

Skeletal-animation completion subscriptions use the same token rules. Bind only
while a single animation is playing; the one-shot callback fires when that
animation stops. Looping playback remains active until the consumer stops it.

## Handles and shutdown

Handles are opaque bridge references to Unreal objects. Each handle has a
typed, monotonic generation and released handles remain tombstoned until module
shutdown, preventing stale pointer acceptance after address reuse. Releasing a
handle does not destroy the Unreal object. Weak object handles become invalid
when Unreal destroys or unloads the object; use `retain_object` when a GC-tracked
strong reference is needed and release that retained handle when finished.
World cleanup, including PIE restart and engine-managed travel, proactively
invalidates handles associated with the old world; reacquire them after the
new world is initialized.

Stop submitting work before unloading the module. Shutdown first rejects new
API entry points, then cancels timers, subscriptions, queued callbacks, asset
requests, save requests, and input bindings. Existing handles and callbacks
must be treated as unusable once shutdown begins. Worker-thread dispatch checks
the shutdown gate while registering its request, so a request cannot be added
after teardown has already drained the queue. Destroying an actor through the
bridge also removes collision and Enhanced Input delegates attached to its
components; an in-flight callback is allowed to return before its native
delegate is removed.
The bridge also watches owners of collision and Enhanced Input subscriptions,
so external actor destruction removes those bindings and invalidates their
actor/component handles.

Level travel cancels timers, world-tick subscriptions, and actor-scoped
collision/input subscriptions for the traveled world before submitting the
request. It then invalidates world, actor, component, and world-bound object
handles from that world; reacquire the new world and its objects after travel.
Global asset handles remain valid.

In networked worlds, call `get_world_net_mode` and `get_world_has_authority`
before mutating gameplay state. The authority query does not provide
replication or RPC behavior; those contracts remain explicit future adapters.
Use `get_world_game_mode` only on an authoritative world, and use
`get_world_game_state` when a world-scoped framework object is needed. Both
return ordinary weak object handles that must be released by the consumer.

## Verification path

Run the portable gate from the repository root:

```sh
sh tests/run_checks.sh
```

This checks the C11 and C++17 public header, links and runs the current and
old-minor consumers against an explicit host stub, checks the C gameplay
example, descriptor JSON, and private source-unit line budgets. The stub
exercises the bootstrap and append-only table prefix without pretending to be
an Unreal runtime. A real integration is complete only after the host project
is built against the latest available
Unreal 5.8.x patch and the C smoke path is exercised in PIE and a packaged
Development build. Record that
run in [`docs/BUILD_MATRIX.md`](BUILD_MATRIX.md).
