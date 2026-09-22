# Changelog

User-visible changes are recorded here. Unreleased entries describe work in
development; they do not imply a published or runtime-verified release.

## Unreleased

### Added

- Initial Unreal Engine C API runtime plugin scaffold targeting the latest UE
  5.8 release.
- Standard `Plugins/UnrealCAPI` project-plugin layout with tracked host Game and
  Editor targets for Unreal Build Tool discovery.
- A tracked C host bootstrap probe that exercises table negotiation, capability
  discovery, logging, and context release when the host starts.
- Explicit private export-marker definition for Windows plugin builds so C
  consumers import the bootstrap symbol through the intended module boundary.
- Calling-thread bounded diagnostics for invalid or stale handles, malformed
  UTF-8 views, missing required-size outputs, and undersized buffers.
- Cross-platform Unreal build requests now skip rebuilding the local Editor
  target while preserving the host-platform default.
- The portable C smoke consumer now verifies that an invalid handle produces a
  bounded readable diagnostic through `get_last_error`.
- The portable gate links and executes the Unreal host's tracked C bootstrap
  translation unit against the host stub.
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
- External Unreal world cleanup now cancels world-owned work and invalidates
  world-bound bridge handles, including during PIE restart and non-bridge travel.
- Collision and Enhanced Input bindings now install per-world actor-destruction
  cleanup, removing external-owner bindings and tombstoning related handles.
- Added an opt-in `tests/run_unreal_build.sh` gate for compiling, cooking,
  staging, and packaging the host project when `UE_ROOT` is available.
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
- Reflected unsigned integer reads and writes now reject values that cannot be
  represented by the ABI's signed 64-bit property field.
- Vector, transform, and box-shape inputs now reject finite doubles that would
  overflow Unreal's float-backed math types.
- Transform writes now reject a zero-length quaternion before constructing the
  Unreal transform.
- Worker-thread dispatch now checks the shutdown gate while registering a
  request, preventing work from being queued after teardown has drained it.
- Actor destruction now removes collision and Enhanced Input subscriptions
  attached to the actor's components, including safe in-flight cancellation.
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
- Linked C consumer smoke fixture using an explicit host stub for portable ABI
  bootstrap and table-call coverage.
- The portable fixture now links and runs the independent old-minor consumer
  against the same stub to exercise the append-only compatibility prefix.
- CI repeats the linked consumers with AddressSanitizer and
  UndefinedBehaviorSanitizer coverage.
- The C smoke consumer now verifies unsupported ABI requests clear output
  pointers before returning.
- `get_last_error` now validates its context and requires the caller's
  required-size output, matching other bounded string APIs.
- Reflected `FFloatProperty` writes now reject finite doubles outside Unreal's
  representable float range before conversion.
- Object, save-game, path-query, and queued-request outputs now clear at entry,
  so invalid calls cannot leave stale handles, booleans, or request ids in the
  caller's variables.
- Diagnostic required-size outputs now clear before context validation, keeping
  invalid-context failures deterministic with the other bounded-output APIs.
- Reflected actor invocation now rejects network functions and authority-only
  functions on client worlds before entering `ProcessEvent`.
- Possession and view-target changes now reject actor handles from different
  worlds or PIE instances before calling Unreal.
- Typed handle validation now checks the underlying weak object reference, so
  metadata queries reject destroyed or unloaded worlds, actors, components,
  classes, and objects consistently.
- World, player, timer, and streaming outputs now clear before validation,
  including required sizes for bounded world names and streaming package names.
- Actor, component, and object handle constructors now recheck the shutdown gate
  while registering, preventing new live handles from appearing during teardown.
- Class lookup now applies the same shutdown gate while registering class handles,
  and reflected class, property, and function outputs are initialized before
  validation so failed calls cannot preserve stale caller data.
- Enhanced Input action-value polling now clears its tagged payload before
  validating controller and action handles.
- Timer, world-tick, input, audio, widget, animation, and component-hit
  registration now rolls back the native engine binding if shutdown begins
  before the bridge records the subscription.
- Asynchronous object-load and save-game requests now register through a gated
  helper that applies queue limits and monotonic request ids atomically with
  shutdown admission.
- Class and object handle release now follows the documented Unreal game-thread
  boundary before clearing weak or strong engine references.
- World handle release now follows the same game-thread boundary before clearing
  its Unreal weak reference.
- Bounded world, class, object, component, and actor-name outputs now reject a
  null required-size pointer before touching the associated Unreal handle.
- Explicit audio-component destruction now tombstones and clears the bridge
  handle, including any retained strong reference, before destroying the
  Unreal component.
- Shutdown object-load cancellation now snapshots requests before canceling
  streamable handles, remaining safe if cancellation completes immediately.
- ABI minor 82 adds `get_runtime_stats`, a game-thread drain diagnostic for
  subscriptions, pending requests, and callbacks before consumer unload.
- ABI minor 83 adds explicit world-kind enumeration for editor, PIE,
  game-preview, inactive, and game contexts.
- ABI minor 84 adds scalar reflected function invocation for boolean, integer,
  enum, float, and double arguments with a return or first-out value.
- ABI minor 85 adds scalar reflected function invocation with a caller-sized
  output array containing the return value and every scalar out parameter.
- ABI minor 86 adds reflected function-parameter metadata for names, property
  kinds, and input/output/return/reference flags.
- ABI minor 87 extends runtime drain statistics with live handle counts by
  context, world, actor, component, class, and object kind.
- ABI minor 88 adds reflected text invocation with caller-owned output buffers
  for the return value and every out parameter.
- ABI minor 89 adds non-loading full-path object lookup with an explicit
  `UEC_RESULT_NOT_INITIALIZED` cache-miss result.
- ABI minor 90 adds cancellable level-travel requests with post-load world
  callbacks and automatic cleanup during module shutdown.
- ABI minor 91 adds component visibility and activation readback adapters.
- ABI minor 92 adds reflected function capability flags for callable, native,
  event, latent, network, and authority-only functions.
- ABI minor 93 adds UMG visibility and text-block readback with caller-sized
  UTF-8 output buffers.
- ABI minor 94 adds primitive collision-mode and audio-component playback
  state readback.
- ABI minor 95 adds bounded, cancellable streaming-level state completion
  requests with game-thread callbacks and world/shutdown cleanup.
- ABI minor 96 adds per-channel collision response readback for ignore, overlap,
  and block modes.
- ABI minor 97 adds validated game-INI integer get/set helpers.
- ABI minor 98 adds one-shot actor-destruction callbacks with explicit
  unbinding and world/shutdown cleanup.
- ABI minor 99 adds validated game-INI boolean readback.
- ABI minor 100 adds actor reflected-array count and caller-sized text-element
  readback.
- ABI minor 101 adds reflected UObject-array count and caller-sized text-element
  readback for retained and other loaded object handles.
- ABI minor 102 adds reflected UObject-map counts and key/value text entries plus
  UObject-set counts and element text readback using `uec_text_output` records.
- ABI minor 103 adds explicit actor and UObject soft object/class property path
  readback and distinct soft-reference property kinds.
- ABI minor 104 adds actor map/set counts and caller-owned key/value or element
  text readback; container adapters now live in a dedicated reflection unit.
- ABI minor 105 adds writable actor and UObject soft object/class path
  properties with reflected access checks and text import validation.
- ABI minor 106 adds one-level nested reflected-struct field text readback for
  actor and UObject properties.
- ABI minor 107 adds writable one-level nested reflected-struct fields with
  outer and field access checks.
- ABI minor 108 adds text writes for indexed reflected array elements on actor
  and UObject properties.
- ABI minor 109 adds actor and UObject map value text writes while preserving
  map keys.
- ABI minor 110 adds reflected class-property access flags for editability,
  Blueprint read-only state, parameter, return, out, and reference metadata.
- ABI minor 111 adds typed scalar reads for reflected actor and UObject array
  elements using `uec_property_value`; compound elements retain text access.
- ABI minor 112 adds typed scalar reads for reflected actor and UObject map
  values and set elements using `uec_property_value`.
- ABI minor 113 adds typed scalar reads for nested reflected-struct fields,
  including dotted paths, using `uec_property_value`.
- ABI minor 114 adds typed scalar writes for reflected actor and UObject array
  elements and map values with the existing access and range validation.
- ABI minor 115 adds typed scalar writes for nested reflected-struct fields,
  including dotted paths.
- ABI minor 116 adds class-default reflected property text readback.
- ABI minor 117 adds referenced-class path metadata for reflected object, class,
  and soft-reference properties.
- ABI minor 118 adds reflected enum name/value enumeration for `FEnumProperty`
  and byte-backed enum properties.
- ABI minor 119 adds reflected struct-field enumeration with field names, kinds,
  and access flags.
- ABI minor 120 adds typed hard class-property reads and writes for actor and
  UObject owners with reflected `MetaClass` validation.
- ABI minor 120 also aligns byte-backed enum kinds and typed writes with
  declared enum-value validation.
- ABI minor 121 adds reflected struct type paths for schema-aware field access.
- ABI minor 122 adds reflected array, map, and set element-kind metadata.
- ABI minor 123 adds duplicate-safe text and typed scalar replacement for existing set elements with rehashing.
- ABI minor 124 adds size-tagged detailed line/sweep hit readback through
  `trace_detailed` and `trace_detailed_filtered`, and advertises
  `UEC_CAPABILITY_COLLISION_DETAILS`.
- ABI minor 125 adds scene-component physics velocity, impulse, and force
  operations with game-thread, simulating-component, and authority validation.
- ABI minor 126 adds scene-component angular-velocity readback, angular-velocity
  writes, and torque application for simulating primitive components.
- ABI minor 127 adds scene-component angular impulse application in radians.
- ABI minor 128 adds actor-root angular-velocity readback, angular velocity
  writes, torque, and angular impulse operations.
- ABI minor 129 adds typed soft-reference path reads and kind-checked writes for
  actor and UObject properties.
- ABI minor 130 adds typed scalar-key reads for actor and UObject reflected maps.
- ABI minor 131 adds mixed typed actor-function arguments and typed, handle, or
  text-backed return/out values.
- ABI minor 132 adds a Blueprint-assignable actor event component, bounded C
  callback subscriptions, event emission from C, and teardown cleanup. The host
  runtime smoke path exercises payload validation, explicit and in-callback
  unbinding, component teardown cleanup, and stale-handle rejection in Game/PIE.
- The portable old-consumer fixture now requests ABI 1.131 from the ABI 1.132
  bridge and exercises only the stable table prefix.
- Adds `UEC_CAPABILITY_REFLECTION_CONTAINERS` so consumers can gate container metadata and set mutation explicitly.
- All subscription categories now have a 1024-entry bound and return
  `UEC_RESULT_QUEUE_FULL` instead of growing without limit.
- Level travel now cancels world-owned timers and tick subscriptions and
  actor-scoped collision/input subscriptions, then invalidates world, actor,
  component, and world-bound object bridge handles immediately before
  submitting the request while leaving global asset handles valid.
- One-shot widget, audio, and component-hit callbacks now avoid touching a raw
  Unreal delegate after cancellation or destruction from inside the callback.
- Asynchronous object-load completions now use the shared gated object-handle
  constructor instead of maintaining a separate registry insertion path.
- Default and indexed world lookup now share one gated world-handle constructor,
  removing duplicate registry insertion behavior.
- Actor and scene-component queries now clear handles, counts, booleans,
  transforms, bounds, and bounded-name sizes before validation.
- Widget, audio, camera, and animation adapters now clear handles, scalar
  values, and subscription ids before validation.
- Collision, retained-object, identity, configuration, and hit-callback
  adapters now clear their handles, records, counts, booleans, sizes, and ids
  before validation.
- Portable old-minor ABI-prefix compatibility fixture for C consumers.
- Public-domain license, contribution guide, and API/feature documentation.
- C consumer integration guide covering bootstrap, compatibility, threading,
  callbacks, handles, and shutdown.
- Cooked-asset guidance covering runtime paths, Asset Manager rules, Primary
  Asset Labels, and path preflight checks.
- Authority-gated actor tag add/remove support appended as ABI minor 81.
- Reflected enum writes now require a declared enum value or valid bitfield
  combination in addition to the underlying integer range.
- Private adapters are split into actor/component, presentation, gameplay,
  input, reflection, async, and world units with an enforced 400–800 line
  source budget.
- Collision queries and primitive collision settings now live in a dedicated
  adapter unit, while gameplay retains audio, configuration, and subscriptions;
  both units remain within the enforced 400–800 line source budget.

### Changed

- Local checks accept `CC` and `CXX` compiler overrides and reject language
  extensions, including C-only constructs when compiling the consumer as C++.
- Nested reflected-struct field readers and writers now resolve dotted paths
  such as `Transform.Location.X` while retaining the existing text contract.

### Validation status

- CI checks syntax and selected type layouts; it does not link or execute the
  consumer, compile the Unreal module, or test engine behavior.
- UE 5.8.2 editor and packaged-build verification remain pending. The API is
  experimental and may change incompatibly before a supported release.
