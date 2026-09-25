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
    uec_world* GEditorWorld = nullptr;
    uec_world* GOldWorld = nullptr;
    uec_actor* GOldActor = nullptr;
    uec_actor* GWorldOwnedActor = nullptr;
    uec_scene_component* GWorldOwnedComponent = nullptr;
    uec_object* GWorldOwnedObject = nullptr;
    uec_object* GWorldOwnedRetainedObject = nullptr;
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

    bool ReleaseWorldOwnedHandles()
    {
        bool released = true;
        if (GApi != nullptr && GWorldOwnedRetainedObject != nullptr) {
            const uec_result result = GApi->release_object(GWorldOwnedRetainedObject);
            if (result == UEC_RESULT_OK) {
                GWorldOwnedRetainedObject = nullptr;
            } else {
                UE_LOG(LogTemp, Error, TEXT("Retained stale object release returned %d"),
                       static_cast<int32>(result));
                released = false;
            }
        }
        if (GApi != nullptr && GWorldOwnedObject != nullptr) {
            const uec_result result = GApi->release_object(GWorldOwnedObject);
            if (result == UEC_RESULT_OK) {
                GWorldOwnedObject = nullptr;
            } else {
                UE_LOG(LogTemp, Error, TEXT("Weak stale object release returned %d"),
                       static_cast<int32>(result));
                released = false;
            }
        }
        if (GApi != nullptr && GWorldOwnedComponent != nullptr) {
            const uec_result result = GApi->release_scene_component(GWorldOwnedComponent);
            if (result == UEC_RESULT_OK) {
                GWorldOwnedComponent = nullptr;
            } else {
                UE_LOG(LogTemp, Error, TEXT("Stale component release returned %d"),
                       static_cast<int32>(result));
                released = false;
            }
        }
        if (GApi != nullptr && GWorldOwnedActor != nullptr) {
            const uec_result result = GApi->release_actor(GWorldOwnedActor);
            if (result == UEC_RESULT_OK) {
                GWorldOwnedActor = nullptr;
            } else {
                UE_LOG(LogTemp, Error, TEXT("Stale actor release returned %d"),
                       static_cast<int32>(result));
                released = false;
            }
        }
        return released;
    }

    void ReleaseCapturedHandles()
    {
        RemoveWorldCleanupObserver();
        (void)ReleaseWorldOwnedHandles();
        if (GApi != nullptr && GOldActor != nullptr) {
            (void)GApi->release_actor(GOldActor);
        }
        if (GApi != nullptr && GOldWorld != nullptr) {
            (void)GApi->release_world(GOldWorld);
        }
        if (GApi != nullptr && GEditorWorld != nullptr) {
            (void)GApi->release_world(GEditorWorld);
        }
        if (GApi != nullptr && GContext != nullptr) {
            (void)GApi->release_context(GContext);
        }
        GCapturedWorld.Reset();
        GOldActor = nullptr;
        GOldWorld = nullptr;
        GEditorWorld = nullptr;
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

    bool CheckWorldName(const uec_api* api, uec_world* world)
    {
        char name[128]{};
        size_t requiredSize = 0;
        return api->get_world_name(world, name, sizeof(name), &requiredSize) == UEC_RESULT_OK &&
            requiredSize > 1u && requiredSize <= sizeof(name) &&
            name[requiredSize - 1u] == '\0';
    }

    bool CheckWorldOwnedHandlesInvalidated(const uec_api* api)
    {
        uec_transform actorTransform{
            {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}};
        const uec_result actorResult = api->get_actor_transform(
            GWorldOwnedActor, &actorTransform);
        const bool actorCleared = actorTransform.translation.x == 0.0 &&
            actorTransform.translation.y == 0.0 && actorTransform.translation.z == 0.0 &&
            actorTransform.rotation.x == 0.0 && actorTransform.rotation.y == 0.0 &&
            actorTransform.rotation.z == 0.0 && actorTransform.rotation.w == 0.0 &&
            actorTransform.scale.x == 0.0 && actorTransform.scale.y == 0.0 &&
            actorTransform.scale.z == 0.0;

        uec_transform componentTransform{
            {1.0, 1.0, 1.0}, {1.0, 1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}};
        const uec_result componentResult = api->get_component_transform(
            GWorldOwnedComponent, &componentTransform);
        const bool componentCleared = componentTransform.translation.x == 0.0 &&
            componentTransform.translation.y == 0.0 &&
            componentTransform.translation.z == 0.0 &&
            componentTransform.rotation.x == 0.0 && componentTransform.rotation.y == 0.0 &&
            componentTransform.rotation.z == 0.0 && componentTransform.rotation.w == 0.0 &&
            componentTransform.scale.x == 0.0 && componentTransform.scale.y == 0.0 &&
            componentTransform.scale.z == 0.0;
        uec_bool componentVisible = UEC_TRUE;
        const uec_result visibleResult = api->get_component_visible(
            GWorldOwnedComponent, &componentVisible);

        char actorName[64]{};
        size_t actorNameSize = SIZE_MAX;
        const uec_result actorNameResult = api->get_actor_name(
            GWorldOwnedActor, actorName, sizeof(actorName), &actorNameSize);

        char objectPath[128]{};
        size_t objectPathSize = SIZE_MAX;
        const uec_result objectResult = api->get_object_path(
            GWorldOwnedObject, objectPath, sizeof(objectPath), &objectPathSize);
        size_t retainedPathSize = SIZE_MAX;
        const uec_result retainedObjectResult = api->get_object_path(
            GWorldOwnedRetainedObject, objectPath, sizeof(objectPath), &retainedPathSize);
        const bool invalidated = actorResult == UEC_RESULT_INVALID_HANDLE && actorCleared &&
            actorNameResult == UEC_RESULT_INVALID_HANDLE && actorNameSize == 0u &&
            componentResult == UEC_RESULT_INVALID_HANDLE && componentCleared &&
            visibleResult == UEC_RESULT_INVALID_HANDLE && componentVisible == UEC_FALSE &&
            objectResult == UEC_RESULT_INVALID_HANDLE && objectPathSize == 0u &&
            retainedObjectResult == UEC_RESULT_INVALID_HANDLE && retainedPathSize == 0u;
        if (!invalidated) {
            UE_LOG(LogTemp, Error,
                TEXT("PIE world cleanup did not invalidate world-owned handles: actor=%d name=%d/%llu component=%d visible=%d/%d object=%d/%llu retained=%d/%llu"),
                static_cast<int32>(actorResult), static_cast<int32>(actorNameResult),
                static_cast<unsigned long long>(actorNameSize),
                static_cast<int32>(componentResult), static_cast<int32>(visibleResult),
                componentVisible, static_cast<int32>(objectResult),
                static_cast<unsigned long long>(objectPathSize),
                static_cast<int32>(retainedObjectResult),
                static_cast<unsigned long long>(retainedPathSize));
        }
        if (invalidated && !ReleaseWorldOwnedHandles()) {
            UE_LOG(LogTemp, Error, TEXT("PIE world-owned stale handles could not be released"));
            return false;
        }
        return invalidated;
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
    uint32_t editorWorldCount = 0;

    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &GApi, &GContext);
    if (result != UEC_RESULT_OK) return result;
    if (GApi == nullptr || GContext == nullptr || GApi->get_runtime_stats == nullptr ||
        GApi->get_world_at_by_kind == nullptr || GApi->get_world_kind == nullptr ||
        GApi->get_world_count_by_kind == nullptr || GApi->get_world_name == nullptr ||
        GApi->get_first_player_controller == nullptr || GApi->get_actor_name == nullptr ||
        GApi->spawn_actor == nullptr || GApi->get_actor_root_component == nullptr ||
        GApi->get_actor_property_object == nullptr || GApi->get_actor_transform == nullptr ||
        GApi->get_component_transform == nullptr || GApi->get_component_visible == nullptr ||
        GApi->get_object_path == nullptr || GApi->retain_object == nullptr ||
        GApi->subscribe_world_tick == nullptr || GApi->release_actor == nullptr ||
        GApi->release_scene_component == nullptr || GApi->release_object == nullptr ||
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

    result = GApi->get_world_count_by_kind(
        GContext, UEC_WORLD_KIND_EDITOR, &editorWorldCount);
    if (result != UEC_RESULT_OK || editorWorldCount == 0u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_NOT_INITIALIZED;
        goto cleanup;
    }
    result = GApi->get_world_at_by_kind(
        GContext, UEC_WORLD_KIND_EDITOR, 0u, &GEditorWorld);
    if (result != UEC_RESULT_OK || GEditorWorld == nullptr) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    {
        uec_world_kind kind = UEC_WORLD_KIND_UNKNOWN;
        result = GApi->get_world_kind(GEditorWorld, &kind);
        if (result != UEC_RESULT_OK || kind != UEC_WORLD_KIND_EDITOR ||
            !CheckWorldName(GApi, GEditorWorld)) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
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
        static constexpr char collisionActorPathData[] =
            "/Script/UnrealCAPIHost.UECAPIHostCollisionSmokeActor";
        static constexpr char rootPropertyData[] = "RootComponent";
        const uec_string_view collisionActorPath{
            collisionActorPathData, sizeof(collisionActorPathData) - 1u};
        const uec_string_view rootProperty{
            rootPropertyData, sizeof(rootPropertyData) - 1u};
        const uec_transform transform{
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
        result = GApi->spawn_actor(
            GOldWorld, collisionActorPath, &transform, &GWorldOwnedActor);
        if (result != UEC_RESULT_OK || GWorldOwnedActor == nullptr) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        result = GApi->get_actor_root_component(
            GWorldOwnedActor, &GWorldOwnedComponent);
        if (result != UEC_RESULT_OK || GWorldOwnedComponent == nullptr) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        result = GApi->get_actor_property_object(
            GWorldOwnedActor, rootProperty, &GWorldOwnedObject);
        if (result != UEC_RESULT_OK || GWorldOwnedObject == nullptr) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        result = GApi->retain_object(GWorldOwnedObject, &GWorldOwnedRetainedObject);
        if (result != UEC_RESULT_OK || GWorldOwnedRetainedObject == nullptr) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        char objectPath[128]{};
        size_t objectPathSize = 0;
        result = GApi->get_object_path(
            GWorldOwnedObject, objectPath, sizeof(objectPath), &objectPathSize);
        if (result != UEC_RESULT_OK || objectPathSize <= 1u ||
            objectPathSize > sizeof(objectPath) || objectPath[objectPathSize - 1u] != '\0') {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        objectPathSize = 0;
        result = GApi->get_object_path(
            GWorldOwnedRetainedObject, objectPath, sizeof(objectPath), &objectPathSize);
        if (result != UEC_RESULT_OK || objectPathSize <= 1u ||
            objectPathSize > sizeof(objectPath) || objectPath[objectPathSize - 1u] != '\0') {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
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
    uec_world_kind editorWorldKind = UEC_WORLD_KIND_UNKNOWN;
    char oldActorName[64]{};
    size_t oldActorNameSize = SIZE_MAX;
    uec_world* newWorld = nullptr;
    uec_actor* newActor = nullptr;
    uec_runtime_stats observed{};
    uec_result result = UEC_RESULT_INTERNAL_ERROR;

    if (!GWorldCleanupObserved || GApi == nullptr ||
        GContext == nullptr || GOldWorld == nullptr || GOldActor == nullptr ||
        GWorldOwnedActor == nullptr || GWorldOwnedComponent == nullptr ||
        GWorldOwnedObject == nullptr || GWorldOwnedRetainedObject == nullptr ||
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
    if (!CheckWorldOwnedHandlesInvalidated(GApi)) goto cleanup;

    result = GApi->get_world_kind(GEditorWorld, &editorWorldKind);
    if (result != UEC_RESULT_OK || editorWorldKind != UEC_WORLD_KIND_EDITOR ||
        !CheckWorldName(GApi, GEditorWorld)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UE_LOG(LogTemp, Error,
            TEXT("Editor-world handle did not survive PIE cleanup: result=%d kind=%d"),
            static_cast<int32>(result), static_cast<int32>(editorWorldKind));
        goto cleanup;
    }
    result = GApi->release_world(GEditorWorld);
    if (result != UEC_RESULT_OK) goto cleanup;
    GEditorWorld = nullptr;

    result = GApi->release_actor(GOldActor);
    if (result != UEC_RESULT_OK) goto cleanup;
    GOldActor = nullptr;
    result = GApi->release_world(GOldWorld);
    if (result != UEC_RESULT_OK) goto cleanup;
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
