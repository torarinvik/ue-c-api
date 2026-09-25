#include "c_consumer_drain.h"

#include <stddef.h>
#include <string.h>

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
