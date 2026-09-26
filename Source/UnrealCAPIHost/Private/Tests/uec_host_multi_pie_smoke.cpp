#include "CoreMinimal.h"

#include "uec_api.h"

namespace
{
    void UEC_CALL CountUnexpectedTimer(uint64_t, void* userData)
    {
        uint32_t* count = static_cast<uint32_t*>(userData);
        if (count != nullptr && *count != UINT32_MAX) ++*count;
    }
}

extern "C" uec_result UEC_CALL uec_host_multi_pie_smoke(void)
{
    const uec_api* api = nullptr;
    uec_context* context = nullptr;
    uec_world* worlds[2]{};
    uec_actor* controllers[2]{};
    uec_object* gameInstances[2]{};
    int32_t instances[2] = {-1, -1};
    char gameInstancePaths[2][256]{};
    uint64_t timerId = 0;
    uint32_t timerCallbacks = 0;
    uec_runtime_stats baseline{};
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;

    if (api == nullptr || context == nullptr || api->get_world_count_by_kind == nullptr ||
        api->get_world_at_by_kind == nullptr || api->get_world_kind == nullptr ||
        api->get_world_pie_instance == nullptr || api->get_first_player_controller == nullptr ||
        api->get_default_world == nullptr || api->get_last_error == nullptr ||
        api->get_actor_name == nullptr || api->get_world_game_instance == nullptr ||
        api->object_is_a == nullptr || api->get_object_path == nullptr ||
        api->release_object == nullptr || api->get_runtime_stats == nullptr ||
        api->release_actor == nullptr || api->release_world == nullptr ||
        api->release_context == nullptr || api->set_timer == nullptr ||
        api->clear_timer == nullptr || api->set_controller_view_target == nullptr) {
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

    {
        static constexpr char expectedError[] =
            "Multiple active Game/PIE worlds; select a world explicitly by kind and index";
        uec_world* ambiguousWorld = nullptr;
        result = api->get_default_world(context, &ambiguousWorld);
        if (result != UEC_RESULT_AMBIGUOUS_CONTEXT || ambiguousWorld != nullptr) {
            UE_LOG(LogTemp, Error,
                TEXT("Ambiguous default-world lookup did not fail with a cleared output: result=%d world=%d"),
                static_cast<int32>(result), ambiguousWorld != nullptr);
            if (ambiguousWorld != nullptr) (void)api->release_world(ambiguousWorld);
            result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        char error[128]{};
        size_t requiredErrorSize = 0;
        result = api->get_last_error(
            context, error, sizeof(error), &requiredErrorSize);
        if (result != UEC_RESULT_OK || requiredErrorSize != sizeof(expectedError) ||
            FMemory::Memcmp(error, expectedError, sizeof(expectedError)) != 0) {
            UE_LOG(LogTemp, Error,
                TEXT("Ambiguous world lookup diagnostic mismatch: result=%d required=%llu"),
                static_cast<int32>(result),
                static_cast<unsigned long long>(requiredErrorSize));
            result = UEC_RESULT_INTERNAL_ERROR;
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
        result = api->get_world_game_instance(worlds[index], &gameInstances[index]);
        if (result != UEC_RESULT_OK || gameInstances[index] == nullptr) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        static constexpr char gameInstanceClassPathData[] =
            "/Script/Engine.GameInstance";
        const uec_string_view gameInstanceClassPath{
            gameInstanceClassPathData, sizeof(gameInstanceClassPathData) - 1u};
        uec_bool isGameInstance = UEC_FALSE;
        result = api->object_is_a(
            gameInstances[index], gameInstanceClassPath, &isGameInstance);
        if (result != UEC_RESULT_OK || isGameInstance != UEC_TRUE) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        size_t gameInstancePathSize = 0;
        result = api->get_object_path(gameInstances[index], gameInstancePaths[index],
                                      sizeof(gameInstancePaths[index]),
                                      &gameInstancePathSize);
        if (result != UEC_RESULT_OK || gameInstancePathSize <= 1u ||
            gameInstancePathSize > sizeof(gameInstancePaths[index]) ||
            gameInstancePaths[index][gameInstancePathSize - 1u] != '\0') {
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
    if (FCStringAnsi::Strcmp(gameInstancePaths[0], gameInstancePaths[1]) == 0) {
        UE_LOG(LogTemp, Error,
            TEXT("Multi-PIE world contexts returned the same game-instance path: %s"),
            UTF8_TO_TCHAR(gameInstancePaths[0]));
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    if (api->set_controller_view_target(controllers[0], controllers[1]) !=
        UEC_RESULT_INVALID_ARGUMENT) {
        UE_LOG(LogTemp, Error,
            TEXT("A player controller accepted a view target from another PIE world"));
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->set_timer(worlds[0], 60.0, UEC_FALSE, &CountUnexpectedTimer,
                            &timerCallbacks, &timerId);
    if (result != UEC_RESULT_OK || timerId == 0) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    if (api->clear_timer(worlds[1], timerId) != UEC_RESULT_INVALID_ARGUMENT) {
        UE_LOG(LogTemp, Error, TEXT("A timer was cleared through a different PIE world"));
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->clear_timer(worlds[0], timerId);
    timerId = 0;
    if (result != UEC_RESULT_OK || timerCallbacks != 0) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = UEC_RESULT_OK;

cleanup:
    if (timerId != 0 && worlds[0] != nullptr && api != nullptr &&
        api->clear_timer != nullptr) {
        const uec_result clearResult = api->clear_timer(worlds[0], timerId);
        if (result == UEC_RESULT_OK && clearResult != UEC_RESULT_OK) result = clearResult;
    }
    for (uec_actor* controller : controllers) {
        if (controller != nullptr) {
            const uec_result releaseResult = api->release_actor(controller);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
        }
    }
    for (uec_object* gameInstance : gameInstances) {
        if (gameInstance != nullptr) {
            const uec_result releaseResult = api->release_object(gameInstance);
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
