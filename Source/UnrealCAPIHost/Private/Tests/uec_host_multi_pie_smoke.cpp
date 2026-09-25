#include "CoreMinimal.h"

#include "uec_api.h"

extern "C" uec_result UEC_CALL uec_host_multi_pie_smoke(void)
{
    const uec_api* api = nullptr;
    uec_context* context = nullptr;
    uec_world* worlds[2]{};
    uec_actor* controllers[2]{};
    int32_t instances[2] = {-1, -1};
    uec_runtime_stats baseline{};
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;

    if (api == nullptr || context == nullptr || api->get_world_count_by_kind == nullptr ||
        api->get_world_at_by_kind == nullptr || api->get_world_kind == nullptr ||
        api->get_world_pie_instance == nullptr || api->get_first_player_controller == nullptr ||
        api->get_actor_name == nullptr || api->get_runtime_stats == nullptr ||
        api->release_actor == nullptr || api->release_world == nullptr ||
        api->release_context == nullptr) {
        result = UEC_RESULT_UNSUPPORTED;
        goto cleanup;
    }

    baseline.struct_size = sizeof(baseline);
    result = api->get_runtime_stats(context, &baseline);
    if (result != UEC_RESULT_OK) goto cleanup;

    {
        uint32_t worldCount = 0;
        result = api->get_world_count_by_kind(context, UEC_WORLD_KIND_PIE, &worldCount);
        if (result != UEC_RESULT_OK) goto cleanup;
        if (worldCount < 2u) {
            result = UEC_RESULT_NOT_INITIALIZED;
            goto cleanup;
        }
    }

    for (uint32_t index = 0; index < 2u; ++index)
    {
        result = api->get_world_at_by_kind(
            context, UEC_WORLD_KIND_PIE, index, &worlds[index]);
        if (result != UEC_RESULT_OK || worlds[index] == nullptr) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        uec_world_kind kind = UEC_WORLD_KIND_UNKNOWN;
        result = api->get_world_kind(worlds[index], &kind);
        if (result != UEC_RESULT_OK || kind != UEC_WORLD_KIND_PIE) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        result = api->get_world_pie_instance(worlds[index], &instances[index]);
        if (result != UEC_RESULT_OK || instances[index] < 0) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        result = api->get_first_player_controller(worlds[index], &controllers[index]);
        if (result != UEC_RESULT_OK || controllers[index] == nullptr) {
            if (result == UEC_RESULT_NOT_INITIALIZED) goto cleanup;
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        char name[128]{};
        size_t requiredSize = 0;
        result = api->get_actor_name(
            controllers[index], name, sizeof(name), &requiredSize);
        if (result != UEC_RESULT_OK || requiredSize <= 1u ||
            requiredSize > sizeof(name) || name[requiredSize - 1u] != '\0') {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
    }

    if (instances[0] == instances[1]) {
        UE_LOG(LogTemp, Error,
            TEXT("Multi-PIE contexts share the same PIE instance id: %d"), instances[0]);
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = UEC_RESULT_OK;

cleanup:
    for (uec_actor* controller : controllers) {
        if (controller != nullptr) {
            const uec_result releaseResult = api->release_actor(controller);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
        }
    }
    for (uec_world* world : worlds) {
        if (world != nullptr) {
            const uec_result releaseResult = api->release_world(world);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
        }
    }
    if (result == UEC_RESULT_OK) {
        uec_runtime_stats observed{};
        observed.struct_size = sizeof(observed);
        result = api->get_runtime_stats(context, &observed);
        if (result == UEC_RESULT_OK &&
            (observed.live_worlds != baseline.live_worlds ||
             observed.live_actors != baseline.live_actors ||
             observed.live_contexts != baseline.live_contexts ||
             observed.active_subscriptions != baseline.active_subscriptions ||
             observed.pending_requests != baseline.pending_requests ||
             observed.active_callbacks != baseline.active_callbacks)) {
            UE_LOG(LogTemp, Error,
                TEXT("Multi-PIE handle counts failed to return to baseline"));
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (api != nullptr && context != nullptr && api->release_context != nullptr) {
        const uec_result releaseResult = api->release_context(context);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    return result;
}
