# Changelog

User-visible changes are recorded here. Unreleased entries describe work in
development; they do not imply a published or runtime-verified release.

## Unreleased

### Added

- Initial Unreal Engine C API runtime plugin scaffold targeting the latest UE
  5.8 release.
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
- Primitive scene-component velocity reads for moving mesh and physics components.
- PIE instance identifiers on world handles for distinguishing simultaneous PIE worlds.
- Read-only world network-mode queries for standalone, client, listen-server, and dedicated-server contexts.
- Actor tag enumeration, world-space actor bounds, and indexed player-start lookup.
- Tokenized one-shot skeletal-animation completion callbacks with cancellation
  and shutdown cleanup.
- Read-only world-authority queries for standalone, client, listen-server, and
  dedicated-server workflows.
- Explicit authoritative game-mode object access from world handles, with a
  clear unsupported result on client worlds.
- Explicit world game-state object access from world handles, with a clear
  not-initialized result before the world has created its game state.
- Class-filtered scene-component count and indexed lookup for actor handles,
  with explicit `USceneComponent` class validation.
- Game-INI string reads and writes with game-thread validation, bounded UTF-8
  output, immediate flushes, and a dedicated configuration capability bit.
- Indexed world streaming-level inspection and game-thread load/visibility state
  requests using Unreal package names.
- Non-loading class-path availability queries for cook and dependency preflight.
- One-shot primitive-component hit callbacks with tokenized unbinding and
  shutdown cleanup.
- Filtered sphere, box, and capsule sweeps plus bounded overlap queries with
  ignored-actor arrays and output counts that match the handles written.
- Float-backed engine parameters reject finite C values that would overflow
  during conversion from the API's double-precision inputs.
- Enhanced Input callbacks now defer native binding removal when unbinding from
  inside the callback and suppress delivery after shutdown begins.
- Async object-load requests are registered before dispatch so an immediate
  completion cannot leave an orphaned request in the shutdown queue.
- World, object, and class name reads now enforce the documented game-thread
  boundary before touching Unreal reflection objects.
- Module shutdown now emits a verbose resource summary before cleanup, covering
  live handles, subscriptions, bindings, and queued requests.
- Actor and attached-audio creation clean up the newly created Unreal object if
  bridge-handle allocation fails.
- Queued game-thread, object-load, and save-game callbacks now recheck the
  shutdown gate immediately before crossing back into consumer code.
- Timer callbacks now apply the same shutdown gate before invoking consumer
  code.
- Handle-adjacent request, timer, binding, and subscription IDs now stop with
  an internal error instead of wrapping to zero after counter exhaustion.
- Timer dispatch drops world-invalidated timers before invoking their consumer
  callback.
- Timer intervals now reject values that cannot be represented by Unreal's
  float-backed timer rate.
- Reflected property writes now reject Unreal read-only and parameter flags,
  while read access remains available for those properties.
- Reflected floating-point property writes reject non-finite values before
  entering Unreal's property storage.
- Reflected integer and enum writes now reject values outside the underlying
  Unreal property's representable range.
- Failed Enhanced Input binding creation now removes the native delegate before
  returning an internal error, preventing an untracked callback from surviving.
- Strict UTF-8 validation for public string views, including rejection of overlong encodings, surrogates, and out-of-range code points.
- Embedded NUL rejection for all public UTF-8 string views, including reflected,
  input, save-game, and configuration adapters.
- Explicit overflow checks on world, component, class-property, and reflected-function enumeration counts.
- Strict 0/1 validation for boolean inputs across transforms, collision,
  attachment, timer, physics, movement, animation, input, and property writes.
- C consumer source and public math-type layout assertions.
- GitHub Actions checks for C11/C++17 headers and descriptor JSON on Linux
  (GCC and Clang) and macOS (Clang), on pushes and pull requests.
- Portable old-minor ABI-prefix compatibility fixture for C consumers.
- Public-domain license, contribution guide, and API/feature documentation.
- C consumer integration guide covering bootstrap, compatibility, threading,
  callbacks, handles, and shutdown.
- Private adapters are split into actor/component, presentation, gameplay,
  input, reflection, async, and world units with an enforced 400–800 line
  source budget.

### Changed

- Local checks accept `CC` and `CXX` compiler overrides and reject language
  extensions, including C-only constructs when compiling the consumer as C++.

### Validation status

- CI checks syntax and selected type layouts; it does not link or execute the
  consumer, compile the Unreal module, or test engine behavior.
- UE 5.8.2 editor and packaged-build verification remain pending. The API is
  experimental and may change incompatibly before a supported release.
