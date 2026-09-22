# Feature matrix

This matrix describes the current implementation against the roadmap. “Runtime
implemented” means code exists in the plugin; “verified” is limited to checks
that can run without the Unreal Engine 5.8 toolchain in this repository.

| Area | Status | Current boundary |
| --- | --- | --- |
| ABI bootstrap and version negotiation | Runtime implemented / header verified | `uec_get_api`, versioned function table, capability bits |
| C and C++ public-header compatibility | Verified | C11 and C++17 syntax checks pass |
| Diagnostics and logging | Runtime implemented | Bounded `get_last_error`; game log output |
| Context/world/actor handles | Runtime implemented / Unreal integration pending | Typed active registries and weak UObject references |
| World selection | Runtime implemented / Unreal integration pending | Indexed active Game/PIE enumeration, world kind, and first-world convenience |
| Actor spawn and destruction | Runtime implemented / Unreal integration pending | Loadable actor class paths; game thread only |
| Actor identity and tags | Runtime implemented / Unreal integration pending | UTF-8 name output and tag lookup; game thread only |
| Actor transforms | Runtime implemented / Unreal integration pending | Double-precision C POD transform; game thread only |
| Scene components | Runtime implemented / Unreal integration pending | Root-component handle, world transforms, visibility, activation |
| Timers | Runtime implemented / Unreal integration pending | One-shot and looping game-thread callbacks with cancellation |
| World tick subscriptions | Runtime implemented / Unreal integration pending | Per-frame game-thread callbacks scoped to a world with unsubscribe tokens and teardown cleanup |
| Class/property metadata | Runtime implemented / Unreal integration pending | Class lookup, inheritance, names, and broad reflected property kinds |
| Reflected scalar/string reads and writes | Runtime implemented / Unreal integration pending | Typed bool, integer, enum, and floating-point values plus string, name, text, hard object references, and Unreal text serialization for supported structs and containers |
| Collision line traces | Runtime implemented / Unreal integration pending | Game-thread single traces with stable channel mapping and ignored-actor filtering |
| Collision sweeps and overlaps | Runtime implemented / Unreal integration pending | World-aligned sphere, box, and capsule queries with bounded actor results |
| One-shot spatial audio | Runtime implemented / Unreal integration pending | Fire-and-forget `USoundBase` playback at a world location |
| Basic UMG widgets | Runtime implemented / Unreal integration pending | Create a `UUserWidget` class and add or remove it from the viewport |
| Camera field of view | Runtime implemented / Unreal integration pending | Read and write perspective FOV on camera scene components |
| Save-game slots and object properties | Runtime implemented / Unreal integration pending | Create, load, save, delete, edit supported reflected save-object fields, and bounded async completion |
| Game-thread dispatch | Runtime implemented / Unreal integration pending | Queue and cancel borrowed callbacks from worker threads; bounded at 1024 pending requests |
| Pawn and character movement | Runtime implemented / Unreal integration pending | Add world-space pawn input and character jump state |
| Mesh presentation | Runtime implemented / Unreal integration pending | Assign loaded static or skeletal meshes to compatible components |
| Animation and material parameters | Runtime implemented / Unreal integration pending | Play/stop skeletal assets and update scalar/vector material parameters |
| Explicit object retention | Runtime implemented / Unreal integration pending | Promote a weak object handle to a GC-tracked strong handle |
| Component type introspection | Runtime implemented / Unreal integration pending | Class-path output and inheritance checks for scene components |
| Component attachment | Runtime implemented / Unreal integration pending | Same-world attach/detach with transform rules and optional sockets |
| Actor type introspection | Runtime implemented / Unreal integration pending | Class-path output, inheritance checks, and indexed world queries by actor class |
| Enhanced Input contexts | Runtime implemented / Unreal integration pending | Add/remove loaded mapping contexts, read/inject typed action values, and bind/unbind game-thread callbacks |
| Reflected function metadata | Runtime implemented / Unreal integration pending | Enumerate names, parameter counts, return presence, and latent flags |
| Collision settings | Runtime implemented / Unreal integration pending | Primitive collision mode and stable trace-channel responses |
| Asset path queries | Runtime implemented / Unreal integration pending | Check whether a soft object path currently resolves in memory |
| Attached audio playback | Runtime implemented / Unreal integration pending | Spawn, stop, destroy, and release non-auto-destroying audio components on scene components |
| C gameplay example | Source and header verified | Spawn, timer-driven movement, callback, and cleanup flow |
| Synchronous object loading | Runtime implemented / Unreal integration pending | Weak path-loaded UObject handles, names, and type checks |
| Asynchronous object loading | Runtime implemented / Unreal integration pending | Streamable-manager requests, cancellation, game-thread callbacks, and a 1024-request bound |
| Level travel | Runtime implemented / Unreal integration pending | Map name queries and game-thread `OpenLevel` requests |
| Player flow | Runtime implemented / Unreal integration pending | Indexed local controller lookup, world game-instance access, possession, and view-target selection |
| Input polling | Runtime implemented / Unreal integration pending | Digital and analog key queries by Unreal key name |
| Basic physics | Runtime implemented / Unreal integration pending | Finite-validated velocity, impulses, and forces on simulating primitive roots |
| Reflection and Blueprint calls | Partial | Property writes, zero-argument calls, and bounded text-marshaled calls with one return/first out value are available; typed ABI calls remain |
| Input | Planned | No public functions yet beyond the input, movement, audio, UI, camera, save-data, and dispatch slices above |
| Async loading, travel, streaming | Partial | Async object requests, cancellation, loaded-state queries, and level-travel submission are available; streaming and completion events remain |
| Multiplayer and replication | Planned | No public functions yet |
| Editor tooling and generated bindings | Planned | Separate editor module not yet created |

The Unreal-dependent statuses require an actual UE 5.8 build, PIE run, and
packaged Development run before they can become verified release features.
