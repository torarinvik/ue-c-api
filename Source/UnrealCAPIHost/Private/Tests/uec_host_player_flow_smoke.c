#include "uec_api.h"

#include <math.h>
#include <string.h>

static uec_string_view PlayerFlowView(const char* text)
{
    return (uec_string_view){text, strlen(text)};
}

static int PlayerFlowNear(double actual, double expected)
{
    return fabs(actual - expected) <= 0.01;
}

static uec_result VerifyComponentMeshPath(const uec_api* api,
                                          uec_scene_component* component,
                                          const char* expectedPath)
{
    uec_object* mesh = NULL;
    char path[512] = {0};
    size_t requiredSize = 0u;
    uec_result result = api->get_component_mesh(component, &mesh);
    if (result == UEC_RESULT_OK && mesh == NULL) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->get_object_path(mesh, path, sizeof(path), &requiredSize);
    if (result == UEC_RESULT_OK && strcmp(path, expectedPath) != 0)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (mesh != NULL) {
        const uec_result releaseResult = api->release_object(mesh);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
    }
    return result;
}

static uec_result DestroyPlayerFlowPawn(const uec_api* api, uec_actor** pawn)
{
    if (api == NULL || pawn == NULL || *pawn == NULL) return UEC_RESULT_OK;
    const uec_result result = api->destroy_actor(*pawn);
    if (result == UEC_RESULT_OK) {
        *pawn = NULL;
        return UEC_RESULT_OK;
    }
    const uec_result releaseResult = api->release_actor(*pawn);
    *pawn = NULL;
    return releaseResult == UEC_RESULT_OK ? result : releaseResult;
}

uec_result UEC_CALL uec_host_player_flow_smoke(void)
{
    static const char pawnClassPath[] =
        "/Script/UnrealCAPIHost.UECAPIHostPlayerFlowPawn";
    static const char cameraClassPath[] = "/Script/Engine.CameraComponent";
    static const char meshComponentClassPath[] = "/Script/Engine.StaticMeshComponent";
    static const char skeletalMeshComponentClassPath[] = "/Script/Engine.SkeletalMeshComponent";
    static const char meshAssetPath[] = "/Engine/BasicShapes/Cube.Cube";
    static const char skeletalMeshAssetPath[] = "/Engine/EngineMeshes/SkeletalCube.SkeletalCube";
    static const char wrongMeshPath[] = "/Script/Engine.Actor";
    static const char scalarParameterPath[] = "UECAPI_SmokeScalar";
    static const char vectorParameterPath[] = "UECAPI_SmokeVector";
    static const char markerProperty[] = "CApiMarker";
    const uec_string_view cameraClass = {
        cameraClassPath, sizeof(cameraClassPath) - 1u};
    const uec_string_view meshComponentClass = {
        meshComponentClassPath, sizeof(meshComponentClassPath) - 1u};
    const uec_string_view skeletalMeshComponentClass = {
        skeletalMeshComponentClassPath, sizeof(skeletalMeshComponentClassPath) - 1u};
    const uec_string_view meshAssetName = {
        meshAssetPath, sizeof(meshAssetPath) - 1u};
    const uec_string_view skeletalMeshAssetName = {
        skeletalMeshAssetPath, sizeof(skeletalMeshAssetPath) - 1u};
    const uec_string_view wrongMeshName = {
        wrongMeshPath, sizeof(wrongMeshPath) - 1u};
    const uec_string_view scalarParameterName = {
        scalarParameterPath, sizeof(scalarParameterPath) - 1u};
    const uec_string_view vectorParameterName = {
        vectorParameterPath, sizeof(vectorParameterPath) - 1u};
    const uec_string_view markerName = {
        markerProperty, sizeof(markerProperty) - 1u};
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* world = NULL;
    uec_actor* controller = NULL;
    uec_actor* originalPawn = NULL;
    uec_actor* pawn = NULL;
    uec_actor* possessedPawn = NULL;
    uec_actor* playerStart = NULL;
    uec_scene_component* camera = NULL;
    uec_scene_component* meshComponent = NULL;
    uec_scene_component* skeletalMeshComponent = NULL;
    uec_object* meshAsset = NULL;
    uec_object* skeletalMeshAsset = NULL;
    uec_object* wrongMeshObject = NULL;
    uec_runtime_stats baseline = {0};
    uec_bool controllerChanged = UEC_FALSE;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL || api->get_runtime_stats == NULL ||
        api->get_default_world == NULL || api->get_world_has_authority == NULL ||
        api->release_world == NULL || api->get_player_controller == NULL ||
        api->get_controller_pawn == NULL || api->possess_pawn == NULL ||
        api->set_controller_view_target == NULL || api->find_player_start == NULL ||
        api->spawn_actor == NULL || api->destroy_actor == NULL ||
        api->release_actor == NULL || api->get_actor_component_count_by_class == NULL ||
        api->get_actor_component_at_by_class == NULL || api->release_scene_component == NULL ||
        api->get_camera_field_of_view == NULL || api->set_camera_field_of_view == NULL ||
        api->get_actor_property_value == NULL || api->load_object == NULL ||
        api->release_object == NULL || api->set_static_mesh == NULL ||
        api->set_skeletal_mesh == NULL || api->get_component_mesh == NULL ||
        api->get_object_path == NULL ||
        api->set_component_material_scalar == NULL ||
        api->set_component_material_vector == NULL ||
        api->release_context == NULL) {
        result = UEC_RESULT_UNSUPPORTED;
        goto cleanup;
    }

    baseline.struct_size = sizeof(baseline);
    result = api->get_runtime_stats(context, &baseline);
    if (result == UEC_RESULT_OK) result = api->get_default_world(context, &world);
    if (result == UEC_RESULT_OK && world == NULL) result = UEC_RESULT_INTERNAL_ERROR;
    uec_bool hasAuthority = UEC_FALSE;
    if (result == UEC_RESULT_OK)
        result = api->get_world_has_authority(world, &hasAuthority);
    if (result == UEC_RESULT_OK && hasAuthority != UEC_TRUE)
        result = UEC_RESULT_UNSUPPORTED;
    if (result == UEC_RESULT_OK)
        result = api->get_player_controller(world, 0u, &controller);
    if (result == UEC_RESULT_OK && controller == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) {
        const uec_result pawnResult = api->get_controller_pawn(controller, &originalPawn);
        if (pawnResult != UEC_RESULT_OK && pawnResult != UEC_RESULT_NOT_INITIALIZED)
            result = pawnResult;
    }
    if (result == UEC_RESULT_OK)
        result = api->find_player_start(world, 0u, &playerStart);
    if (result == UEC_RESULT_OK && playerStart == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;

    const uec_transform transform = {
        {100000.0, 100000.0, 100000.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    if (result == UEC_RESULT_OK)
        result = api->spawn_actor(world, PlayerFlowView(pawnClassPath), &transform, &pawn);
    if (result == UEC_RESULT_OK && pawn == NULL) result = UEC_RESULT_INTERNAL_ERROR;

    uint32_t cameraCount = 0u;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_component_count_by_class(pawn, cameraClass, &cameraCount);
    if (result == UEC_RESULT_OK && cameraCount != 1u) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_component_at_by_class(pawn, cameraClass, 0u, &camera);
    if (result == UEC_RESULT_OK && camera == NULL) result = UEC_RESULT_INTERNAL_ERROR;

    double fieldOfView = 0.0;
    if (result == UEC_RESULT_OK) result = api->get_camera_field_of_view(camera, &fieldOfView);
    if (result == UEC_RESULT_OK && !PlayerFlowNear(fieldOfView, 87.0))
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->set_camera_field_of_view(camera, 73.5);
    if (result == UEC_RESULT_OK &&
        api->set_camera_field_of_view(camera, 360.0) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) result = api->get_camera_field_of_view(camera, &fieldOfView);
    if (result == UEC_RESULT_OK && !PlayerFlowNear(fieldOfView, 73.5))
        result = UEC_RESULT_INTERNAL_ERROR;

    uint32_t meshComponentCount = 0u;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_component_count_by_class(
            pawn, meshComponentClass, &meshComponentCount);
    if (result == UEC_RESULT_OK && meshComponentCount == 0u)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_component_at_by_class(
            pawn, meshComponentClass, 0u, &meshComponent);
    if (result == UEC_RESULT_OK && meshComponent == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->load_object(context, meshAssetName, &meshAsset);
    if (result == UEC_RESULT_OK && meshAsset == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    uec_object* observedMesh = meshAsset;
    if (result == UEC_RESULT_OK &&
        (api->get_component_mesh(meshComponent, &observedMesh) !=
             UEC_RESULT_NOT_INITIALIZED || observedMesh != NULL))
        result = UEC_RESULT_INTERNAL_ERROR;
    observedMesh = meshAsset;
    if (result == UEC_RESULT_OK &&
        (api->get_component_mesh(camera, &observedMesh) !=
             UEC_RESULT_INVALID_ARGUMENT || observedMesh != NULL))
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK &&
        api->get_component_mesh(meshComponent, NULL) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK && api->set_static_mesh(camera, meshAsset) !=
            UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->set_static_mesh(meshComponent, meshAsset);
    if (result == UEC_RESULT_OK)
        result = VerifyComponentMeshPath(api, meshComponent, meshAssetPath);
    if (result == UEC_RESULT_OK)
        result = api->load_object(context, wrongMeshName, &wrongMeshObject);
    if (result == UEC_RESULT_OK && wrongMeshObject == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK && api->set_static_mesh(
            meshComponent, wrongMeshObject) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;

    uint32_t skeletalMeshComponentCount = 0u;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_component_count_by_class(
            pawn, skeletalMeshComponentClass, &skeletalMeshComponentCount);
    if (result == UEC_RESULT_OK && skeletalMeshComponentCount == 0u)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_component_at_by_class(
            pawn, skeletalMeshComponentClass, 0u, &skeletalMeshComponent);
    if (result == UEC_RESULT_OK && skeletalMeshComponent == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->load_object(context, skeletalMeshAssetName, &skeletalMeshAsset);
    if (result == UEC_RESULT_OK && skeletalMeshAsset == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    observedMesh = skeletalMeshAsset;
    if (result == UEC_RESULT_OK &&
        (api->get_component_mesh(skeletalMeshComponent, &observedMesh) !=
             UEC_RESULT_NOT_INITIALIZED || observedMesh != NULL))
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK && api->set_skeletal_mesh(
            meshComponent, skeletalMeshAsset, UEC_FALSE) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->set_skeletal_mesh(
            skeletalMeshComponent, skeletalMeshAsset, UEC_FALSE);
    if (result == UEC_RESULT_OK)
        result = VerifyComponentMeshPath(api, skeletalMeshComponent,
                                         skeletalMeshAssetPath);
    if (result == UEC_RESULT_OK && api->set_skeletal_mesh(
            skeletalMeshComponent, wrongMeshObject, UEC_FALSE) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    const uec_vector3 materialVector = {0.25, 0.5, 0.75};
    if (result == UEC_RESULT_OK)
        result = api->set_component_material_scalar(
            meshComponent, scalarParameterName, 0.625);
    if (result == UEC_RESULT_OK)
        result = api->set_component_material_vector(
            meshComponent, vectorParameterName, materialVector);
    if (result == UEC_RESULT_OK)
        result = api->set_component_material_scalar(
            skeletalMeshComponent, scalarParameterName, 0.375);
    if (result == UEC_RESULT_OK)
        result = api->set_component_material_vector(
            skeletalMeshComponent, vectorParameterName, materialVector);
    if (result == UEC_RESULT_OK && api->set_component_material_scalar(
            camera, scalarParameterName, 0.5) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK && api->set_component_material_vector(
            camera, vectorParameterName, materialVector) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK && api->set_component_material_scalar(
            meshComponent, scalarParameterName, 1.0e300) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK && api->set_component_material_vector(
            skeletalMeshComponent, vectorParameterName,
            (uec_vector3){0.0, 1.0e300, 0.0}) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;

    if (result == UEC_RESULT_OK)
        result = api->set_controller_view_target(controller, pawn);
    if (result == UEC_RESULT_OK) controllerChanged = UEC_TRUE;
    if (result == UEC_RESULT_OK) result = api->possess_pawn(controller, pawn);
    if (result == UEC_RESULT_OK)
        result = api->get_controller_pawn(controller, &possessedPawn);
    if (result == UEC_RESULT_OK && possessedPawn == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    uec_property_value marker = {0};
    marker.struct_size = sizeof(marker);
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_value(possessedPawn, markerName, &marker);
    if (result == UEC_RESULT_OK &&
        (marker.kind != UEC_PROPERTY_INTEGER || marker.integer_value != 42))
        result = UEC_RESULT_INTERNAL_ERROR;

cleanup:
    if (controllerChanged == UEC_TRUE && controller != NULL &&
        originalPawn != NULL && api != NULL) {
        const uec_result viewResult = api->set_controller_view_target(controller, originalPawn);
        if (result == UEC_RESULT_OK && viewResult != UEC_RESULT_OK) result = viewResult;
        const uec_result possessResult = api->possess_pawn(controller, originalPawn);
        if (result == UEC_RESULT_OK && possessResult != UEC_RESULT_OK) result = possessResult;
    }
    if (camera != NULL && api != NULL) {
        const uec_result releaseResult = api->release_scene_component(camera);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (meshComponent != NULL && api != NULL) {
        const uec_result releaseResult = api->release_scene_component(meshComponent);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (skeletalMeshComponent != NULL && api != NULL) {
        const uec_result releaseResult = api->release_scene_component(skeletalMeshComponent);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (skeletalMeshAsset != NULL && api != NULL) {
        const uec_result releaseResult = api->release_object(skeletalMeshAsset);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (wrongMeshObject != NULL && api != NULL) {
        const uec_result releaseResult = api->release_object(wrongMeshObject);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (meshAsset != NULL && api != NULL) {
        const uec_result releaseResult = api->release_object(meshAsset);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (possessedPawn != NULL && api != NULL) {
        const uec_result releaseResult = api->release_actor(possessedPawn);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (playerStart != NULL && api != NULL) {
        const uec_result releaseResult = api->release_actor(playerStart);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (pawn != NULL && api != NULL) {
        const uec_result destroyResult = DestroyPlayerFlowPawn(api, &pawn);
        if (result == UEC_RESULT_OK && destroyResult != UEC_RESULT_OK) result = destroyResult;
    }
    if (originalPawn != NULL && api != NULL) {
        const uec_result releaseResult = api->release_actor(originalPawn);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (controller != NULL && api != NULL) {
        const uec_result releaseResult = api->release_actor(controller);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (world != NULL && api != NULL) {
        const uec_result releaseResult = api->release_world(world);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (api != NULL && context != NULL && api->get_runtime_stats != NULL) {
        uec_runtime_stats observed = {0};
        observed.struct_size = sizeof(observed);
        const uec_result statsResult = api->get_runtime_stats(context, &observed);
        if (result == UEC_RESULT_OK && statsResult != UEC_RESULT_OK) result = statsResult;
        if (result == UEC_RESULT_OK &&
            (observed.live_worlds != baseline.live_worlds ||
             observed.live_actors != baseline.live_actors ||
             observed.live_components != baseline.live_components ||
             observed.live_contexts != baseline.live_contexts ||
             observed.live_objects != baseline.live_objects ||
             observed.active_subscriptions != baseline.active_subscriptions ||
             observed.pending_requests != baseline.pending_requests ||
             observed.active_callbacks != baseline.active_callbacks)) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (api != NULL && context != NULL && api->release_context != NULL) {
        const uec_result releaseResult = api->release_context(context);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    return result;
}
