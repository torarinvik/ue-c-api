#include "uec_api.h"

#include <stdbool.h>
#include <string.h>

static uec_string_view View(const char* text)
{
    return (uec_string_view){text, strlen(text)};
}

static bool MatchesGuid(const uec_guid* actual, const uec_guid* expected)
{
    return actual->a == expected->a && actual->b == expected->b &&
        actual->c == expected->c && actual->d == expected->d;
}

uec_result UEC_CALL uec_host_reflection_guid_smoke(void)
{
    static const char classPathData[] =
        "/Script/UnrealCAPIHost.UECAPIHostReflectionSmokeActor";
    const uec_guid initial = {0x01234567u, 0x89ABCDEFu, 0xA0B0C0D0u, 0xFFFFFFFFu};
    const uec_guid updated = {0xFEDCBA98u, 0x76543210u, 0x12345678u, 0xABCDEF01u};
    const uec_guid functionInput = {0x10203040u, 0x50607080u, 0x90A0B0C0u, 0xD0E0F001u};

    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* world = NULL;
    uec_actor* actor = NULL;
    uec_object* selfObject = NULL;
    const char* stage = "API bootstrap";
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL || api->get_default_world == NULL ||
        api->release_world == NULL || api->release_context == NULL ||
        api->spawn_actor == NULL || api->destroy_actor == NULL ||
        api->release_actor == NULL || api->get_actor_property_struct_value == NULL ||
        api->set_actor_property_struct_value == NULL ||
        api->get_object_property_struct_value == NULL ||
        api->set_object_property_struct_value == NULL ||
        api->get_actor_property_object == NULL || api->release_object == NULL ||
        api->invoke_actor_function_arguments == NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }

    const uec_transform transform = {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    if (result == UEC_RESULT_OK) stage = "spawn reflected FGuid actor";
    if (result == UEC_RESULT_OK) result = api->get_default_world(context, &world);
    if (result == UEC_RESULT_OK && world == NULL) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->spawn_actor(world, View(classPathData), &transform, &actor);
    if (result == UEC_RESULT_OK && actor == NULL) result = UEC_RESULT_INTERNAL_ERROR;

    uec_property_struct_value property = {0};
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK) stage = "typed FGuid actor property read";
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_struct_value(actor, View("StableId"), &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_GUID ||
        !MatchesGuid(&property.value.guid, &initial))) result = UEC_RESULT_INTERNAL_ERROR;

    uec_property_struct_value written = {0};
    written.struct_size = sizeof(written);
    written.kind = UEC_PROPERTY_STRUCT_GUID;
    written.value.guid = updated;
    if (result == UEC_RESULT_OK) stage = "typed FGuid actor property write";
    if (result == UEC_RESULT_OK)
        result = api->set_actor_property_struct_value(actor, View("StableId"), &written);
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_object(actor, View("SelfObject"), &selfObject);
    if (result == UEC_RESULT_OK && selfObject == NULL) result = UEC_RESULT_INTERNAL_ERROR;
    memset(&property, 0, sizeof(property));
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK) stage = "typed FGuid UObject property read";
    if (result == UEC_RESULT_OK)
        result = api->get_object_property_struct_value(selfObject, View("StableId"), &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_GUID ||
        !MatchesGuid(&property.value.guid, &updated))) result = UEC_RESULT_INTERNAL_ERROR;

    uec_property_struct_value wrongKind = {0};
    wrongKind.struct_size = sizeof(wrongKind);
    wrongKind.kind = UEC_PROPERTY_STRUCT_INT_VECTOR;
    wrongKind.value.int_vector = (uec_int_vector){1, 2, 3};
    if (result == UEC_RESULT_OK) stage = "FGuid property kind rejection";
    if (result == UEC_RESULT_OK &&
        api->set_actor_property_struct_value(actor, View("StableId"), &wrongKind) !=
            UEC_RESULT_INVALID_ARGUMENT) result = UEC_RESULT_INTERNAL_ERROR;

    uec_function_argument argument = {0};
    argument.struct_size = sizeof(argument);
    argument.kind = UEC_PROPERTY_STRUCT;
    argument.struct_value.kind = UEC_FUNCTION_STRUCT_GUID;
    argument.struct_value.value.guid = functionInput;
    uec_function_output output = {0};
    output.struct_size = sizeof(output);
    output.struct_value.kind = UEC_FUNCTION_STRUCT_GUID;
    uint32_t outputCount = 0u;
    if (result == UEC_RESULT_OK) stage = "typed FGuid mixed function call";
    if (result == UEC_RESULT_OK)
        result = api->invoke_actor_function_arguments(actor, View("EchoGuid"),
            &argument, 1u, &output, 1u, &outputCount);
    if (result == UEC_RESULT_OK &&
        (outputCount != 1u || output.kind != UEC_PROPERTY_STRUCT ||
         output.struct_value.kind != UEC_FUNCTION_STRUCT_GUID ||
         !MatchesGuid(&output.struct_value.value.guid, &functionInput)))
        result = UEC_RESULT_INTERNAL_ERROR;

    if (result != UEC_RESULT_OK && api != NULL && api->log != NULL &&
        context != NULL) api->log(context, View(stage));
    if (selfObject != NULL) {
        const uec_result cleanup = api->release_object(selfObject);
        if (result == UEC_RESULT_OK && cleanup != UEC_RESULT_OK) result = cleanup;
    }
    if (actor != NULL) {
        const uec_result cleanup = api->destroy_actor(actor);
        if (result == UEC_RESULT_OK && cleanup != UEC_RESULT_OK) result = cleanup;
        if (cleanup != UEC_RESULT_OK) api->release_actor(actor);
    }
    if (world != NULL) {
        const uec_result cleanup = api->release_world(world);
        if (result == UEC_RESULT_OK && cleanup != UEC_RESULT_OK) result = cleanup;
    }
    if (context != NULL) {
        const uec_result cleanup = api->release_context(context);
        if (result == UEC_RESULT_OK && cleanup != UEC_RESULT_OK) result = cleanup;
    }
    return result;
}
