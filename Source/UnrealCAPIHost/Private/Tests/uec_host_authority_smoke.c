#include "uec_api.h"

static int IsNear(double actual, double expected)
{
    const double tolerance = 0.05;
    return actual >= expected - tolerance && actual <= expected + tolerance;
}

static int IsSameVector(uec_vector3 actual, uec_vector3 expected)
{
    return IsNear(actual.x, expected.x) && IsNear(actual.y, expected.y) &&
        IsNear(actual.z, expected.z);
}

uec_result UEC_CALL uec_host_authority_smoke(void)
{
    static const char actorClassPath[] =
        "/Script/UnrealCAPIHost.UECAPIHostCollisionSmokeActor";
    const uec_string_view classPath = {actorClassPath, sizeof(actorClassPath) - 1u};
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* clientWorld = NULL;
    uec_actor* actor = NULL;
    uec_scene_component* component = NULL;
    uec_net_mode netMode = UEC_NET_MODE_UNKNOWN;
    uec_bool hasAuthority = UEC_TRUE;
    uec_bool simulating = UEC_FALSE;
    uec_vector3 linearBefore = {0};
    uec_vector3 angularBefore = {0};
    uec_vector3 readback = {0};
    uint32_t worldCount = 0u;
    uint32_t actorCount = 0u;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL || api->release_context == NULL ||
        api->get_world_count == NULL || api->get_world_at == NULL ||
        api->get_world_net_mode == NULL || api->get_world_has_authority == NULL ||
        api->release_world == NULL || api->get_actor_count_by_class == NULL ||
        api->get_actor_at_by_class == NULL || api->release_actor == NULL ||
        api->get_actor_root_component == NULL || api->release_scene_component == NULL ||
        api->get_component_simulating_physics == NULL ||
        api->get_component_velocity == NULL ||
        api->get_component_physics_angular_velocity == NULL ||
        api->set_component_simulating_physics == NULL ||
        api->set_component_physics_velocity == NULL ||
        api->set_component_physics_angular_velocity == NULL ||
        api->apply_component_impulse == NULL || api->apply_component_force == NULL ||
        api->apply_component_torque == NULL || api->apply_component_angular_impulse == NULL ||
        api->get_actor_velocity == NULL || api->get_actor_physics_angular_velocity == NULL ||
        api->set_actor_physics_velocity == NULL || api->set_actor_physics_angular_velocity == NULL ||
        api->apply_actor_impulse == NULL || api->apply_actor_force == NULL ||
        api->apply_actor_torque == NULL || api->apply_actor_angular_impulse == NULL) {
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
        if (netMode == UEC_NET_MODE_CLIENT) {
            clientWorld = candidate;
            break;
        }
        (void)api->release_world(candidate);
    }
    if (clientWorld == NULL) {
        result = UEC_RESULT_NOT_INITIALIZED;
        goto cleanup;
    }
    result = api->get_world_has_authority(clientWorld, &hasAuthority);
    if (result != UEC_RESULT_OK || hasAuthority != UEC_FALSE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_actor_count_by_class(clientWorld, classPath, &actorCount);
    if (result != UEC_RESULT_OK || actorCount != 1u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_actor_at_by_class(clientWorld, classPath, 0u, &actor);
    if (result != UEC_RESULT_OK || actor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_actor_root_component(actor, &component);
    if (result != UEC_RESULT_OK || component == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_component_simulating_physics(component, &simulating);
    if (result != UEC_RESULT_OK || simulating != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_component_velocity(component, &linearBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_physics_angular_velocity(component, &angularBefore);
    if (result != UEC_RESULT_OK) goto cleanup;

    if (api->set_component_simulating_physics(component, UEC_FALSE) != UEC_RESULT_UNSUPPORTED ||
        api->set_component_simulating_physics(component, UEC_TRUE) != UEC_RESULT_UNSUPPORTED ||
        api->set_component_physics_velocity(component, (uec_vector3){10.0, 0.0, 0.0}, UEC_FALSE) !=
            UEC_RESULT_UNSUPPORTED ||
        api->set_component_physics_angular_velocity(
            component, (uec_vector3){0.0, 10.0, 0.0}, UEC_FALSE) != UEC_RESULT_UNSUPPORTED ||
        api->apply_component_impulse(component, (uec_vector3){10.0, 0.0, 0.0}, UEC_TRUE) !=
            UEC_RESULT_UNSUPPORTED ||
        api->apply_component_force(component, (uec_vector3){10.0, 0.0, 0.0}) !=
            UEC_RESULT_UNSUPPORTED ||
        api->apply_component_torque(component, (uec_vector3){0.0, 10.0, 0.0}, UEC_FALSE) !=
            UEC_RESULT_UNSUPPORTED ||
        api->apply_component_angular_impulse(
            component, (uec_vector3){0.0, 10.0, 0.0}, UEC_TRUE) != UEC_RESULT_UNSUPPORTED ||
        api->set_actor_physics_velocity(actor, (uec_vector3){10.0, 0.0, 0.0}, UEC_FALSE) !=
            UEC_RESULT_UNSUPPORTED ||
        api->set_actor_physics_angular_velocity(
            actor, (uec_vector3){0.0, 10.0, 0.0}, UEC_FALSE) != UEC_RESULT_UNSUPPORTED ||
        api->apply_actor_impulse(actor, (uec_vector3){10.0, 0.0, 0.0}, UEC_TRUE) !=
            UEC_RESULT_UNSUPPORTED ||
        api->apply_actor_force(actor, (uec_vector3){10.0, 0.0, 0.0}) != UEC_RESULT_UNSUPPORTED ||
        api->apply_actor_torque(actor, (uec_vector3){0.0, 10.0, 0.0}, UEC_FALSE) !=
            UEC_RESULT_UNSUPPORTED ||
        api->apply_actor_angular_impulse(actor, (uec_vector3){0.0, 10.0, 0.0}, UEC_TRUE) !=
            UEC_RESULT_UNSUPPORTED) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_component_velocity(component, &readback);
    if (result != UEC_RESULT_OK || !IsSameVector(readback, linearBefore)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_actor_velocity(actor, &readback);
    if (result != UEC_RESULT_OK || !IsSameVector(readback, linearBefore)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_component_physics_angular_velocity(component, &readback);
    if (result != UEC_RESULT_OK || !IsSameVector(readback, angularBefore)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_actor_physics_angular_velocity(actor, &readback);
    if (result != UEC_RESULT_OK || !IsSameVector(readback, angularBefore)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
    }

cleanup:
    if (api != NULL) {
        if (component != NULL && api->release_scene_component != NULL) {
            (void)api->release_scene_component(component);
        }
        if (actor != NULL && api->release_actor != NULL) (void)api->release_actor(actor);
        if (clientWorld != NULL && api->release_world != NULL) {
            (void)api->release_world(clientWorld);
        }
        if (context != NULL && api->release_context != NULL) {
            (void)api->release_context(context);
        }
    }
    return result;
}
