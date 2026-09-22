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
| Class/property metadata | Runtime implemented / Unreal integration pending | Class lookup, inheritance, names, and broad reflected property kinds |
| Reflected scalar/string reads and writes | Runtime implemented / Unreal integration pending | Bool, integer, floating-point, string, name, and text values |
| Collision line traces | Runtime implemented / Unreal integration pending | Game-thread single traces with stable channel mapping |
| Collision sweeps and overlaps | Runtime implemented / Unreal integration pending | World-aligned sphere, box, and capsule queries with bounded actor results |
| Synchronous object loading | Runtime implemented / Unreal integration pending | Weak path-loaded UObject handles, names, and type checks |
| Asynchronous object loading | Runtime implemented / Unreal integration pending | Streamable-manager requests, cancellation, and game-thread callbacks |
| Level travel | Runtime implemented / Unreal integration pending | Map name queries and game-thread `OpenLevel` requests |
| Player flow | Runtime implemented / Unreal integration pending | Local controller/pawn lookup, possession, and view-target selection |
| Input polling | Runtime implemented / Unreal integration pending | Digital and analog key queries by Unreal key name |
| Basic physics | Runtime implemented / Unreal integration pending | Velocity, impulses, and forces on simulating primitive roots |
| Reflection and Blueprint calls | Partial | Property writes and zero-argument calls are available; typed calls remain |
| Input, movement, camera, UI, audio, save data | Planned | No public functions yet beyond the input and movement slices above |
| Async loading, travel, streaming | Planned | No public functions yet |
| Multiplayer and replication | Planned | No public functions yet |
| Editor tooling and generated bindings | Planned | Separate editor module not yet created |

The Unreal-dependent statuses require an actual UE 5.8 build, PIE run, and
packaged Development run before they can become verified release features.
