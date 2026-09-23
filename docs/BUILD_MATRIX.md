# Build and verification matrix

This file records the engine and toolchain combinations used to validate the
plugin. A row is only marked verified after the Unreal module and host project
build, launch, and exercise the C smoke path.

| Engine | Host platform | Compiler/toolchain | C consumer | Plugin/host status |
| --- | --- | --- | --- | --- |
| UE 5.7.4 installed distribution | macOS arm64 local workstation | Unreal Build Tool 5.7.4 | C11/C++17 linked host-stub smoke verified | Compatibility builds attempted with `UEC_ALLOW_ENGINE_MISMATCH=1`; UBT rejected Mac because platform support files are missing and reports Win64 unsupported in this distribution. Neither attempt compiled project code; the engine is also below the UE 5.8.3 target. |
| UE 5.8.3 local installation | macOS 27.0 arm64 | UnrealBuildTool 5.8.3; Xcode 27.0 (27A266a); Apple Clang 21.0.0 / compiler 21.1.6; macOS SDK 27.0; Metal Toolchain 27A266a | C11/C++17 linked host-stub smoke verified; Unreal C smoke translation units compiled and executed | Game and Editor Development targets compile and link. Full Mac Development build, cook, stage, pak, and archive pass. The staged app and a NullRHI Editor PIE world pass bootstrap, collision traces/sweeps/overlaps, event, latent, queue, save/load, object-load, gameplay, and travel C smoke. The Game Shipping target compiles and cooks, but clean UAT app finalization still fails at the generated Xcode `Touch UBT generated tiles` pre-action (exit 65), even after installing Metal Toolchain 27A266a. A prior manual finalization and stage-only run produced a staged Shipping app that remained running for a startup check; that run did not emit the Development C smoke markers. A Mac dedicated-server build is rejected by this installed distribution before project compilation. The host explicitly stages Unreal's TBB runtime dylibs. `UE_BUILD_FROM_XCODE=1` bypasses UAT's failing generated `Touch UBT generated tiles` pre-action for Development. This toolchain combination is not listed in Epic's supported UE 5.8 macOS row. |
| UE 5.8.3 (latest 5.8.x hotfix as of September 2026; descriptor target 5.8) | Linux CI | GCC and Clang | C11/C++17 syntax and linked host-stub smoke verified | Engine build unavailable |
| UE 5.8.3 (latest 5.8.x hotfix as of September 2026; descriptor target 5.8) | macOS CI | Clang | C11/C++17 syntax and linked host-stub smoke verified | Engine build unavailable |

Epic's [UE 5.8 macOS requirements](https://dev.epicgames.com/documentation/en-us/unreal-engine/macos-development-requirements-for-unreal-engine)
list macOS Sonoma 14.5 as the minimum, Sequoia 15 as recommended, Xcode 26.0
as the minimum, and Xcode 26.1.1 as recommended; they explicitly say Xcode
26.4 is incompatible. This workstation runs macOS 27 with Xcode 27.0, which
is not listed in that compatibility row. The local build, cook, stage, and
packaged Development smoke pass, but that does not establish official Epic
support for macOS 27 or Xcode 27.

The project plugin lives under `Plugins/UnrealCAPI/`, which is the standard
project-plugin layout Unreal uses to discover the descriptor and module source.
The host project includes tracked Game and Editor target files plus a
dedicated-server target definition that this Mac distribution cannot build,
along with a minimal primary module, a C bootstrap probe, and a native latent-test actor fixture
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
design. The collision fixture executes line, filtered line, sweep, filtered
sweep, overlap, filtered overlap, and detailed-hit calls against a known
query-only box, then checks collision settings and handle drainage. The queue
fixture checks concurrent admission, output clearing, unique
request ids, cancellation suppression, callback reentrancy, and drainage. The event-bridge fixture
queries runtime statistics from inside event and actor-destroyed callbacks to
verify `active_callbacks` includes in-flight
code and subscription counts drain; the travel callback checks the same counter
while confirming the new world handle is released. The host stub proves
consumer-side bootstrap, table calls, and the append-only prefix;
the CI matrix also repeats those linked consumers with AddressSanitizer and
UndefinedBehaviorSanitizer. These checks do not compile the Unreal module or
run PIE.

The separate Windows x64 ABI job cross-compiles the host stub as a DLL,
verifies that `uec_get_api` is exported, and links the current C smoke consumer
and an older C++ consumer against the DLL import library. This exercises
Windows C/C++ header layout and DLL import/export behavior; it does not count
as an Unreal plugin build or declare a Windows engine target supported.

With an installed engine, run `UE_ROOT=/path/to/UnrealEngine
sh tests/run_unreal_build.sh` to compile, cook, stage, and package the minimal
host project for the current platform. A same-platform Mac Development build
first runs the host C smoke in an Editor PIE world with NullRHI, then launches
the staged app bundle. Other hosts launch the archived executable. Each run
waits for successful bootstrap, collision, event-bridge, latent-call,
concurrent queue, async save/load, async object-load,
gameplay-example, and travel C smoke messages; failures and timeouts fail the
gate with recent host output.
Set `UEC_UNREAL_CONFIGURATION=Shipping` to repeat the build in Shipping mode.
The repeatable runtime marker smoke currently covers Development only; on this
Mac toolchain the clean Shipping package gate has the finalization limitation
recorded in the matrix above.
After a Shipping app has been staged, `python3 tests/unreal_runtime.py <app-executable> --startup-only` checks that the executable remains running through engine startup without relying on Development log markers.
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

`Source/UnrealCAPIHostServer.Target.cs` declares the host's dedicated-server
target for engine distributions that support it. The installed UE 5.8.3 Mac
distribution reports that server targets are unsupported before compiling the
project, so this workstation does not claim dedicated-server verification.

The minimum consumer language standard is C11. The plugin implementation uses
C++17 through Unreal Build Tool; consumers may compile the public header as C11
or C++17. The local UE 5.8.3 Game and Editor targets have compiled and linked,
and the staged Mac Development app passed the C runtime smoke with the recorded
toolchain. Epic's published UE 5.8 macOS requirements do not list macOS 27 or
Xcode 27, so local success does not establish official support for that
toolchain combination.

Record whether remaining checks ran in Editor PIE, packaged Development,
packaged Shipping, or dedicated-server mode. Keep generated engine output and
local installation paths ignored.
