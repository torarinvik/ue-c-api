#ifndef UEC_CONSUMER_DRAIN_H
#define UEC_CONSUMER_DRAIN_H

#include "uec_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Admission gate for every consumer operation that can create callbacks or
 * queued work. Start with UEC_CONSUMER_DRAIN_GATE_INITIALIZER and initialize
 * before producers start. A successful try_begin must be paired with end
 * after the API submission returns. Initialize the gate from
 * UEC_CONSUMER_DRAIN_GATE_INITIALIZER. Stop all gate callers before destroy.
 * The opaque storage keeps this header usable from both C and C++.
 */
typedef struct uec_consumer_drain_gate {
    void* state;
} uec_consumer_drain_gate;

#define UEC_CONSUMER_DRAIN_GATE_INITIALIZER { NULL }

uec_result UEC_CALL uec_consumer_drain_gate_init(uec_consumer_drain_gate* gate);
uec_bool UEC_CALL uec_consumer_drain_gate_try_begin(uec_consumer_drain_gate* gate);
uec_result UEC_CALL uec_consumer_drain_gate_end(uec_consumer_drain_gate* gate);
uec_result UEC_CALL uec_consumer_drain_gate_close(uec_consumer_drain_gate* gate);
uec_bool UEC_CALL uec_consumer_drain_gate_is_quiescent(
    const uec_consumer_drain_gate* gate);
uec_result UEC_CALL uec_consumer_drain_gate_destroy(uec_consumer_drain_gate* gate);

/* Stats-only poll: it does not synchronize with concurrent submitters. */
uec_result UEC_CALL uec_consumer_drain_poll(const uec_api* api,
                                             uec_context* context,
                                             uec_bool* out_drained,
                                             uec_runtime_stats* out_stats);

/* This form cannot report drained until the admission gate is closed and idle. */
uec_result UEC_CALL uec_consumer_drain_poll_gated(
    const uec_consumer_drain_gate* gate,
    const uec_api* api,
    uec_context* context,
    uec_bool* out_drained,
    uec_runtime_stats* out_stats);

#ifdef __cplusplus
}
#endif

#endif
