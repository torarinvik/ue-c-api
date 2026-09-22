#include "uec_api.h"

#include <stddef.h>
#include <stdint.h>

/* The host must keep this state alive until done becomes UEC_TRUE. */
typedef struct uec_gameplay_example_state {
    const uec_api* api;
    uec_world* world;
    uec_actor* actor;
    uint64_t timer_id;
    uint32_t ticks;
    uec_bool done;
} uec_gameplay_example_state;

static void UEC_CALL MoveActorOnTimer(uint64_t timer_id, void* raw_state)
{
    uec_gameplay_example_state* state = (uec_gameplay_example_state*)raw_state;
    if (state == NULL || state->api == NULL || state->actor == NULL) return;

    uec_transform transform;
    if (state->api->get_actor_transform(state->actor, &transform) == UEC_RESULT_OK)
    {
        transform.translation.x += 10.0;
        (void)state->api->set_actor_transform(state->actor, &transform, UEC_TRUE);
    }
    ++state->ticks;
    if (state->ticks < 3u) return;

    (void)state->api->clear_timer(state->world, timer_id);
    (void)state->api->release_actor(state->actor);
    (void)state->api->release_world(state->world);
    state->actor = NULL;
    state->world = NULL;
    state->timer_id = 0;
    state->done = UEC_TRUE;
}

/* Starts a small game-thread example. The supplied state is borrowed by the
 * timer callback and must remain valid until state->done is UEC_TRUE. */
uec_result uec_gameplay_example_start(const uec_api* api,
                                      uec_context* context,
                                      uec_string_view actor_class_path,
                                      const uec_transform* initial_transform,
                                      uec_gameplay_example_state* state)
{
    if (api == NULL || context == NULL || initial_transform == NULL || state == NULL)
    {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    *state = (uec_gameplay_example_state){api, NULL, NULL, 0, 0, UEC_FALSE};

    uec_result result = api->get_default_world(context, &state->world);
    if (result != UEC_RESULT_OK) return result;
    result = api->spawn_actor(state->world, actor_class_path, initial_transform, &state->actor);
    if (result != UEC_RESULT_OK)
    {
        (void)api->release_world(state->world);
        state->world = NULL;
        return result;
    }

    result = api->set_timer(state->world, 0.1, UEC_TRUE, &MoveActorOnTimer, state, &state->timer_id);
    if (result != UEC_RESULT_OK)
    {
        (void)api->release_actor(state->actor);
        (void)api->release_world(state->world);
        state->actor = NULL;
        state->world = NULL;
        return result;
    }
    return UEC_RESULT_OK;
}
