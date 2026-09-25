#include "uec_api.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace
{
    uec_string_view View(const char* text)
    {
        return {text, std::strlen(text)};
    }

    uec_property_value IntegerValue(int64_t value)
    {
        uec_property_value result{};
        result.struct_size = sizeof(result);
        result.kind = UEC_PROPERTY_INTEGER;
        result.integer_value = value;
        return result;
    }

    uec_property_value RealValue(double value)
    {
        uec_property_value result{};
        result.struct_size = sizeof(result);
        result.kind = UEC_PROPERTY_DOUBLE;
        result.real_value = value;
        return result;
    }

    bool IsInteger(const uec_property_value& value, int64_t expected)
    {
        return value.struct_size >= sizeof(value) &&
            value.kind == UEC_PROPERTY_INTEGER && value.integer_value == expected;
    }

    bool IsReal(const uec_property_value& value, double expected)
    {
        return value.struct_size >= sizeof(value) &&
            (value.kind == UEC_PROPERTY_FLOAT || value.kind == UEC_PROPERTY_DOUBLE) &&
            std::fabs(value.real_value - expected) <= 0.0001;
    }

    uec_result ReleaseActor(const uec_api* api, uec_actor*& actor)
    {
        if (actor == nullptr) return UEC_RESULT_OK;
        const uec_result destroyResult = api->destroy_actor(actor);
        if (destroyResult == UEC_RESULT_OK)
        {
            actor = nullptr;
            return UEC_RESULT_OK;
        }
        const uec_result releaseResult = api->release_actor(actor);
        actor = nullptr;
        return releaseResult == UEC_RESULT_OK ? destroyResult : releaseResult;
    }
}

extern "C" uec_result UEC_CALL uec_host_reflection_containers_smoke(void)
{
    static constexpr char classPathData[] =
        "/Script/UnrealCAPIHost.UECAPIHostReflectionSmokeActor";
    static constexpr char numbersData[] = "Numbers";
    static constexpr char countsData[] = "Counts";
    static constexpr char valuesData[] = "Values";
    static constexpr char positionData[] = "Position";
    static constexpr char yData[] = "Y";

    const uec_api* api = nullptr;
    uec_context* context = nullptr;
    uec_world* world = nullptr;
    uec_actor* actor = nullptr;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == nullptr || context == nullptr || api->get_default_world == nullptr ||
        api->release_world == nullptr || api->release_context == nullptr ||
        api->spawn_actor == nullptr || api->destroy_actor == nullptr ||
        api->release_actor == nullptr || api->get_actor_property_array_count == nullptr ||
        api->get_actor_property_array_element_value == nullptr ||
        api->set_actor_property_array_element_value == nullptr ||
        api->set_actor_property_array_element_text == nullptr ||
        api->get_actor_property_map_count == nullptr || api->get_actor_property_map_key == nullptr ||
        api->get_actor_property_map_value == nullptr ||
        api->get_actor_property_map_entry_text == nullptr ||
        api->set_actor_property_map_value == nullptr ||
        api->set_actor_property_map_value_text == nullptr ||
        api->get_actor_property_set_count == nullptr ||
        api->get_actor_property_set_element_value == nullptr ||
        api->get_actor_property_set_element_text == nullptr ||
        api->set_actor_property_set_element_value == nullptr ||
        api->set_actor_property_set_element_text == nullptr ||
        api->get_actor_property_struct_field_value == nullptr ||
        api->get_actor_property_struct_field_text == nullptr ||
        api->set_actor_property_struct_field_value == nullptr ||
        api->set_actor_property_struct_field_text == nullptr)
    {
        result = UEC_RESULT_INTERNAL_ERROR;
    }

    const uec_transform transform{
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    if (result == UEC_RESULT_OK) result = api->get_default_world(context, &world);
    if (result == UEC_RESULT_OK && world == nullptr) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) {
        result = api->spawn_actor(world, View(classPathData), &transform, &actor);
    }
    if (result == UEC_RESULT_OK && actor == nullptr) result = UEC_RESULT_INTERNAL_ERROR;

    uint32_t count = 0;
    uec_property_value observed{};
    observed.struct_size = sizeof(observed);
    if (result == UEC_RESULT_OK) result = api->get_actor_property_array_count(
        actor, View(numbersData), &count);
    if (result == UEC_RESULT_OK && count != 2u) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) result = api->get_actor_property_array_element_value(
        actor, View(numbersData), 0u, &observed);
    if (result == UEC_RESULT_OK && !IsInteger(observed, 3)) result = UEC_RESULT_INTERNAL_ERROR;
    const uec_property_value arrayValue = IntegerValue(23);
    if (result == UEC_RESULT_OK) result = api->set_actor_property_array_element_value(
        actor, View(numbersData), 1u, &arrayValue);
    if (result == UEC_RESULT_OK) result = api->set_actor_property_array_element_text(
        actor, View(numbersData), 0u, View("31"));
    if (result == UEC_RESULT_OK) result = api->get_actor_property_array_element_value(
        actor, View(numbersData), 0u, &observed);
    if (result == UEC_RESULT_OK && !IsInteger(observed, 31)) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) result = api->get_actor_property_array_element_value(
        actor, View(numbersData), 1u, &observed);
    if (result == UEC_RESULT_OK && !IsInteger(observed, 23)) result = UEC_RESULT_INTERNAL_ERROR;
    uint32_t mapIndex = UINT32_MAX;
    if (result == UEC_RESULT_OK) result = api->get_actor_property_map_count(
        actor, View(countsData), &count);
    if (result == UEC_RESULT_OK && count != 2u) result = UEC_RESULT_INTERNAL_ERROR;
    for (uint32_t index = 0u; result == UEC_RESULT_OK && index < count; ++index)
    {
        uec_property_value key{};
        key.struct_size = sizeof(key);
        result = api->get_actor_property_map_key(actor, View(countsData), index, &key);
        if (result == UEC_RESULT_OK && key.kind == UEC_PROPERTY_INTEGER &&
            key.integer_value == 7) {
            mapIndex = index;
        }
    }
    if (result == UEC_RESULT_OK && mapIndex == UINT32_MAX) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) result = api->get_actor_property_map_value(
        actor, View(countsData), mapIndex, &observed);
    if (result == UEC_RESULT_OK && !IsInteger(observed, 70)) result = UEC_RESULT_INTERNAL_ERROR;
    const uec_property_value mapValue = IntegerValue(75);
    if (result == UEC_RESULT_OK) result = api->set_actor_property_map_value(
        actor, View(countsData), mapIndex, &mapValue);
    if (result == UEC_RESULT_OK) result = api->set_actor_property_map_value_text(
        actor, View(countsData), mapIndex, View("77"));
    if (result == UEC_RESULT_OK) result = api->get_actor_property_map_value(
        actor, View(countsData), mapIndex, &observed);
    if (result == UEC_RESULT_OK && !IsInteger(observed, 77)) result = UEC_RESULT_INTERNAL_ERROR;
    char keyBuffer[32] = {};
    char valueBuffer[32] = {};
    uec_text_output keyText{sizeof(uec_text_output), UEC_PROPERTY_UNKNOWN,
                            keyBuffer, sizeof(keyBuffer), 0u};
    uec_text_output valueText{sizeof(uec_text_output), UEC_PROPERTY_UNKNOWN,
                              valueBuffer, sizeof(valueBuffer), 0u};
    if (result == UEC_RESULT_OK) result = api->get_actor_property_map_entry_text(
        actor, View(countsData), mapIndex, &keyText, &valueText);
    if (result == UEC_RESULT_OK &&
        (std::strcmp(keyBuffer, "7") != 0 || std::strcmp(valueBuffer, "77") != 0 ||
         keyText.kind != UEC_PROPERTY_INTEGER || valueText.kind != UEC_PROPERTY_INTEGER)) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }

    uint32_t setIndex = UINT32_MAX;
    if (result == UEC_RESULT_OK) result = api->get_actor_property_set_count(
        actor, View(valuesData), &count);
    if (result == UEC_RESULT_OK && count != 2u) result = UEC_RESULT_INTERNAL_ERROR;
    for (uint32_t index = 0u; result == UEC_RESULT_OK && index < count; ++index)
    {
        result = api->get_actor_property_set_element_value(
            actor, View(valuesData), index, &observed);
        if (result == UEC_RESULT_OK && IsInteger(observed, 13)) setIndex = index;
    }
    if (result == UEC_RESULT_OK && setIndex == UINT32_MAX) result = UEC_RESULT_INTERNAL_ERROR;
    const uec_property_value duplicateSetValue = IntegerValue(17);
    if (result == UEC_RESULT_OK && api->set_actor_property_set_element_value(
            actor, View(valuesData), setIndex, &duplicateSetValue) != UEC_RESULT_INVALID_ARGUMENT) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK) result = api->set_actor_property_set_element_text(
        actor, View(valuesData), setIndex, View("19"));
    if (result == UEC_RESULT_OK) result = api->get_actor_property_set_count(
        actor, View(valuesData), &count);
    if (result == UEC_RESULT_OK && count != 2u) result = UEC_RESULT_INTERNAL_ERROR;
    bool foundNineteen = false;
    bool foundSeventeen = false;
    for (uint32_t index = 0u; result == UEC_RESULT_OK && index < count; ++index)
    {
        result = api->get_actor_property_set_element_value(
            actor, View(valuesData), index, &observed);
        if (result == UEC_RESULT_OK && observed.kind == UEC_PROPERTY_INTEGER) {
            foundNineteen |= observed.integer_value == 19;
            foundSeventeen |= observed.integer_value == 17;
        }
    }
    if (result == UEC_RESULT_OK && (!foundNineteen || !foundSeventeen)) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    char setBuffer[32] = {};
    uec_text_output setText{sizeof(uec_text_output), UEC_PROPERTY_UNKNOWN,
                            setBuffer, sizeof(setBuffer), 0u};
    if (result == UEC_RESULT_OK) result = api->get_actor_property_set_element_text(
        actor, View(valuesData), setIndex, &setText);
    if (result == UEC_RESULT_OK &&
        (std::strcmp(setBuffer, "19") != 0 || setText.kind != UEC_PROPERTY_INTEGER)) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    const uec_string_view position = View(positionData);
    const uec_string_view y = View(yData);
    if (result == UEC_RESULT_OK) result = api->get_actor_property_struct_field_value(
        actor, position, y, &observed);
    if (result == UEC_RESULT_OK && !IsReal(observed, 2.0)) result = UEC_RESULT_INTERNAL_ERROR;
    const uec_property_value positionValue = RealValue(42.5);
    if (result == UEC_RESULT_OK) result = api->set_actor_property_struct_field_value(
        actor, position, y, &positionValue);
    char positionBuffer[64] = {};
    size_t positionRequired = 0u;
    uec_property_kind positionKind = UEC_PROPERTY_UNKNOWN;
    if (result == UEC_RESULT_OK) result = api->get_actor_property_struct_field_text(
        actor, position, y, positionBuffer, sizeof(positionBuffer),
        &positionRequired, &positionKind);
    if (result == UEC_RESULT_OK &&
        (positionKind != UEC_PROPERTY_DOUBLE || positionRequired > sizeof(positionBuffer) ||
         std::fabs(std::strtod(positionBuffer, nullptr) - 42.5) > 0.0001)) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK) result = api->set_actor_property_struct_field_text(
        actor, position, y, View("17.25"));
    if (result == UEC_RESULT_OK) result = api->get_actor_property_struct_field_value(
        actor, position, y, &observed);
    if (result == UEC_RESULT_OK && !IsReal(observed, 17.25)) result = UEC_RESULT_INTERNAL_ERROR;
    if (actor != nullptr) {
        const uec_result cleanup = ReleaseActor(api, actor);
        if (result == UEC_RESULT_OK && cleanup != UEC_RESULT_OK) result = cleanup;
    }
    if (world != nullptr) {
        const uec_result cleanup = api->release_world(world);
        if (result == UEC_RESULT_OK && cleanup != UEC_RESULT_OK) result = cleanup;
    }
    if (context != nullptr) {
        const uec_result cleanup = api->release_context(context);
        if (result == UEC_RESULT_OK && cleanup != UEC_RESULT_OK) result = cleanup;
    }
    return result;
}
