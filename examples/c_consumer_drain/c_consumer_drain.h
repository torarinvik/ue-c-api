#ifndef UEC_CONSUMER_DRAIN_H
#define UEC_CONSUMER_DRAIN_H

#include "uec_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Call on the game thread after stopping new submissions and canceling or
 * unsubscribing all work owned by the consumer module. Keep callback code,
 * user data, the API table, and context alive until out_drained becomes true.
 */
uec_result UEC_CALL uec_consumer_drain_poll(const uec_api* api,
                                             uec_context* context,
                                             uec_bool* out_drained,
                                             uec_runtime_stats* out_stats);

#ifdef __cplusplus
}
#endif

#endif
