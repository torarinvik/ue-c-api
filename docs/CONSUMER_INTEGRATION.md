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

## Compatibility

The requested major version must match the bridge. A consumer may request an
older minor version to use an earlier prefix of the append-only `uec_api`
table. Before calling an optional entry, compare its byte offset with
`api->struct_size`; an older bridge can return a smaller table. The
`abi_major`, `abi_minor`, and `struct_size` fields identify the table that was
actually returned.

The public header is C11-compatible and the table contains only fixed-width
integers, opaque handles, callbacks, and POD values. Keep the header in the
consumer's build without adding Unreal include paths.

## Threads and callbacks

World, object, actor, component, reflection, input, UI, audio, and save-game
operations run on the Unreal game thread unless their API entry explicitly
queues work. A worker thread should submit a borrowed callback with
`run_on_game_thread` or use an asynchronous request API. The callback's
`user_data` pointer is not copied or retained; keep its storage alive until the
request completes or is canceled.

Timer, tick, input, audio, and widget subscriptions return tokens. Unsubscribe
with the matching context before releasing consumer state. A callback may
unsubscribe itself; the bridge suppresses later calls after cancellation and
during module shutdown.

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
must be treated as unusable once shutdown begins.

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

This checks the C11 and C++17 public header, the C gameplay example, descriptor
JSON, and private source-unit line budgets. A real integration is complete only
after the host project is built against the latest available Unreal 5.8.x
patch and the C smoke path is exercised in PIE and a packaged Development
build. Record that
run in [`docs/BUILD_MATRIX.md`](BUILD_MATRIX.md).
