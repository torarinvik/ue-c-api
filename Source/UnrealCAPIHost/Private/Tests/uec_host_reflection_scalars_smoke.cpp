#include "uec_api.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
    uec_string_view View(const char* text)
    {
        return {text, std::strlen(text)};
    }

    uec_property_value BooleanValue(uec_bool value)
    {
        uec_property_value result{};
        result.struct_size = sizeof(result);
        result.kind = UEC_PROPERTY_BOOL;
        result.bool_value = value;
        return result;
    }

    uec_property_value IntegerValue(int64_t value, uec_property_kind kind)
    {
        uec_property_value result{};
        result.struct_size = sizeof(result);
        result.kind = kind;
        result.integer_value = value;
        return result;
    }

    uec_property_value RealValue(double value)
    {
        uec_property_value result{};
        result.struct_size = sizeof(result);
        result.kind = UEC_PROPERTY_FLOAT;
        result.real_value = value;
        return result;
    }

    uec_result ReadValue(const uec_api* api,
                         uec_actor* actor,
                         const char* propertyName,
                         uec_property_kind expectedKind,
                         uec_property_value& outValue)
    {
        outValue = {};
        outValue.struct_size = sizeof(outValue);
        const uec_result result = api->get_actor_property_value(
            actor, View(propertyName), &outValue);
        if (result != UEC_RESULT_OK) return result;
        return outValue.kind == expectedKind ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
    }

    uec_result CheckTextProperty(const uec_api* api,
                                 uec_context* context,
                                 uec_actor* actor,
                                 const char* propertyName,
                                 uec_property_kind expectedKind,
                                 const char* initialValue,
                                 const char* updatedValue)
    {
        char buffer[128] = {};
        size_t requiredSize = 0u;
        uec_property_kind kind = UEC_PROPERTY_UNKNOWN;
        uec_result result = api->get_actor_property_string(
            actor, View(propertyName), buffer, sizeof(buffer), &requiredSize, &kind);
        if (result != UEC_RESULT_OK) return result;
        if (kind != expectedKind || std::strcmp(buffer, initialValue) != 0 ||
            requiredSize != std::strlen(initialValue) + 1u) {
            api->log(context, View("reflected text initial value mismatch"));
            return UEC_RESULT_INTERNAL_ERROR;
        }

        result = api->set_actor_property_string(
            actor, View(propertyName), View(updatedValue));
        if (result != UEC_RESULT_OK) return result;

        requiredSize = 0u;
        kind = UEC_PROPERTY_UNKNOWN;
        result = api->get_actor_property_string(
            actor, View(propertyName), nullptr, 0u, &requiredSize, &kind);
        if (result != UEC_RESULT_BUFFER_TOO_SMALL) return result;
        if (kind != expectedKind || requiredSize != std::strlen(updatedValue) + 1u) {
            api->log(context, View("reflected text required-size mismatch"));
            return UEC_RESULT_INTERNAL_ERROR;
        }

        std::memset(buffer, 0, sizeof(buffer));
        result = api->get_actor_property_string(
            actor, View(propertyName), buffer, sizeof(buffer), &requiredSize, &kind);
        if (result != UEC_RESULT_OK) return result;
        if (kind != expectedKind || std::strcmp(buffer, updatedValue) != 0 ||
            requiredSize != std::strlen(updatedValue) + 1u) {
            api->log(context, View("reflected text readback mismatch"));
            return UEC_RESULT_INTERNAL_ERROR;
        }
        return UEC_RESULT_OK;
    }

    uec_result CheckSoftProperty(const uec_api* api,
                                 uec_context* context,
                                 uec_actor* actor,
                                 const char* propertyName,
                                 uec_property_kind kind,
                                 const char* initialPath,
                                 const char* updatedPath)
    {
        char buffer[256] = {};
        uec_text_output output{sizeof(uec_text_output), UEC_PROPERTY_UNKNOWN,
                               buffer, sizeof(buffer), 0u};
        uec_result result = api->get_actor_property_soft_value(
            actor, View(propertyName), &output);
        if (result != UEC_RESULT_OK) return result;
        if (output.kind != kind || std::strstr(buffer, initialPath) == nullptr ||
            output.required_size > sizeof(buffer)) {
            api->log(context, View("soft property initial path mismatch"));
            return UEC_RESULT_INTERNAL_ERROR;
        }

        char shortBuffer[1] = {};
        uec_text_output shortOutput{sizeof(uec_text_output), UEC_PROPERTY_UNKNOWN,
                                    shortBuffer, sizeof(shortBuffer), 0u};
        result = api->get_actor_property_soft_value(actor, View(propertyName), &shortOutput);
        if (result != UEC_RESULT_BUFFER_TOO_SMALL || shortOutput.kind != kind ||
            shortOutput.required_size != output.required_size) {
            api->log(context, View("soft property short-buffer result mismatch"));
            return UEC_RESULT_INTERNAL_ERROR;
        }

        const uec_property_kind wrongKind = kind == UEC_PROPERTY_SOFT_OBJECT ?
            UEC_PROPERTY_SOFT_CLASS : UEC_PROPERTY_SOFT_OBJECT;
        if (api->set_actor_property_soft_value(
                actor, View(propertyName), wrongKind, View(updatedPath)) !=
            UEC_RESULT_INVALID_ARGUMENT) {
            api->log(context, View("soft property accepted a mismatched kind"));
            return UEC_RESULT_INTERNAL_ERROR;
        }
        result = api->set_actor_property_soft_value(
            actor, View(propertyName), kind, View(updatedPath));
        if (result != UEC_RESULT_OK) return result;

        std::memset(buffer, 0, sizeof(buffer));
        output.kind = UEC_PROPERTY_UNKNOWN;
        output.required_size = 0u;
        result = api->get_actor_property_soft_value(actor, View(propertyName), &output);
        if (result != UEC_RESULT_OK) return result;
        if (output.kind != kind || std::strstr(buffer, updatedPath) == nullptr ||
            output.required_size > sizeof(buffer)) {
            api->log(context, View("soft property updated path mismatch"));
            return UEC_RESULT_INTERNAL_ERROR;
        }
        return UEC_RESULT_OK;
    }

    uec_result DestroyActor(const uec_api* api, uec_actor*& actor)
    {
        if (actor == nullptr) return UEC_RESULT_OK;
        const uec_result result = api->destroy_actor(actor);
        if (result == UEC_RESULT_OK) {
            actor = nullptr;
            return UEC_RESULT_OK;
        }
        const uec_result releaseResult = api->release_actor(actor);
        actor = nullptr;
        return releaseResult == UEC_RESULT_OK ? result : releaseResult;
    }
}

extern "C" uec_result UEC_CALL uec_host_reflection_scalars_smoke(void)
{
    static constexpr char classPathData[] =
        "/Script/UnrealCAPIHost.UECAPIHostReflectionSmokeActor";
    static constexpr char updatedLabel[] = u8"Crème brûlée";

    const uec_api* api = nullptr;
    uec_context* context = nullptr;
    uec_world* world = nullptr;
    uec_actor* actor = nullptr;
    const char* stage = "API bootstrap";
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == nullptr || context == nullptr || api->get_default_world == nullptr ||
        api->release_world == nullptr || api->release_context == nullptr ||
        api->spawn_actor == nullptr || api->destroy_actor == nullptr ||
        api->release_actor == nullptr || api->get_actor_property_value == nullptr ||
        api->set_actor_property_value == nullptr ||
        api->get_actor_property_string == nullptr ||
        api->set_actor_property_string == nullptr ||
        api->get_actor_property_soft_value == nullptr ||
        api->set_actor_property_soft_value == nullptr) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }

    const uec_transform transform{
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    if (result == UEC_RESULT_OK) stage = "spawn reflected test actor";
    if (result == UEC_RESULT_OK) result = api->get_default_world(context, &world);
    if (result == UEC_RESULT_OK && world == nullptr) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) {
        result = api->spawn_actor(world, View(classPathData), &transform, &actor);
    }
    if (result == UEC_RESULT_OK && actor == nullptr) result = UEC_RESULT_INTERNAL_ERROR;

    uec_property_value observed{};
    if (result == UEC_RESULT_OK) stage = "Boolean property";
    if (result == UEC_RESULT_OK) result = ReadValue(
        api, actor, "Enabled", UEC_PROPERTY_BOOL, observed);
    if (result == UEC_RESULT_OK && observed.bool_value != UEC_TRUE)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) {
        const uec_property_value updated = BooleanValue(UEC_FALSE);
        result = api->set_actor_property_value(actor, View("Enabled"), &updated);
    }
    if (result == UEC_RESULT_OK) result = ReadValue(
        api, actor, "Enabled", UEC_PROPERTY_BOOL, observed);
    if (result == UEC_RESULT_OK && observed.bool_value != UEC_FALSE)
        result = UEC_RESULT_INTERNAL_ERROR;

    if (result == UEC_RESULT_OK) stage = "integer property";
    if (result == UEC_RESULT_OK) result = ReadValue(
        api, actor, "Count", UEC_PROPERTY_INTEGER, observed);
    if (result == UEC_RESULT_OK && observed.integer_value != 7)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) {
        const uec_property_value wrongKind = RealValue(99.0);
        if (api->set_actor_property_value(actor, View("Count"), &wrongKind) !=
            UEC_RESULT_INVALID_ARGUMENT) result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK) {
        const uec_property_value updated = IntegerValue(-12, UEC_PROPERTY_INTEGER);
        result = api->set_actor_property_value(actor, View("Count"), &updated);
    }
    if (result == UEC_RESULT_OK) result = ReadValue(
        api, actor, "Count", UEC_PROPERTY_INTEGER, observed);
    if (result == UEC_RESULT_OK && observed.integer_value != -12)
        result = UEC_RESULT_INTERNAL_ERROR;

    if (result == UEC_RESULT_OK) stage = "enum property";
    if (result == UEC_RESULT_OK) result = ReadValue(
        api, actor, "Mode", UEC_PROPERTY_ENUM, observed);
    if (result == UEC_RESULT_OK && observed.integer_value != 0) {
        char diagnostic[96] = {};
        std::snprintf(diagnostic, sizeof(diagnostic),
            "initial enum value was %lld", static_cast<long long>(observed.integer_value));
        api->log(context, View(diagnostic));
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK) {
        const uec_property_value updated = IntegerValue(1, UEC_PROPERTY_ENUM);
        result = api->set_actor_property_value(actor, View("Mode"), &updated);
    }
    if (result == UEC_RESULT_OK) result = ReadValue(
        api, actor, "Mode", UEC_PROPERTY_ENUM, observed);
    if (result == UEC_RESULT_OK && observed.integer_value != 1) {
        char diagnostic[96] = {};
        std::snprintf(diagnostic, sizeof(diagnostic),
            "typed enum write yielded %lld", static_cast<long long>(observed.integer_value));
        api->log(context, View(diagnostic));
        result = UEC_RESULT_INTERNAL_ERROR;
    }

    if (result == UEC_RESULT_OK) stage = "float property";
    if (result == UEC_RESULT_OK) result = ReadValue(
        api, actor, "Ratio", UEC_PROPERTY_FLOAT, observed);
    if (result == UEC_RESULT_OK && std::fabs(observed.real_value - 1.25) > 0.0001)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) {
        const uec_property_value updated = RealValue(-3.5);
        result = api->set_actor_property_value(actor, View("Ratio"), &updated);
    }
    if (result == UEC_RESULT_OK) result = ReadValue(
        api, actor, "Ratio", UEC_PROPERTY_FLOAT, observed);
    if (result == UEC_RESULT_OK && std::fabs(observed.real_value + 3.5) > 0.0001)
        result = UEC_RESULT_INTERNAL_ERROR;

    if (result == UEC_RESULT_OK) stage = "FString property";
    if (result == UEC_RESULT_OK) result = CheckTextProperty(
        api, context, actor, "Label", UEC_PROPERTY_STRING, "initial label", updatedLabel);
    if (result == UEC_RESULT_OK) stage = "FName property";
    if (result == UEC_RESULT_OK) result = CheckTextProperty(
        api, context, actor, "Identifier", UEC_PROPERTY_NAME,
        "InitialName", "UpdatedName");
    if (result == UEC_RESULT_OK) stage = "FText property";
    if (result == UEC_RESULT_OK) result = CheckTextProperty(
        api, context, actor, "Description", UEC_PROPERTY_TEXT,
        "initial description", "updated description");

    if (result == UEC_RESULT_OK) stage = "enum string conversion";
    if (result == UEC_RESULT_OK) {
        const uec_property_value updated = IntegerValue(0, UEC_PROPERTY_ENUM);
        result = api->set_actor_property_value(actor, View("Mode"), &updated);
    }
    if (result == UEC_RESULT_OK) result = CheckTextProperty(
        api, context, actor, "Mode", UEC_PROPERTY_ENUM, "First", "Second");
    if (result == UEC_RESULT_OK) result = ReadValue(
        api, actor, "Mode", UEC_PROPERTY_ENUM, observed);
    if (result == UEC_RESULT_OK && observed.integer_value != 1) {
        char diagnostic[96] = {};
        std::snprintf(diagnostic, sizeof(diagnostic),
            "enum text conversion yielded value %lld",
            static_cast<long long>(observed.integer_value));
        api->log(context, View(diagnostic));
        result = UEC_RESULT_INTERNAL_ERROR;
    }

    if (result == UEC_RESULT_OK) stage = "soft object property";
    if (result == UEC_RESULT_OK) result = CheckSoftProperty(
        api, context, actor, "SoftMesh", UEC_PROPERTY_SOFT_OBJECT,
        "/Engine/BasicShapes/Cube.Cube", "/Engine/BasicShapes/Sphere.Sphere");
    if (result == UEC_RESULT_OK) stage = "soft class property";
    if (result == UEC_RESULT_OK) result = CheckSoftProperty(
        api, context, actor, "SoftActorClass", UEC_PROPERTY_SOFT_CLASS,
        "/Script/Engine.Actor", "/Script/Engine.Pawn");

    if (result != UEC_RESULT_OK && api != nullptr && api->log != nullptr &&
        context != nullptr) {
        api->log(context, View(stage));
    }
    if (actor != nullptr) {
        const uec_result cleanup = DestroyActor(api, actor);
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
