#include "c_consumer_drain.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UEC_CONSUMER_GATE_CLOSED_BIT (UINT64_C(1) << 63)
#define UEC_CONSUMER_GATE_COUNT_MASK (UEC_CONSUMER_GATE_CLOSED_BIT - 1u)

typedef struct uec_consumer_drain_gate_state {
    _Atomic uint64_t state;
} uec_consumer_drain_gate_state;

uec_result UEC_CALL uec_consumer_drain_gate_init(uec_consumer_drain_gate* gate)
{
    if (gate == NULL || gate->state != NULL) return UEC_RESULT_INVALID_ARGUMENT;
    uec_consumer_drain_gate_state* state =
        (uec_consumer_drain_gate_state*)malloc(sizeof(*state));
    if (state == NULL) return UEC_RESULT_INTERNAL_ERROR;
    atomic_init(&state->state, 0u);
    gate->state = state;
    return UEC_RESULT_OK;
}

uec_bool UEC_CALL uec_consumer_drain_gate_try_begin(uec_consumer_drain_gate* gate)
{
    if (gate == NULL || gate->state == NULL) return UEC_FALSE;
    uec_consumer_drain_gate_state* gate_state =
        (uec_consumer_drain_gate_state*)gate->state;
    uint64_t state = atomic_load_explicit(&gate_state->state, memory_order_acquire);
    for (;;) {
        if ((state & UEC_CONSUMER_GATE_CLOSED_BIT) != 0u ||
            (state & UEC_CONSUMER_GATE_COUNT_MASK) == UEC_CONSUMER_GATE_COUNT_MASK) {
            return UEC_FALSE;
        }
        if (atomic_compare_exchange_weak_explicit(
                &gate_state->state, &state, state + 1u,
                memory_order_acq_rel, memory_order_acquire)) {
            return UEC_TRUE;
        }
    }
}

uec_result UEC_CALL uec_consumer_drain_gate_end(uec_consumer_drain_gate* gate)
{
    if (gate == NULL || gate->state == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    uec_consumer_drain_gate_state* gate_state =
        (uec_consumer_drain_gate_state*)gate->state;
    uint64_t state = atomic_load_explicit(&gate_state->state, memory_order_acquire);
    for (;;) {
        if ((state & UEC_CONSUMER_GATE_COUNT_MASK) == 0u) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (atomic_compare_exchange_weak_explicit(
                &gate_state->state, &state, state - 1u,
                memory_order_acq_rel, memory_order_acquire)) {
            return UEC_RESULT_OK;
        }
    }
}

uec_result UEC_CALL uec_consumer_drain_gate_close(uec_consumer_drain_gate* gate)
{
    if (gate == NULL || gate->state == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    uec_consumer_drain_gate_state* gate_state =
        (uec_consumer_drain_gate_state*)gate->state;
    atomic_fetch_or_explicit(
        &gate_state->state, UEC_CONSUMER_GATE_CLOSED_BIT, memory_order_acq_rel);
    return UEC_RESULT_OK;
}

uec_bool UEC_CALL uec_consumer_drain_gate_is_quiescent(
    const uec_consumer_drain_gate* gate)
{
    if (gate == NULL || gate->state == NULL) return UEC_FALSE;
    const uec_consumer_drain_gate_state* gate_state =
        (const uec_consumer_drain_gate_state*)gate->state;
    const uint64_t state = atomic_load_explicit(&gate_state->state, memory_order_acquire);
    return (state & UEC_CONSUMER_GATE_CLOSED_BIT) != 0u &&
        (state & UEC_CONSUMER_GATE_COUNT_MASK) == 0u ? UEC_TRUE : UEC_FALSE;
}

uec_result UEC_CALL uec_consumer_drain_gate_destroy(uec_consumer_drain_gate* gate)
{
    if (gate == NULL || gate->state == NULL ||
        uec_consumer_drain_gate_is_quiescent(gate) != UEC_TRUE) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    free(gate->state);
    gate->state = NULL;
    return UEC_RESULT_OK;
}

uec_result UEC_CALL uec_consumer_drain_poll(const uec_api* api,
                                             uec_context* context,
                                             uec_bool* out_drained,
                                             uec_runtime_stats* out_stats)
{
    const size_t required_api_size = offsetof(uec_api, get_runtime_stats) +
                                    sizeof(api->get_runtime_stats);
    if (out_drained != NULL) *out_drained = UEC_FALSE;
    if (api == NULL || context == NULL || out_drained == NULL) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    if (api->struct_size < required_api_size || api->get_runtime_stats == NULL) {
        return UEC_RESULT_UNSUPPORTED;
    }

    const size_t minimum_stats_size = offsetof(uec_runtime_stats, live_contexts);
    size_t stats_size = out_stats == NULL ? sizeof(uec_runtime_stats) :
        (size_t)out_stats->struct_size;
    if (stats_size < minimum_stats_size) return UEC_RESULT_INVALID_ARGUMENT;
    if (stats_size > sizeof(uec_runtime_stats)) stats_size = sizeof(uec_runtime_stats);

    uec_runtime_stats stats = {0};
    stats.struct_size = (uint32_t)stats_size;
    const uec_result result = api->get_runtime_stats(context, &stats);
    if (out_stats != NULL) memcpy(out_stats, &stats, stats_size);
    if (result != UEC_RESULT_OK) return result;
    if (stats.struct_size < minimum_stats_size) return UEC_RESULT_INTERNAL_ERROR;

    *out_drained = stats.active_subscriptions == 0u &&
        stats.pending_requests == 0u && stats.active_callbacks == 0u
        ? UEC_TRUE : UEC_FALSE;
    return UEC_RESULT_OK;
}

uec_result UEC_CALL uec_consumer_drain_poll_gated(
    const uec_consumer_drain_gate* gate,
    const uec_api* api,
    uec_context* context,
    uec_bool* out_drained,
    uec_runtime_stats* out_stats)
{
    if (out_drained != NULL) *out_drained = UEC_FALSE;
    if (gate == NULL || out_drained == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (uec_consumer_drain_gate_is_quiescent(gate) != UEC_TRUE) {
        return UEC_RESULT_OK;
    }
    return uec_consumer_drain_poll(api, context, out_drained, out_stats);
}
