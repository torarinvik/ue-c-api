#include "uec_api.h"

#include <stdio.h>

#define UEC_COLLISION_SMOKE_FAIL() do { failureLine = __LINE__; goto cleanup; } while (0)

static int IsNear(double actual, double expected)
{
    const double difference = actual > expected ? actual - expected : expected - actual;
    return difference <= 1.0;
}

static int IsAt(const uec_api* api, uec_actor* actor, uec_vector3 position)
{
    uec_transform transform = {0};
    if (api->get_actor_transform(actor, &transform) != UEC_RESULT_OK) return 0;
    return IsNear(transform.translation.x, position.x) &&
        IsNear(transform.translation.y, position.y) &&
        IsNear(transform.translation.z, position.z);
}

static int RuntimeStatsMatch(const uec_runtime_stats* expected,
                             const uec_runtime_stats* actual)
{
    return expected->active_subscriptions == actual->active_subscriptions &&
        expected->pending_requests == actual->pending_requests &&
        expected->active_callbacks == actual->active_callbacks &&
        expected->live_contexts == actual->live_contexts &&
        expected->live_worlds == actual->live_worlds &&
        expected->live_actors == actual->live_actors &&
        expected->live_components == actual->live_components &&
        expected->live_classes == actual->live_classes &&
        expected->live_objects == actual->live_objects;
}

uec_result UEC_CALL uec_host_collision_smoke(void)
{
    static const char actorClassPath[] =
        "/Script/UnrealCAPIHost.UECAPIHostCollisionSmokeActor";
    const uec_string_view classPath = {actorClassPath, sizeof(actorClassPath) - 1u};
    const uec_vector3 center = {12000.0, -24000.0, 50000.0};
    const uec_vector3 start = {center.x - 250.0, center.y, center.z};
    const uec_vector3 end = {center.x + 250.0, center.y, center.z};
    const uec_collision_shape sphere = {
        sizeof(uec_collision_shape), UEC_COLLISION_SHAPE_SPHERE, 0u,
        20.0, {0.0, 0.0, 0.0}, 0.0};
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* world = NULL;
    uec_actor* actor = NULL;
    uec_scene_component* component = NULL;
    uec_hit_result hit = {0};
    uec_hit_result_details details = {0};
    uec_actor* overlaps[4] = {0};
    uec_actor* ignored[1] = {0};
    uec_runtime_stats baseline = {0};
    uec_runtime_stats observed = {0};
    uint32_t overlapCount = 0u;
    int failureLine = 0;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL || api->release_context == NULL ||
        api->get_default_world == NULL || api->release_world == NULL ||
        api->get_runtime_stats == NULL || api->spawn_actor == NULL ||
        api->destroy_actor == NULL || api->get_actor_root_component == NULL ||
        api->release_scene_component == NULL || api->release_actor == NULL ||
        api->set_component_collision_enabled == NULL ||
        api->get_component_collision_enabled == NULL ||
        api->set_component_simulating_physics == NULL ||
        api->get_component_simulating_physics == NULL ||
        api->set_component_collision_channel_response == NULL ||
        api->get_component_collision_response == NULL || api->line_trace == NULL ||
        api->line_trace_filtered == NULL || api->sweep_trace_filtered == NULL ||
        api->overlap_shape_filtered == NULL || api->trace_detailed_filtered == NULL ||
        api->get_actor_transform == NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    baseline.struct_size = sizeof(baseline);
    result = api->get_runtime_stats(context, &baseline);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_default_world(context, &world);
    if (result != UEC_RESULT_OK || world == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    const uec_transform transform = {
        {center.x, center.y, center.z}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    result = api->spawn_actor(world, classPath, &transform, &actor);
    if (result != UEC_RESULT_OK || actor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->get_actor_root_component(actor, &component);
    if (result != UEC_RESULT_OK || component == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->set_component_collision_enabled(component, UEC_COLLISION_QUERY_ONLY);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->set_component_collision_channel_response(
        component, UEC_TRACE_VISIBILITY, UEC_COLLISION_RESPONSE_BLOCK);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    uec_collision_enabled enabled = UEC_COLLISION_DISABLED;
    uec_collision_response response = UEC_COLLISION_RESPONSE_IGNORE;
    if (api->get_component_collision_enabled(component, &enabled) != UEC_RESULT_OK ||
        enabled != UEC_COLLISION_QUERY_ONLY ||
        api->get_component_collision_response(component, UEC_TRACE_VISIBILITY,
                                               &response) != UEC_RESULT_OK ||
        response != UEC_COLLISION_RESPONSE_BLOCK) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }

    result = api->line_trace(world, start, end, UEC_TRACE_VISIBILITY, UEC_FALSE, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_TRUE || hit.actor == NULL ||
        !IsAt(api, hit.actor, center)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->release_actor(hit.actor);
    hit.actor = NULL;
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();

    ignored[0] = actor;
    result = api->line_trace_filtered(world, start, end, UEC_TRACE_VISIBILITY, UEC_FALSE,
                                      (const uec_actor* const*)ignored, 1u, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_FALSE || hit.actor != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->sweep_trace_filtered(world, start, end, &sphere, UEC_TRACE_VISIBILITY,
                                       UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_TRUE || hit.actor == NULL ||
        !IsAt(api, hit.actor, center)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->release_actor(hit.actor);
    hit.actor = NULL;
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->sweep_trace_filtered(world, start, end, &sphere, UEC_TRACE_VISIBILITY,
                                       UEC_FALSE, (const uec_actor* const*)ignored, 1u, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_FALSE || hit.actor != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }

    result = api->overlap_shape_filtered(world, center, &sphere, UEC_TRACE_VISIBILITY,
                                         4u, NULL, 0u, overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 1u || overlaps[0] == NULL ||
        !IsAt(api, overlaps[0], center)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->release_actor(overlaps[0]);
    overlaps[0] = NULL;
    overlapCount = 0u;
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->overlap_shape_filtered(world, center, &sphere, UEC_TRACE_VISIBILITY,
                                         4u, (const uec_actor* const*)ignored, 1u,
                                         overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 0u || overlaps[0] != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }

    details.struct_size = sizeof(details);
    result = api->trace_detailed_filtered(world, start, end, &sphere,
        UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &details);
    if (result != UEC_RESULT_OK || details.hit.blocking_hit != UEC_TRUE ||
        details.hit.actor == NULL || details.component == NULL ||
        !IsAt(api, details.hit.actor, center)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->release_actor(details.hit.actor);
    details.hit.actor = NULL;
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->release_scene_component(details.component);
    details.component = NULL;
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    details.struct_size = sizeof(details);
    result = api->trace_detailed_filtered(world, start, end, &sphere,
        UEC_TRACE_VISIBILITY, UEC_FALSE, (const uec_actor* const*)ignored, 1u, &details);
    if (result != UEC_RESULT_OK || details.hit.blocking_hit != UEC_FALSE ||
        details.hit.actor != NULL || details.component != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }

    result = api->set_component_collision_enabled(component,
                                                  UEC_COLLISION_QUERY_AND_PHYSICS);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->set_component_simulating_physics(component, UEC_TRUE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    uec_bool simulating = UEC_FALSE;
    result = api->get_component_simulating_physics(component, &simulating);
    if (result != UEC_RESULT_OK || simulating != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->set_component_simulating_physics(component, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_component_simulating_physics(component, &simulating);
    if (result != UEC_RESULT_OK || simulating != UEC_FALSE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->set_component_collision_enabled(component, UEC_COLLISION_QUERY_ONLY);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = UEC_RESULT_OK;

cleanup:
    if (api != NULL) {
        if (hit.actor != NULL) (void)api->release_actor(hit.actor);
        if (details.hit.actor != NULL) (void)api->release_actor(details.hit.actor);
        if (details.component != NULL) (void)api->release_scene_component(details.component);
        for (uint32_t index = 0u; index < overlapCount; ++index) {
            if (overlaps[index] != NULL) (void)api->release_actor(overlaps[index]);
        }
        if (component != NULL) (void)api->release_scene_component(component);
        if (actor != NULL) {
            const uec_result destroyResult = api->destroy_actor(actor);
            if (destroyResult != UEC_RESULT_OK) (void)api->release_actor(actor);
            else actor = NULL;
        }
        if (world != NULL) (void)api->release_world(world);
        if (result == UEC_RESULT_OK && api->get_runtime_stats != NULL) {
            observed.struct_size = sizeof(observed);
            result = api->get_runtime_stats(context, &observed);
            if (result == UEC_RESULT_OK && !RuntimeStatsMatch(&baseline, &observed)) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }
        if (result != UEC_RESULT_OK && context != NULL && api->log != NULL) {
            char message[128];
            const int length = snprintf(message, sizeof(message),
                "Collision smoke stopped at line %d with result %d", failureLine, (int)result);
            if (length > 0 && (size_t)length < sizeof(message)) {
                const uec_string_view view = {message, (size_t)length};
                (void)api->log(context, view);
            }
        }
        if (context != NULL && api->release_context != NULL) {
            const uec_result releaseResult = api->release_context(context);
            if (result == UEC_RESULT_OK) result = releaseResult;
        }
    }
    return result;
}
