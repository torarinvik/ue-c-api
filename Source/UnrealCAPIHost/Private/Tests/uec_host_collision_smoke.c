#include "uec_api.h"

#include <float.h>
#include <math.h>
#include <stdio.h>

#define UEC_COLLISION_SMOKE_FAIL() do { failureLine = __LINE__; goto cleanup; } while (0)

static int IsNear(double actual, double expected)
{
    const double difference = actual > expected ? actual - expected : expected - actual;
    return difference <= 1.0;
}

static int IsMassNear(double actual, double expected)
{
    const double difference = actual > expected ? actual - expected : expected - actual;
    return difference <= 0.01;
}

static int IsPhysicsVectorNear(uec_vector3 actual, uec_vector3 expected)
{
    const double tolerance = 0.05;
    return actual.x >= expected.x - tolerance && actual.x <= expected.x + tolerance &&
        actual.y >= expected.y - tolerance && actual.y <= expected.y + tolerance &&
        actual.z >= expected.z - tolerance && actual.z <= expected.z + tolerance;
}

static int IsTransformNear(uec_transform actual, uec_transform expected)
{
    const double tolerance = 0.05;
    return IsPhysicsVectorNear(actual.translation, expected.translation) &&
        actual.rotation.x >= expected.rotation.x - tolerance &&
        actual.rotation.x <= expected.rotation.x + tolerance &&
        actual.rotation.y >= expected.rotation.y - tolerance &&
        actual.rotation.y <= expected.rotation.y + tolerance &&
        actual.rotation.z >= expected.rotation.z - tolerance &&
        actual.rotation.z <= expected.rotation.z + tolerance &&
        actual.rotation.w >= expected.rotation.w - tolerance &&
        actual.rotation.w <= expected.rotation.w + tolerance &&
        actual.scale.x >= expected.scale.x - tolerance &&
        actual.scale.x <= expected.scale.x + tolerance &&
        actual.scale.y >= expected.scale.y - tolerance &&
        actual.scale.y <= expected.scale.y + tolerance &&
        actual.scale.z >= expected.scale.z - tolerance &&
        actual.scale.z <= expected.scale.z + tolerance;
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

typedef struct uec_hit_smoke_capture {
    const uec_api* api;
    uint64_t expected_subscription_id;
    uint32_t callback_count;
    uec_bool subscription_id_matched;
    uec_actor* other_actor;
} uec_hit_smoke_capture;

static void UEC_CALL CaptureComponentHit(uint64_t subscriptionId,
                                         uec_actor* otherActor,
                                         uec_vector3 normalImpulse,
                                         void* userData)
{
    (void)normalImpulse;
    uec_hit_smoke_capture* capture = (uec_hit_smoke_capture*)userData;
    if (capture == NULL) return;
    ++capture->callback_count;
    capture->subscription_id_matched = subscriptionId == capture->expected_subscription_id
        ? UEC_TRUE : UEC_FALSE;
    if (capture->callback_count == 1u) {
        capture->other_actor = otherActor;
    } else if (otherActor != NULL && capture->api != NULL) {
        (void)capture->api->release_actor(otherActor);
    }
}

uec_result UEC_CALL uec_host_collision_smoke(void)
{
    static const char actorClassPath[] =
        "/Script/UnrealCAPIHost.UECAPIHostCollisionSmokeActor";
    static const char staticMeshComponentClassPath[] =
        "/Script/Engine.StaticMeshComponent";
    static const char attachmentSocketText[] = "UECAPIHostAttachmentSocket";
    const uec_string_view classPath = {actorClassPath, sizeof(actorClassPath) - 1u};
    const uec_string_view staticMeshClassPath = {
        staticMeshComponentClassPath, sizeof(staticMeshComponentClassPath) - 1u};
    const uec_string_view attachmentSocket = {
        attachmentSocketText, sizeof(attachmentSocketText) - 1u};
    const uec_vector3 center = {12000.0, -24000.0, 50000.0};
    const uec_vector3 start = {center.x - 250.0, center.y, center.z};
    const uec_vector3 end = {center.x + 250.0, center.y, center.z};
    const uec_vector3 orientedStart = {center.x + 120.0, center.y - 200.0, center.z};
    const uec_vector3 orientedEnd = {center.x + 120.0, center.y + 200.0, center.z};
    const uec_vector3 orientedCenter = {center.x + 120.0, center.y, center.z};
    const uec_transform movingTransform = {
        {center.x - 300.0, center.y, center.z}, {0.0, 0.0, 0.0, 1.0},
        {1.0, 1.0, 1.0}};
    const uec_vector3 sweptMoveDelta = {600.0, 0.0, 0.0};
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
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* world = NULL;
    uec_actor* actor = NULL;
    uec_actor* movingActor = NULL;
    uec_scene_component* component = NULL;
    uec_scene_component* movingComponent = NULL;
    uec_scene_component* socketComponent = NULL;
    uec_hit_result hit = {0};
    uec_hit_result_details details = {0};
    uec_actor* overlaps[4] = {0};
    uec_actor* ignored[1] = {0};
    uec_runtime_stats baseline = {0};
    uec_runtime_stats observed = {0};
    uec_hit_smoke_capture hitCapture = {0};
    uec_vector3 boundsOrigin = {0};
    uec_vector3 boundsExtent = {0};
    uec_vector3 appliedDelta = {0};
    double massBeforeOverride = 0.0;
    double massReadback = 0.0;
    uec_transform scaledTransform = {0};
    uec_transform scaleReadback = {0};
    uec_transform socketParentTransform = {0};
    uec_transform childTransformBeforeSocketAttach = {0};
    uec_transform childTransformAfterSocketAttach = {0};
    uec_transform expectedSocketTransform = {0};
    uec_transform childTransformAfterSocketDetach = {0};
    uint32_t overlapCount = 0u;
    uint32_t socketComponentCount = 0u;
    uint64_t hitSubscriptionId = 0u;
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
        api->bind_component_hit == NULL || api->unbind_component_hit == NULL ||
        api->set_component_simulating_physics == NULL ||
        api->get_component_simulating_physics == NULL ||
        api->get_component_mass == NULL || api->set_component_mass_override == NULL ||
        api->get_component_velocity == NULL || api->get_actor_velocity == NULL ||
        api->set_component_physics_velocity == NULL || api->apply_component_impulse == NULL ||
        api->apply_component_force == NULL ||
        api->get_component_physics_angular_velocity == NULL ||
        api->set_component_physics_angular_velocity == NULL ||
        api->apply_component_torque == NULL || api->apply_component_angular_impulse == NULL ||
        api->set_actor_physics_velocity == NULL ||
        api->set_component_collision_channel_response == NULL ||
        api->get_component_collision_response == NULL || api->line_trace == NULL ||
        api->line_trace_filtered == NULL || api->sweep_trace_filtered == NULL ||
        api->overlap_shape_filtered == NULL || api->trace_detailed_filtered == NULL ||
        api->move_actor_swept == NULL ||
        api->get_actor_transform == NULL || api->get_actor_bounds == NULL ||
        api->get_actor_component_count_by_class == NULL ||
        api->get_actor_component_at_by_class == NULL ||
        api->attach_scene_component == NULL || api->detach_scene_component == NULL ||
        api->get_component_transform == NULL) {
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
    result = api->get_actor_bounds(actor, &boundsOrigin, &boundsExtent);
    if (result != UEC_RESULT_OK ||
        !IsNear(boundsOrigin.x, center.x) || !IsNear(boundsOrigin.y, center.y) ||
        !IsNear(boundsOrigin.z, center.z) || !IsNear(boundsExtent.x, 50.0) ||
        !IsNear(boundsExtent.y, 50.0) || !IsNear(boundsExtent.z, 50.0)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    scaledTransform = transform;
    scaledTransform.scale = (uec_vector3){2.0, 0.5, 1.5};
    result = api->set_actor_transform(actor, &scaledTransform, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_actor_transform(actor, &scaleReadback);
    if (result != UEC_RESULT_OK || !IsTransformNear(scaleReadback, scaledTransform)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->get_actor_bounds(actor, &boundsOrigin, &boundsExtent);
    if (result != UEC_RESULT_OK ||
        !IsNear(boundsOrigin.x, center.x) || !IsNear(boundsOrigin.y, center.y) ||
        !IsNear(boundsOrigin.z, center.z) || !IsNear(boundsExtent.x, 100.0) ||
        !IsNear(boundsExtent.y, 25.0) || !IsNear(boundsExtent.z, 75.0)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->get_actor_root_component(actor, &component);
    if (result != UEC_RESULT_OK || component == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->get_component_transform(component, &scaleReadback);
    if (result != UEC_RESULT_OK || !IsTransformNear(scaleReadback, scaledTransform)) {
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

    const uec_vector3 scaledMissStart = {start.x, center.y + 40.0, center.z};
    const uec_vector3 scaledMissEnd = {end.x, center.y + 40.0, center.z};
    result = api->line_trace(world, scaledMissStart, scaledMissEnd,
                             UEC_TRACE_VISIBILITY, UEC_FALSE, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_FALSE || hit.actor != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    const uec_vector3 scaledHitStart = {start.x, center.y + 20.0, center.z};
    const uec_vector3 scaledHitEnd = {end.x, center.y + 20.0, center.z};
    result = api->line_trace(world, scaledHitStart, scaledHitEnd,
                             UEC_TRACE_VISIBILITY, UEC_FALSE, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_TRUE || hit.actor == NULL ||
        !IsAt(api, hit.actor, center)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->release_actor(hit.actor);
    hit.actor = NULL;
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->set_actor_transform(actor, &transform, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_component_transform(component, &scaleReadback);
    if (result != UEC_RESULT_OK || !IsTransformNear(scaleReadback, transform)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
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

    result = api->sweep_trace_filtered(world, orientedStart, orientedEnd, &legacyBox,
        UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_TRUE || hit.actor == NULL ||
        !IsAt(api, hit.actor, center)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->release_actor(hit.actor);
    hit.actor = NULL;
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->sweep_trace_filtered(world, orientedStart, orientedEnd, &rotatedBox,
        UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_OK || hit.blocking_hit != UEC_FALSE || hit.actor != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }

    result = api->overlap_shape_filtered(world, orientedCenter, &legacyBox,
        UEC_TRACE_VISIBILITY, 4u, NULL, 0u, overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 1u || overlaps[0] == NULL ||
        !IsAt(api, overlaps[0], center)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->release_actor(overlaps[0]);
    overlaps[0] = NULL;
    overlapCount = 0u;
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->overlap_shape_filtered(world, orientedCenter, &rotatedBox,
        UEC_TRACE_VISIBILITY, 4u, NULL, 0u, overlaps, &overlapCount);
    if (result != UEC_RESULT_OK || overlapCount != 0u || overlaps[0] != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }

    uec_collision_shape invalidRotation = rotatedBox;
    invalidRotation.rotation = (uec_quaternion){0.0, 0.0, 0.0, 0.0};
    hit.blocking_hit = UEC_TRUE;
    hit.distance = 1.0;
    result = api->sweep_trace_filtered(world, orientedStart, orientedEnd,
        &invalidRotation, UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &hit);
    if (result != UEC_RESULT_INVALID_ARGUMENT || hit.blocking_hit != UEC_FALSE ||
        hit.distance != 0.0 || hit.actor != NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
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
    details.struct_size = sizeof(details);
    result = api->trace_detailed_filtered(world, orientedStart, orientedEnd, &rotatedBox,
        UEC_TRACE_VISIBILITY, UEC_FALSE, NULL, 0u, &details);
    if (result != UEC_RESULT_OK || details.hit.blocking_hit != UEC_FALSE ||
        details.hit.actor != NULL || details.component != NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }

    result = api->set_component_collision_enabled(component,
                                                  UEC_COLLISION_QUERY_AND_PHYSICS);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    massReadback = -1.0;
    result = api->get_component_mass(component, &massReadback);
    if (result != UEC_RESULT_OK || massReadback != 0.0) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->set_component_simulating_physics(component, UEC_TRUE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    uec_bool simulating = UEC_FALSE;
    result = api->get_component_simulating_physics(component, &simulating);
    if (result != UEC_RESULT_OK || simulating != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->get_component_mass(component, &massBeforeOverride);
    if (result != UEC_RESULT_OK || massBeforeOverride <= 0.0 ||
        !isfinite(massBeforeOverride)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->set_component_mass_override(component, 13.75, UEC_TRUE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_component_mass(component, &massReadback);
    if (result != UEC_RESULT_OK || !IsMassNear(massReadback, 13.75)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    if (api->set_component_mass_override(component, 0.0, UEC_TRUE) !=
            UEC_RESULT_INVALID_ARGUMENT ||
        api->set_component_mass_override(component, NAN, UEC_TRUE) !=
            UEC_RESULT_INVALID_ARGUMENT ||
        api->set_component_mass_override(component, INFINITY, UEC_TRUE) !=
            UEC_RESULT_INVALID_ARGUMENT ||
        api->set_component_mass_override(component, (double)FLT_MAX * 2.0, UEC_TRUE) !=
            UEC_RESULT_INVALID_ARGUMENT ||
        api->set_component_mass_override(component, 13.75, (uec_bool)2) !=
            UEC_RESULT_INVALID_ARGUMENT) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    massReadback = -1.0;
    if (api->get_component_mass(NULL, &massReadback) != UEC_RESULT_INVALID_HANDLE ||
        massReadback != 0.0) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->get_component_mass(component, &massReadback);
    if (result != UEC_RESULT_OK || !IsMassNear(massReadback, 13.75)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->set_component_mass_override(
        component, massBeforeOverride, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_component_mass(component, &massReadback);
    if (result != UEC_RESULT_OK || !IsMassNear(massReadback, massBeforeOverride)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    const uec_vector3 linearVelocity = {12.0, -6.0, 3.0};
    const uec_vector3 linearDelta = {4.0, 2.0, -1.0};
    const uec_vector3 angularVelocity = {0.25, -0.5, 0.75};
    const uec_vector3 angularDelta = {0.125, 0.25, -0.125};
    uec_vector3 physicsReadback = {99.0, 99.0, 99.0};
    result = api->set_component_physics_velocity(component, linearVelocity, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_component_velocity(component, &physicsReadback);
    if (result != UEC_RESULT_OK || !IsPhysicsVectorNear(physicsReadback, linearVelocity)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    const uec_vector3 addedLinearVelocity = {
        linearVelocity.x + linearDelta.x,
        linearVelocity.y + linearDelta.y,
        linearVelocity.z + linearDelta.z};
    result = api->set_component_physics_velocity(component, linearDelta, UEC_TRUE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_actor_velocity(actor, &physicsReadback);
    if (result != UEC_RESULT_OK || !IsPhysicsVectorNear(physicsReadback, addedLinearVelocity)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->set_actor_physics_velocity(actor, linearVelocity, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_component_velocity(component, &physicsReadback);
    if (result != UEC_RESULT_OK || !IsPhysicsVectorNear(physicsReadback, linearVelocity)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->set_component_physics_angular_velocity(component, angularVelocity, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_component_physics_angular_velocity(component, &physicsReadback);
    if (result != UEC_RESULT_OK || !IsPhysicsVectorNear(physicsReadback, angularVelocity)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    const uec_vector3 addedAngularVelocity = {
        angularVelocity.x + angularDelta.x,
        angularVelocity.y + angularDelta.y,
        angularVelocity.z + angularDelta.z};
    result = api->set_component_physics_angular_velocity(component, angularDelta, UEC_TRUE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_component_physics_angular_velocity(component, &physicsReadback);
    if (result != UEC_RESULT_OK || !IsPhysicsVectorNear(physicsReadback, addedAngularVelocity)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->set_component_physics_velocity(component, (uec_vector3){0}, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    const uec_vector3 impulse = {2.0, -3.0, 4.0};
    result = api->apply_component_impulse(component, impulse, UEC_TRUE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    const uec_vector3 angularImpulse = {0.5, -0.25, 0.125};
    result = api->set_component_physics_angular_velocity(component, (uec_vector3){0}, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->apply_component_angular_impulse(component, angularImpulse, UEC_TRUE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->apply_component_force(component, (uec_vector3){100000.0, 0.0, 0.0});
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->apply_component_torque(component, (uec_vector3){0.0, 100000.0, 0.0},
                                         UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->set_component_simulating_physics(component, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_component_simulating_physics(component, &simulating);
    if (result != UEC_RESULT_OK || simulating != UEC_FALSE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    physicsReadback = (uec_vector3){99.0, 99.0, 99.0};
    if (api->set_component_physics_velocity(component, linearVelocity, UEC_FALSE) !=
            UEC_RESULT_UNSUPPORTED ||
        api->apply_component_impulse(component, impulse, UEC_TRUE) != UEC_RESULT_UNSUPPORTED ||
        api->apply_component_force(component, linearVelocity) != UEC_RESULT_UNSUPPORTED ||
        api->get_component_physics_angular_velocity(component, &physicsReadback) !=
            UEC_RESULT_UNSUPPORTED || !IsPhysicsVectorNear(physicsReadback, (uec_vector3){0}) ||
        api->set_component_physics_angular_velocity(component, angularVelocity, UEC_FALSE) !=
            UEC_RESULT_UNSUPPORTED ||
        api->apply_component_torque(component, angularVelocity, UEC_FALSE) !=
            UEC_RESULT_UNSUPPORTED ||
        api->apply_component_angular_impulse(component, angularImpulse, UEC_TRUE) !=
            UEC_RESULT_UNSUPPORTED) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->set_component_collision_enabled(component, UEC_COLLISION_QUERY_ONLY);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();

    result = api->spawn_actor(world, classPath, &movingTransform, &movingActor);
    if (result != UEC_RESULT_OK || movingActor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->get_actor_root_component(movingActor, &movingComponent);
    if (result != UEC_RESULT_OK || movingComponent == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->get_actor_component_count_by_class(
        actor, staticMeshClassPath, &socketComponentCount);
    if (result != UEC_RESULT_OK || socketComponentCount != 1u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->get_actor_component_at_by_class(
        actor, staticMeshClassPath, 0u, &socketComponent);
    if (result != UEC_RESULT_OK || socketComponent == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->get_component_transform(socketComponent, &socketParentTransform);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_component_transform(movingComponent,
                                          &childTransformBeforeSocketAttach);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->attach_scene_component(movingComponent, socketComponent,
                                         UEC_FALSE, attachmentSocket);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_component_transform(movingComponent,
                                          &childTransformAfterSocketAttach);
    expectedSocketTransform = childTransformBeforeSocketAttach;
    expectedSocketTransform.translation.x += socketParentTransform.translation.x + 20.0;
    expectedSocketTransform.translation.y += socketParentTransform.translation.y;
    expectedSocketTransform.translation.z += socketParentTransform.translation.z;
    if (result != UEC_RESULT_OK ||
        !IsTransformNear(childTransformAfterSocketAttach, expectedSocketTransform)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->detach_scene_component(movingComponent, UEC_FALSE);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->get_component_transform(movingComponent,
                                          &childTransformAfterSocketDetach);
    if (result != UEC_RESULT_OK ||
        !IsTransformNear(childTransformAfterSocketDetach,
                         childTransformBeforeSocketAttach)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->set_component_collision_channel_response(
        component, UEC_TRACE_WORLD_DYNAMIC, UEC_COLLISION_RESPONSE_BLOCK);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->set_component_collision_channel_response(
        movingComponent, UEC_TRACE_WORLD_DYNAMIC, UEC_COLLISION_RESPONSE_BLOCK);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    hitCapture.api = api;
    result = api->bind_component_hit(movingComponent, &CaptureComponentHit,
                                      &hitCapture, &hitSubscriptionId);
    if (result != UEC_RESULT_OK || hitSubscriptionId == 0u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    hitCapture.expected_subscription_id = hitSubscriptionId;
    hit.blocking_hit = UEC_TRUE;
    appliedDelta = (uec_vector3){1.0, 2.0, 3.0};
    result = api->move_actor_swept(movingActor, (uec_vector3){NAN, 0.0, 0.0},
                                   &hit, &appliedDelta);
    if (result != UEC_RESULT_INVALID_ARGUMENT || hit.blocking_hit != UEC_FALSE ||
        hit.actor != NULL || appliedDelta.x != 0.0 || appliedDelta.y != 0.0 ||
        appliedDelta.z != 0.0) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->move_actor_swept(movingActor, sweptMoveDelta, &hit, &appliedDelta);
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    if (hit.blocking_hit != UEC_TRUE || hit.actor == NULL ||
        !IsAt(api, hit.actor, center) || appliedDelta.x <= 150.0 ||
        appliedDelta.x >= sweptMoveDelta.x || !IsNear(appliedDelta.y, 0.0) ||
        !IsNear(appliedDelta.z, 0.0)) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    if (hitCapture.callback_count != 1u ||
        hitCapture.subscription_id_matched != UEC_TRUE ||
        hitCapture.other_actor == NULL || !IsAt(api, hitCapture.other_actor, center)) {
        result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    result = api->release_actor(hitCapture.other_actor);
    hitCapture.other_actor = NULL;
    if (result != UEC_RESULT_OK) UEC_COLLISION_SMOKE_FAIL();
    result = api->unbind_component_hit(context, hitSubscriptionId);
    if (result != UEC_RESULT_INVALID_ARGUMENT) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        UEC_COLLISION_SMOKE_FAIL();
    }
    hitSubscriptionId = 0u;
    result = UEC_RESULT_OK;

cleanup:
    if (api != NULL) {
        if (hit.actor != NULL) (void)api->release_actor(hit.actor);
        if (details.hit.actor != NULL) (void)api->release_actor(details.hit.actor);
        if (details.component != NULL) (void)api->release_scene_component(details.component);
        for (uint32_t index = 0u; index < overlapCount; ++index) {
            if (overlaps[index] != NULL) (void)api->release_actor(overlaps[index]);
        }
        if (hitCapture.other_actor != NULL) {
            (void)api->release_actor(hitCapture.other_actor);
        }
        if (hitSubscriptionId != 0u && context != NULL &&
            api->unbind_component_hit != NULL) {
            (void)api->unbind_component_hit(context, hitSubscriptionId);
        }
        if (movingComponent != NULL) (void)api->release_scene_component(movingComponent);
        if (socketComponent != NULL) (void)api->release_scene_component(socketComponent);
        if (movingActor != NULL) {
            const uec_result destroyResult = api->destroy_actor(movingActor);
            if (destroyResult != UEC_RESULT_OK) (void)api->release_actor(movingActor);
            else movingActor = NULL;
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
