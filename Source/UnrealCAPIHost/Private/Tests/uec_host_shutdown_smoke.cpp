#include "CoreMinimal.h"

#include "uec_api.h"

namespace
{
    const uec_api* GApi = nullptr;
    uec_context* GContext = nullptr;
    uec_world* GShutdownWorld = nullptr;
    uec_actor* GActor = nullptr;
    uec_object* GEventBridge = nullptr;
    uint64_t GGameThreadRequestId = 0;
    uint64_t GSaveLoadRequestId = 0;
    uint64_t GObjectLoadRequestId = 0;
    uint64_t GEventBridgeSubscriptionId = 0;
    bool GPrepared = false;
    bool GArmed = false;
    bool GGameThreadCallbackExecuted = false;
    bool GSaveLoadCallbackExecuted = false;
    bool GObjectLoadCallbackExecuted = false;
    bool GEventBridgeCallbackExecuted = false;
    bool GEventBridgeWasRegistered = false;

    void UEC_CALL MarkGameThreadCallback(void*)
    {
        GGameThreadCallbackExecuted = true;
    }

    void UEC_CALL MarkSaveLoadCallback(
        uint64_t, uec_result, uec_object*, uec_bool, void*)
    {
        GSaveLoadCallbackExecuted = true;
    }

    void UEC_CALL MarkObjectLoadCallback(
        uint64_t, uec_result, uec_object* object, void*)
    {
        GObjectLoadCallbackExecuted = true;
        if (object != nullptr && GApi != nullptr && GApi->release_object != nullptr) {
            (void)GApi->release_object(object);
        }
    }

    void UEC_CALL MarkEventBridgeCallback(
        uint64_t, int64_t, int64_t, double, uec_string_view, void*)
    {
        GEventBridgeCallbackExecuted = true;
    }

    void ReleaseShutdownSmokeHandles(bool unbindEventBridge)
    {
        if (unbindEventBridge && GEventBridgeSubscriptionId != 0u &&
            GApi != nullptr && GContext != nullptr &&
            GApi->unbind_actor_event_bridge != nullptr) {
            (void)GApi->unbind_actor_event_bridge(GContext, GEventBridgeSubscriptionId);
            GEventBridgeSubscriptionId = 0u;
        }
        if (GEventBridge != nullptr && GApi != nullptr && GApi->release_object != nullptr) {
            (void)GApi->release_object(GEventBridge);
        }
        GEventBridge = nullptr;
        if (GActor != nullptr && GApi != nullptr && GApi->release_actor != nullptr) {
            (void)GApi->release_actor(GActor);
        }
        GActor = nullptr;
        if (GShutdownWorld != nullptr && GApi != nullptr && GApi->release_world != nullptr) {
            (void)GApi->release_world(GShutdownWorld);
        }
        GShutdownWorld = nullptr;
    }

    void ReleaseShutdownSmokeContext()
    {
        if (GApi != nullptr && GContext != nullptr &&
            GApi->release_context != nullptr) {
            (void)GApi->release_context(GContext);
        }
        GContext = nullptr;
        GApi = nullptr;
    }
}

extern "C" uec_result UEC_CALL uec_host_shutdown_pending_smoke_prepare(void)
{
    if (GPrepared || GArmed) return UEC_RESULT_INVALID_ARGUMENT;

    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &GApi, &GContext);
    if (result != UEC_RESULT_OK) goto cleanup;
    if (GApi == nullptr || GContext == nullptr || GApi->get_default_world == nullptr ||
        GApi->spawn_actor == nullptr || GApi->get_or_create_actor_event_bridge == nullptr ||
        GApi->bind_actor_event_bridge == nullptr || GApi->unbind_actor_event_bridge == nullptr ||
        GApi->release_world == nullptr || GApi->release_actor == nullptr ||
        GApi->release_object == nullptr || GApi->run_on_game_thread == nullptr ||
        GApi->async_load_game_from_slot == nullptr ||
        GApi->request_object_load == nullptr || GApi->release_context == nullptr) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    {
        static constexpr char actorClassPath[] = "/Script/Engine.StaticMeshActor";
        const uec_string_view classPath{actorClassPath, sizeof(actorClassPath) - 1u};
        const uec_transform transform{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0},
                                      {1.0, 1.0, 1.0}};
        result = GApi->get_default_world(GContext, &GShutdownWorld);
        if (result != UEC_RESULT_OK || GShutdownWorld == nullptr) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        result = GApi->spawn_actor(GShutdownWorld, classPath, &transform, &GActor);
        if (result != UEC_RESULT_OK || GActor == nullptr) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        result = GApi->get_or_create_actor_event_bridge(GActor, &GEventBridge);
        if (result != UEC_RESULT_OK || GEventBridge == nullptr) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        result = GApi->bind_actor_event_bridge(
            GEventBridge, &MarkEventBridgeCallback, nullptr, &GEventBridgeSubscriptionId);
        if (result != UEC_RESULT_OK || GEventBridgeSubscriptionId == 0u) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        uec_runtime_stats stats{};
        if (GApi->get_runtime_stats == nullptr) {
            result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        stats.struct_size = sizeof(stats);
        result = GApi->get_runtime_stats(GContext, &stats);
        if (result != UEC_RESULT_OK || stats.active_subscriptions == 0u) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        GEventBridgeWasRegistered = true;
        /* Keep the native delegate registered while dropping its caller handles. */
        ReleaseShutdownSmokeHandles(false);
    }

    GPrepared = true;
    return UEC_RESULT_OK;

cleanup:
    ReleaseShutdownSmokeHandles(true);
    ReleaseShutdownSmokeContext();
    return result;
}

extern "C" uec_result UEC_CALL uec_host_shutdown_pending_smoke_arm(void)
{
    if (!GPrepared || GArmed || GApi == nullptr || GContext == nullptr ||
        GEventBridgeSubscriptionId == 0u) return UEC_RESULT_INVALID_ARGUMENT;
    GArmed = true;

    uec_result result = UEC_RESULT_OK;
    if (GApi->run_on_game_thread == nullptr ||
        GApi->async_load_game_from_slot == nullptr ||
        GApi->request_object_load == nullptr) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    result = GApi->run_on_game_thread(
        GContext, &MarkGameThreadCallback, nullptr, &GGameThreadRequestId);
    if (result != UEC_RESULT_OK || GGameThreadRequestId == 0u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    {
        static constexpr char missingSlot[] = "UEC_ShutdownPending_Missing_NoSave";
        result = GApi->async_load_game_from_slot(
            GContext,
            uec_string_view{missingSlot, sizeof(missingSlot) - 1u},
            0,
            &MarkSaveLoadCallback,
            nullptr,
            &GSaveLoadRequestId);
    }
    if (result != UEC_RESULT_OK || GSaveLoadRequestId == 0u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    {
        static constexpr char missingObjectPath[] =
            "/UEC_ShutdownPending_Missing_Asset/Asset.Asset";
        result = GApi->request_object_load(
            GContext,
            uec_string_view{missingObjectPath, sizeof(missingObjectPath) - 1u},
            &MarkObjectLoadCallback,
            nullptr,
            &GObjectLoadRequestId);
    }
    if (result != UEC_RESULT_OK || GObjectLoadRequestId == 0u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    return UEC_RESULT_OK;

cleanup:
    ReleaseShutdownSmokeHandles(true);
    ReleaseShutdownSmokeContext();
    return result;
}

extern "C" uec_result UEC_CALL uec_host_shutdown_pending_smoke_verify(void)
{
    uec_result result = UEC_RESULT_INTERNAL_ERROR;
    uec_runtime_stats stats{};
    if (!GPrepared || !GArmed || !GEventBridgeWasRegistered ||
        GApi == nullptr || GContext == nullptr ||
        GApi->get_runtime_stats == nullptr || GGameThreadRequestId == 0u ||
        GSaveLoadRequestId == 0u || GObjectLoadRequestId == 0u ||
        GGameThreadCallbackExecuted || GSaveLoadCallbackExecuted ||
        GObjectLoadCallbackExecuted || GEventBridgeCallbackExecuted) {
        UE_LOG(LogTemp, Error,
            TEXT("Shutdown smoke precondition failed: armed=%d api=%d context=%d stats=%d game_id=%llu save_id=%llu object_id=%llu game_callback=%d save_callback=%d object_callback=%d bridge_callback=%d"),
            GArmed, GApi != nullptr, GContext != nullptr,
            GApi != nullptr && GApi->get_runtime_stats != nullptr,
            static_cast<unsigned long long>(GGameThreadRequestId),
            static_cast<unsigned long long>(GSaveLoadRequestId),
            static_cast<unsigned long long>(GObjectLoadRequestId),
            GGameThreadCallbackExecuted, GSaveLoadCallbackExecuted,
            GObjectLoadCallbackExecuted, GEventBridgeCallbackExecuted);
        goto cleanup;
    }

    stats.struct_size = sizeof(stats);
    result = GApi->get_runtime_stats(GContext, &stats);
    if (result != UEC_RESULT_OK) {
        UE_LOG(LogTemp, Error,
            TEXT("Shutdown smoke could not read pending work before module shutdown: result=%d"),
            static_cast<int32>(result));
    }
    else if (stats.pending_requests < 3u || stats.active_callbacks != 0u) {
        UE_LOG(LogTemp, Error,
            TEXT("Shutdown smoke expected pending work with no callbacks after world cleanup: pending=%u subscriptions=%u callbacks=%u"),
            stats.pending_requests, stats.active_subscriptions, stats.active_callbacks);
        result = UEC_RESULT_INTERNAL_ERROR;
    }

cleanup:
    GEventBridgeSubscriptionId = 0u;
    ReleaseShutdownSmokeContext();
    return result;
}
