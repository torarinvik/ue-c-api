#include "CoreMinimal.h"

#include "uec_api.h"

namespace
{
    const uec_api* GApi = nullptr;
    uec_context* GContext = nullptr;
    uint64_t GGameThreadRequestId = 0;
    uint64_t GSaveLoadRequestId = 0;
    bool GArmed = false;
    bool GGameThreadCallbackExecuted = false;
    bool GSaveLoadCallbackExecuted = false;

    void UEC_CALL MarkGameThreadCallback(void*)
    {
        GGameThreadCallbackExecuted = true;
    }

    void UEC_CALL MarkSaveLoadCallback(
        uint64_t, uec_result, uec_object*, uec_bool, void*)
    {
        GSaveLoadCallbackExecuted = true;
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

extern "C" uec_result UEC_CALL uec_host_shutdown_pending_smoke_arm(void)
{
    if (GArmed) return UEC_RESULT_INVALID_ARGUMENT;
    GArmed = true;

    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &GApi, &GContext);
    if (result != UEC_RESULT_OK) goto cleanup;
    if (GApi == nullptr || GContext == nullptr || GApi->run_on_game_thread == nullptr ||
        GApi->async_load_game_from_slot == nullptr || GApi->release_context == nullptr) {
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
    return UEC_RESULT_OK;

cleanup:
    ReleaseShutdownSmokeContext();
    return result;
}

extern "C" uec_result UEC_CALL uec_host_shutdown_pending_smoke_verify(void)
{
    uec_result result = UEC_RESULT_INTERNAL_ERROR;
    uec_runtime_stats stats{};
    if (!GArmed || GApi == nullptr || GContext == nullptr ||
        GApi->get_runtime_stats == nullptr || GGameThreadRequestId == 0u ||
        GSaveLoadRequestId == 0u || GGameThreadCallbackExecuted ||
        GSaveLoadCallbackExecuted) {
        UE_LOG(LogTemp, Error,
            TEXT("Shutdown smoke precondition failed: armed=%d api=%d context=%d stats=%d game_id=%llu save_id=%llu game_callback=%d save_callback=%d"),
            GArmed, GApi != nullptr, GContext != nullptr,
            GApi != nullptr && GApi->get_runtime_stats != nullptr,
            static_cast<unsigned long long>(GGameThreadRequestId),
            static_cast<unsigned long long>(GSaveLoadRequestId),
            GGameThreadCallbackExecuted, GSaveLoadCallbackExecuted);
        goto cleanup;
    }

    stats.struct_size = sizeof(stats);
    result = GApi->get_runtime_stats(GContext, &stats);
    if (result != UEC_RESULT_OK) {
        UE_LOG(LogTemp, Error,
            TEXT("Shutdown smoke could not read pending work before module shutdown: result=%d"),
            static_cast<int32>(result));
    }
    else if (stats.pending_requests < 2u || stats.active_callbacks != 0u) {
        UE_LOG(LogTemp, Error,
            TEXT("Shutdown smoke expected queued callbacks and async requests: pending=%u active=%u"),
            stats.pending_requests, stats.active_callbacks);
        result = UEC_RESULT_INTERNAL_ERROR;
    }

cleanup:
    ReleaseShutdownSmokeContext();
    return result;
}
