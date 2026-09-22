# Private API organization

The implementation is split into focused include units so the module keeps one
shared Unreal handle registry and one exported ABI table while each subsystem
has a reviewable source file of its own. These `.inl` files are included from
`uec_api.cpp` inside its private implementation namespace; they are not public
headers and are not compiled as independent translation units.

- `uec_api_world_actor.inl` owns context, world, player, timer, world-tick, and world-framework access.
- `uec_api_actor_component.inl` owns actor/component lifetime, transforms,
  component enumeration, actor queries, and cross-subsystem handle cleanup.
- `uec_api_reflection.inl` owns class metadata, reflected properties, and object references.
- `uec_api_presentation.inl` owns UMG, camera, audio (including completion
  subscriptions), mesh assignment, and transient animation playback.
- `uec_api_gameplay.inl` owns collision, spatial sound, reflection invocation,
  attachment, component configuration, and physics queries.
- `uec_api_input.inl` owns controller input, movement, Enhanced Input mappings, and action bindings.
- `uec_api_async.inl` owns object loading, save-game operations, game-thread dispatch, and shutdown cancellation.

The shared handle types, registries, conversion helpers, and ABI function table
remain in `uec_api.cpp` so moving a subsystem cannot change the public function
ordering or the lifetime rules for opaque handles.
