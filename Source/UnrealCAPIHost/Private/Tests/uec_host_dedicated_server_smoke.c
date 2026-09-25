#include "uec_api.h"

uec_result UEC_CALL uec_host_dedicated_server_context_smoke(void)
{
    static const char actorClassPathText[] =
        "/Script/UnrealCAPIHost.UECAPIHostCollisionSmokeActor";
    const uec_string_view actorClassPath = {
        actorClassPathText, sizeof(actorClassPathText) - 1u
    };
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* serverWorld = NULL;
    uec_actor* actor = NULL;
    uec_world_kind worldKind = UEC_WORLD_KIND_UNKNOWN;
    uec_net_mode netMode = UEC_NET_MODE_UNKNOWN;
    uec_bool hasAuthority = UEC_FALSE;
    uec_transform transform = {0};
    uint32_t worldCount = 0u;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL || api->release_context == NULL ||
        api->get_world_count == NULL || api->get_world_at == NULL ||
        api->get_world_kind == NULL || api->get_world_net_mode == NULL ||
        api->get_world_has_authority == NULL || api->spawn_actor == NULL ||
        api->destroy_actor == NULL || api->release_world == NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    result = api->get_world_count(context, &worldCount);
    if (result != UEC_RESULT_OK) goto cleanup;
    for (uint32_t index = 0u; index < worldCount; ++index) {
        uec_world* candidate = NULL;
        result = api->get_world_at(context, index, &candidate);
        if (result != UEC_RESULT_OK || candidate == NULL) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
        result = api->get_world_net_mode(candidate, &netMode);
        if (result != UEC_RESULT_OK) {
            (void)api->release_world(candidate);
            goto cleanup;
        }
        if (netMode == UEC_NET_MODE_DEDICATED_SERVER && serverWorld == NULL) {
            serverWorld = candidate;
        } else {
            (void)api->release_world(candidate);
        }
    }
    if (serverWorld == NULL) {
        result = UEC_RESULT_NOT_INITIALIZED;
        goto cleanup;
    }
    result = api->get_world_kind(serverWorld, &worldKind);
    if (result != UEC_RESULT_OK ||
        (worldKind != UEC_WORLD_KIND_GAME && worldKind != UEC_WORLD_KIND_PIE)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_world_has_authority(serverWorld, &hasAuthority);
    if (result != UEC_RESULT_OK || hasAuthority != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    transform.rotation.w = 1.0;
    transform.scale.x = 1.0;
    transform.scale.y = 1.0;
    transform.scale.z = 1.0;
    result = api->spawn_actor(serverWorld, actorClassPath, &transform, &actor);
    if (result != UEC_RESULT_OK || actor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->destroy_actor(actor);
    if (result == UEC_RESULT_OK) actor = NULL;

cleanup:
    if (actor != NULL && api != NULL && api->destroy_actor != NULL) {
        (void)api->destroy_actor(actor);
    }
    if (serverWorld != NULL && api != NULL && api->release_world != NULL) {
        const uec_result releaseResult = api->release_world(serverWorld);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    if (context != NULL && api != NULL && api->release_context != NULL) {
        const uec_result releaseResult = api->release_context(context);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    return result;
}
