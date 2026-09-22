# Build and verification matrix

This file records the engine and toolchain combinations used to validate the
plugin. A row is only marked verified after the Unreal module and host project
build, launch, and exercise the C smoke path.

| Engine | Host platform | Compiler/toolchain | C consumer | Plugin/host status |
| --- | --- | --- | --- | --- |
| UE 5.8.2 target (engine unavailable) | macOS 27 arm64 local workstation | Apple Clang 21.0.0; Python 3.9.6 | C11/C++17 linked host-stub smoke verified | Unreal build pending (`UE_ROOT` unavailable) |
| UE 5.8.2 (latest 5.8.x hotfix; descriptor target 5.8) | Linux CI | GCC and Clang | C11/C++17 syntax and linked host-stub smoke verified | Engine build unavailable |
| UE 5.8.2 (latest 5.8.x hotfix; descriptor target 5.8) | macOS CI | Clang | C11/C++17 syntax and linked host-stub smoke verified | Engine build unavailable |

The portable gate is `sh tests/run_checks.sh`. It validates the public header
as C11 and C++17, links and runs the current and old-minor C consumers against
an explicit host stub, checks the C gameplay example and Unreal descriptor JSON,
and enforces the 400–800 line budget for private implementation units. The host
stub proves consumer-side bootstrap, table calls, and the append-only prefix;
the CI matrix also repeats those linked consumers with AddressSanitizer and
UndefinedBehaviorSanitizer. These checks do not compile the Unreal module or
run PIE.

With an installed engine, run `UE_ROOT=/path/to/UnrealEngine
sh tests/run_unreal_build.sh` to compile, cook, stage, and package the minimal
host project for the current platform. Set `UEC_UNREAL_CONFIGURATION=Shipping`
to repeat the build in Shipping mode. The script exits with status 2 when the
engine path is unavailable, so the portable gate remains usable on contributors'
machines without Unreal installed.

The minimum consumer language standard is C11. The plugin implementation uses
C++17 through Unreal Build Tool; consumers may compile the public header as C11
or C++17. The recorded local baseline above is informational until a matching
UE 5.8.2 installation is available.

When an engine installation is available, record the exact UE patch, host OS,
architecture, compiler version, build configuration, and whether the check ran
in Editor PIE, packaged Development, packaged Shipping, or dedicated-server
mode. Keep generated engine output and local installation paths ignored.
