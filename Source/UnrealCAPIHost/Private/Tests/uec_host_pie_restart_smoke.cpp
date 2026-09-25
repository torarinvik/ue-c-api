#include "CoreMinimal.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include "uec_api.h"

namespace
{
    const uec_api* GApi = nullptr;
    uec_context* GContext = nullptr;
    uec_world* GOldWorld = nullptr;
    uec_actor* GOldActor = nullptr;
    uec_runtime_stats GBaseline{};
    TWeakObjectPtr<UWorld> GCapturedWorld;
    uint64_t GTickCallbackCount = 0;
    uint64_t GTickCallbackCountAtCleanup = 0;
    bool GWorldCleanupObserved = false;
#if WITH_EDITOR
    FDelegateHandle GWorldCleanupDelegate;
#endif

    void UEC_CALL CountWorldTicks(uint64_t, double, void*)
    {
        if (GTickCallbackCount != UINT64_MAX) ++GTickCallbackCount;
    }

#if WITH_EDITOR
    void ObserveWorldCleanup(UWorld* world, bool, bool)
    {
        if (world == nullptr || GCapturedWorld.Get() != world) return;
        GWorldCleanupObserved = true;
        GTickCallbackCountAtCleanup = GTickCallbackCount;
    }

    void RemoveWorldCleanupObserver()
    {
        if (GWorldCleanupDelegate.IsValid()) {
            FWorldDelegates::OnWorldCleanup.Remove(GWorldCleanupDelegate);
            GWorldCleanupDelegate.Reset();
        }
    }
#else
    void RemoveWorldCleanupObserver() {}
#endif

    void ReleaseCapturedHandles()
    {
        RemoveWorldCleanupObserver();
        if (GApi != nullptr && GOldActor != nullptr) {
            (void)GApi->release_actor(GOldActor);
        }
        if (GApi != nullptr && GOldWorld != nullptr) {
            (void)GApi->release_world(GOldWorld);
        }
        if (GApi != nullptr && GContext != nullptr) {
            (void)GApi->release_context(GContext);
        }
        GCapturedWorld.Reset();
        GOldActor = nullptr;
        GOldWorld = nullptr;
        GContext = nullptr;
        GApi = nullptr;
    }

    bool CheckActorName(const uec_api* api, uec_actor* actor)
    {
        char name[128]{};
        size_t requiredSize = 0;
        return api->get_actor_name(actor, name, sizeof(name), &requiredSize) == UEC_RESULT_OK &&
            requiredSize > 1u && name[requiredSize - 1u] == '\0';
    }
}

extern "C" uec_result UEC_CALL uec_host_pie_restart_smoke_capture(UWorld* world)
{
#if !WITH_EDITOR
    (void)world;
    return UEC_RESULT_UNSUPPORTED;
#else
    if (world == nullptr || GContext != nullptr) return UEC_RESULT_INVALID_ARGUMENT;
    GWorldCleanupObserved = false;
    GTickCallbackCount = 0;
    GTickCallbackCountAtCleanup = 0;

    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &GApi, &GContext);
    if (result != UEC_RESULT_OK) return result;
    if (GApi == nullptr || GContext == nullptr || GApi->get_runtime_stats == nullptr ||
        GApi->get_world_at_by_kind == nullptr || GApi->get_world_kind == nullptr ||
        GApi->get_first_player_controller == nullptr || GApi->get_actor_name == nullptr ||
        GApi->subscribe_world_tick == nullptr || GApi->release_actor == nullptr ||
        GApi->release_world == nullptr || GApi->release_context == nullptr) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    GBaseline.struct_size = sizeof(GBaseline);
    result = GApi->get_runtime_stats(GContext, &GBaseline);
    if (result != UEC_RESULT_OK || GBaseline.active_callbacks != 0u ||
        GBaseline.pending_requests != 0u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    result = GApi->get_world_at_by_kind(GContext, UEC_WORLD_KIND_PIE, 0u, &GOldWorld);
    if (result != UEC_RESULT_OK || GOldWorld == nullptr) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = GApi->get_first_player_controller(GOldWorld, &GOldActor);
    if (result != UEC_RESULT_OK || GOldActor == nullptr || !CheckActorName(GApi, GOldActor)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    {
        uint64_t subscriptionId = 0;
        result = GApi->subscribe_world_tick(
            GOldWorld, &CountWorldTicks, nullptr, &subscriptionId);
        if (result != UEC_RESULT_OK || subscriptionId == 0u) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
    }

    GCapturedWorld = world;
    GWorldCleanupDelegate = FWorldDelegates::OnWorldCleanup.AddStatic(&ObserveWorldCleanup);
    return UEC_RESULT_OK;

cleanup:
    ReleaseCapturedHandles();
    return result;
#endif
}

extern "C" uec_result UEC_CALL uec_host_pie_restart_smoke_verify(void)
{
#if !WITH_EDITOR
    return UEC_RESULT_UNSUPPORTED;
#else
    uec_world_kind oldWorldKind = UEC_WORLD_KIND_GAME;
    char oldActorName[64]{};
    size_t oldActorNameSize = SIZE_MAX;
    uec_world* newWorld = nullptr;
    uec_actor* newActor = nullptr;
    uec_runtime_stats observed{};
    uec_result result = UEC_RESULT_INTERNAL_ERROR;

    if (!GWorldCleanupObserved || GApi == nullptr ||
        GContext == nullptr || GOldWorld == nullptr || GOldActor == nullptr ||
        GTickCallbackCount == 0u || GTickCallbackCount != GTickCallbackCountAtCleanup) {
        UE_LOG(LogTemp, Error,
            TEXT("PIE restart cleanup invariant failed: cleanup=%d tick=%llu atCleanup=%llu api=%d context=%d world=%d actor=%d"),
            GWorldCleanupObserved,
            static_cast<unsigned long long>(GTickCallbackCount),
            static_cast<unsigned long long>(GTickCallbackCountAtCleanup),
            GApi != nullptr, GContext != nullptr, GOldWorld != nullptr, GOldActor != nullptr);
        goto cleanup;
    }
    {
        const uec_result oldWorldResult = GApi->get_world_kind(GOldWorld, &oldWorldKind);
        const uec_result oldActorResult = GApi->get_actor_name(
            GOldActor, oldActorName, sizeof(oldActorName), &oldActorNameSize);
        if (oldWorldResult != UEC_RESULT_INVALID_HANDLE ||
            oldWorldKind != UEC_WORLD_KIND_UNKNOWN ||
            oldActorResult != UEC_RESULT_INVALID_HANDLE || oldActorNameSize != 0u) {
            UE_LOG(LogTemp, Error,
                TEXT("PIE restart stale-handle invariant failed: world=%d kind=%d actor=%d nameSize=%llu"),
                static_cast<int32>(oldWorldResult), static_cast<int32>(oldWorldKind),
                static_cast<int32>(oldActorResult),
                static_cast<unsigned long long>(oldActorNameSize));
            goto cleanup;
        }
    }

    result = GApi->release_actor(GOldActor);
    if (result != UEC_RESULT_OK) goto cleanup;
    GOldActor = nullptr;
    result = GApi->release_world(GOldWorld);
    if (result != UEC_RESULT_OK && result != UEC_RESULT_INVALID_HANDLE) goto cleanup;
    GOldWorld = nullptr;

    result = GApi->get_world_at_by_kind(GContext, UEC_WORLD_KIND_PIE, 0u, &newWorld);
    if (result != UEC_RESULT_OK || newWorld == nullptr) {
        UE_LOG(LogTemp, Error, TEXT("PIE restart fresh-world lookup failed: result=%d world=%d"),
               static_cast<int32>(result), newWorld != nullptr);
        goto cleanup;
    }
    result = GApi->get_first_player_controller(newWorld, &newActor);
    if (result != UEC_RESULT_OK || newActor == nullptr || !CheckActorName(GApi, newActor)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UE_LOG(LogTemp, Error,
            TEXT("PIE restart fresh-controller check failed: result=%d actor=%d"),
            static_cast<int32>(result), newActor != nullptr);
        goto cleanup;
    }
    if (GApi->release_actor(newActor) != UEC_RESULT_OK) {
        newActor = nullptr;
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    newActor = nullptr;
    if (GApi->release_world(newWorld) != UEC_RESULT_OK) {
        newWorld = nullptr;
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    newWorld = nullptr;

    observed.struct_size = sizeof(observed);
    result = GApi->get_runtime_stats(GContext, &observed);
    if (result == UEC_RESULT_OK &&
        (observed.live_worlds != GBaseline.live_worlds ||
         observed.live_actors != GBaseline.live_actors ||
         observed.live_contexts != GBaseline.live_contexts ||
         observed.active_subscriptions != GBaseline.active_subscriptions ||
         observed.pending_requests != GBaseline.pending_requests ||
         observed.active_callbacks != GBaseline.active_callbacks)) {
        UE_LOG(LogTemp, Error,
            TEXT("PIE restart runtime counts differ: worlds=%u/%u actors=%u/%u contexts=%u/%u subscriptions=%u/%u requests=%u/%u callbacks=%u/%u"),
            observed.live_worlds, GBaseline.live_worlds,
            observed.live_actors, GBaseline.live_actors,
            observed.live_contexts, GBaseline.live_contexts,
            observed.active_subscriptions, GBaseline.active_subscriptions,
            observed.pending_requests, GBaseline.pending_requests,
            observed.active_callbacks, GBaseline.active_callbacks);
        result = UEC_RESULT_INTERNAL_ERROR;
    }

cleanup:
    if (newActor != nullptr) (void)GApi->release_actor(newActor);
    if (newWorld != nullptr) (void)GApi->release_world(newWorld);
    ReleaseCapturedHandles();
    return result;
#endif
}

extern "C" void UEC_CALL uec_host_pie_restart_smoke_cancel(void)
{
    ReleaseCapturedHandles();
}
