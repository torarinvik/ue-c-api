# Unreal Engine C API

This project is an Unreal Engine C API: an in-process runtime plugin that exposes selected Unreal Engine functionality through a versioned C ABI. It is intended for C programs and other languages that can call C functions through an FFI. The public API is under `Source/UnrealCAPI/Public`; engine-dependent code remains private to the module.

The project currently targets the latest Unreal Engine 5.8 release. It will be updated as newer Unreal Engine versions are released so the API remains compatible with the state of the art.

The runtime API provides ABI negotiation, bounded diagnostics and logging,
explicit opaque handles, world and actor operations, reflection, collision and
physics queries, input and movement, camera and mesh presentation, audio, UMG,
save-game slots, and cancellable game-thread dispatch. See [docs/API.md](docs/API.md)
for ownership, threading, and unsupported-operation rules.

Run `sh tests/run_checks.sh` to validate the public C/C++ ABI headers and Unreal descriptors without an engine installation. The current feature boundary is tracked in [docs/FEATURES.md](docs/FEATURES.md).
The engine/toolchain verification fields and portable gate are recorded in [docs/BUILD_MATRIX.md](docs/BUILD_MATRIX.md).

GitHub Actions runs these checks on Linux and macOS for pushes and pull requests.
They check header syntax, selected layouts, and descriptor JSON; Unreal module
compilation and runtime tests still need an engine installation. See
[CONTRIBUTING.md](CONTRIBUTING.md) to participate and [CHANGELOG.md](CHANGELOG.md)
for changes in development.

Consumer startup, table compatibility, threading, callback ownership, handle
lifetime, and shutdown rules are collected in
[docs/CONSUMER_INTEGRATION.md](docs/CONSUMER_INTEGRATION.md).

The plugin must be built against a specific Unreal Engine 5.x version and
toolchain. The host project targets the current UE 5.8 release; record the
exact engine patch and toolchain used for each build. The project will track
newer Unreal releases when they become the state-of-the-art supported target.

`UnrealCAPIHost.uproject` is the minimal host project for opening the plugin in Unreal Editor and running integration tests.

This project is dedicated to the public domain under the [Unlicense](LICENSE).
