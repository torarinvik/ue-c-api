#ifndef UEC_GAMEPLAY_EXAMPLE_H
#define UEC_GAMEPLAY_EXAMPLE_H

#include "uec_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The host owns this state and its context until done becomes UEC_TRUE. */
typedef struct uec_gameplay_example_state {
    const uec_api* api;
    uec_context* context;
    uec_world* world;
    uec_actor* actor;
    uec_object* event_bridge;
    uint64_t timer_id;
    uint64_t event_subscription_id;
    uint32_t ticks;
    uint32_t events_received;
    uec_result last_result;
    uec_bool done;
} uec_gameplay_example_state;

/* Start and cancel must run on the game thread. The caller retains context. */
uec_result UEC_CALL uec_gameplay_example_start(
    const uec_api* api,
    uec_context* context,
    uec_string_view actor_class_path,
    const uec_transform* initial_transform,
    uec_gameplay_example_state* state);
void UEC_CALL uec_gameplay_example_cancel(uec_gameplay_example_state* state);

#ifdef __cplusplus
}
#endif

#endif
