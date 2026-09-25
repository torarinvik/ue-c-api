#include "c_consumer_drain.h"

#include <stdint.h>
#include <string.h>

static uec_runtime_stats g_reported_stats;

static uec_result UEC_CALL FakeGetRuntimeStats(uec_context* context,
                                                uec_runtime_stats* out_stats)
{
    if (context != (uec_context*)(uintptr_t)1u || out_stats == NULL ||
        out_stats->struct_size < offsetof(uec_runtime_stats, live_contexts)) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    const size_t struct_size = out_stats->struct_size;
    const size_t copy_size = struct_size < sizeof(g_reported_stats) ?
        struct_size : sizeof(g_reported_stats);
    memcpy(out_stats, &g_reported_stats, copy_size);
    out_stats->struct_size = (uint32_t)struct_size;
    return UEC_RESULT_OK;
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

    api.struct_size = (uint32_t)offsetof(uec_api, get_runtime_stats);
    if (uec_consumer_drain_poll(&api, context, &drained, NULL) !=
            UEC_RESULT_UNSUPPORTED || drained != UEC_FALSE) {
        return 2;
    }
    api.struct_size = (uint32_t)(offsetof(uec_api, get_runtime_stats) +
                                 sizeof(api.get_runtime_stats));

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
    return 0;
}
