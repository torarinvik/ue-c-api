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
all are zero.

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

Stop submitting work before unloading the module. Shutdown first rejects new
API entry points, then cancels timers, subscriptions, queued callbacks, asset
requests, save requests, and input bindings. Existing handles and callbacks
must be treated as unusable once shutdown begins. Worker-thread dispatch checks
the shutdown gate while registering its request, so a request cannot be added
after teardown has already drained the queue. Destroying an actor through the
bridge also removes collision and Enhanced Input delegates attached to its
components; an in-flight callback is allowed to return before its native
delegate is removed.

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
