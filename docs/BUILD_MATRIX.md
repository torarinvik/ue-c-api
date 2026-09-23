# Build and verification matrix

This file records the engine and toolchain combinations used to validate the
plugin. A row is only marked verified after the Unreal module and host project
build, launch, and exercise the C smoke path.

| Engine | Host platform | Compiler/toolchain | C consumer | Plugin/host status |
| --- | --- | --- | --- | --- |
| UE 5.7.4 installed distribution | macOS arm64 local workstation | Unreal Build Tool 5.7.4 | C11/C++17 linked host-stub smoke verified | Compatibility build attempted with `UEC_ALLOW_ENGINE_MISMATCH=1`; UBT rejected Mac as an invalid target because this distribution lacks Mac platform support files. The engine is also below the UE 5.8.3 target. |
| UE 5.8.3 target (engine unavailable) | macOS 27 arm64 local workstation | Apple Clang 21.0.0; Python 3.9.6 | C11/C++17 linked host-stub smoke verified | Unreal build pending; `UE_ROOT` is unset and no 5.8.3 installation is available |
| UE 5.8.3 (latest 5.8.x hotfix as of September 2026; descriptor target 5.8) | Linux CI | GCC and Clang | C11/C++17 syntax and linked host-stub smoke verified | Engine build unavailable |
| UE 5.8.3 (latest 5.8.x hotfix as of September 2026; descriptor target 5.8) | macOS CI | Clang | C11/C++17 syntax and linked host-stub smoke verified | Engine build unavailable |

The project plugin lives under `Plugins/UnrealCAPI/`, which is the standard
project-plugin layout Unreal uses to discover the descriptor and module source.
The host project includes tracked Game and Editor target files plus a minimal
primary module, a C bootstrap probe, and a native latent-test actor fixture
that exercises scalar and text-backed mixed invocation, output-capacity
preflight, pure out-parameter ordering, mixed-call argument-count and
scalar-kind rejection, typed FVector/FQuat/FTransform round trips and mismatch
rejection, short text-output sizing and retry, invalid world-kind handling,
completion, cancellation, signature rejection, explicit and cross-world
context handling, stale actor and bridge handle rejection, and pending request
counts. After latent invocation, the packaged host submits 1152 game-thread
callbacks concurrently and verifies that exactly 1024 are admitted, the rest
report queue-full with cleared request ids, accepted ids are unique, callbacks
observe in-flight accounting, 128 cancellations suppress their callbacks, and
all remaining requests drain. It then saves and reloads a temporary save-game
slot asynchronously, validates callback accounting and object-handle cleanup,
and deletes the slot. An async asset probe cancels a request for the already
loaded Engine `Actor` class and checks request drainage and callback
suppression. It then requests a GUID-named missing asset and verifies the
failure result and request drainage before resolving the class and checking its
returned object path, callback accounting, and handle release. The host runs
the documented C
gameplay example through three
timer-driven actor moves and
validates each event-bridge callback. A travel probe reloads the configured
OpenWorld map and checks immediate old-world handle invalidation, post-load
callback delivery, request drainage, and release of the callback's new world
handle, so Unreal Build Tool does not need to synthesize temporary targets
before compiling the plugin and its first C consumer. `Config/DefaultEngine.ini`
selects Unreal's OpenWorld template for editor, game, and server startup so the
packaged host enters a runtime world and can execute its world-scoped C smoke.
The portable gate is `sh tests/run_checks.sh`. It validates the public header
as C11 and C++17, links and runs the current and old-minor C consumers against
an explicit host stub, including the tracked Unreal host's C bootstrap
translation unit, checks the C gameplay example and Unreal descriptor JSON,
and enforces the 400–800 line budget for private implementation units and the
tracked primary C host smoke translation unit. Focused probes under
`Source/UnrealCAPIHost/Private/Tests/` are test fixtures and stay small by
design. The queue fixture checks concurrent admission, output clearing, unique
request ids, cancellation suppression, callback reentrancy, and drainage. The event-bridge fixture
queries runtime statistics from inside event and actor-destroyed callbacks to
verify `active_callbacks` includes in-flight
code and subscription counts drain; the travel callback checks the same counter
while confirming the new world handle is released. The host stub proves
consumer-side bootstrap, table calls, and the append-only prefix;
the CI matrix also repeats those linked consumers with AddressSanitizer and
UndefinedBehaviorSanitizer. These checks do not compile the Unreal module or
run PIE.

With an installed engine, run `UE_ROOT=/path/to/UnrealEngine
sh tests/run_unreal_build.sh` to compile, cook, stage, and package the minimal
host project for the current platform. A same-platform Development build then
launches the packaged host with NullRHI and waits for successful bootstrap,
event-bridge, latent-call, concurrent queue, async save/load, async object-load,
gameplay-example, and travel C smoke messages; failures and timeouts fail the
gate with recent host output.
Set `UEC_UNREAL_CONFIGURATION=Shipping` to repeat
the build in Shipping mode; runtime smoke is limited to Development builds.
Set `UEC_UNREAL_PLATFORM=Win64` (or
another platform supplied by the engine installation) to validate a target
different from the host platform; cross-platform requests skip rebuilding the
local Editor target and skip runtime launch. The script runs the portable gate
first, reads the exact patch from `Engine/Build/Build.version`, and rejects an
engine whose major/minor version does not match the host descriptor's
`EngineAssociation`.
`UE_TARGET_VERSION` records the minimum supported patch (`5.8.3`); later 5.8.x
hotfixes are accepted, while older patches are rejected. Use
`UEC_ALLOW_ENGINE_MISMATCH=1` only for an explicit compatibility probe. The
script exits with status 2 when the engine path, version metadata, or requested
platform is unavailable, so the portable gate remains usable on contributors'
machines without Unreal installed.

The minimum consumer language standard is C11. The plugin implementation uses
C++17 through Unreal Build Tool; consumers may compile the public header as C11
or C++17. The recorded local baseline above is informational until a matching
UE 5.8.3 installation is available.

When an engine installation is available, record the exact UE patch, host OS,
architecture, compiler version, build configuration, and whether the check ran
in Editor PIE, packaged Development, packaged Shipping, or dedicated-server
mode. Keep generated engine output and local installation paths ignored.
