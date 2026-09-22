# Build and verification matrix

This file records the engine and toolchain combinations used to validate the
plugin. A row is only marked verified after the Unreal module and host project
build, launch, and exercise the C smoke path.

| Engine | Host platform | Compiler/toolchain | C consumer | Plugin/host status |
| --- | --- | --- | --- | --- |
| UE 5.8 (host descriptor target) | Not recorded | Not recorded | C11 syntax verified | Unreal build pending |
| UE 5.8 (host descriptor target) | Linux CI | GCC and Clang | C11/C++17 header syntax verified | Engine build unavailable |
| UE 5.8 (host descriptor target) | macOS CI | Clang | C11/C++17 header syntax verified | Engine build unavailable |

The portable gate is `sh tests/run_checks.sh`. It validates the public header
as C11 and C++17, the C gameplay example, Unreal descriptor JSON, and the
400–800 line budget for private implementation units. It does not compile the
Unreal module or run PIE.

When an engine installation is available, record the exact UE patch, host OS,
architecture, compiler version, build configuration, and whether the check ran
in Editor PIE, packaged Development, packaged Shipping, or dedicated-server
mode. Keep generated engine output and local installation paths ignored.
