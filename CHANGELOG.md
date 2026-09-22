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
- Digital and analog input polling by Unreal key name.
- Actor velocity reads plus physics velocity, impulse, and force adapters for
  simulating primitive roots.
- World-aligned sphere, box, and capsule sweeps and bounded overlap queries.
- Fire-and-forget spatial playback for loaded `USoundBase` objects.
- Basic UMG widget creation and viewport add/remove operations.
- Camera-component field-of-view reads and writes.
- Generic reflected object property access plus synchronous save-game slot create,
  load, save, and delete operations.
- Cancellable game-thread callback dispatch for worker-thread callers.
- Pawn movement-input forwarding plus character jump and stop-jump adapters.
- Static and skeletal mesh assignment for compatible scene components.
- Skeletal animation play/stop and scalar/vector material parameter adapters.
- Explicit GC-tracked strong object handles alongside weak object handles.
- Scene-component class-path output and inheritance checks.
- Same-world scene-component attachment and detachment with transform rules.
- Actor class-path output and inheritance checks.
- Enhanced Input mapping-context add/remove adapters for local controllers.
- Reflected function enumeration with parameter, return, and latent metadata.
- Primitive collision-enabled modes and per-channel block/ignore responses.
- Attached audio-component spawn and stop adapters.
- Bounded async request queues with an explicit queue-full result.
- Typed Enhanced Input action-value polling for boolean and axis actions.
- Ignored-actor filtering for finite-endpoint line traces.
- Enhanced Input action-value injection for synthetic input and tests.
- Reflected enum property reads, names, and integer writes.
- Finite-value validation for physics, collision, and spatial-audio inputs.
- Typed hard object-reference property reads and writes.
- Bounded asynchronous save-game slot load and save callbacks.
- Indexed world actor queries by reflected actor class.
- Explicit destruction for attached audio components.
- Tokenized Enhanced Input action callbacks with unbinding and shutdown cleanup.
- Explicit local-player-index controller lookup and world game-instance access.
- Soft object-path loaded-state queries.
- C-only gameplay example covering spawn, transforms, timer callbacks, and cleanup.
- Input validation for oversized UTF-8 views and non-finite transform values.
- Reflected actor property writes and reads for supported scalar, string, name,
  and text types.
- Reflected property string access for Unreal's supported struct and container
  text serialization, including import and export for actor and object owners.
- Text-marshaled reflected actor-function calls with positional arguments and a
  bounded return or first-out value, excluding latent and network functions.
- A module-shutdown gate that rejects new contexts, invalidates handle checks,
  and suppresses late work while teardown cancels pending operations.
- World tick subscriptions with per-frame game-thread callbacks, unsubscribe
  tokens, world-destruction invalidation, and shutdown cleanup.
- One-shot audio-finished subscriptions for attached audio components, with
  unsubscribe tokens and cleanup when playback stops or components are destroyed.
- Full Unreal object-path and class-path queries for valid object handles.
- Typed UMG visibility and `UTextBlock` text adapters for C-driven interfaces.
- One-shot `UButton` click subscriptions with unsubscribe tokens and shutdown cleanup.
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
