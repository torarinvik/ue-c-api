#include "uec_api.h"

#include <cstring>

namespace
{
    uec_string_view View(const char* text)
    {
        return {text, std::strlen(text)};
    }

    uec_result VerifyBlueprintFunctionMetadata(
        const uec_api* api, uec_context* context)
    {
        static constexpr char classPathData[] =
            "/Game/Tests/BP_UECAPIHostFunctionSmoke.BP_UECAPIHostFunctionSmoke_C";
        static constexpr char functionNameData[] = "BlueprintGeneratedSmokeCall";
        static constexpr char inputNameData[] = "Value";
        static constexpr char returnNameData[] = "ReturnValue";

        if (api->find_class == nullptr || api->release_class == nullptr ||
            api->get_class_function_count == nullptr ||
            api->get_class_function_at == nullptr ||
            api->get_class_function_flags == nullptr ||
            api->get_class_function_parameter_at == nullptr)
        {
            return UEC_RESULT_INTERNAL_ERROR;
        }

        uec_class* blueprintClass = nullptr;
        uec_result result = api->find_class(
            context, View(classPathData), &blueprintClass);
        if (result == UEC_RESULT_OK && blueprintClass == nullptr) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }

        uint32_t functionIndex = UINT32_MAX;
        uint32_t parameterCount = 0u;
        uec_bool hasReturnValue = UEC_FALSE;
        uec_bool isLatent = UEC_TRUE;
        if (result == UEC_RESULT_OK) {
            uint32_t classFunctionCount = 0u;
            result = api->get_class_function_count(blueprintClass, &classFunctionCount);
            if (result == UEC_RESULT_OK && classFunctionCount == 0u) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
            for (uint32_t index = 0u;
                 result == UEC_RESULT_OK && index < classFunctionCount; ++index)
            {
                char functionName[128] = {};
                size_t requiredSize = 0u;
                result = api->get_class_function_at(
                    blueprintClass, index, functionName, sizeof(functionName),
                    &requiredSize, &parameterCount, &hasReturnValue, &isLatent);
                if (result == UEC_RESULT_OK &&
                    std::strcmp(functionName, functionNameData) == 0)
                {
                    functionIndex = index;
                    break;
                }
            }
        }

        if (result == UEC_RESULT_OK &&
            (functionIndex == UINT32_MAX || parameterCount != 1u ||
             hasReturnValue != UEC_TRUE || isLatent != UEC_FALSE))
        {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            uint32_t functionFlags = 0u;
            result = api->get_class_function_flags(
                blueprintClass, functionIndex, &functionFlags);
            const uint32_t requiredFlags = UEC_FUNCTION_FLAG_BLUEPRINT_CALLABLE |
                UEC_FUNCTION_FLAG_EVENT;
            if (result == UEC_RESULT_OK &&
                (functionFlags & requiredFlags) != requiredFlags)
            {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }

        if (result == UEC_RESULT_OK) {
            bool foundInput = false;
            bool foundReturn = false;
            for (uint32_t index = 0u; result == UEC_RESULT_OK && index < 2u; ++index) {
                char parameterName[64] = {};
                size_t requiredSize = 0u;
                uec_property_kind kind = UEC_PROPERTY_UNKNOWN;
                uint32_t flags = 0u;
                result = api->get_class_function_parameter_at(
                    blueprintClass, functionIndex, index, parameterName,
                    sizeof(parameterName), &requiredSize, &kind, &flags);
                if (result == UEC_RESULT_OK) {
                    const bool isInput =
                        (flags & UEC_FUNCTION_PARAMETER_INPUT) != 0u;
                    const bool isReturn =
                        (flags & UEC_FUNCTION_PARAMETER_RETURN) != 0u;
                    if (kind != UEC_PROPERTY_INTEGER || isInput == isReturn ||
                        (isInput && std::strcmp(parameterName, inputNameData) != 0) ||
                        (isReturn && std::strcmp(parameterName, returnNameData) != 0))
                    {
                        result = UEC_RESULT_INTERNAL_ERROR;
                    }
                    foundInput |= isInput;
                    foundReturn |= isReturn;
                }
            }
            if (result == UEC_RESULT_OK && (!foundInput || !foundReturn)) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }

        if (blueprintClass != nullptr) {
            const uec_result releaseResult = api->release_class(blueprintClass);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
        }
        return result;
    }
}

extern "C" uec_result UEC_CALL uec_host_blueprint_invocation_smoke(void)
{
    static constexpr char classPathData[] =
        "/Game/Tests/BP_UECAPIHostFunctionSmoke.BP_UECAPIHostFunctionSmoke_C";
    static constexpr char functionNameData[] = "BlueprintGeneratedSmokeCall";

    const uec_api* api = nullptr;
    uec_context* context = nullptr;
    uec_world* world = nullptr;
    uec_actor* actor = nullptr;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;

    if (api == nullptr || context == nullptr) return UEC_RESULT_INTERNAL_ERROR;

    if (api->get_default_world == nullptr ||
        api->release_world == nullptr || api->release_context == nullptr ||
        api->spawn_actor == nullptr || api->destroy_actor == nullptr ||
        api->release_actor == nullptr || api->invoke_actor_function_value == nullptr)
    {
        if (api->release_context != nullptr) api->release_context(context);
        return UEC_RESULT_INTERNAL_ERROR;
    }

    result = VerifyBlueprintFunctionMetadata(api, context);

    const uec_transform transform{
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    if (result == UEC_RESULT_OK) result = api->get_default_world(context, &world);
    if (result == UEC_RESULT_OK && world == nullptr) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) {
        result = api->spawn_actor(world, View(classPathData), &transform, &actor);
    }
    if (result == UEC_RESULT_OK && actor == nullptr) result = UEC_RESULT_INTERNAL_ERROR;

    const uec_property_value mismatchedArgument{
        sizeof(uec_property_value), UEC_PROPERTY_DOUBLE, UEC_FALSE, {0u, 0u, 0u},
        0, 1.5};
    uec_property_value rejectedReturn{};
    rejectedReturn.struct_size = sizeof(rejectedReturn);
    rejectedReturn.kind = UEC_PROPERTY_INTEGER;
    rejectedReturn.integer_value = 99;
    if (result == UEC_RESULT_OK) {
        const uec_result rejectedResult = api->invoke_actor_function_value(
            actor, View(functionNameData), &mismatchedArgument, 1u, &rejectedReturn);
        if (rejectedResult != UEC_RESULT_INVALID_ARGUMENT ||
            rejectedReturn.kind != UEC_PROPERTY_UNKNOWN ||
            rejectedReturn.bool_value != UEC_FALSE ||
            rejectedReturn.integer_value != 0 || rejectedReturn.real_value != 0.0)
        {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }

    const uec_property_value argument{
        sizeof(uec_property_value), UEC_PROPERTY_INTEGER, UEC_FALSE, {0u, 0u, 0u},
        867, 0.0};
    uec_property_value returnValue{};
    returnValue.struct_size = sizeof(returnValue);
    if (result == UEC_RESULT_OK) {
        result = api->invoke_actor_function_value(
            actor, View(functionNameData), &argument, 1u, &returnValue);
    }
    if (result == UEC_RESULT_OK &&
        (returnValue.struct_size < sizeof(returnValue) ||
         returnValue.kind != UEC_PROPERTY_INTEGER || returnValue.integer_value != 868))
    {
        result = UEC_RESULT_INTERNAL_ERROR;
    }

    if (actor != nullptr) {
        const uec_result destroyResult = api->destroy_actor(actor);
        if (destroyResult != UEC_RESULT_OK) {
            const uec_result releaseResult = api->release_actor(actor);
            if (result == UEC_RESULT_OK) {
                result = releaseResult == UEC_RESULT_OK ? destroyResult : releaseResult;
            }
        }
    }
    if (world != nullptr) {
        const uec_result releaseResult = api->release_world(world);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (context != nullptr) {
        const uec_result releaseResult = api->release_context(context);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    return result;
}
