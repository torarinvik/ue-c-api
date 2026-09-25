#include "CoreMinimal.h"

#include "uec_api.h"

namespace
{
    constexpr uint32_t TickSubscriptionCapacity = 1024u;
    const uec_api* GApi = nullptr;
    uec_context* GContext = nullptr;
    uint64_t GSubscriptionIds[TickSubscriptionCapacity]{};

    void ResetIfDrained()
    {
        for (uint64_t id : GSubscriptionIds) {
            if (id != 0u) return;
        }
        GApi = nullptr;
        GContext = nullptr;
    }
}

extern "C" void UEC_CALL uec_host_world_tick_capacity_cancel();

extern "C" uec_result UEC_CALL uec_host_world_tick_capacity_capture(
    uec_world* world, const uec_api* api, uec_context* context,
    uec_tick_callback callback, void* userData, uint32_t priorSubscriptions)
{
    if (world == nullptr || api == nullptr || context == nullptr || callback == nullptr ||
        GApi != nullptr || api->subscribe_world_tick == nullptr ||
        api->unsubscribe_world_tick == nullptr || api->get_runtime_stats == nullptr ||
        priorSubscriptions > UINT32_MAX - TickSubscriptionCapacity) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    GApi = api;
    GContext = context;

    uec_result result = UEC_RESULT_OK;
    for (uint32_t index = 0u; index < TickSubscriptionCapacity; ++index) {
        uint64_t id = 0u;
        result = api->subscribe_world_tick(world, callback, userData, &id);
        if (result != UEC_RESULT_OK || id == 0u ||
            (index != 0u && id <= GSubscriptionIds[index - 1u])) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            break;
        }
        GSubscriptionIds[index] = id;
    }
    if (result == UEC_RESULT_OK) {
        uint64_t overflowId = UINT64_MAX;
        const uec_result overflowResult = api->subscribe_world_tick(
            world, callback, userData, &overflowId);
        result = overflowResult == UEC_RESULT_QUEUE_FULL && overflowId == 0u
            ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK) {
        uec_runtime_stats stats{};
        stats.struct_size = sizeof(stats);
        result = api->get_runtime_stats(context, &stats);
        if (result == UEC_RESULT_OK &&
            stats.active_subscriptions != priorSubscriptions + TickSubscriptionCapacity) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result != UEC_RESULT_OK) uec_host_world_tick_capacity_cancel();
    return result;
}

extern "C" bool UEC_CALL uec_host_world_tick_capacity_verify()
{
    if (GApi == nullptr || GContext == nullptr || GApi->unsubscribe_world_tick == nullptr) {
        return false;
    }
    bool allRetired = true;
    for (uint64_t& id : GSubscriptionIds) {
        if (id == 0u) {
            allRetired = false;
            continue;
        }
        const uec_result result = GApi->unsubscribe_world_tick(GContext, id);
        if (result != UEC_RESULT_INVALID_ARGUMENT) allRetired = false;
        if (result == UEC_RESULT_INVALID_ARGUMENT || result == UEC_RESULT_OK) id = 0u;
    }
    ResetIfDrained();
    return allRetired;
}

extern "C" void UEC_CALL uec_host_world_tick_capacity_cancel()
{
    if (GApi != nullptr && GContext != nullptr && GApi->unsubscribe_world_tick != nullptr) {
        for (uint64_t& id : GSubscriptionIds) {
            if (id != 0u) (void)GApi->unsubscribe_world_tick(GContext, id);
            id = 0u;
        }
    }
    GApi = nullptr;
    GContext = nullptr;
}
