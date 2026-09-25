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
    uec_actor* GLatentSmokeActor = nullptr;
    uec_object* GOldWorldGameInstance = nullptr;
    uec_actor* GWorldOwnedActor = nullptr;
    uec_scene_component* GWorldOwnedComponent = nullptr;
    uec_object* GWorldOwnedObject = nullptr;
    uec_object* GWorldOwnedRetainedObject = nullptr;
    uec_runtime_stats GBaseline{};
    TWeakObjectPtr<UWorld> GCapturedWorld;
    uint64_t GTickCallbackCount = 0;
    uint64_t GTickCallbackCountAtCleanup = 0;
    uint64_t GLatentSmokeRequestId = 0;
    bool GWorldCleanupObserved = false;
    bool GLatentSmokeCallbackExecuted = false;
#if WITH_EDITOR
    FDelegateHandle GWorldCleanupDelegate;
#endif

    void UEC_CALL CountWorldTicks(uint64_t, double, void*)
    {
        if (GTickCallbackCount != UINT64_MAX) ++GTickCallbackCount;
    }

    void UEC_CALL ObserveLatentCompletion(uint64_t, uec_result, void* userData)
    {
        bool* callbackExecuted = static_cast<bool*>(userData);
        if (callbackExecuted != nullptr) *callbackExecuted = true;
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
        if (GApi != nullptr && GOldWorldGameInstance != nullptr) {
            const uec_result result = GApi->release_object(GOldWorldGameInstance);
            if (result == UEC_RESULT_OK) {
                GOldWorldGameInstance = nullptr;
            } else {
                UE_LOG(LogTemp, Error, TEXT("Stale world game-instance release returned %d"),
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
        if (GApi != nullptr && GLatentSmokeActor != nullptr) {
            (void)GApi->release_actor(GLatentSmokeActor);
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
        GLatentSmokeActor = nullptr;
        GLatentSmokeRequestId = 0;
        GLatentSmokeCallbackExecuted = false;
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

    bool CheckWorldHandleAddressReuse(const uec_api* api, uec_context* context)
    {
        static constexpr uint32_t stressCount = 128u;
        uec_world* staleWorlds[stressCount]{};
        for (uint32_t index = 0u; index < stressCount; ++index) {
            uec_world* world = nullptr;
            if (api->get_world_at_by_kind(
                    context, UEC_WORLD_KIND_PIE, 0u, &world) != UEC_RESULT_OK ||
                world == nullptr) {
                if (world != nullptr) (void)api->release_world(world);
                return false;
            }
            for (uint32_t prior = 0u; prior < index; ++prior) {
                if (staleWorlds[prior] == world) {
                    (void)api->release_world(world);
                    return false;
                }
            }
            if (api->release_world(world) != UEC_RESULT_OK) return false;
            staleWorlds[index] = world;
            uec_world_kind staleKind = UEC_WORLD_KIND_GAME;
            if (api->get_world_kind(world, &staleKind) != UEC_RESULT_INVALID_HANDLE ||
                staleKind != UEC_WORLD_KIND_UNKNOWN) {
                return false;
            }
        }
        return true;
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
        size_t gameInstancePathSize = SIZE_MAX;
        const uec_result gameInstanceResult = api->get_object_path(
            GOldWorldGameInstance, objectPath, sizeof(objectPath), &gameInstancePathSize);
        const bool invalidated = actorResult == UEC_RESULT_INVALID_HANDLE && actorCleared &&
            actorNameResult == UEC_RESULT_INVALID_HANDLE && actorNameSize == 0u &&
            componentResult == UEC_RESULT_INVALID_HANDLE && componentCleared &&
            visibleResult == UEC_RESULT_INVALID_HANDLE && componentVisible == UEC_FALSE &&
            objectResult == UEC_RESULT_INVALID_HANDLE && objectPathSize == 0u &&
            retainedObjectResult == UEC_RESULT_INVALID_HANDLE && retainedPathSize == 0u &&
            gameInstanceResult == UEC_RESULT_INVALID_HANDLE && gameInstancePathSize == 0u;
        if (!invalidated) {
            UE_LOG(LogTemp, Error,
                TEXT("PIE world cleanup did not invalidate world-owned handles: actor=%d name=%d/%llu component=%d visible=%d/%d object=%d/%llu retained=%d/%llu gameInstance=%d/%llu"),
                static_cast<int32>(actorResult), static_cast<int32>(actorNameResult),
                static_cast<unsigned long long>(actorNameSize),
                static_cast<int32>(componentResult), static_cast<int32>(visibleResult),
                componentVisible, static_cast<int32>(objectResult),
                static_cast<unsigned long long>(objectPathSize),
                static_cast<int32>(retainedObjectResult),
                static_cast<unsigned long long>(retainedPathSize),
                static_cast<int32>(gameInstanceResult),
                static_cast<unsigned long long>(gameInstancePathSize));
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
    GLatentSmokeCallbackExecuted = false;
    GLatentSmokeRequestId = 0;
    GTickCallbackCount = 0;
    GTickCallbackCountAtCleanup = 0;
    uint32_t editorWorldCount = 0;

    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &GApi, &GContext);
    if (result != UEC_RESULT_OK) return result;
    if (GApi == nullptr || GContext == nullptr || GApi->get_runtime_stats == nullptr ||
        GApi->get_world_at_by_kind == nullptr || GApi->get_world_kind == nullptr ||
        GApi->get_world_count_by_kind == nullptr || GApi->get_world_name == nullptr ||
        GApi->get_first_player_controller == nullptr || GApi->get_actor_name == nullptr ||
        GApi->get_world_game_instance == nullptr || GApi->object_is_a == nullptr ||
        GApi->spawn_actor == nullptr || GApi->get_actor_root_component == nullptr ||
        GApi->get_actor_property_object == nullptr || GApi->get_actor_transform == nullptr ||
        GApi->get_component_transform == nullptr || GApi->get_component_visible == nullptr ||
        GApi->get_object_path == nullptr || GApi->retain_object == nullptr ||
        GApi->subscribe_world_tick == nullptr || GApi->invoke_actor_function_latent == nullptr ||
        GApi->release_actor == nullptr ||
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
    if (!CheckWorldHandleAddressReuse(GApi, GContext)) {
        UE_LOG(LogTemp, Error,
            TEXT("Released PIE world handles were reused or remained valid during the stress pass"));
        result = UEC_RESULT_INTERNAL_ERROR;
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
    result = GApi->get_world_game_instance(GOldWorld, &GOldWorldGameInstance);
    if (result != UEC_RESULT_OK || GOldWorldGameInstance == nullptr) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    {
        static constexpr char gameInstanceClassPathData[] = "/Script/Engine.GameInstance";
        const uec_string_view gameInstanceClassPath{
            gameInstanceClassPathData, sizeof(gameInstanceClassPathData) - 1u};
        uec_bool isGameInstance = UEC_FALSE;
        result = GApi->object_is_a(
            GOldWorldGameInstance, gameInstanceClassPath, &isGameInstance);
        if (result != UEC_RESULT_OK || isGameInstance != UEC_TRUE) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        char gameInstancePath[256]{};
        size_t gameInstancePathSize = 0;
        result = GApi->get_object_path(GOldWorldGameInstance, gameInstancePath,
                                       sizeof(gameInstancePath), &gameInstancePathSize);
        if (result != UEC_RESULT_OK || gameInstancePathSize <= 1u ||
            gameInstancePathSize > sizeof(gameInstancePath) ||
            gameInstancePath[gameInstancePathSize - 1u] != '\0') {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
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
    {
        static constexpr char latentActorPathData[] =
            "/Script/UnrealCAPIHost.UECAPIHostLatentSmokeActor";
        static constexpr char latentFunctionNameData[] = "WaitForSmokeDuration";
        const uec_string_view latentActorPath{
            latentActorPathData, sizeof(latentActorPathData) - 1u};
        const uec_string_view latentFunctionName{
            latentFunctionNameData, sizeof(latentFunctionNameData) - 1u};
        const uec_transform transform{
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
        result = GApi->spawn_actor(
            GOldWorld, latentActorPath, &transform, &GLatentSmokeActor);
        if (result != UEC_RESULT_OK || GLatentSmokeActor == nullptr) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }

        uec_function_argument latentArguments[2]{};
        latentArguments[0].struct_size = sizeof(latentArguments[0]);
        latentArguments[0].kind = UEC_PROPERTY_OBJECT;
        latentArguments[0].world_value = GOldWorld;
        latentArguments[1].struct_size = sizeof(latentArguments[1]);
        latentArguments[1].kind = UEC_PROPERTY_FLOAT;
        latentArguments[1].real_value = 3600.0;
        result = GApi->invoke_actor_function_latent(
            GLatentSmokeActor, latentFunctionName, latentArguments,
            2u, &ObserveLatentCompletion, &GLatentSmokeCallbackExecuted,
            &GLatentSmokeRequestId);
        if (result != UEC_RESULT_OK || GLatentSmokeRequestId == 0u) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }

        uec_runtime_stats pending{};
        pending.struct_size = sizeof(pending);
        result = GApi->get_runtime_stats(GContext, &pending);
        if (result != UEC_RESULT_OK ||
            pending.pending_requests != GBaseline.pending_requests + 1u) {
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
        GOldWorldGameInstance == nullptr ||
        GLatentSmokeActor == nullptr || GLatentSmokeRequestId == 0u ||
        GLatentSmokeCallbackExecuted ||
        GTickCallbackCount == 0u || GTickCallbackCount != GTickCallbackCountAtCleanup) {
        UE_LOG(LogTemp, Error,
            TEXT("PIE restart cleanup invariant failed: cleanup=%d tick=%llu atCleanup=%llu latentRequest=%llu latentCallback=%d api=%d context=%d world=%d actor=%d"),
            GWorldCleanupObserved,
            static_cast<unsigned long long>(GTickCallbackCount),
            static_cast<unsigned long long>(GTickCallbackCountAtCleanup),
            static_cast<unsigned long long>(GLatentSmokeRequestId),
            GLatentSmokeCallbackExecuted,
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
#include "CoreMinimal.h"

#include "uec_api.h"

extern "C" uec_result UEC_CALL uec_host_multi_pie_smoke(void)
{
    const uec_api* api = nullptr;
    uec_context* context = nullptr;
    uec_world* worlds[2]{};
    uec_actor* controllers[2]{};
    uec_object* gameInstances[2]{};
    int32_t instances[2] = {-1, -1};
    char gameInstancePaths[2][256]{};
    uec_runtime_stats baseline{};
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;

    if (api == nullptr || context == nullptr || api->get_world_count_by_kind == nullptr ||
        api->get_world_at_by_kind == nullptr || api->get_world_kind == nullptr ||
        api->get_world_pie_instance == nullptr || api->get_first_player_controller == nullptr ||
        api->get_actor_name == nullptr || api->get_world_game_instance == nullptr ||
        api->object_is_a == nullptr || api->get_object_path == nullptr ||
        api->release_object == nullptr || api->get_runtime_stats == nullptr ||
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
