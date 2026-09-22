# C consumer integration

Unreal loads `UnrealCAPI` as an in-process runtime module. A consumer includes
`Source/UnrealCAPI/Public/uec_api.h` and receives the function table from the
exported bootstrap entry point after the host has initialized the engine:

```c
const uec_api* api = NULL;
uec_context* context = NULL;
uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
if (result != UEC_RESULT_OK) {
    /* The host can report the failure or retry after module startup. */
}
```

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
`user_data`. Cancel the request through `cancel_travel_request` before the
callback fires; module shutdown cancels all remaining travel requests.
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
