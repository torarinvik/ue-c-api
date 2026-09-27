#include "uec_api.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>

#define UEC_COLLISION_QUERIES_FAIL() \
    do { failureLine = __LINE__; goto cleanup; } while (0)

static int QueryNear(double actual, double expected)
{
    return fabs(actual - expected) <= 1.0;
}

static int QueryVectorNear(uec_vector3 actual, uec_vector3 expected, double tolerance)
{
    return fabs(actual.x - expected.x) <= tolerance &&
        fabs(actual.y - expected.y) <= tolerance &&
        fabs(actual.z - expected.z) <= tolerance;
}

static int QueryActorAt(const uec_api* api, uec_actor* actor, uec_vector3 expected)
{
    uec_transform transform = {0};
    if (api->get_actor_transform(actor, &transform) != UEC_RESULT_OK) return 0;
    return QueryNear(transform.translation.x, expected.x) &&
        QueryNear(transform.translation.y, expected.y) &&
        QueryNear(transform.translation.z, expected.z);
}

static int ResponseOverlapResultsMatch(const uec_api* api,
                                       uec_actor* const* actors,
                                       uint32_t count,
                                       uint32_t expectedCount,
                                       uec_vector3 expectedLocation)
{
    if (count != expectedCount) return 0;
    if (expectedCount == 0u) return actors[0] == NULL;
    return actors[0] != NULL && QueryActorAt(api, actors[0], expectedLocation);
}

static int QueryStatsMatch(const uec_runtime_stats* before,
                           const uec_runtime_stats* after)
{
    return before->active_subscriptions == after->active_subscriptions &&
        before->pending_requests == after->pending_requests &&
        before->active_callbacks == after->active_callbacks &&
        before->live_contexts == after->live_contexts &&
        before->live_worlds == after->live_worlds &&
        before->live_actors == after->live_actors &&
        before->live_components == after->live_components &&
        before->live_classes == after->live_classes &&
        before->live_objects == after->live_objects;
}

static int QueryHitIsClear(const uec_hit_result* hit)
{
    return hit->blocking_hit == UEC_FALSE && hit->actor == NULL &&
        hit->distance == 0.0 && hit->location.x == 0.0 &&
        hit->location.y == 0.0 && hit->location.z == 0.0 &&
        hit->normal.x == 0.0 && hit->normal.y == 0.0 && hit->normal.z == 0.0;
}

static int ResponseHitMatches(const uec_api* api, const uec_hit_result* hit,
                              uec_collision_response response,
                              uec_vector3 expectedLocation)
{
    if (response != UEC_COLLISION_RESPONSE_BLOCK) return QueryHitIsClear(hit);
    return hit->actor != NULL && hit->blocking_hit == UEC_TRUE &&
        QueryActorAt(api, hit->actor, expectedLocation);
}

static int InvalidShapeSweepClears(const uec_api* api, uec_world* world,
                                   uec_vector3 start, uec_vector3 end,
                                   const uec_collision_shape* shape,
                                   uec_actor* sentinel)
{
    uec_hit_result hit = {0};
    hit.blocking_hit = UEC_TRUE;
    hit.actor = sentinel;
    hit.distance = 1.0;
    return api->sweep_trace_filtered(world, start, end, shape,
        UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit) ==
        UEC_RESULT_INVALID_ARGUMENT && QueryHitIsClear(&hit);
}

static void ReleaseQueryHit(const uec_api* api, uec_hit_result* hit)
{
    if (hit->actor != NULL) {
        (void)api->release_actor(hit->actor);
        hit->actor = NULL;
    }
}

static void ReleaseQueryDetails(const uec_api* api,
                                uec_hit_result_details* details)
{
    if (details->hit.actor != NULL) {
        (void)api->release_actor(details->hit.actor);
        details->hit.actor = NULL;
    }
    if (details->component != NULL) {
        (void)api->release_scene_component(details->component);
        details->component = NULL;
    }
}

uec_result UEC_CALL uec_host_collision_queries_smoke(void)
{
    static const char actorClassPath[] =
        "/Script/UnrealCAPIHost.UECAPIHostCollisionSmokeActor";
    const uec_string_view classPath = {actorClassPath, sizeof(actorClassPath) - 1u};
    const uec_vector3 center = {18000.0, -22000.0, 48000.0};
    const uec_vector3 start = {center.x - 250.0, center.y, center.z};
    const uec_vector3 end = {center.x + 250.0, center.y, center.z};
    const uec_vector3 orientedStart = {center.x + 120.0, center.y - 200.0, center.z};
    const uec_vector3 orientedEnd = {center.x + 120.0, center.y + 200.0, center.z};
    const uec_vector3 orientedCenter = {center.x + 120.0, center.y, center.z};
    const uec_vector3 capsuleStart = {center.x - 200.0, center.y + 100.0, center.z};
    const uec_vector3 capsuleEnd = {center.x + 200.0, center.y + 100.0, center.z};
    const uec_vector3 capsuleCenter = {center.x, center.y + 100.0, center.z};
    const uec_transform transform = {
        {center.x, center.y, center.z}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    const uec_collision_shape sphere = {
        sizeof(uec_collision_shape), UEC_COLLISION_SHAPE_SPHERE, 0u,
        20.0, {0.0, 0.0, 0.0}, 0.0, {0.0, 0.0, 0.0, 1.0}};
    const uec_collision_shape legacyBox = {
        (uint32_t)offsetof(uec_collision_shape, rotation), UEC_COLLISION_SHAPE_BOX,
        0u, 0.0, {100.0, 10.0, 10.0}, 0.0, {0.0, 0.0, 0.0, 0.0}};
    const uec_collision_shape rotatedBox = {
        sizeof(uec_collision_shape), UEC_COLLISION_SHAPE_BOX, 0u,
        0.0, {100.0, 10.0, 10.0}, 0.0,
        {0.0, 0.0, 0.7071067811865476, 0.7071067811865476}};
    const uec_collision_shape verticalCapsule = {
        sizeof(uec_collision_shape), UEC_COLLISION_SHAPE_CAPSULE, 0u,
        10.0, {0.0, 0.0, 0.0}, 80.0, {0.0, 0.0, 0.0, 1.0}};
    const uec_collision_shape horizontalCapsule = {
        sizeof(uec_collision_shape), UEC_COLLISION_SHAPE_CAPSULE, 0u,
        10.0, {0.0, 0.0, 0.0}, 80.0,
        {0.7071067811865476, 0.0, 0.0, 0.7071067811865476}};
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* world = NULL;
    uec_actor* actor = NULL;
    uec_actor* secondActor = NULL;
    uec_scene_component* root = NULL;
    uec_actor* ignored[1] = {NULL};
    uec_actor* overlaps[4] = {NULL};
    uec_hit_result hit = {0};
    uec_hit_result_details details = {0};
    uec_runtime_stats baseline = {0};
    uec_runtime_stats observed = {0};
    uec_transform scaledTransform = transform;
    uec_vector3 boundsOrigin = {0};
    uec_vector3 boundsExtent = {0};
    uint32_t overlapCount = 0u;
    int failureLine = 0;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL || api->release_context == NULL ||
        api->get_default_world == NULL || api->release_world == NULL ||
        api->get_runtime_stats == NULL || api->spawn_actor == NULL ||
        api->destroy_actor == NULL || api->release_actor == NULL ||
        api->get_actor_transform == NULL || api->set_actor_transform == NULL ||
        api->get_actor_bounds == NULL || api->get_actor_root_component == NULL ||
        api->release_scene_component == NULL || api->set_component_collision_enabled == NULL ||
        api->get_component_collision_enabled == NULL ||
        api->set_component_collision_channel_response == NULL ||
        api->get_component_collision_response == NULL ||
        api->sweep_trace_filtered == NULL || api->line_trace_filtered == NULL ||
        api->overlap_shape == NULL || api->overlap_shape_filtered == NULL ||
        api->trace_detailed_filtered == NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }

    baseline.struct_size = sizeof(baseline);
    result = api->get_runtime_stats(context, &baseline);
    if (result != UEC_RESULT_OK) UEC_COLLISION_QUERIES_FAIL();
    result = api->get_default_world(context, &world);
    if (result != UEC_RESULT_OK || world == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    result = api->spawn_actor(world, classPath, &transform, &actor);
    if (result != UEC_RESULT_OK || actor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    result = api->get_actor_bounds(actor, &boundsOrigin, &boundsExtent);
    if (result != UEC_RESULT_OK || !QueryNear(boundsOrigin.x, center.x) ||
        !QueryNear(boundsOrigin.y, center.y) || !QueryNear(boundsOrigin.z, center.z) ||
        !QueryNear(boundsExtent.x, 50.0) || !QueryNear(boundsExtent.y, 50.0) ||
        !QueryNear(boundsExtent.z, 50.0)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    scaledTransform.scale = (uec_vector3){2.0, 0.5, 1.5};
    result = api->set_actor_transform(actor, &scaledTransform, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_QUERIES_FAIL();
    result = api->get_actor_bounds(actor, &boundsOrigin, &boundsExtent);
    if (result != UEC_RESULT_OK || !QueryNear(boundsExtent.x, 100.0) ||
        !QueryNear(boundsExtent.y, 25.0) || !QueryNear(boundsExtent.z, 75.0)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    result = api->get_actor_root_component(actor, &root);
    if (result != UEC_RESULT_OK || root == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    result = api->set_component_collision_enabled(root, UEC_COLLISION_QUERY_ONLY);
    if (result != UEC_RESULT_OK) UEC_COLLISION_QUERIES_FAIL();
    result = api->set_component_collision_channel_response(
        root, UEC_TRACE_VISIBILITY, UEC_COLLISION_RESPONSE_BLOCK);
    if (result != UEC_RESULT_OK) UEC_COLLISION_QUERIES_FAIL();

    const uec_collision_enabled collisionModes[] = {
        UEC_COLLISION_DISABLED,
        UEC_COLLISION_QUERY_ONLY,
        UEC_COLLISION_PHYSICS_ONLY,
        UEC_COLLISION_QUERY_AND_PHYSICS};
    const uec_vector3 modeTraceStart = {start.x, center.y + 20.0, center.z};
    const uec_vector3 modeTraceEnd = {end.x, center.y + 20.0, center.z};
    for (size_t index = 0u;
         index < sizeof(collisionModes) / sizeof(collisionModes[0]); ++index) {
        const uec_collision_enabled mode = collisionModes[index];
        const int queryEnabled = mode == UEC_COLLISION_QUERY_ONLY ||
            mode == UEC_COLLISION_QUERY_AND_PHYSICS;
        result = api->set_component_collision_enabled(root, mode);
        if (result != UEC_RESULT_OK) UEC_COLLISION_QUERIES_FAIL();
        uec_collision_enabled observedMode = UEC_COLLISION_DISABLED;
        result = api->get_component_collision_enabled(root, &observedMode);
        if (result != UEC_RESULT_OK || observedMode != mode) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            UEC_COLLISION_QUERIES_FAIL();
        }

        result = api->overlap_shape_filtered(world, center, &sphere,
            UEC_TRACE_VISIBILITY, 4u, NULL, 0u, overlaps, &overlapCount);
        const uint32_t expectedOverlapCount = queryEnabled ? 1u : 0u;
        if (result != UEC_RESULT_OK || overlapCount != expectedOverlapCount ||
            (queryEnabled && (overlaps[0] == NULL ||
                              !QueryActorAt(api, overlaps[0], center))) ||
            (!queryEnabled && overlaps[0] != NULL)) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            UEC_COLLISION_QUERIES_FAIL();
        }
        if (overlaps[0] != NULL) {
            (void)api->release_actor(overlaps[0]);
            overlaps[0] = NULL;
        }
        overlapCount = 0u;

        result = api->line_trace_filtered(world, modeTraceStart, modeTraceEnd,
            UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
        if (result != UEC_RESULT_OK ||
            (queryEnabled && (hit.blocking_hit != UEC_TRUE || hit.actor == NULL ||
                              !QueryActorAt(api, hit.actor, center))) ||
            (!queryEnabled && !QueryHitIsClear(&hit))) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            UEC_COLLISION_QUERIES_FAIL();
        }
        ReleaseQueryHit(api, &hit);
    }
    result = api->set_component_collision_enabled(root, UEC_COLLISION_QUERY_ONLY);
    if (result != UEC_RESULT_OK) UEC_COLLISION_QUERIES_FAIL();

    const uec_vector3 scaledMissStart = {start.x, center.y + 40.0, center.z};
    const uec_vector3 scaledMissEnd = {end.x, center.y + 40.0, center.z};
    result = api->line_trace_filtered(world, scaledMissStart, scaledMissEnd,
        UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || !QueryHitIsClear(&hit)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    const uec_vector3 scaledHitStart = {start.x, center.y + 20.0, center.z};
    const uec_vector3 scaledHitEnd = {end.x, center.y + 20.0, center.z};
    result = api->line_trace_filtered(world, scaledHitStart, scaledHitEnd,
        UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_TRUE || hit.actor == NULL ||
        !QueryActorAt(api, hit.actor, center) || !QueryNear(hit.distance, 150.0) ||
        !QueryVectorNear(hit.location, (uec_vector3){center.x - 100.0,
            center.y + 20.0, center.z}, 1.0) ||
        !QueryVectorNear(hit.normal, (uec_vector3){-1.0, 0.0, 0.0}, 0.05)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    ReleaseQueryHit(api, &hit);

    const uec_vector3 scaledOrientedStart = {center.x + 160.0, center.y - 200.0, center.z};
    const uec_vector3 scaledOrientedEnd = {center.x + 160.0, center.y + 200.0, center.z};
    result = api->sweep_trace_filtered(world, scaledOrientedStart, scaledOrientedEnd,
        &legacyBox, UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_TRUE || hit.actor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    ReleaseQueryHit(api, &hit);
    result = api->sweep_trace_filtered(world, scaledOrientedStart, scaledOrientedEnd,
        &rotatedBox, UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || !QueryHitIsClear(&hit)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    result = api->overlap_shape_filtered(world,
        (uec_vector3){center.x + 115.0, center.y, center.z}, &sphere,
        UEC_TRACE_VISIBILITY, 4u, NULL, 0u, overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 1u || overlaps[0] == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    (void)api->release_actor(overlaps[0]);
    overlaps[0] = NULL;
    overlapCount = 0u;
    result = api->overlap_shape_filtered(world,
        (uec_vector3){center.x + 125.0, center.y, center.z}, &sphere,
        UEC_TRACE_VISIBILITY, 4u, NULL, 0u, overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 0u || overlaps[0] != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    result = api->set_actor_transform(actor, &transform, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_QUERIES_FAIL();

    ignored[0] = actor;
    result = api->line_trace_filtered(world, start, end, UEC_TRACE_VISIBILITY,
        UEC_FALSE, (const uec_actor* const*)ignored, 1u, &hit);
    if (result != UEC_RESULT_OK || !QueryHitIsClear(&hit)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    result = api->sweep_trace_filtered(world, start, end, &sphere,
        UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_TRUE || hit.actor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    ReleaseQueryHit(api, &hit);
    result = api->sweep_trace_filtered(world, start, end, &sphere,
        UEC_TRACE_VISIBILITY, UEC_FALSE, (const uec_actor* const*)ignored, 1u, &hit);
    if (result != UEC_RESULT_OK || !QueryHitIsClear(&hit)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    result = api->sweep_trace_filtered(world, orientedStart, orientedEnd,
        &legacyBox, UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_TRUE || hit.actor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    ReleaseQueryHit(api, &hit);
    result = api->sweep_trace_filtered(world, orientedStart, orientedEnd,
        &rotatedBox, UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || !QueryHitIsClear(&hit)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    result = api->overlap_shape_filtered(world, orientedCenter, &legacyBox,
        UEC_TRACE_VISIBILITY, 4u, NULL, 0u, overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 1u || overlaps[0] == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    (void)api->release_actor(overlaps[0]);
    overlaps[0] = NULL;
    overlapCount = 0u;
    result = api->overlap_shape_filtered(world, orientedCenter, &rotatedBox,
        UEC_TRACE_VISIBILITY, 4u, NULL, 0u, overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 0u || overlaps[0] != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }

    details.struct_size = sizeof(details);
    result = api->trace_detailed_filtered(world, start, end, &sphere,
        UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &details);
    if (result != UEC_RESULT_OK || details.hit.blocking_hit != UEC_TRUE ||
        details.hit.actor == NULL || details.component == NULL ||
        !QueryActorAt(api, details.hit.actor, center) ||
        !QueryNear(details.hit.distance, 180.0) ||
        !QueryVectorNear(details.hit.location,
            (uec_vector3){center.x - 70.0, center.y, center.z}, 1.0) ||
        !QueryVectorNear(details.hit.normal, (uec_vector3){-1.0, 0.0, 0.0}, 0.05) ||
        !QueryVectorNear(details.impact_point,
            (uec_vector3){center.x - 50.0, center.y, center.z}, 1.0) ||
        !QueryVectorNear(details.impact_normal,
            (uec_vector3){-1.0, 0.0, 0.0}, 0.05) ||
        !QueryVectorNear(details.trace_start, start, 1.0) ||
        !QueryVectorNear(details.trace_end, end, 1.0)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    ReleaseQueryDetails(api, &details);
    details.struct_size = sizeof(details);
    result = api->trace_detailed_filtered(world, start, end, &sphere,
        UEC_TRACE_VISIBILITY, UEC_FALSE, (const uec_actor* const*)ignored, 1u, &details);
    if (result != UEC_RESULT_OK || !QueryHitIsClear(&details.hit) ||
        details.component != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    details.struct_size = sizeof(details);
    result = api->trace_detailed_filtered(world, orientedStart, orientedEnd,
        &rotatedBox, UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &details);
    if (result != UEC_RESULT_OK || !QueryHitIsClear(&details.hit) ||
        details.component != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }

    uec_collision_shape invalidShape = sphere;
    invalidShape.radius = 0.0;
    if (!InvalidShapeSweepClears(api, world, start, end, &invalidShape, actor)) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    invalidShape = legacyBox;
    invalidShape.struct_size = (uint32_t)offsetof(uec_collision_shape, rotation) - 1u;
    if (!InvalidShapeSweepClears(api, world, start, end, &invalidShape, actor)) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    invalidShape = sphere;
    invalidShape.kind = (uec_collision_shape_kind)99;
    if (!InvalidShapeSweepClears(api, world, start, end, &invalidShape, actor)) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    invalidShape = rotatedBox;
    invalidShape.rotation = (uec_quaternion){0.0, 0.0, 0.0, 0.0};
    if (!InvalidShapeSweepClears(api, world, orientedStart, orientedEnd,
                                 &invalidShape, actor)) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    overlapCount = 17u;
    overlaps[0] = actor;
    result = api->overlap_shape_filtered(world, center, &sphere,
        UEC_TRACE_VISIBILITY, UEC_MAX_COLLISION_QUERY_ACTORS + 1u,
        NULL, 0u, overlaps, &overlapCount);
    if (result != UEC_RESULT_INVALID_ARGUMENT || overlapCount != 0u ||
        overlaps[0] != actor) {
        overlaps[0] = NULL;
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    overlaps[0] = NULL;

    result = api->sweep_trace_filtered(world, capsuleStart, capsuleEnd,
        &verticalCapsule, UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || !QueryHitIsClear(&hit)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    result = api->sweep_trace_filtered(world, capsuleStart, capsuleEnd,
        &horizontalCapsule, UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_TRUE || hit.actor == NULL ||
        !QueryActorAt(api, hit.actor, center)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    ReleaseQueryHit(api, &hit);
    result = api->sweep_trace_filtered(world, capsuleStart, capsuleEnd,
        &horizontalCapsule, UEC_TRACE_VISIBILITY, UEC_FALSE,
        (const uec_actor* const*)ignored, 1u, &hit);
    if (result != UEC_RESULT_OK || !QueryHitIsClear(&hit)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    result = api->overlap_shape_filtered(world, capsuleCenter, &verticalCapsule,
        UEC_TRACE_VISIBILITY, 4u, NULL, 0u, overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 0u || overlaps[0] != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    result = api->overlap_shape_filtered(world, capsuleCenter, &horizontalCapsule,
        UEC_TRACE_VISIBILITY, 4u, NULL, 0u, overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 1u || overlaps[0] == NULL ||
        !QueryActorAt(api, overlaps[0], center)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    (void)api->release_actor(overlaps[0]);
    overlaps[0] = NULL;
    overlapCount = 0u;
    result = api->overlap_shape_filtered(world, capsuleCenter, &horizontalCapsule,
        UEC_TRACE_VISIBILITY, 4u, (const uec_actor* const*)ignored,
        1u, overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 0u || overlaps[0] != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    details.struct_size = sizeof(details);
    result = api->trace_detailed_filtered(world, capsuleStart, capsuleEnd,
        &horizontalCapsule, UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &details);
    if (result != UEC_RESULT_OK || details.hit.blocking_hit != UEC_TRUE ||
        details.hit.actor == NULL || details.component == NULL ||
        !QueryActorAt(api, details.hit.actor, center)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    ReleaseQueryDetails(api, &details);
    uec_collision_shape invalidCapsule = verticalCapsule;
    invalidCapsule.half_height = invalidCapsule.radius - 1.0;
    if (!InvalidShapeSweepClears(api, world, capsuleStart, capsuleEnd,
                                 &invalidCapsule, actor)) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    uec_collision_shape normalizedCapsule = horizontalCapsule;
    normalizedCapsule.rotation.x *= 2.0;
    normalizedCapsule.rotation.w *= 2.0;
    result = api->sweep_trace_filtered(world, capsuleStart, capsuleEnd,
        &normalizedCapsule, UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_TRUE || hit.actor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    ReleaseQueryHit(api, &hit);

    const uec_vector3 secondCenter = {center.x + 200.0, center.y, center.z};
    const uec_transform secondTransform = {
        {secondCenter.x, secondCenter.y, secondCenter.z},
        {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    result = api->spawn_actor(world, classPath, &secondTransform, &secondActor);
    if (result != UEC_RESULT_OK || secondActor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    const uec_collision_shape pairOverlap = {
        sizeof(uec_collision_shape), UEC_COLLISION_SHAPE_SPHERE, 0u,
        160.0, {0.0, 0.0, 0.0}, 0.0, {0.0, 0.0, 0.0, 1.0}};
    const uec_vector3 betweenActors = {center.x + 100.0, center.y, center.z};
    result = api->overlap_shape_filtered(world, betweenActors, &pairOverlap,
        UEC_TRACE_VISIBILITY, 4u, NULL, 0u, overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 2u || overlaps[0] == NULL ||
        overlaps[1] == NULL || overlaps[0] == overlaps[1]) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    int foundFirst = 0;
    int foundSecond = 0;
    for (uint32_t index = 0u; index < overlapCount; ++index) {
        if (QueryActorAt(api, overlaps[index], center)) foundFirst = 1;
        if (QueryActorAt(api, overlaps[index], secondCenter)) foundSecond = 1;
    }
    if (!foundFirst || !foundSecond) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    for (uint32_t index = 0u; index < overlapCount; ++index) {
        (void)api->release_actor(overlaps[index]);
        overlaps[index] = NULL;
    }
    overlapCount = 0u;
    ignored[0] = actor;
    result = api->overlap_shape_filtered(world, betweenActors, &pairOverlap,
        UEC_TRACE_VISIBILITY, 4u, (const uec_actor* const*)ignored,
        1u, overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 1u || overlaps[0] == NULL ||
        !QueryActorAt(api, overlaps[0], secondCenter)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_QUERIES_FAIL();
    }
    (void)api->release_actor(overlaps[0]);
    overlaps[0] = NULL;
    overlapCount = 0u;

    const uec_collision_response collisionResponses[] = {
        UEC_COLLISION_RESPONSE_IGNORE,
        UEC_COLLISION_RESPONSE_OVERLAP,
        UEC_COLLISION_RESPONSE_BLOCK};
    for (size_t index = 0u;
         index < sizeof(collisionResponses) / sizeof(collisionResponses[0]); ++index) {
        uec_collision_response response = (uec_collision_response)99;
        result = api->set_component_collision_channel_response(
            root, UEC_TRACE_VISIBILITY, collisionResponses[index]);
        if (result != UEC_RESULT_OK) UEC_COLLISION_QUERIES_FAIL();
        result = api->get_component_collision_response(
            root, UEC_TRACE_VISIBILITY, &response);
        if (result != UEC_RESULT_OK || response != collisionResponses[index]) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            UEC_COLLISION_QUERIES_FAIL();
        }
        const uec_vector3 responseTraceStart =
            {center.x - 100.0, center.y, center.z};
        const uec_vector3 responseTraceEnd =
            {center.x + 100.0, center.y, center.z};
        result = api->line_trace_filtered(world, responseTraceStart,
            responseTraceEnd, UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
        if (result != UEC_RESULT_OK || !ResponseHitMatches(
                api, &hit, collisionResponses[index], center)) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            ReleaseQueryHit(api, &hit);
            UEC_COLLISION_QUERIES_FAIL();
        }
        ReleaseQueryHit(api, &hit);
        result = api->sweep_trace_filtered(world, responseTraceStart,
            responseTraceEnd, &sphere, UEC_TRACE_VISIBILITY, UEC_FALSE,
            NULL, 0u, &hit);
        if (result != UEC_RESULT_OK || !ResponseHitMatches(
                api, &hit, collisionResponses[index], center)) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            ReleaseQueryHit(api, &hit);
            UEC_COLLISION_QUERIES_FAIL();
        }
        ReleaseQueryHit(api, &hit);
        const uint32_t expectedOverlapCount =
            collisionResponses[index] == UEC_COLLISION_RESPONSE_IGNORE ? 0u : 1u;
        for (size_t queryIndex = 0u; queryIndex < 2u; ++queryIndex) {
            const char* queryName = queryIndex == 0u ? "filtered" : "unfiltered";
            if (queryIndex == 0u) {
                result = api->overlap_shape_filtered(world, center, &sphere,
                    UEC_TRACE_VISIBILITY, 4u, NULL, 0u, overlaps, &overlapCount);
            } else {
                result = api->overlap_shape(world, center, &sphere,
                    UEC_TRACE_VISIBILITY, 4u, overlaps, &overlapCount);
            }
            if (result != UEC_RESULT_OK || !ResponseOverlapResultsMatch(
                    api, overlaps, overlapCount, expectedOverlapCount, center)) {
                if (context != NULL && api->log != NULL) {
                    char message[176];
                    const int length = snprintf(message, sizeof(message),
                        "Collision response %s overlap mismatch: index=%u response=%d result=%d count=%u expected=%u",
                        queryName, (unsigned int)index, (int)collisionResponses[index],
                        (int)result, (unsigned int)overlapCount,
                        (unsigned int)expectedOverlapCount);
                    if (length > 0 && (size_t)length < sizeof(message)) {
                        const uec_string_view view = {message, (size_t)length};
                        (void)api->log(context, view);
                    }
                }
                if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
                UEC_COLLISION_QUERIES_FAIL();
            }
            if (overlaps[0] != NULL) {
                (void)api->release_actor(overlaps[0]);
                overlaps[0] = NULL;
            }
            overlapCount = 0u;
        }
    }
    result = UEC_RESULT_OK;

cleanup:
    if (api != NULL) {
        ReleaseQueryHit(api, &hit);
        ReleaseQueryDetails(api, &details);
        for (uint32_t index = 0u; index < overlapCount && index < 4u; ++index) {
            if (overlaps[index] != NULL) (void)api->release_actor(overlaps[index]);
        }
        if (root != NULL) (void)api->release_scene_component(root);
        if (secondActor != NULL) {
            const uec_result destroyResult = api->destroy_actor(secondActor);
            if (destroyResult != UEC_RESULT_OK) (void)api->release_actor(secondActor);
            else secondActor = NULL;
        }
        if (actor != NULL) {
            const uec_result destroyResult = api->destroy_actor(actor);
            if (destroyResult != UEC_RESULT_OK) (void)api->release_actor(actor);
            else actor = NULL;
        }
        if (world != NULL) (void)api->release_world(world);
        if (context != NULL && api->get_runtime_stats != NULL) {
            observed.struct_size = sizeof(observed);
            const uec_result statsResult = api->get_runtime_stats(context, &observed);
            if (result == UEC_RESULT_OK && statsResult != UEC_RESULT_OK)
                result = statsResult;
            if (result == UEC_RESULT_OK && !QueryStatsMatch(&baseline, &observed))
                result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result != UEC_RESULT_OK && context != NULL && api->log != NULL) {
            char message[128];
            const int length = snprintf(message, sizeof(message),
                "Collision query smoke stopped at line %d with result %d",
                failureLine, (int)result);
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
