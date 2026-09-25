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

static int IsSameTransform(uec_transform actual, uec_transform expected)
{
    return IsSameVector(actual.translation, expected.translation) &&
        IsSameVector(actual.scale, expected.scale) && IsNear(actual.rotation.x, expected.rotation.x) &&
        IsNear(actual.rotation.y, expected.rotation.y) && IsNear(actual.rotation.z, expected.rotation.z) &&
        IsNear(actual.rotation.w, expected.rotation.w);
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
    uec_transform actorTransformBefore = {0};
    uec_transform componentTransformBefore = {0};
    uec_transform attemptedTransform = {0};
    uec_transform transformAfter = {0};
    uec_collision_enabled collisionBefore = UEC_COLLISION_DISABLED;
    uec_collision_enabled collisionAfter = UEC_COLLISION_DISABLED;
    uec_collision_response responseBefore = UEC_COLLISION_RESPONSE_IGNORE;
    uec_collision_response responseAfter = UEC_COLLISION_RESPONSE_IGNORE;
    uec_bool activeBefore = UEC_FALSE;
    uec_bool activeAfter = UEC_FALSE;
    uec_bool hasTag = UEC_FALSE;
    uec_property_value replicatedValueBefore = {0};
    uec_property_value attemptedReplicatedValue = {0};
    uec_property_value replicatedValueAfter = {0};
    uec_property_value localValueBefore = {0};
    uec_property_value attemptedLocalValue = {0};
    uec_property_value localValueAfter = {0};
    uec_property_value containerValueBefore = {0};
    uec_property_value attemptedContainerValue = {0};
    uec_property_value containerValueAfter = {0};
    static const char replicatedPropertyText[] = "AuthoritySmokeReplicatedValue";
    const uec_string_view replicatedProperty = {
        replicatedPropertyText, sizeof(replicatedPropertyText) - 1u
    };
    static const char localPropertyText[] = "AuthoritySmokeLocalValue";
    const uec_string_view localProperty = {
        localPropertyText, sizeof(localPropertyText) - 1u
    };
    static const char replicatedArrayPropertyText[] = "AuthoritySmokeReplicatedArray";
    const uec_string_view replicatedArrayProperty = {
        replicatedArrayPropertyText, sizeof(replicatedArrayPropertyText) - 1u
    };
    static const char replicatedMapPropertyText[] = "AuthoritySmokeNetMap";
    const uec_string_view replicatedMapProperty = {
        replicatedMapPropertyText, sizeof(replicatedMapPropertyText) - 1u
    };
    static const char replicatedSetPropertyText[] = "AuthoritySmokeNetSet";
    const uec_string_view replicatedSetProperty = {
        replicatedSetPropertyText, sizeof(replicatedSetPropertyText) - 1u
    };
    static const char replicatedStructPropertyText[] = "AuthoritySmokeReplicatedStruct";
    const uec_string_view replicatedStructProperty = {
        replicatedStructPropertyText, sizeof(replicatedStructPropertyText) - 1u
    };
    static const char replicatedStructFieldText[] = "IntegerValue";
    const uec_string_view replicatedStructField = {
        replicatedStructFieldText, sizeof(replicatedStructFieldText) - 1u
    };
    static const char attemptedTextValue[] = "97";
    const uec_string_view attemptedText = {
        attemptedTextValue, sizeof(attemptedTextValue) - 1u
    };
    static const char authorityTagText[] = "UEC_ClientAuthoritySmoke";
    const uec_string_view authorityTag = {
        authorityTagText, sizeof(authorityTagText) - 1u
    };
    uint32_t worldCount = 0u;
    uint32_t actorCount = 0u;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL || api->release_context == NULL ||
        api->get_world_count == NULL || api->get_world_at == NULL ||
        api->get_world_net_mode == NULL || api->get_world_has_authority == NULL ||
        api->release_world == NULL || api->get_actor_count_by_class == NULL ||
        api->get_actor_at_by_class == NULL || api->release_actor == NULL ||
        api->get_actor_property_value == NULL || api->set_actor_property_value == NULL ||
        api->get_actor_property_array_element_value == NULL ||
        api->set_actor_property_array_element_value == NULL ||
        api->set_actor_property_array_element_text == NULL ||
        api->get_actor_property_map_value == NULL ||
        api->set_actor_property_map_value == NULL ||
        api->set_actor_property_map_value_text == NULL ||
        api->get_actor_property_set_element_value == NULL ||
        api->set_actor_property_set_element_value == NULL ||
        api->set_actor_property_set_element_text == NULL ||
        api->get_actor_property_struct_field_value == NULL ||
        api->set_actor_property_struct_field_value == NULL ||
        api->set_actor_property_struct_field_text == NULL ||
        api->get_actor_root_component == NULL || api->release_scene_component == NULL ||
        api->get_actor_transform == NULL || api->set_actor_transform == NULL ||
        api->get_component_transform == NULL || api->set_component_transform == NULL ||
        api->get_component_active == NULL || api->set_component_active == NULL ||
        api->get_component_collision_enabled == NULL ||
        api->set_component_collision_enabled == NULL ||
        api->get_component_collision_response == NULL ||
        api->set_component_collision_channel_response == NULL ||
        api->actor_has_tag == NULL || api->set_actor_tag == NULL ||
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
    replicatedValueBefore.struct_size = sizeof(replicatedValueBefore);
    result = api->get_actor_property_value(
        actor, replicatedProperty, &replicatedValueBefore);
    if (result != UEC_RESULT_OK ||
        replicatedValueBefore.kind != UEC_PROPERTY_INTEGER) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    attemptedReplicatedValue = replicatedValueBefore;
    attemptedReplicatedValue.integer_value =
        replicatedValueBefore.integer_value == INT32_MAX
            ? replicatedValueBefore.integer_value - 1
            : replicatedValueBefore.integer_value + 1;
    if (api->set_actor_property_value(
            actor, replicatedProperty, &attemptedReplicatedValue) !=
        UEC_RESULT_UNSUPPORTED) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    replicatedValueAfter.struct_size = sizeof(replicatedValueAfter);
    result = api->get_actor_property_value(
        actor, replicatedProperty, &replicatedValueAfter);
    if (result != UEC_RESULT_OK ||
        replicatedValueAfter.kind != UEC_PROPERTY_INTEGER ||
        replicatedValueAfter.integer_value != replicatedValueBefore.integer_value) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    containerValueBefore.struct_size = sizeof(containerValueBefore);
    result = api->get_actor_property_array_element_value(
        actor, replicatedArrayProperty, 0u, &containerValueBefore);
    if (result != UEC_RESULT_OK || containerValueBefore.kind != UEC_PROPERTY_INTEGER) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    attemptedContainerValue = containerValueBefore;
    attemptedContainerValue.integer_value++;
    if (api->set_actor_property_array_element_value(
            actor, replicatedArrayProperty, 0u, &attemptedContainerValue) !=
            UEC_RESULT_UNSUPPORTED ||
        api->set_actor_property_array_element_text(
            actor, replicatedArrayProperty, 0u, attemptedText) != UEC_RESULT_UNSUPPORTED) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    containerValueAfter.struct_size = sizeof(containerValueAfter);
    result = api->get_actor_property_array_element_value(
        actor, replicatedArrayProperty, 0u, &containerValueAfter);
    if (result != UEC_RESULT_OK || containerValueAfter.kind != UEC_PROPERTY_INTEGER ||
        containerValueAfter.integer_value != containerValueBefore.integer_value) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    containerValueBefore = (uec_property_value){0};
    containerValueBefore.struct_size = sizeof(containerValueBefore);
    result = api->get_actor_property_map_value(
        actor, replicatedMapProperty, 0u, &containerValueBefore);
    if (result != UEC_RESULT_OK || containerValueBefore.kind != UEC_PROPERTY_INTEGER) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    attemptedContainerValue = containerValueBefore;
    attemptedContainerValue.integer_value++;
    if (api->set_actor_property_map_value(
            actor, replicatedMapProperty, 0u, &attemptedContainerValue) !=
            UEC_RESULT_UNSUPPORTED ||
        api->set_actor_property_map_value_text(
            actor, replicatedMapProperty, 0u, attemptedText) != UEC_RESULT_UNSUPPORTED) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    containerValueAfter = (uec_property_value){0};
    containerValueAfter.struct_size = sizeof(containerValueAfter);
    result = api->get_actor_property_map_value(
        actor, replicatedMapProperty, 0u, &containerValueAfter);
    if (result != UEC_RESULT_OK || containerValueAfter.kind != UEC_PROPERTY_INTEGER ||
        containerValueAfter.integer_value != containerValueBefore.integer_value) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    containerValueBefore = (uec_property_value){0};
    containerValueBefore.struct_size = sizeof(containerValueBefore);
    result = api->get_actor_property_set_element_value(
        actor, replicatedSetProperty, 0u, &containerValueBefore);
    if (result != UEC_RESULT_OK || containerValueBefore.kind != UEC_PROPERTY_INTEGER) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    attemptedContainerValue = containerValueBefore;
    attemptedContainerValue.integer_value++;
    if (api->set_actor_property_set_element_value(
            actor, replicatedSetProperty, 0u, &attemptedContainerValue) !=
            UEC_RESULT_UNSUPPORTED ||
        api->set_actor_property_set_element_text(
            actor, replicatedSetProperty, 0u, attemptedText) != UEC_RESULT_UNSUPPORTED) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    containerValueAfter = (uec_property_value){0};
    containerValueAfter.struct_size = sizeof(containerValueAfter);
    result = api->get_actor_property_set_element_value(
        actor, replicatedSetProperty, 0u, &containerValueAfter);
    if (result != UEC_RESULT_OK || containerValueAfter.kind != UEC_PROPERTY_INTEGER ||
        containerValueAfter.integer_value != containerValueBefore.integer_value) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    containerValueBefore = (uec_property_value){0};
    containerValueBefore.struct_size = sizeof(containerValueBefore);
    result = api->get_actor_property_struct_field_value(
        actor, replicatedStructProperty, replicatedStructField, &containerValueBefore);
    if (result != UEC_RESULT_OK || containerValueBefore.kind != UEC_PROPERTY_INTEGER) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    attemptedContainerValue = containerValueBefore;
    attemptedContainerValue.integer_value++;
    if (api->set_actor_property_struct_field_value(
            actor, replicatedStructProperty, replicatedStructField,
            &attemptedContainerValue) != UEC_RESULT_UNSUPPORTED ||
        api->set_actor_property_struct_field_text(
            actor, replicatedStructProperty, replicatedStructField,
            attemptedText) != UEC_RESULT_UNSUPPORTED) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    containerValueAfter = (uec_property_value){0};
    containerValueAfter.struct_size = sizeof(containerValueAfter);
    result = api->get_actor_property_struct_field_value(
        actor, replicatedStructProperty, replicatedStructField, &containerValueAfter);
    if (result != UEC_RESULT_OK || containerValueAfter.kind != UEC_PROPERTY_INTEGER ||
        containerValueAfter.integer_value != containerValueBefore.integer_value) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    localValueBefore.struct_size = sizeof(localValueBefore);
    result = api->get_actor_property_value(actor, localProperty, &localValueBefore);
    if (result != UEC_RESULT_OK || localValueBefore.kind != UEC_PROPERTY_INTEGER) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    attemptedLocalValue = localValueBefore;
    attemptedLocalValue.integer_value = localValueBefore.integer_value + 1;
    if (api->set_actor_property_value(actor, localProperty, &attemptedLocalValue) !=
        UEC_RESULT_OK) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    localValueAfter.struct_size = sizeof(localValueAfter);
    result = api->get_actor_property_value(actor, localProperty, &localValueAfter);
    if (result != UEC_RESULT_OK || localValueAfter.kind != UEC_PROPERTY_INTEGER ||
        localValueAfter.integer_value != attemptedLocalValue.integer_value) {
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

    result = api->get_actor_transform(actor, &actorTransformBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_transform(component, &componentTransformBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_active(component, &activeBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_collision_enabled(component, &collisionBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_collision_response(
        component, UEC_TRACE_VISIBILITY, &responseBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    attemptedTransform = actorTransformBefore;
    attemptedTransform.translation.x += 250.0;

    if (api->set_actor_transform(actor, &attemptedTransform, UEC_FALSE) != UEC_RESULT_UNSUPPORTED ||
        api->set_component_transform(component, &attemptedTransform, UEC_FALSE) != UEC_RESULT_UNSUPPORTED ||
        api->set_actor_tag(actor, authorityTag, UEC_TRUE) != UEC_RESULT_UNSUPPORTED ||
        api->set_component_active(component, activeBefore == UEC_FALSE ? UEC_TRUE : UEC_FALSE,
                                  UEC_FALSE) != UEC_RESULT_UNSUPPORTED ||
        api->set_component_collision_enabled(
            component, collisionBefore == UEC_COLLISION_DISABLED
                ? UEC_COLLISION_QUERY_ONLY : UEC_COLLISION_DISABLED) != UEC_RESULT_UNSUPPORTED ||
        api->set_component_collision_channel_response(
            component, UEC_TRACE_VISIBILITY,
            responseBefore == UEC_COLLISION_RESPONSE_BLOCK
                ? UEC_COLLISION_RESPONSE_IGNORE : UEC_COLLISION_RESPONSE_BLOCK) != UEC_RESULT_UNSUPPORTED ||
        api->set_component_simulating_physics(component, UEC_FALSE) != UEC_RESULT_UNSUPPORTED ||
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
    result = api->get_actor_transform(actor, &transformAfter);
    if (result != UEC_RESULT_OK || !IsSameTransform(transformAfter, actorTransformBefore)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_component_transform(component, &transformAfter);
    if (result != UEC_RESULT_OK || !IsSameTransform(transformAfter, componentTransformBefore)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_component_active(component, &activeAfter);
    if (result != UEC_RESULT_OK || activeAfter != activeBefore) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_component_collision_enabled(component, &collisionAfter);
    if (result != UEC_RESULT_OK || collisionAfter != collisionBefore) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_component_collision_response(
        component, UEC_TRACE_VISIBILITY, &responseAfter);
    if (result != UEC_RESULT_OK || responseAfter != responseBefore) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->actor_has_tag(actor, authorityTag, &hasTag);
    if (result != UEC_RESULT_OK || hasTag != UEC_FALSE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
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
