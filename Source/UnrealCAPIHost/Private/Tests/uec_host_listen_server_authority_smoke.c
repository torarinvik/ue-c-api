#include "uec_api.h"

uec_result UEC_CALL uec_host_listen_server_authority_smoke(void)
{
    static const char actorClassText[] =
        "/Script/UnrealCAPIHost.UECAPIHostCollisionSmokeActor";
    static const char replicatedPropertyText[] = "AuthoritySmokeReplicatedValue";
    static const char tagText[] = "UEC_ListenServerAuthoritySmoke";
    static const char textValue[] = "25";
    const uec_string_view actorClass = {actorClassText, sizeof(actorClassText) - 1u};
    const uec_string_view replicatedProperty = {
        replicatedPropertyText, sizeof(replicatedPropertyText) - 1u
    };
    const uec_string_view tag = {tagText, sizeof(tagText) - 1u};
    const uec_string_view text = {textValue, sizeof(textValue) - 1u};
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* listenServer = NULL;
    uec_actor* actor = NULL;
    uec_net_mode netMode = UEC_NET_MODE_UNKNOWN;
    uec_bool hasAuthority = UEC_FALSE;
    uec_bool hasTag = UEC_FALSE;
    uec_transform spawnTransform = {0};
    uec_transform transformBefore = {0};
    uec_transform attemptedTransform = {0};
    uec_transform transformAfter = {0};
    uec_property_value valueBefore = {0};
    uec_property_value attemptedValue = {0};
    uec_property_value valueAfter = {0};
    uint32_t worldCount = 0u;
    int actorDestroyed = 0;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL || api->release_context == NULL ||
        api->get_world_count == NULL || api->get_world_at == NULL ||
        api->get_world_net_mode == NULL || api->get_world_has_authority == NULL ||
        api->release_world == NULL || api->spawn_actor == NULL ||
        api->destroy_actor == NULL || api->release_actor == NULL ||
        api->get_actor_transform == NULL || api->set_actor_transform == NULL ||
        api->get_actor_property_value == NULL || api->set_actor_property_value == NULL ||
        api->set_actor_property_string == NULL || api->set_actor_tag == NULL ||
        api->actor_has_tag == NULL) {
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
        if (netMode == UEC_NET_MODE_LISTEN_SERVER) {
            listenServer = candidate;
            break;
        }
        (void)api->release_world(candidate);
    }
    if (listenServer == NULL) {
        result = UEC_RESULT_NOT_INITIALIZED;
        goto cleanup;
    }
    result = api->get_world_net_mode(listenServer, &netMode);
    if (result != UEC_RESULT_OK || netMode != UEC_NET_MODE_LISTEN_SERVER) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_world_has_authority(listenServer, &hasAuthority);
    if (result != UEC_RESULT_OK || hasAuthority != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    spawnTransform.rotation.w = 1.0;
    spawnTransform.scale.x = 1.0;
    spawnTransform.scale.y = 1.0;
    spawnTransform.scale.z = 1.0;
    result = api->spawn_actor(listenServer, actorClass, &spawnTransform, &actor);
    if (result != UEC_RESULT_OK || actor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    valueBefore.struct_size = sizeof(valueBefore);
    result = api->get_actor_property_value(actor, replicatedProperty, &valueBefore);
    if (result != UEC_RESULT_OK || valueBefore.kind != UEC_PROPERTY_INTEGER) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    attemptedValue = valueBefore;
    attemptedValue.integer_value++;
    result = api->set_actor_property_value(actor, replicatedProperty, &attemptedValue);
    if (result != UEC_RESULT_OK) goto cleanup;
    valueAfter.struct_size = sizeof(valueAfter);
    result = api->get_actor_property_value(actor, replicatedProperty, &valueAfter);
    if (result != UEC_RESULT_OK || valueAfter.kind != UEC_PROPERTY_INTEGER ||
        valueAfter.integer_value != attemptedValue.integer_value) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->set_actor_property_string(actor, replicatedProperty, text);
    if (result != UEC_RESULT_OK) goto cleanup;
    valueAfter = (uec_property_value){0};
    valueAfter.struct_size = sizeof(valueAfter);
    result = api->get_actor_property_value(actor, replicatedProperty, &valueAfter);
    if (result != UEC_RESULT_OK || valueAfter.kind != UEC_PROPERTY_INTEGER ||
        valueAfter.integer_value != 25) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    result = api->set_actor_tag(actor, tag, UEC_TRUE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->actor_has_tag(actor, tag, &hasTag);
    if (result != UEC_RESULT_OK || hasTag != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    result = api->get_actor_transform(actor, &transformBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    attemptedTransform = transformBefore;
    attemptedTransform.translation.x += 30.0;
    result = api->set_actor_transform(actor, &attemptedTransform, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_actor_transform(actor, &transformAfter);
    if (result != UEC_RESULT_OK ||
        transformAfter.translation.x != attemptedTransform.translation.x) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    result = api->destroy_actor(actor);
    if (result != UEC_RESULT_OK) goto cleanup;
    actorDestroyed = 1;

cleanup:
    if (actor != NULL) {
        if (!actorDestroyed && api != NULL && api->destroy_actor != NULL) {
            (void)api->destroy_actor(actor);
        }
        if (api != NULL && api->release_actor != NULL) (void)api->release_actor(actor);
    }
    if (listenServer != NULL && api != NULL && api->release_world != NULL) {
        (void)api->release_world(listenServer);
    }
    if (context != NULL && api != NULL && api->release_context != NULL) {
        (void)api->release_context(context);
    }
    return result;
}
