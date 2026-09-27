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
        api->get_actor_property_array_struct_value == NULL ||
        api->get_object_property_array_struct_value == NULL ||
        api->set_actor_property_array_struct_value == NULL ||
        api->set_object_property_array_struct_value == NULL ||
        api->get_actor_property_map_struct_value == NULL ||
        api->get_object_property_map_struct_value == NULL ||
        api->set_actor_property_map_struct_value == NULL ||
        api->set_object_property_map_struct_value == NULL ||
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

uec_result UEC_CALL uec_host_reflection_temporal_smoke(void)
{
    static const char classPathData[] =
        "/Script/UnrealCAPIHost.UECAPIHostReflectionSmokeActor";
    const int64_t initialDateTime = INT64_C(1234567890123456789);
    const int64_t updatedDateTime = INT64_C(2345678901234567890);
    const int64_t functionDateTime = INT64_C(987654321098765432);
    const int64_t initialTimespan = -INT64_C(1234567890123456789);
    const int64_t updatedTimespan = INT64_MIN;
    const int64_t functionTimespan = INT64_MAX;
    const uec_guid initialSetIds[] = {
        {0x01020304u, 0x11121314u, 0x21222324u, 0x31323334u},
        {0x41424344u, 0x51525354u, 0x61626364u, 0x71727374u}};
    const uec_guid updatedActorSetId = {
        0x81828384u, 0x91929394u, 0xA1A2A3A4u, 0xB1B2B3B4u};
    const uec_guid updatedObjectSetId = {
        0xC1C2C3C4u, 0xD1D2D3D4u, 0xE1E2E3E4u, 0xF1F2F3F4u};

    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_world* world = NULL;
    uec_actor* actor = NULL;
    uec_object* selfObject = NULL;
    const char* stage = "temporal API bootstrap";
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL || api->get_default_world == NULL ||
        api->release_world == NULL || api->release_context == NULL ||
        api->spawn_actor == NULL || api->destroy_actor == NULL ||
        api->release_actor == NULL || api->get_actor_property_struct_value == NULL ||
        api->set_actor_property_struct_value == NULL ||
        api->get_object_property_struct_value == NULL ||
        api->set_object_property_struct_value == NULL ||
        api->get_actor_property_set_count == NULL ||
        api->get_actor_property_set_element_struct_value == NULL ||
        api->get_object_property_set_element_struct_value == NULL ||
        api->set_actor_property_set_element_struct_value == NULL ||
        api->set_object_property_set_element_struct_value == NULL ||
        api->get_actor_property_object == NULL || api->release_object == NULL ||
        api->invoke_actor_function_arguments == NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }

    const uec_transform transform = {
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    if (result == UEC_RESULT_OK) result = api->get_default_world(context, &world);
    if (result == UEC_RESULT_OK && world == NULL) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->spawn_actor(world, View(classPathData), &transform, &actor);
    if (result == UEC_RESULT_OK && actor == NULL) result = UEC_RESULT_INTERNAL_ERROR;

    uec_property_struct_value property = {0};
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK) stage = "typed FDateTime property read";
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_struct_value(actor, View("RecordedAt"), &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_DATETIME ||
         property.value.datetime.ticks != initialDateTime)) result = UEC_RESULT_INTERNAL_ERROR;

    uec_property_struct_value datetimeValue = {0};
    datetimeValue.struct_size = sizeof(datetimeValue);
    datetimeValue.kind = UEC_PROPERTY_STRUCT_DATETIME;
    datetimeValue.value.datetime.ticks = updatedDateTime;
    if (result == UEC_RESULT_OK) stage = "typed FDateTime property write";
    if (result == UEC_RESULT_OK)
        result = api->set_actor_property_struct_value(actor, View("RecordedAt"), &datetimeValue);
    if (result == UEC_RESULT_OK) {
        datetimeValue.value.datetime.ticks = -1;
        if (api->set_actor_property_struct_value(actor, View("RecordedAt"), &datetimeValue) !=
            UEC_RESULT_INVALID_ARGUMENT) result = UEC_RESULT_INTERNAL_ERROR;
        datetimeValue.value.datetime.ticks = INT64_MAX;
        if (result == UEC_RESULT_OK &&
            api->set_actor_property_struct_value(actor, View("RecordedAt"), &datetimeValue) !=
                UEC_RESULT_INVALID_ARGUMENT) result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_object(actor, View("SelfObject"), &selfObject);
    if (result == UEC_RESULT_OK && selfObject == NULL) result = UEC_RESULT_INTERNAL_ERROR;
    memset(&property, 0, sizeof(property));
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK)
        result = api->get_object_property_struct_value(selfObject, View("RecordedAt"), &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_DATETIME ||
         property.value.datetime.ticks != updatedDateTime)) result = UEC_RESULT_INTERNAL_ERROR;

    property = (uec_property_struct_value){0};
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK) stage = "typed FTimespan property read";
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_struct_value(actor, View("Elapsed"), &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_TIMESPAN ||
         property.value.timespan.ticks != initialTimespan)) result = UEC_RESULT_INTERNAL_ERROR;

    uec_property_struct_value timespanValue = {0};
    timespanValue.struct_size = sizeof(timespanValue);
    timespanValue.kind = UEC_PROPERTY_STRUCT_TIMESPAN;
    timespanValue.value.timespan.ticks = updatedTimespan;
    if (result == UEC_RESULT_OK) stage = "typed FTimespan property write";
    if (result == UEC_RESULT_OK)
        result = api->set_actor_property_struct_value(actor, View("Elapsed"), &timespanValue);
    if (result == UEC_RESULT_OK)
        result = api->set_object_property_struct_value(selfObject, View("Elapsed"), &timespanValue);
    memset(&property, 0, sizeof(property));
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_struct_value(actor, View("Elapsed"), &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_TIMESPAN ||
         property.value.timespan.ticks != updatedTimespan)) result = UEC_RESULT_INTERNAL_ERROR;

    uec_function_argument argument = {0};
    argument.struct_size = sizeof(argument);
    argument.kind = UEC_PROPERTY_STRUCT;
    uec_function_output output = {0};
    output.struct_size = sizeof(output);
    uint32_t outputCount = 0u;
    argument.struct_value.kind = UEC_FUNCTION_STRUCT_DATETIME;
    argument.struct_value.value.datetime.ticks = functionDateTime;
    output.struct_value.kind = UEC_FUNCTION_STRUCT_DATETIME;
    if (result == UEC_RESULT_OK) stage = "typed FDateTime function call";
    if (result == UEC_RESULT_OK)
        result = api->invoke_actor_function_arguments(actor, View("EchoDateTime"),
            &argument, 1u, &output, 1u, &outputCount);
    if (result == UEC_RESULT_OK &&
        (outputCount != 1u || output.kind != UEC_PROPERTY_STRUCT ||
         output.struct_value.kind != UEC_FUNCTION_STRUCT_DATETIME ||
         output.struct_value.value.datetime.ticks != functionDateTime))
        result = UEC_RESULT_INTERNAL_ERROR;

    argument.struct_value.kind = UEC_FUNCTION_STRUCT_TIMESPAN;
    argument.struct_value.value.timespan.ticks = functionTimespan;
    output = (uec_function_output){0};
    output.struct_size = sizeof(output);
    output.struct_value.kind = UEC_FUNCTION_STRUCT_TIMESPAN;
    outputCount = 0u;
    if (result == UEC_RESULT_OK) stage = "typed FTimespan function call";
    if (result == UEC_RESULT_OK)
        result = api->invoke_actor_function_arguments(actor, View("EchoTimespan"),
            &argument, 1u, &output, 1u, &outputCount);
    if (result == UEC_RESULT_OK &&
        (outputCount != 1u || output.kind != UEC_PROPERTY_STRUCT ||
         output.struct_value.kind != UEC_FUNCTION_STRUCT_TIMESPAN ||
         output.struct_value.value.timespan.ticks != functionTimespan))
        result = UEC_RESULT_INTERNAL_ERROR;

    property = (uec_property_struct_value){0};
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK) stage = "typed FVector array actor read";
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_array_struct_value(
            actor, View("VectorPositions"), 0u, &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_VECTOR3 ||
         property.value.vector3.x != 4.0 || property.value.vector3.y != -5.5 ||
         property.value.vector3.z != 6.25)) result = UEC_RESULT_INTERNAL_ERROR;

    memset(&property, 0, sizeof(property));
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK) stage = "typed FVector array UObject read";
    if (result == UEC_RESULT_OK)
        result = api->get_object_property_array_struct_value(
            selfObject, View("VectorPositions"), 0u, &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_VECTOR3 ||
         property.value.vector3.x != 4.0 || property.value.vector3.y != -5.5 ||
         property.value.vector3.z != 6.25)) result = UEC_RESULT_INTERNAL_ERROR;

    uec_property_struct_value vectorValue = {0};
    vectorValue.struct_size = sizeof(vectorValue);
    vectorValue.kind = UEC_PROPERTY_STRUCT_VECTOR3;
    vectorValue.value.vector3 = (uec_vector3){-10.0, 20.25, 30.5};
    if (result == UEC_RESULT_OK) stage = "typed FVector array actor write";
    if (result == UEC_RESULT_OK)
        result = api->set_actor_property_array_struct_value(
            actor, View("VectorPositions"), 1u, &vectorValue);
    memset(&property, 0, sizeof(property));
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK)
        result = api->get_object_property_array_struct_value(
            selfObject, View("VectorPositions"), 1u, &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_VECTOR3 ||
         property.value.vector3.x != -10.0 || property.value.vector3.y != 20.25 ||
         property.value.vector3.z != 30.5)) result = UEC_RESULT_INTERNAL_ERROR;

    vectorValue.value.vector3 = (uec_vector3){1.5, -2.5, 3.5};
    if (result == UEC_RESULT_OK) stage = "typed FVector array UObject write";
    if (result == UEC_RESULT_OK)
        result = api->set_object_property_array_struct_value(
            selfObject, View("VectorPositions"), 0u, &vectorValue);
    memset(&property, 0, sizeof(property));
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_array_struct_value(
            actor, View("VectorPositions"), 0u, &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_VECTOR3 ||
         property.value.vector3.x != 1.5 || property.value.vector3.y != -2.5 ||
         property.value.vector3.z != 3.5)) result = UEC_RESULT_INTERNAL_ERROR;

    vectorValue.kind = UEC_PROPERTY_STRUCT_COLOR;
    if (result == UEC_RESULT_OK &&
        api->set_actor_property_array_struct_value(
            actor, View("VectorPositions"), 0u, &vectorValue) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    property.kind = UEC_PROPERTY_STRUCT_VECTOR3;
    property.value.vector3 = (uec_vector3){9.0, 9.0, 9.0};
    if (result == UEC_RESULT_OK &&
        api->get_actor_property_array_struct_value(
            actor, View("VectorPositions"), 2u, &property) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_NONE || property.value.vector3.x != 0.0 ||
         property.value.vector3.y != 0.0 || property.value.vector3.z != 0.0))
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK &&
        api->get_actor_property_array_struct_value(
            actor, View("Numbers"), 0u, &property) != UEC_RESULT_UNSUPPORTED)
        result = UEC_RESULT_INTERNAL_ERROR;

    memset(&property, 0, sizeof(property));
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK) stage = "typed FVector map actor read";
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_map_struct_value(
            actor, View("TypedVectors"), 0u, &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_VECTOR3 ||
         property.value.vector3.x != 12.5 || property.value.vector3.y != -3.0 ||
         property.value.vector3.z != 8.25)) result = UEC_RESULT_INTERNAL_ERROR;
    memset(&property, 0, sizeof(property));
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK) stage = "typed FVector map UObject read";
    if (result == UEC_RESULT_OK)
        result = api->get_object_property_map_struct_value(
            selfObject, View("TypedVectors"), 0u, &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_VECTOR3 ||
         property.value.vector3.x != 12.5 || property.value.vector3.y != -3.0 ||
         property.value.vector3.z != 8.25)) result = UEC_RESULT_INTERNAL_ERROR;

    vectorValue.kind = UEC_PROPERTY_STRUCT_VECTOR3;
    vectorValue.value.vector3 = (uec_vector3){-2.25, 4.5, 6.75};
    if (result == UEC_RESULT_OK) stage = "typed FVector map actor write";
    if (result == UEC_RESULT_OK)
        result = api->set_actor_property_map_struct_value(
            actor, View("TypedVectors"), 0u, &vectorValue);
    memset(&property, 0, sizeof(property));
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK)
        result = api->get_object_property_map_struct_value(
            selfObject, View("TypedVectors"), 0u, &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_VECTOR3 ||
         property.value.vector3.x != -2.25 || property.value.vector3.y != 4.5 ||
         property.value.vector3.z != 6.75)) result = UEC_RESULT_INTERNAL_ERROR;

    vectorValue.value.vector3 = (uec_vector3){7.5, 8.25, -9.0};
    if (result == UEC_RESULT_OK) stage = "typed FVector map UObject write";
    if (result == UEC_RESULT_OK)
        result = api->set_object_property_map_struct_value(
            selfObject, View("TypedVectors"), 0u, &vectorValue);
    memset(&property, 0, sizeof(property));
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_map_struct_value(
            actor, View("TypedVectors"), 0u, &property);
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_VECTOR3 ||
         property.value.vector3.x != 7.5 || property.value.vector3.y != 8.25 ||
         property.value.vector3.z != -9.0)) result = UEC_RESULT_INTERNAL_ERROR;

    vectorValue.kind = UEC_PROPERTY_STRUCT_COLOR;
    if (result == UEC_RESULT_OK &&
        api->set_actor_property_map_struct_value(
            actor, View("TypedVectors"), 0u, &vectorValue) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    property.kind = UEC_PROPERTY_STRUCT_VECTOR3;
    if (result == UEC_RESULT_OK &&
        api->get_actor_property_map_struct_value(
            actor, View("TypedVectors"), 1u, &property) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK && property.kind != UEC_PROPERTY_STRUCT_NONE)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK &&
        api->get_actor_property_map_struct_value(
            actor, View("Counts"), 0u, &property) != UEC_RESULT_UNSUPPORTED)
        result = UEC_RESULT_INTERNAL_ERROR;

    uint32_t setCount = 0u;
    if (result == UEC_RESULT_OK) stage = "typed FGuid set count";
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_set_count(actor, View("TypedIds"), &setCount);
    if (result == UEC_RESULT_OK && setCount != 2u) result = UEC_RESULT_INTERNAL_ERROR;

    memset(&property, 0, sizeof(property));
    property.struct_size = sizeof(property);
    if (result == UEC_RESULT_OK) stage = "typed FGuid set actor read";
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_set_element_struct_value(
            actor, View("TypedIds"), 0u, &property);
    if (result == UEC_RESULT_OK && property.kind != UEC_PROPERTY_STRUCT_GUID)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK &&
        !MatchesGuid(&property.value.guid, &initialSetIds[0]) &&
        !MatchesGuid(&property.value.guid, &initialSetIds[1]))
        result = UEC_RESULT_INTERNAL_ERROR;

    uec_property_struct_value setGuidValue = {0};
    setGuidValue.struct_size = sizeof(setGuidValue);
    setGuidValue.kind = UEC_PROPERTY_STRUCT_GUID;
    setGuidValue.value.guid = updatedActorSetId;
    if (result == UEC_RESULT_OK) stage = "typed FGuid set actor write";
    if (result == UEC_RESULT_OK)
        result = api->set_actor_property_set_element_struct_value(
            actor, View("TypedIds"), 0u, &setGuidValue);
    bool foundGuid = false;
    for (uint32_t index = 0u; result == UEC_RESULT_OK && index < setCount; ++index) {
        memset(&property, 0, sizeof(property));
        property.struct_size = sizeof(property);
        result = api->get_object_property_set_element_struct_value(
            selfObject, View("TypedIds"), index, &property);
        if (result == UEC_RESULT_OK && property.kind != UEC_PROPERTY_STRUCT_GUID)
            result = UEC_RESULT_INTERNAL_ERROR;
        if (result == UEC_RESULT_OK && MatchesGuid(&property.value.guid, &updatedActorSetId))
            foundGuid = true;
    }
    if (result == UEC_RESULT_OK && !foundGuid) result = UEC_RESULT_INTERNAL_ERROR;

    setGuidValue.value.guid = updatedObjectSetId;
    if (result == UEC_RESULT_OK) stage = "typed FGuid set UObject write";
    if (result == UEC_RESULT_OK)
        result = api->set_object_property_set_element_struct_value(
            selfObject, View("TypedIds"), 0u, &setGuidValue);
    foundGuid = false;
    for (uint32_t index = 0u; result == UEC_RESULT_OK && index < setCount; ++index) {
        memset(&property, 0, sizeof(property));
        property.struct_size = sizeof(property);
        result = api->get_actor_property_set_element_struct_value(
            actor, View("TypedIds"), index, &property);
        if (result == UEC_RESULT_OK && property.kind != UEC_PROPERTY_STRUCT_GUID)
            result = UEC_RESULT_INTERNAL_ERROR;
        if (result == UEC_RESULT_OK && MatchesGuid(&property.value.guid, &updatedObjectSetId))
            foundGuid = true;
    }
    if (result == UEC_RESULT_OK && !foundGuid) result = UEC_RESULT_INTERNAL_ERROR;

    uec_property_struct_value setBefore[2] = {{0}, {0}};
    for (uint32_t index = 0u; result == UEC_RESULT_OK && index < setCount; ++index) {
        setBefore[index].struct_size = sizeof(setBefore[index]);
        result = api->get_actor_property_set_element_struct_value(
            actor, View("TypedIds"), index, &setBefore[index]);
    }
    if (result == UEC_RESULT_OK &&
        (setBefore[0].kind != UEC_PROPERTY_STRUCT_GUID ||
         setBefore[1].kind != UEC_PROPERTY_STRUCT_GUID))
        result = UEC_RESULT_INTERNAL_ERROR;
    setGuidValue.value.guid = setBefore[1].value.guid;
    if (result == UEC_RESULT_OK) stage = "typed FGuid set duplicate rejection";
    if (result == UEC_RESULT_OK &&
        api->set_actor_property_set_element_struct_value(
            actor, View("TypedIds"), 0u, &setGuidValue) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    for (uint32_t index = 0u; result == UEC_RESULT_OK && index < setCount; ++index) {
        memset(&property, 0, sizeof(property));
        property.struct_size = sizeof(property);
        result = api->get_actor_property_set_element_struct_value(
            actor, View("TypedIds"), index, &property);
        if (result == UEC_RESULT_OK &&
            (property.kind != UEC_PROPERTY_STRUCT_GUID ||
             !MatchesGuid(&property.value.guid, &setBefore[index].value.guid)))
            result = UEC_RESULT_INTERNAL_ERROR;
    }

    setGuidValue.kind = UEC_PROPERTY_STRUCT_INT_VECTOR;
    setGuidValue.value.int_vector = (uec_int_vector){1, 2, 3};
    if (result == UEC_RESULT_OK &&
        api->set_actor_property_set_element_struct_value(
            actor, View("TypedIds"), 0u, &setGuidValue) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    property.kind = UEC_PROPERTY_STRUCT_GUID;
    property.value.guid = updatedObjectSetId;
    if (result == UEC_RESULT_OK &&
        api->get_actor_property_set_element_struct_value(
            actor, View("TypedIds"), setCount, &property) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK &&
        (property.kind != UEC_PROPERTY_STRUCT_NONE || property.value.guid.a != 0u))
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK &&
        api->get_actor_property_set_element_struct_value(
            actor, View("Values"), 0u, &property) != UEC_RESULT_UNSUPPORTED)
        result = UEC_RESULT_INTERNAL_ERROR;

    if (result != UEC_RESULT_OK && api != NULL && api->log != NULL && context != NULL)
        api->log(context, View(stage));
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
