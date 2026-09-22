# Unreal Engine C API

This project is an Unreal Engine C API: an in-process runtime plugin that exposes selected Unreal Engine functionality through a versioned C ABI. It is intended for C programs and other languages that can call C functions through an FFI. The public API is under `Source/UnrealCAPI/Public`; engine-dependent code remains private to the module.

The project currently targets the latest Unreal Engine 5.8 release. It will be updated as newer Unreal Engine versions are released so the API remains compatible with the state of the art.

The first runtime slice establishes ABI negotiation, an opaque context, bounded diagnostics, logging, and explicit context release. The standalone C smoke consumer checks that the public header remains valid C11 without requiring Unreal headers.

The initial runtime API also provides game-thread world lookup plus opaque actor handles for spawning, destroying, reading, and setting transforms. See [docs/API.md](docs/API.md) for ownership and threading rules.

Run `sh tests/run_checks.sh` to validate the public C/C++ ABI headers and Unreal descriptors without an engine installation. The current feature boundary is tracked in [docs/FEATURES.md](docs/FEATURES.md).

The plugin must be built against a specific Unreal Engine 5.x version and toolchain. The host project is set to UE 5.8; record the exact engine patch and toolchain used for each build.

`UnrealCAPIHost.uproject` is the minimal host project for opening the plugin in Unreal Editor and running integration tests.

This project is dedicated to the public domain under the [Unlicense](LICENSE).
