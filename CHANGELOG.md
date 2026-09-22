# Changelog

User-visible changes are recorded here. Unreleased entries describe work in
development; they do not imply a published or runtime-verified release.

## Unreleased

### Added

- Initial Unreal Engine C API runtime plugin scaffold targeting UE 5.8.
- Public C function table with version negotiation and capability discovery.
- Initial world, actor, scene-component, timer, collision line-trace, and class
  metadata adapters.
- Bounded reflected scalar/string/name/text property reads and writes.
- A bounded zero-argument reflected actor-function invocation path.
- Stable collision-channel line traces with POD hit results.
- Synchronous weak UObject loading with names and type checks.
- Asynchronous streamable object loading with cancellation and game-thread callbacks.
- World map-name queries and game-thread level-travel requests.
- Local player-controller/pawn lookup, possession, and view-target selection.
- Reflected actor property writes and reads for supported scalar, string, name,
  and text types.
- C consumer source and public math-type layout assertions.
- GitHub Actions checks for C11/C++17 headers and descriptor JSON on Linux
  (GCC and Clang) and macOS (Clang), on pushes and pull requests.
- Public-domain license, contribution guide, and API/feature documentation.

### Changed

- Local checks accept `CC` and `CXX` compiler overrides and reject language
  extensions, including C-only constructs when compiling the consumer as C++.

### Validation status

- CI checks syntax and selected type layouts; it does not link or execute the
  consumer, compile the Unreal module, or test engine behavior.
- UE 5.8 editor and packaged-build verification remain pending. The API is
  experimental and may change incompatibly before a supported release.
