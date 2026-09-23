# Feature matrix

This matrix describes the current implementation against the roadmap. “Runtime
implemented” means code exists in the plugin; verification reflects only the
checks explicitly recorded for each feature in the Unreal build matrix.

| Area | Status | Current boundary |
| --- | --- | --- |
| ABI bootstrap and version negotiation | Runtime implemented / header verified | `uec_get_api`, versioned function table, capability bits |
| C and C++ public-header compatibility | Verified | C11 and C++17 syntax checks pass |
| Diagnostics and logging | Runtime implemented | Bounded `get_last_error`; game log output; strict UTF-8 input validation |
| Runtime drain diagnostics | Runtime implemented | Game-thread counts for subscriptions, pending requests, and in-flight callbacks before consumer unload |
| Context/world/actor handles | Runtime implemented / Unreal integration pending | Typed active registries and weak UObject references |
| World selection | Runtime implemented / Unreal integration pending | Indexed active Game/PIE enumeration, explicit world-kind enumeration including editor/preview/inactive contexts, world kind, PIE instance identifiers, network mode, authority query, game-mode/game-state access, authority-gated mutators, and first-world convenience |
| Actor spawn and destruction | Runtime implemented / Unreal integration pending | Loadable actor class paths, stale-handle invalidation, and one-shot destruction callbacks; game thread only |
| Actor identity and tags | Runtime implemented / Unreal integration pending | UTF-8 name output, tag lookup/enumeration, authority-gated tag add/remove, and game-thread access |
| Actor transforms | Runtime implemented / Unreal integration pending | Double-precision C POD transform; game thread only |
| Scene components | Runtime implemented / Unreal integration pending | Root-component handle, world transforms, visibility and activation read/write, and class-filtered enumeration |
| Timers | Runtime implemented / Unreal integration pending | One-shot and looping game-thread callbacks with cancellation and a 1024-entry bound |
| World tick subscriptions | Runtime implemented / Unreal integration pending | Per-frame game-thread callbacks scoped to a world with unsubscribe tokens, teardown cleanup, and a 1024-entry bound |
| Class/property metadata | Runtime implemented / Unreal integration pending | Class lookup, inheritance, names, and broad reflected property kinds |
| Reflected scalar/string reads and writes | Runtime implemented / Unreal integration pending | Typed bool, integer, enum, and floating-point values plus string, name, text, hard object references, and Unreal text serialization for supported structs and containers |
| Reflected actor function invocation | Runtime implemented / Unreal integration pending | ABI 131 mixes typed scalar, hard object/class/world, and text-backed positional arguments; ABI 134 adds typed FVector/FQuat/FTransform values; rejects world handles and world-bound object handles from a different actor world and returns typed scalar/struct outputs, owned reference handles, or caller-buffered Unreal property text |
| Blueprint-to-C event bridge | Runtime implemented / Unreal integration pending | ABI 132 adds a local Blueprint-assignable actor component, `UEC_CAPABILITY_EVENT_BRIDGE`, synchronous typed C callbacks, bounded subscriptions, self-unbind, and actor/world/shutdown cleanup |
| Async reflected latent invocation | Runtime implemented / Unreal integration pending | ABI 133 adds bounded completion requests with mixed input arguments, unique latent callback targets, cancellation, and actor/world/shutdown cleanup; return, out, and reference parameters are rejected |
| Reflected container schema metadata | Runtime implemented / Unreal integration pending | ABI 122 reports array/set element kinds and map key/value kinds; ABI 130 adds typed scalar map-key reads; non-container properties return unsupported |
| Reflected set mutation | Runtime implemented / Unreal integration pending | ABI 123 replaces existing text or scalar set elements with duplicate rejection and rehashing |
| Reflected soft references | Runtime implemented / Unreal integration pending | ABI 129 provides typed soft object/class path reads and kind-checked writes for actor and UObject properties |
| Collision line traces | Packaged Development and Editor PIE smoke verified | Game-thread traces with stable channel mapping, ignored-actor filtering, and ABI 124 detailed hit readback; smoke checks blocking hits, detailed component/actor handles, and ignored-actor filtering |
| Collision sweeps and overlaps | Packaged Development and Editor PIE smoke verified | World-aligned sphere, box, and capsule queries with up to 1024 ignored actors and up to 1024 unique overlap results; smoke checks blocking sweeps, overlap output handles, and ignored-actor filtering |
| Collision event callbacks | Packaged Development and Editor PIE smoke verified | One-shot primitive-component hit callbacks with unsubscribe tokens and shutdown cleanup; smoke sweeps one query box into another and checks the callback token, other-actor handle, and automatic one-shot unsubscription |
| One-shot spatial audio | Runtime implemented / Unreal integration pending | Fire-and-forget `USoundBase` playback at a world location |
| Basic UMG widgets | Runtime implemented / Unreal integration pending | Create a `UUserWidget` class, obtain weak handles to named tree children (ABI 142; `examples/c_widget_ui/` includes a C text-update helper with mocked handle-release coverage), add or remove it from the viewport, set/read five visibility modes and enabled state, update/read `UTextBlock` text, read/write normalized `UProgressBar` percent and all three `UCheckBox` states, and receive one-shot button clicks |
| Camera field of view | Runtime implemented / Unreal integration pending | Read and write perspective FOV on camera scene components |
| Save-game slots and object properties | Runtime implemented / Unreal integration pending | Create, load, save, delete, edit supported reflected save-object fields, bounded async completion, game-INI string/integer/boolean configuration access, and ABI 135 caller-versioned opaque payloads up to 16 MiB; packaged host probe saves and reloads a temporary slot, checks callback/handle accounting, and deletes it |
| Game-thread dispatch | Runtime implemented / Unreal integration pending | Queue and cancel borrowed callbacks from worker threads; FIFO dispatch drains at most 64 callbacks per engine tick and admits 1024 pending requests; packaged probe checks concurrent admission, queue-full output clearing, unique ids, released-context rejection, atomic submit-versus-release outcomes, successful-cancel suppression, in-callback accounting, the dequeued-cancel result, and request drainage |
| Pawn and character movement | Runtime implemented / Unreal integration pending | Add world-space pawn input and character jump state; ABI 125 linear, ABI 126 angular, ABI 127 angular impulse, and ABI 128 actor-root angular physics operations |
| Mesh presentation | Runtime implemented / Unreal integration pending | Assign loaded static or skeletal meshes to compatible components |
| Animation and material parameters | Runtime implemented / Unreal integration pending | Play/stop skeletal assets, tokenized one-shot completion callbacks, and scalar/vector material parameters |
| Explicit object retention | Runtime implemented / Unreal integration pending | Promote a weak object handle to a GC-tracked strong handle |
| Component type introspection | Runtime implemented / Unreal integration pending | Class-path output and inheritance checks for scene components |
| Component attachment | Runtime implemented / Unreal integration pending | Same-world attach/detach with transform rules and optional sockets |
| Actor type introspection | Runtime implemented / Unreal integration pending | Class-path output, inheritance checks, and indexed world queries by actor class |
| Enhanced Input contexts | Runtime implemented / Unreal integration pending | Add/remove loaded mapping contexts, expose the local-player subsystem as a weak object handle, read/inject typed action values, and bind/unbind game-thread callbacks for Started, Ongoing, Triggered, Canceled, and Completed phases |
| Reflected function metadata | Runtime implemented / Unreal integration pending | Enumerate names, parameter counts, return presence, and latent flags |
| Collision settings | Runtime implemented / Unreal integration pending | Primitive collision mode read/write, legacy per-channel block/ignore writes, and ABI 137 typed per-channel Ignore/Overlap/Block writes plus response readback |
| Asset path queries | Runtime implemented / Unreal integration pending | Check whether soft object and class paths currently resolve in memory |
| Attached audio playback | Runtime implemented / Unreal integration pending | Spawn, stop, playing-state readback, destroy, and release non-auto-destroying audio components on scene components |
| Audio completion subscriptions | Runtime implemented / Unreal integration pending | One-shot native finished callbacks with unsubscribe tokens and component-destruction cleanup |
| C gameplay example | Packaged Development and Editor PIE smoke verified | Spawn, timer-driven movement, synchronous event-bridge callback, and actor/component/handle cleanup flow |
| Synchronous object loading and lookup | Runtime implemented / Unreal integration pending | Non-loading full-path lookup plus weak path-loaded UObject handles, names, full object paths, class paths, and type checks |
| Asynchronous object loading | Runtime implemented / Unreal integration pending | Streamable-manager requests, cancellation, game-thread callbacks, and a 1024-request bound; packaged host checks cancellation drainage and callback suppression, reports failure for a GUID-named missing asset, and completes a native `Actor` load with path, callback, and handle accounting |
| Level travel | Packaged Development and Editor PIE smoke verified | Map name queries, immediate and callback-based game-thread `OpenLevel` requests, cancellation of completion callbacks, PIE-prefixed destination matching, world-owned timer/tick and actor-scoped subscription cancellation, and invalidation of old-world handles |
| Player flow | Runtime implemented / Unreal integration pending | Indexed local controller lookup, player-start lookup, world game-instance, game-mode, and game-state access, possession, and view-target selection |
| Input polling | Runtime implemented / Unreal integration pending | Digital and analog key queries by Unreal key name |
| Basic physics | Partial / simulation smoke verified | ABI 143 enables/disables primitive-component simulation and reads the state; PIE and packaged Development smoke verify both transitions. Finite-validated actor and primitive-component linear/angular velocity, impulse, force, torque, and angular-impulse operations act on simulating primitive roots; broader physics behavior remains unverified |
| Reflection and Blueprint calls | Partial | Property writes, class/function/parameter/flag metadata including property access flags, class-default text, referenced-class paths, enum names/values, reflected struct-field metadata and type paths, and typed hard class references, reflected actor/object-array counts/text elements, typed scalar array/map/set reads and map-key reads, array/map writes, typed nested-struct scalar reads and writes, actor/UObject map/set counts and text entries, soft object/class path readback and writes, nested struct field text readback and writes, zero-argument calls, scalar typed calls, bounded text-marshaled calls, scalar multi-output calls, text multi-output calls, ABI 132 Blueprint-to-C event bridges, ABI 133 async latent calls, and ABI 134 typed FVector/FQuat/FTransform calls are available; broader typed ABI calls remain |
| Input | Partial | Enhanced Input mapping contexts, action polling/injection, and tokenized callbacks are implemented; broader action semantics remain |
| Async loading, travel, streaming | Partial | Async object requests, cancellation, loaded-state queries, level-travel submission with cancellable completion callbacks, indexed streaming-level state requests, and cancellable streaming completion callbacks are available |
| Multiplayer and replication | Partial | Network-mode and authority queries plus authority-gated actor, possession, transform, and physics mutators; replication/RPC adapters remain planned |
| Editor tooling and generated bindings | Planned | Separate editor module not yet created |

The shared host smoke now runs collision line traces, sweeps, overlaps, and
detailed hit queries in a UE 5.8.3 Editor PIE world and a packaged Development
build. Those runs verify only the operations the smoke exercises;
other Unreal-dependent features still need focused probes before release.
