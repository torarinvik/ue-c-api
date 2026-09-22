# Feature matrix

This matrix describes the current implementation against the roadmap. “Runtime
implemented” means code exists in the plugin; “verified” is limited to checks
that can run without the Unreal Engine 5.8.2 toolchain in this repository.

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
| Collision line traces | Runtime implemented / Unreal integration pending | Game-thread single traces with stable channel mapping and ignored-actor filtering |
| Collision sweeps and overlaps | Runtime implemented / Unreal integration pending | World-aligned sphere, box, and capsule queries with ignored-actor filters and bounded unique-actor results |
| Collision event callbacks | Runtime implemented / Unreal integration pending | One-shot primitive-component hit callbacks with unsubscribe tokens and shutdown cleanup |
| One-shot spatial audio | Runtime implemented / Unreal integration pending | Fire-and-forget `USoundBase` playback at a world location |
| Basic UMG widgets | Runtime implemented / Unreal integration pending | Create a `UUserWidget` class, add or remove it from the viewport, set/read visibility, update/read `UTextBlock` text, and receive one-shot button clicks |
| Camera field of view | Runtime implemented / Unreal integration pending | Read and write perspective FOV on camera scene components |
| Save-game slots and object properties | Runtime implemented / Unreal integration pending | Create, load, save, delete, edit supported reflected save-object fields, bounded async completion, and game-INI string/integer/boolean configuration access |
| Game-thread dispatch | Runtime implemented / Unreal integration pending | Queue and cancel borrowed callbacks from worker threads; bounded at 1024 pending requests |
| Pawn and character movement | Runtime implemented / Unreal integration pending | Add world-space pawn input and character jump state |
| Mesh presentation | Runtime implemented / Unreal integration pending | Assign loaded static or skeletal meshes to compatible components |
| Animation and material parameters | Runtime implemented / Unreal integration pending | Play/stop skeletal assets, tokenized one-shot completion callbacks, and scalar/vector material parameters |
| Explicit object retention | Runtime implemented / Unreal integration pending | Promote a weak object handle to a GC-tracked strong handle |
| Component type introspection | Runtime implemented / Unreal integration pending | Class-path output and inheritance checks for scene components |
| Component attachment | Runtime implemented / Unreal integration pending | Same-world attach/detach with transform rules and optional sockets |
| Actor type introspection | Runtime implemented / Unreal integration pending | Class-path output, inheritance checks, and indexed world queries by actor class |
| Enhanced Input contexts | Runtime implemented / Unreal integration pending | Add/remove loaded mapping contexts, read/inject typed action values, and bind/unbind game-thread callbacks |
| Reflected function metadata | Runtime implemented / Unreal integration pending | Enumerate names, parameter counts, return presence, and latent flags |
| Collision settings | Runtime implemented / Unreal integration pending | Primitive collision mode read/write and per-channel ignore/overlap/block response readback |
| Asset path queries | Runtime implemented / Unreal integration pending | Check whether soft object and class paths currently resolve in memory |
| Attached audio playback | Runtime implemented / Unreal integration pending | Spawn, stop, playing-state readback, destroy, and release non-auto-destroying audio components on scene components |
| Audio completion subscriptions | Runtime implemented / Unreal integration pending | One-shot native finished callbacks with unsubscribe tokens and component-destruction cleanup |
| C gameplay example | Source and header verified | Spawn, timer-driven movement, callback, and cleanup flow |
| Synchronous object loading and lookup | Runtime implemented / Unreal integration pending | Non-loading full-path lookup plus weak path-loaded UObject handles, names, full object paths, class paths, and type checks |
| Asynchronous object loading | Runtime implemented / Unreal integration pending | Streamable-manager requests, cancellation, game-thread callbacks, and a 1024-request bound |
| Level travel | Runtime implemented / Unreal integration pending | Map name queries, immediate and callback-based game-thread `OpenLevel` requests, cancellation, world-owned timer/tick and actor-scoped subscription cancellation, and invalidation of old-world handles |
| Player flow | Runtime implemented / Unreal integration pending | Indexed local controller lookup, player-start lookup, world game-instance, game-mode, and game-state access, possession, and view-target selection |
| Input polling | Runtime implemented / Unreal integration pending | Digital and analog key queries by Unreal key name |
| Basic physics | Runtime implemented / Unreal integration pending | Finite-validated actor and primitive-component velocity reads, impulses, and forces on simulating primitive roots |
| Reflection and Blueprint calls | Partial | Property writes, class/function/parameter/flag metadata, reflected actor/object-array counts/text elements, actor/UObject map/set counts and text entries, soft object/class path readback, zero-argument calls, scalar typed calls, bounded text-marshaled calls, scalar multi-output calls, and text multi-output calls are available; broader typed ABI calls remain |
| Input | Partial | Enhanced Input mapping contexts, action polling/injection, and tokenized callbacks are implemented; broader action semantics remain |
| Async loading, travel, streaming | Partial | Async object requests, cancellation, loaded-state queries, level-travel submission, indexed streaming-level state requests, and cancellable streaming completion callbacks are available |
| Multiplayer and replication | Partial | Network-mode and authority queries plus authority-gated actor, possession, transform, and physics mutators; replication/RPC adapters remain planned |
| Editor tooling and generated bindings | Planned | Separate editor module not yet created |

The Unreal-dependent statuses require an actual UE 5.8.2 build, PIE run, and
packaged Development run before they can become verified release features.
