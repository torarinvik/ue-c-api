# Private API organization

The implementation is split into focused include units so the module keeps one
shared Unreal handle registry and one exported ABI table while each subsystem
has a reviewable source file of its own. These `.inl` files are included from
`uec_api.cpp` inside its private implementation namespace; they are not public
headers and are not compiled as independent translation units.

- `uec_api_world_actor.inl` owns context, world, player, timer, world-tick, and world-framework access.
- `uec_api_actor_component.inl` owns actor/component lifetime, transforms,
  component enumeration, actor queries, and cross-subsystem handle cleanup.
- `uec_api_reflection.inl` owns reflected scalar/string properties and conversion helpers.
- `uec_api_reflection_metadata.inl` owns reflected class metadata, enum metadata,
  struct-field enumeration, and container element-kind metadata.
- `uec_api_reflection_containers.inl` owns reflected arrays, maps, sets, soft
  reference paths, nested structs, and caller-owned array text outputs.
- `uec_api_reflection_map_set.inl` owns reflected map/set enumeration, map
  value writes, and rehashed set element replacement, keeping hashed-container
  iteration and mutation together.
- `uec_api_reflection_invoke.inl` owns reflected invocation, scalar marshaling,
  and object-reference property adapters.
- `uec_api_presentation.inl` owns UMG, camera, audio (including completion
  subscriptions), mesh assignment, and transient animation playback.
- `uec_api_collision.inl` owns hit-result conversion, line/sweep/overlap queries,
  ignored-actor filters, and primitive collision settings.
- `uec_api_gameplay.inl` owns spatial sound, attachment, actor/component utilities,
  configuration, and component-hit subscriptions.
- `uec_api_input.inl` owns controller input, movement, Enhanced Input mappings,
  action bindings, and actor/scene-component physics adapters.
- `uec_api_async.inl` owns object loading, callback-based travel completion,
  save-game operations, game-thread dispatch, and shutdown cancellation.
- `uec_api_streaming.inl` owns level streaming state, immediate and callback-
  based travel, and cancellable streaming completion requests.
- `uec_api_runtime.inl` owns shared handle validation, UTF-8/numeric/geometry
  conversion, ID allocation, callback scope, queue limits, and shutdown
  diagnostics.

The shared handle types, registries, and ABI function table remain in
`uec_api.cpp` so moving a subsystem cannot change the public function ordering
or the lifetime rules for opaque handles.
