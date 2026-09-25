#include "c_consumer_drain.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

static uec_runtime_stats g_reported_stats;
static uec_result g_stats_result = UEC_RESULT_OK;
static uint32_t g_returned_stats_size;
static uint32_t g_stats_calls;

static uec_result UEC_CALL FakeGetRuntimeStats(uec_context* context,
                                                uec_runtime_stats* out_stats)
{
    ++g_stats_calls;
    if (context != (uec_context*)(uintptr_t)1u || out_stats == NULL ||
        out_stats->struct_size < offsetof(uec_runtime_stats, live_contexts)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    const size_t struct_size = out_stats->struct_size;
    const size_t copy_size = struct_size < sizeof(g_reported_stats) ?
        struct_size : sizeof(g_reported_stats);
    memcpy(out_stats, &g_reported_stats, copy_size);
    out_stats->struct_size = g_returned_stats_size == 0u ?
        (uint32_t)struct_size : g_returned_stats_size;
    return g_stats_result;
}

typedef struct uec_gate_worker_state {
    uec_consumer_drain_gate* gate;
    atomic_uint* ready_count;
    atomic_bool* start;
    atomic_uint* error_count;
    atomic_uint* admission_count;
} uec_gate_worker_state;

static void* GateWorker(void* opaque)
{
    uec_gate_worker_state* const worker = (uec_gate_worker_state*)opaque;
    atomic_fetch_add_explicit(worker->ready_count, 1u, memory_order_release);
    while (!atomic_load_explicit(worker->start, memory_order_acquire)) { }

    while (uec_consumer_drain_gate_try_begin(worker->gate) == UEC_TRUE) {
        atomic_fetch_add_explicit(worker->admission_count, 1u, memory_order_relaxed);
        if (uec_consumer_drain_gate_end(worker->gate) != UEC_RESULT_OK) {
            atomic_fetch_add_explicit(worker->error_count, 1u, memory_order_relaxed);
        }
    }
    return NULL;
}

static int VerifyDrainGate(const uec_api* api, uec_context* context)
{
    uec_consumer_drain_gate gate = UEC_CONSUMER_DRAIN_GATE_INITIALIZER;
    uec_bool drained = UEC_TRUE;
    const uint32_t initial_stats_calls = g_stats_calls;
    if (uec_consumer_drain_gate_init(&gate) != UEC_RESULT_OK ||
        uec_consumer_drain_gate_is_quiescent(&gate) != UEC_FALSE ||
        uec_consumer_drain_gate_try_begin(&gate) != UEC_TRUE ||
        uec_consumer_drain_gate_close(&gate) != UEC_RESULT_OK ||
        uec_consumer_drain_gate_try_begin(&gate) != UEC_FALSE) {
        return 0;
    }
    if (uec_consumer_drain_poll_gated(&gate, api, context, &drained, NULL) !=
            UEC_RESULT_OK || drained != UEC_FALSE || g_stats_calls != initial_stats_calls ||
        uec_consumer_drain_gate_destroy(&gate) != UEC_RESULT_INVALID_ARGUMENT) {
        return 0;
    }
    if (uec_consumer_drain_gate_end(&gate) != UEC_RESULT_OK ||
        uec_consumer_drain_gate_is_quiescent(&gate) != UEC_TRUE ||
        uec_consumer_drain_poll_gated(&gate, api, context, &drained, NULL) !=
            UEC_RESULT_OK || drained != UEC_TRUE ||
        g_stats_calls != initial_stats_calls + 1u ||
        uec_consumer_drain_gate_end(&gate) != UEC_RESULT_INVALID_ARGUMENT ||
        uec_consumer_drain_gate_destroy(&gate) != UEC_RESULT_OK ||
        uec_consumer_drain_gate_destroy(&gate) != UEC_RESULT_INVALID_ARGUMENT) {
        return 0;
    }

    if (uec_consumer_drain_gate_init(&gate) != UEC_RESULT_OK) return 0;
    enum { WORKER_COUNT = 8 };
    pthread_t threads[WORKER_COUNT];
    uec_gate_worker_state worker_states[WORKER_COUNT];
    atomic_uint ready_count = ATOMIC_VAR_INIT(0u);
    atomic_bool start = ATOMIC_VAR_INIT(false);
    atomic_uint error_count = ATOMIC_VAR_INIT(0u);
    atomic_uint admission_count = ATOMIC_VAR_INIT(0u);
    uint32_t created_count = 0u;
    for (; created_count < WORKER_COUNT; ++created_count) {
        worker_states[created_count].gate = &gate;
        worker_states[created_count].ready_count = &ready_count;
        worker_states[created_count].start = &start;
        worker_states[created_count].error_count = &error_count;
        worker_states[created_count].admission_count = &admission_count;
        if (pthread_create(&threads[created_count], NULL, GateWorker,
                           &worker_states[created_count]) != 0) {
            break;
        }
    }
    if (created_count != WORKER_COUNT) {
        (void)uec_consumer_drain_gate_close(&gate);
        atomic_store_explicit(&start, true, memory_order_release);
        for (uint32_t index = 0u; index < created_count; ++index) {
            (void)pthread_join(threads[index], NULL);
        }
        (void)uec_consumer_drain_gate_destroy(&gate);
        return 0;
    }
    while (atomic_load_explicit(&ready_count, memory_order_acquire) != WORKER_COUNT) { }
    atomic_store_explicit(&start, true, memory_order_release);
    while (atomic_load_explicit(&admission_count, memory_order_relaxed) < 1000u) { }
    const uec_result close_result = uec_consumer_drain_gate_close(&gate);
    for (uint32_t index = 0u; index < WORKER_COUNT; ++index) {
        if (pthread_join(threads[index], NULL) != 0) return 0;
    }
    if (close_result != UEC_RESULT_OK ||
        atomic_load_explicit(&error_count, memory_order_relaxed) != 0u ||
        uec_consumer_drain_gate_is_quiescent(&gate) != UEC_TRUE ||
        uec_consumer_drain_gate_destroy(&gate) != UEC_RESULT_OK) {
        return 0;
    }
    return 1;
}

static int ExpectNotDrained(const uec_api* api, uec_context* context)
{
    uec_bool drained = UEC_TRUE;
    if (uec_consumer_drain_poll(api, context, &drained, NULL) != UEC_RESULT_OK ||
        drained != UEC_FALSE) {
        return 0;
    }
    return 1;
}

int main(void)
{
    uec_api api = {0};
    uec_context* const context = (uec_context*)(uintptr_t)1u;
    uec_bool drained = UEC_TRUE;
    uec_runtime_stats observed = {0};
    api.struct_size = (uint32_t)(offsetof(uec_api, get_runtime_stats) +
                                 sizeof(api.get_runtime_stats));
    api.get_runtime_stats = &FakeGetRuntimeStats;
    g_reported_stats.struct_size = sizeof(g_reported_stats);

    if (uec_consumer_drain_poll(NULL, context, &drained, NULL) !=
            UEC_RESULT_INVALID_ARGUMENT || drained != UEC_FALSE) {
        return 1;
    }
    if (uec_consumer_drain_poll(&api, NULL, &drained, NULL) !=
            UEC_RESULT_INVALID_ARGUMENT || drained != UEC_FALSE) {
        return 8;
    }
    api.get_runtime_stats = NULL;
    if (uec_consumer_drain_poll(&api, context, &drained, NULL) !=
            UEC_RESULT_UNSUPPORTED || drained != UEC_FALSE) {
        return 9;
    }
    api.get_runtime_stats = &FakeGetRuntimeStats;

    api.struct_size = (uint32_t)offsetof(uec_api, get_runtime_stats);
    if (uec_consumer_drain_poll(&api, context, &drained, NULL) !=
            UEC_RESULT_UNSUPPORTED || drained != UEC_FALSE) {
        return 2;
    }
    api.struct_size = (uint32_t)(offsetof(uec_api, get_runtime_stats) +
                                 sizeof(api.get_runtime_stats));

    observed.struct_size = (uint32_t)(offsetof(uec_runtime_stats, live_contexts) - 1u);
    if (uec_consumer_drain_poll(&api, context, &drained, &observed) !=
            UEC_RESULT_INVALID_ARGUMENT || drained != UEC_FALSE) {
        return 10;
    }
    observed.struct_size = sizeof(observed);
    g_stats_result = UEC_RESULT_WRONG_THREAD;
    if (uec_consumer_drain_poll(&api, context, &drained, &observed) !=
            UEC_RESULT_WRONG_THREAD || drained != UEC_FALSE) {
        return 11;
    }
    g_stats_result = UEC_RESULT_OK;
    g_returned_stats_size = (uint32_t)(offsetof(uec_runtime_stats, live_contexts) - 1u);
    if (uec_consumer_drain_poll(&api, context, &drained, &observed) !=
            UEC_RESULT_INTERNAL_ERROR || drained != UEC_FALSE) {
        return 12;
    }
    g_returned_stats_size = 0u;

    g_reported_stats.active_subscriptions = 1u;
    if (!ExpectNotDrained(&api, context)) return 3;
    g_reported_stats.active_subscriptions = 0u;
    g_reported_stats.pending_requests = 1u;
    if (!ExpectNotDrained(&api, context)) return 4;
    g_reported_stats.pending_requests = 0u;
    g_reported_stats.active_callbacks = 1u;
    if (!ExpectNotDrained(&api, context)) return 5;
    g_reported_stats.active_callbacks = 0u;

    observed.struct_size = (uint32_t)offsetof(uec_runtime_stats, live_contexts);
    if (uec_consumer_drain_poll(&api, context, &drained, &observed) !=
            UEC_RESULT_OK || drained != UEC_TRUE ||
        observed.active_subscriptions != 0u || observed.pending_requests != 0u ||
        observed.active_callbacks != 0u) {
        return 6;
    }

    observed.struct_size = sizeof(observed);
    if (uec_consumer_drain_poll(&api, context, &drained, &observed) !=
            UEC_RESULT_OK || drained != UEC_TRUE ||
        observed.active_subscriptions != 0u || observed.pending_requests != 0u ||
        observed.active_callbacks != 0u) {
        return 7;
    }
    if (!VerifyDrainGate(&api, context)) return 13;
    return 0;
}
