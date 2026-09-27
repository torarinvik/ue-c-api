#include "uec_api.h"

typedef struct uec_listen_server_impulse_state {
    const uec_api* api;
    uec_context* context;
    uec_world* world;
    uec_actor* actor;
    uec_scene_component* component;
    uec_vector3 linear_before;
    uec_vector3 angular_before;
    uint32_t phase;
} uec_listen_server_impulse_state;

static uec_listen_server_impulse_state g_listen_server_impulse;

static void CleanupListenServerImpulse(void)
{
    uec_listen_server_impulse_state* state = &g_listen_server_impulse;
    if (state->api != NULL) {
        if (state->component != NULL) {
            if (state->api->set_component_simulating_physics != NULL) {
                (void)state->api->set_component_simulating_physics(
                    state->component, UEC_FALSE);
            }
            if (state->api->release_scene_component != NULL) {
                (void)state->api->release_scene_component(state->component);
            }
            state->component = NULL;
        }
        if (state->actor != NULL) {
            if (state->api->destroy_actor != NULL &&
                state->api->destroy_actor(state->actor) == UEC_RESULT_OK) {
                state->actor = NULL;
            } else {
                if (state->api->release_actor != NULL) {
                    (void)state->api->release_actor(state->actor);
                }
                state->actor = NULL;
            }
        }
        if (state->world != NULL && state->api->release_world != NULL) {
            (void)state->api->release_world(state->world);
            state->world = NULL;
        }
        if (state->context != NULL && state->api->release_context != NULL) {
            (void)state->api->release_context(state->context);
            state->context = NULL;
        }
    }
    *state = (uec_listen_server_impulse_state){0};
}

static int IsNear(double actual, double expected, double tolerance)
{
    const double difference = actual > expected ? actual - expected : expected - actual;
    return difference <= tolerance;
}

uec_result UEC_CALL uec_host_listen_server_impulse_smoke_start(void)
{
    static const char actorClassText[] =
        "/Script/UnrealCAPIHost.UECAPIHostCollisionSmokeActor";
    const uec_string_view actorClass = {actorClassText, sizeof(actorClassText) - 1u};
    const uec_transform spawnTransform = {
        {22000.0, -24000.0, 50000.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    uec_listen_server_impulse_state* state = &g_listen_server_impulse;
    uint32_t worldCount = 0u;
    uec_result result;
    if (state->phase != 0u) return UEC_RESULT_INVALID_ARGUMENT;

    result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &state->api, &state->context);
    if (result != UEC_RESULT_OK) return result;
    if (state->api == NULL || state->context == NULL ||
        state->api->get_world_count == NULL || state->api->get_world_at == NULL ||
        state->api->get_world_net_mode == NULL || state->api->release_world == NULL ||
        state->api->spawn_actor == NULL || state->api->destroy_actor == NULL ||
        state->api->release_actor == NULL || state->api->get_actor_root_component == NULL ||
        state->api->release_scene_component == NULL ||
        state->api->set_component_collision_enabled == NULL ||
        state->api->set_component_simulating_physics == NULL ||
        state->api->get_component_simulating_physics == NULL ||
        state->api->set_component_physics_velocity == NULL ||
        state->api->get_component_velocity == NULL ||
        state->api->set_component_physics_angular_velocity == NULL ||
        state->api->get_component_physics_angular_velocity == NULL ||
        state->api->apply_component_impulse == NULL ||
        state->api->apply_component_angular_impulse == NULL ||
        state->api->release_context == NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto failed;
    }
    result = state->api->get_world_count(state->context, &worldCount);
    if (result != UEC_RESULT_OK) goto failed;
    for (uint32_t index = 0u; index < worldCount; ++index) {
        uec_world* candidate = NULL;
        uec_net_mode netMode = UEC_NET_MODE_UNKNOWN;
        result = state->api->get_world_at(state->context, index, &candidate);
        if (result != UEC_RESULT_OK || candidate == NULL) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto failed;
        }
        result = state->api->get_world_net_mode(candidate, &netMode);
        if (result != UEC_RESULT_OK) {
            (void)state->api->release_world(candidate);
            goto failed;
        }
        if (netMode == UEC_NET_MODE_LISTEN_SERVER) {
            state->world = candidate;
            break;
        }
        (void)state->api->release_world(candidate);
    }
    if (state->world == NULL) {
        result = UEC_RESULT_NOT_INITIALIZED;
        goto failed;
    }
    result = state->api->spawn_actor(state->world, actorClass, &spawnTransform, &state->actor);
    if (result != UEC_RESULT_OK || state->actor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto failed;
    }
    result = state->api->get_actor_root_component(state->actor, &state->component);
    if (result != UEC_RESULT_OK || state->component == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto failed;
    }
    result = state->api->set_component_collision_enabled(
        state->component, UEC_COLLISION_QUERY_AND_PHYSICS);
    if (result != UEC_RESULT_OK) goto failed;
    result = state->api->set_component_simulating_physics(state->component, UEC_TRUE);
    if (result != UEC_RESULT_OK) goto failed;
    uec_bool simulating = UEC_FALSE;
    result = state->api->get_component_simulating_physics(state->component, &simulating);
    if (result != UEC_RESULT_OK || simulating != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto failed;
    }
    result = state->api->set_component_physics_velocity(
        state->component, (uec_vector3){0.0, 0.0, 0.0}, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto failed;
    result = state->api->set_component_physics_angular_velocity(
        state->component, (uec_vector3){0.0, 0.0, 0.0}, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto failed;
    result = state->api->get_component_velocity(state->component, &state->linear_before);
    if (result != UEC_RESULT_OK) goto failed;
    result = state->api->get_component_physics_angular_velocity(
        state->component, &state->angular_before);
    if (result != UEC_RESULT_OK) goto failed;
    result = state->api->apply_component_impulse(
        state->component, (uec_vector3){20.0, -10.0, 0.0}, UEC_TRUE);
    if (result != UEC_RESULT_OK) goto failed;
    result = state->api->apply_component_angular_impulse(
        state->component, (uec_vector3){0.25, -0.5, 0.75}, UEC_TRUE);
    if (result != UEC_RESULT_OK) goto failed;
    state->phase = 1u;
    return UEC_RESULT_OK;

failed:
    CleanupListenServerImpulse();
    return result;
}

uec_bool UEC_CALL uec_host_listen_server_impulse_smoke_poll(uec_result* outResult)
{
    uec_listen_server_impulse_state* state = &g_listen_server_impulse;
    uec_vector3 linearAfter = {0};
    uec_vector3 angularAfter = {0};
    uec_result result;
    if (outResult == NULL) return UEC_FALSE;
    *outResult = UEC_RESULT_NOT_INITIALIZED;
    if (state->phase == 0u || state->api == NULL || state->component == NULL) {
        *outResult = UEC_RESULT_INVALID_ARGUMENT;
        return UEC_TRUE;
    }
    result = state->api->get_component_velocity(state->component, &linearAfter);
    if (result == UEC_RESULT_OK) {
        result = state->api->get_component_physics_angular_velocity(
            state->component, &angularAfter);
    }
    if (result == UEC_RESULT_OK &&
        (!IsNear(linearAfter.x, state->linear_before.x + 20.0, 1.0) ||
         !IsNear(linearAfter.y, state->linear_before.y - 10.0, 1.0) ||
         !IsNear(angularAfter.x, state->angular_before.x + 0.25, 0.1) ||
         !IsNear(angularAfter.y, state->angular_before.y - 0.5, 0.1) ||
         !IsNear(angularAfter.z, state->angular_before.z + 0.75, 0.1))) {
        *outResult = UEC_RESULT_OK;
        return UEC_FALSE;
    }
    *outResult = result;
    CleanupListenServerImpulse();
    return UEC_TRUE;
}

void UEC_CALL uec_host_listen_server_impulse_smoke_cancel(void)
{
    CleanupListenServerImpulse();
}
