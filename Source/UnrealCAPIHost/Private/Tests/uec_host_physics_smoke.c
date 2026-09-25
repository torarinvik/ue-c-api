#include "uec_api.h"

typedef struct uec_physics_smoke_state {
    const uec_api* api;
    uec_context* context;
    uec_world* world;
    uec_actor* actor;
    uec_scene_component* component;
    double velocity_before_force;
    double angular_velocity_before_torque;
    uint32_t phase;
} uec_physics_smoke_state;

static uec_physics_smoke_state g_physics_smoke;

static void CleanupPhysicsSmoke(void)
{
    uec_physics_smoke_state* state = &g_physics_smoke;
    if (state->api != NULL) {
        if (state->component != NULL) {
            if (state->api->set_component_simulating_physics != NULL) {
                (void)state->api->set_component_simulating_physics(state->component, UEC_FALSE);
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
        if (state->world != NULL) {
            if (state->api->release_world != NULL) {
                (void)state->api->release_world(state->world);
            }
            state->world = NULL;
        }
        if (state->context != NULL) {
            if (state->api->release_context != NULL) {
                (void)state->api->release_context(state->context);
            }
            state->context = NULL;
        }
    }
    state->api = NULL;
    state->velocity_before_force = 0.0;
    state->angular_velocity_before_torque = 0.0;
    state->phase = 0u;
}

uec_result UEC_CALL uec_host_physics_smoke_start(void)
{
    static const char actorClassPath[] =
        "/Script/UnrealCAPIHost.UECAPIHostCollisionSmokeActor";
    const uec_string_view classPath = {actorClassPath, sizeof(actorClassPath) - 1u};
    const uec_transform transform = {
        {18000.0, -24000.0, 50000.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_result result;
    if (g_physics_smoke.phase != 0u) return UEC_RESULT_INVALID_ARGUMENT;

    result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    g_physics_smoke.api = api;
    g_physics_smoke.context = context;
    if (api == NULL || context == NULL || api->release_context == NULL ||
        api->get_default_world == NULL || api->release_world == NULL ||
        api->spawn_actor == NULL || api->destroy_actor == NULL ||
        api->get_actor_root_component == NULL || api->release_actor == NULL ||
        api->release_scene_component == NULL || api->set_component_collision_enabled == NULL ||
        api->set_component_simulating_physics == NULL ||
        api->get_component_simulating_physics == NULL ||
        api->get_component_velocity == NULL ||
        api->get_component_physics_angular_velocity == NULL ||
        api->set_component_physics_velocity == NULL ||
        api->set_component_physics_angular_velocity == NULL ||
        api->apply_component_impulse == NULL || api->apply_component_force == NULL ||
        api->apply_component_torque == NULL || api->apply_component_angular_impulse == NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto failed;
    }
    result = api->get_default_world(context, &g_physics_smoke.world);
    if (result != UEC_RESULT_OK || g_physics_smoke.world == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto failed;
    }
    result = api->spawn_actor(g_physics_smoke.world, classPath, &transform,
                              &g_physics_smoke.actor);
    if (result != UEC_RESULT_OK || g_physics_smoke.actor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto failed;
    }
    result = api->get_actor_root_component(g_physics_smoke.actor,
                                            &g_physics_smoke.component);
    if (result != UEC_RESULT_OK || g_physics_smoke.component == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto failed;
    }
    result = api->set_component_collision_enabled(g_physics_smoke.component,
                                                   UEC_COLLISION_QUERY_AND_PHYSICS);
    if (result != UEC_RESULT_OK) goto failed;
    result = api->set_component_simulating_physics(g_physics_smoke.component, UEC_TRUE);
    if (result != UEC_RESULT_OK) goto failed;
    result = api->set_component_physics_velocity(g_physics_smoke.component,
                                                  (uec_vector3){0}, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto failed;
    result = api->set_component_physics_angular_velocity(g_physics_smoke.component,
                                                          (uec_vector3){0}, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto failed;
    result = api->apply_component_impulse(g_physics_smoke.component,
                                          (uec_vector3){10.0, 0.0, 0.0}, UEC_TRUE);
    if (result != UEC_RESULT_OK) goto failed;
    result = api->apply_component_angular_impulse(g_physics_smoke.component,
                                                  (uec_vector3){0.0, 0.25, 0.0}, UEC_TRUE);
    if (result != UEC_RESULT_OK) goto failed;
    g_physics_smoke.phase = 1u;
    return UEC_RESULT_OK;

failed:
    CleanupPhysicsSmoke();
    return result;
}

uec_bool UEC_CALL uec_host_physics_smoke_poll(uec_result* outResult)
{
    uec_physics_smoke_state* state = &g_physics_smoke;
    uec_vector3 linearVelocity = {0};
    uec_vector3 angularVelocity = {0};
    uec_bool simulating = UEC_FALSE;
    uec_result result = UEC_RESULT_OK;
    if (outResult == NULL) return UEC_FALSE;
    *outResult = UEC_RESULT_NOT_INITIALIZED;
    if (state->phase == 0u || state->api == NULL || state->component == NULL) {
        *outResult = UEC_RESULT_INVALID_ARGUMENT;
        return UEC_TRUE;
    }
    if (state->phase == 1u) {
        result = state->api->get_component_simulating_physics(state->component, &simulating);
        if (result == UEC_RESULT_OK && simulating != UEC_TRUE) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            result = state->api->get_component_velocity(state->component, &linearVelocity);
        }
        if (result == UEC_RESULT_OK) {
            result = state->api->get_component_physics_angular_velocity(
                state->component, &angularVelocity);
        }
        if (result == UEC_RESULT_OK &&
            (linearVelocity.x < 1.0 || angularVelocity.y < 0.05)) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            state->velocity_before_force = linearVelocity.x;
            state->angular_velocity_before_torque = angularVelocity.y;
            result = state->api->apply_component_force(
                state->component, (uec_vector3){100000000.0, 0.0, 0.0});
        }
        if (result == UEC_RESULT_OK) {
            result = state->api->apply_component_torque(
                state->component, (uec_vector3){0.0, 100000000.0, 0.0}, UEC_FALSE);
        }
        if (result == UEC_RESULT_OK) {
            state->phase = 2u;
            return UEC_FALSE;
        }
    } else {
        result = state->api->get_component_velocity(state->component, &linearVelocity);
        if (result == UEC_RESULT_OK) {
            result = state->api->get_component_physics_angular_velocity(
                state->component, &angularVelocity);
        }
        if (result == UEC_RESULT_OK &&
            (linearVelocity.x <= state->velocity_before_force + 1.0 ||
             angularVelocity.y <= state->angular_velocity_before_torque + 0.01)) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    *outResult = result;
    CleanupPhysicsSmoke();
    return UEC_TRUE;
}

void UEC_CALL uec_host_physics_smoke_cancel(void)
{
    CleanupPhysicsSmoke();
}
