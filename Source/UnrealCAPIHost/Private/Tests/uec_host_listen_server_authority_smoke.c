#include "uec_api.h"

static int IsNear(double actual, double expected)
{
    const double tolerance = 0.05;
    return actual >= expected - tolerance && actual <= expected + tolerance;
}

static int IsSameTransform(uec_transform actual, uec_transform expected)
{
    return IsNear(actual.translation.x, expected.translation.x) &&
        IsNear(actual.translation.y, expected.translation.y) &&
        IsNear(actual.translation.z, expected.translation.z) &&
        IsNear(actual.rotation.x, expected.rotation.x) &&
        IsNear(actual.rotation.y, expected.rotation.y) &&
        IsNear(actual.rotation.z, expected.rotation.z) &&
        IsNear(actual.rotation.w, expected.rotation.w) &&
        IsNear(actual.scale.x, expected.scale.x) &&
        IsNear(actual.scale.y, expected.scale.y) &&
        IsNear(actual.scale.z, expected.scale.z);
}

uec_result UEC_CALL uec_host_listen_server_authority_smoke(void)
{
    static const char actorClassText[] =
        "/Script/UnrealCAPIHost.UECAPIHostCollisionSmokeActor";
    static const char componentClassText[] = "/Script/Engine.BoxComponent";
    static const char primitiveComponentClassText[] = "/Script/Engine.PrimitiveComponent";
    static const char sceneComponentClassText[] = "/Script/Engine.SceneComponent";
    static const char invalidComponentClassText[] = "/Script/Engine.Actor";
    static const char baseActorClassText[] = "/Script/Engine.Actor";
    static const char invalidActorClassText[] = "/Script/CoreUObject.Object";
    static const char replicatedPropertyText[] = "AuthoritySmokeReplicatedValue";
    static const char tagText[] = "UEC_ListenServerAuthoritySmoke";
    static const char textValue[] = "25";
    const uec_string_view actorClass = {actorClassText, sizeof(actorClassText) - 1u};
    const uec_string_view baseActorClass = {
        baseActorClassText, sizeof(baseActorClassText) - 1u
    };
    const uec_string_view invalidActorClass = {
        invalidActorClassText, sizeof(invalidActorClassText) - 1u
    };
    const uec_string_view componentClass = {
        componentClassText, sizeof(componentClassText) - 1u
    };
    const uec_string_view primitiveComponentClass = {
        primitiveComponentClassText, sizeof(primitiveComponentClassText) - 1u
    };
    const uec_string_view sceneComponentClass = {
        sceneComponentClassText, sizeof(sceneComponentClassText) - 1u
    };
    const uec_string_view invalidComponentClass = {
        invalidComponentClassText, sizeof(invalidComponentClassText) - 1u
    };
    const uec_string_view replicatedProperty = {
        replicatedPropertyText, sizeof(replicatedPropertyText) - 1u
    };
    const uec_string_view tag = {tagText, sizeof(tagText) - 1u};
    const uec_string_view text = {textValue, sizeof(textValue) - 1u};
    static const char emptySocketName[] = "";
    const uec_string_view emptySocket = {emptySocketName, 0u};
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* listenServer = NULL;
    uec_actor* actor = NULL;
    uec_actor* childActor = NULL;
    uec_scene_component* parentComponent = NULL;
    uec_scene_component* childComponent = NULL;
    uec_scene_component* filteredComponent = NULL;
    uec_net_mode netMode = UEC_NET_MODE_UNKNOWN;
    uec_bool hasAuthority = UEC_FALSE;
    uec_bool hasTag = UEC_FALSE;
    uec_bool visibleBefore = UEC_FALSE;
    uec_bool visibleAfter = UEC_FALSE;
    uec_bool activeBefore = UEC_FALSE;
    uec_bool activeAfter = UEC_FALSE;
    uec_bool isComponentType = UEC_FALSE;
    uec_bool isActorType = UEC_FALSE;
    uec_transform spawnTransform = {0};
    uec_transform transformBefore = {0};
    uec_transform attemptedTransform = {0};
    uec_transform transformAfter = {0};
    uec_transform childTransformBeforeAttach = {0};
    uec_transform childTransformAfterAttach = {0};
    uec_transform childTransformBeforeRelativeAttach = {0};
    uec_transform childTransformAfterRelativeAttach = {0};
    uec_transform relativeExpectedTransform = {0};
    uec_transform childTransformAfterParentMove = {0};
    uec_transform childTransformAfterDetach = {0};
    uec_transform childTransformAfterDetachedParentMove = {0};
    uint32_t tagCountBefore = 0u;
    uint32_t tagCountAfter = 0u;
    char tagOutput[128] = {0};
    size_t tagRequiredSize = 0u;
    char actorClassName[192] = {0};
    size_t actorClassRequiredSize = 0u;
    uec_property_value valueBefore = {0};
    uec_property_value attemptedValue = {0};
    uec_property_value valueAfter = {0};
    uint32_t worldCount = 0u;
    uint32_t filteredComponentCount = 0u;
    char componentClassName[128] = {0};
    size_t componentClassRequiredSize = 0u;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL || api->release_context == NULL ||
        api->get_world_count == NULL || api->get_world_at == NULL ||
        api->get_world_net_mode == NULL || api->get_world_has_authority == NULL ||
        api->release_world == NULL || api->spawn_actor == NULL ||
        api->destroy_actor == NULL || api->release_actor == NULL ||
        api->get_actor_class_name == NULL || api->actor_is_a == NULL ||
        api->get_actor_root_component == NULL || api->release_scene_component == NULL ||
        api->get_actor_component_count_by_class == NULL ||
        api->get_actor_component_at_by_class == NULL ||
        api->get_component_class_name == NULL || api->component_is_a == NULL ||
        api->get_component_transform == NULL || api->set_component_transform == NULL ||
        api->get_component_visible == NULL || api->set_component_visible == NULL ||
        api->get_component_active == NULL || api->set_component_active == NULL ||
        api->attach_scene_component == NULL || api->detach_scene_component == NULL ||
        api->get_actor_transform == NULL || api->set_actor_transform == NULL ||
        api->get_actor_property_value == NULL || api->set_actor_property_value == NULL ||
        api->set_actor_property_string == NULL || api->set_actor_tag == NULL ||
        api->actor_has_tag == NULL || api->get_actor_tag_count == NULL ||
        api->get_actor_tag_at == NULL) {
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
    result = api->get_actor_class_name(actor, actorClassName, sizeof(actorClassName),
                                       &actorClassRequiredSize);
    if (result != UEC_RESULT_OK || actorClassRequiredSize != sizeof(actorClassText)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    for (size_t index = 0u; index < sizeof(actorClassText); ++index) {
        if (actorClassName[index] != actorClassText[index]) {
            result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
    }
    result = api->actor_is_a(actor, actorClass, &isActorType);
    if (result != UEC_RESULT_OK || isActorType != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->actor_is_a(actor, baseActorClass, &isActorType);
    if (result != UEC_RESULT_OK || isActorType != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    isActorType = UEC_TRUE;
    if (api->actor_is_a(actor, invalidActorClass, &isActorType) !=
            UEC_RESULT_INVALID_ARGUMENT || isActorType != UEC_FALSE) {
        result = UEC_RESULT_INTERNAL_ERROR;
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

    result = api->get_actor_tag_count(actor, &tagCountBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->set_actor_tag(actor, tag, UEC_TRUE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->set_actor_tag(actor, tag, UEC_TRUE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->actor_has_tag(actor, tag, &hasTag);
    if (result != UEC_RESULT_OK || hasTag != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_actor_tag_count(actor, &tagCountAfter);
    if (result != UEC_RESULT_OK || tagCountAfter != tagCountBefore + 1u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_actor_tag_at(actor, tagCountBefore, tagOutput,
                                   sizeof(tagOutput), &tagRequiredSize);
    if (result != UEC_RESULT_OK || tagRequiredSize != sizeof(tagText)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    for (size_t index = 0u; index < sizeof(tagText); ++index) {
        if (tagOutput[index] != tagText[index]) {
            result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
    }
    tagRequiredSize = SIZE_MAX;
    if (api->get_actor_tag_at(actor, tagCountAfter, tagOutput,
                              sizeof(tagOutput), &tagRequiredSize) != UEC_RESULT_INVALID_ARGUMENT ||
        tagRequiredSize != 0u) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->set_actor_tag(actor, tag, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->set_actor_tag(actor, tag, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->actor_has_tag(actor, tag, &hasTag);
    if (result != UEC_RESULT_OK || hasTag != UEC_FALSE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_actor_tag_count(actor, &tagCountAfter);
    if (result != UEC_RESULT_OK || tagCountAfter != tagCountBefore) {
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
    if (result != UEC_RESULT_OK || !IsSameTransform(transformAfter, attemptedTransform)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    spawnTransform.translation.x = 1000.0;
    spawnTransform.translation.y = 500.0;
    result = api->spawn_actor(listenServer, actorClass, &spawnTransform, &childActor);
    if (result != UEC_RESULT_OK || childActor == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_actor_root_component(actor, &parentComponent);
    if (result != UEC_RESULT_OK || parentComponent == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_component_transform(parentComponent, &transformBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_actor_component_count_by_class(actor, componentClass,
                                                     &filteredComponentCount);
    if (result != UEC_RESULT_OK || filteredComponentCount != 1u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_actor_component_at_by_class(actor, componentClass, 0u,
                                                  &filteredComponent);
    if (result != UEC_RESULT_OK || filteredComponent == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_component_class_name(filteredComponent, componentClassName,
                                           sizeof(componentClassName),
                                           &componentClassRequiredSize);
    if (result != UEC_RESULT_OK ||
        componentClassRequiredSize != sizeof(componentClassText)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    for (size_t index = 0u; index < sizeof(componentClassText); ++index) {
        if (componentClassName[index] != componentClassText[index]) {
            result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
    }
    result = api->component_is_a(filteredComponent, componentClass, &isComponentType);
    if (result != UEC_RESULT_OK || isComponentType != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->component_is_a(filteredComponent, primitiveComponentClass,
                                 &isComponentType);
    if (result != UEC_RESULT_OK || isComponentType != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->component_is_a(filteredComponent, sceneComponentClass,
                                 &isComponentType);
    if (result != UEC_RESULT_OK || isComponentType != UEC_TRUE) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    isComponentType = UEC_TRUE;
    if (api->component_is_a(filteredComponent, invalidComponentClass,
                            &isComponentType) != UEC_RESULT_INVALID_ARGUMENT ||
        isComponentType != UEC_FALSE) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_component_transform(filteredComponent, &transformAfter);
    if (result != UEC_RESULT_OK || !IsSameTransform(transformAfter, transformBefore)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->release_scene_component(filteredComponent);
    if (result != UEC_RESULT_OK) goto cleanup;
    filteredComponent = NULL;
    result = api->get_actor_root_component(childActor, &childComponent);
    if (result != UEC_RESULT_OK || childComponent == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_component_transform(childComponent, &childTransformBeforeAttach);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->attach_scene_component(childComponent, parentComponent,
                                         UEC_TRUE, emptySocket);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_transform(childComponent, &childTransformAfterAttach);
    if (result != UEC_RESULT_OK ||
        !IsSameTransform(childTransformAfterAttach, childTransformBeforeAttach)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    result = api->get_actor_transform(actor, &transformBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    attemptedTransform = transformBefore;
    attemptedTransform.translation.x += 40.0;
    result = api->set_actor_transform(actor, &attemptedTransform, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_transform(childComponent, &childTransformAfterParentMove);
    if (result != UEC_RESULT_OK ||
        !IsNear(childTransformAfterParentMove.translation.x,
                childTransformBeforeAttach.translation.x + 40.0) ||
        !IsNear(childTransformAfterParentMove.translation.y,
                childTransformBeforeAttach.translation.y) ||
        !IsNear(childTransformAfterParentMove.translation.z,
                childTransformBeforeAttach.translation.z)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->detach_scene_component(childComponent, UEC_TRUE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_transform(childComponent, &childTransformAfterDetach);
    if (result != UEC_RESULT_OK ||
        !IsSameTransform(childTransformAfterDetach, childTransformAfterParentMove)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    attemptedTransform.translation.x += 25.0;
    result = api->set_actor_transform(actor, &attemptedTransform, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_transform(childComponent,
                                          &childTransformAfterDetachedParentMove);
    if (result != UEC_RESULT_OK ||
        !IsSameTransform(childTransformAfterDetachedParentMove,
                         childTransformAfterDetach)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    transformBefore = childTransformAfterDetachedParentMove;
    attemptedTransform = transformBefore;
    attemptedTransform.translation.x += 5.0;
    attemptedTransform.translation.y -= 10.0;
    result = api->set_component_transform(childComponent, &attemptedTransform, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_transform(childComponent, &transformAfter);
    if (result != UEC_RESULT_OK || !IsSameTransform(transformAfter, attemptedTransform)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    childTransformBeforeRelativeAttach = transformAfter;
    result = api->get_actor_transform(actor, &transformBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->attach_scene_component(childComponent, parentComponent,
                                         UEC_FALSE, emptySocket);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_transform(childComponent,
                                          &childTransformAfterRelativeAttach);
    relativeExpectedTransform = childTransformBeforeRelativeAttach;
    relativeExpectedTransform.translation.x += transformBefore.translation.x;
    relativeExpectedTransform.translation.y += transformBefore.translation.y;
    relativeExpectedTransform.translation.z += transformBefore.translation.z;
    if (result != UEC_RESULT_OK ||
        !IsSameTransform(childTransformAfterRelativeAttach, relativeExpectedTransform)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->detach_scene_component(childComponent, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_transform(childComponent, &transformAfter);
    if (result != UEC_RESULT_OK ||
        !IsSameTransform(transformAfter, childTransformBeforeRelativeAttach)) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    result = api->get_component_visible(childComponent, &visibleBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    const uec_bool toggledVisibility = visibleBefore == UEC_FALSE ? UEC_TRUE : UEC_FALSE;
    result = api->set_component_visible(childComponent, toggledVisibility, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_visible(childComponent, &visibleAfter);
    if (result != UEC_RESULT_OK || visibleAfter != toggledVisibility) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->set_component_visible(childComponent, visibleBefore, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_visible(childComponent, &visibleAfter);
    if (result != UEC_RESULT_OK || visibleAfter != visibleBefore) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    result = api->get_component_active(childComponent, &activeBefore);
    if (result != UEC_RESULT_OK) goto cleanup;
    const uec_bool toggledActivation = activeBefore == UEC_FALSE ? UEC_TRUE : UEC_FALSE;
    result = api->set_component_active(childComponent, toggledActivation, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_active(childComponent, &activeAfter);
    if (result != UEC_RESULT_OK || activeAfter != toggledActivation) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->set_component_active(childComponent, activeBefore, UEC_FALSE);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = api->get_component_active(childComponent, &activeAfter);
    if (result != UEC_RESULT_OK || activeAfter != activeBefore) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    result = api->destroy_actor(actor);
    if (result != UEC_RESULT_OK) goto cleanup;
    actor = NULL;
    result = api->destroy_actor(childActor);
    if (result != UEC_RESULT_OK) goto cleanup;
    childActor = NULL;

cleanup:
    if (childComponent != NULL && api != NULL && api->detach_scene_component != NULL) {
        (void)api->detach_scene_component(childComponent, UEC_TRUE);
    }
    if (childActor != NULL) {
        if (api != NULL && api->destroy_actor != NULL &&
            api->destroy_actor(childActor) == UEC_RESULT_OK) {
            childActor = NULL;
        } else if (api != NULL && api->release_actor != NULL) {
            (void)api->release_actor(childActor);
            childActor = NULL;
        }
    }
    if (actor != NULL) {
        if (api != NULL && api->destroy_actor != NULL &&
            api->destroy_actor(actor) == UEC_RESULT_OK) {
            actor = NULL;
        }
        if (actor != NULL && api != NULL && api->release_actor != NULL) {
            (void)api->release_actor(actor);
        }
    }
    if (childComponent != NULL && api != NULL && api->release_scene_component != NULL) {
        (void)api->release_scene_component(childComponent);
    }
    if (parentComponent != NULL && api != NULL && api->release_scene_component != NULL) {
        (void)api->release_scene_component(parentComponent);
    }
    if (filteredComponent != NULL && api != NULL && api->release_scene_component != NULL) {
        (void)api->release_scene_component(filteredComponent);
    }
    if (listenServer != NULL && api != NULL && api->release_world != NULL) {
        (void)api->release_world(listenServer);
    }
    if (context != NULL && api != NULL && api->release_context != NULL) {
        (void)api->release_context(context);
    }
    return result;
}
