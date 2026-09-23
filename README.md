# Unreal Engine C API

This project is an Unreal Engine C API: an in-process runtime plugin that exposes selected Unreal Engine functionality through a versioned C ABI. It is intended for C programs and other languages that can call C functions through an FFI. The public API is under `Plugins/UnrealCAPI/Source/UnrealCAPI/Public`; engine-dependent code remains private to the module.

The project currently targets Unreal Engine 5.8.3, the latest 5.8 hotfix
available as of September 23, 2026. See [Epic's release notice](https://forums.unrealengine.com/t/5-8-3-hotfix-released/2833315).
It will be updated and verified as newer Unreal Engine versions are released,
so the API stays compatible with the state of the art.

The runtime API provides ABI negotiation, bounded diagnostics and logging,
explicit opaque handles, world and actor operations, reflection, collision and
physics queries, input and movement, camera and mesh presentation, audio, UMG,
save-game slots, configuration, streaming-level controls, asset preflight,
authority gates, a Blueprint-to-C event bridge, asynchronous reflected latent
calls, and cancellable game-thread dispatch. See [docs/API.md](docs/API.md)
for ownership, threading, and unsupported-operation rules.

Run `sh tests/run_checks.sh` to validate the public C/C++ ABI headers and Unreal descriptors without an engine installation. The current feature boundary is tracked in [docs/FEATURES.md](docs/FEATURES.md).
The engine/toolchain verification fields and portable gate are recorded in [docs/BUILD_MATRIX.md](docs/BUILD_MATRIX.md).

GitHub Actions runs these checks on Linux and macOS for pushes and pull requests.
They check header syntax, selected layouts, and descriptor JSON; Unreal module
compilation and runtime tests still need an engine installation. See
[CONTRIBUTING.md](CONTRIBUTING.md) to participate and [CHANGELOG.md](CHANGELOG.md)
for changes in development. The self-cleaning C host example is in
[examples/c_gameplay/README.md](examples/c_gameplay/README.md).

When Unreal Engine is installed locally, `UE_ROOT=/path/to/UnrealEngine sh
tests/run_unreal_build.sh` compiles, cooks, stages, and packages the host project.
For a same-platform Development build, it launches the packaged host and waits
for the C bootstrap, event-bridge, latent-call, concurrent and cancellable
game-thread queue, asynchronous save/load, asynchronous object load,
gameplay-example, and world-travel smoke checks. The persistence check verifies
callback accounting and removes its temporary save slot; the object-load check
resolves the native `Actor` class and verifies the returned path and handle
accounting.
Set `UEC_UNREAL_CONFIGURATION=Shipping` for a Shipping package; runtime smoke is
limited to Development builds.

Consumer startup, table compatibility, threading, callback ownership, handle
lifetime, and shutdown rules are collected in
[docs/CONSUMER_INTEGRATION.md](docs/CONSUMER_INTEGRATION.md).
The append-only ABI, deprecation, and migration rules are in
[docs/ABI_COMPATIBILITY.md](docs/ABI_COMPATIBILITY.md).

The plugin must be built against a specific Unreal Engine version and
toolchain. Record the exact engine patch and toolchain used in
[`docs/BUILD_MATRIX.md`](docs/BUILD_MATRIX.md) when adding a verified build.

`UnrealCAPIHost.uproject` is the minimal host project for opening the plugin in Unreal Editor and running integration tests.

This project is dedicated to the public domain under the [Unlicense](LICENSE).
