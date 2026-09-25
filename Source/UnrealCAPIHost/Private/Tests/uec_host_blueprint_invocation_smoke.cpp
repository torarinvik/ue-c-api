#include "uec_api.h"

#include <cstring>

namespace
{
    uec_string_view View(const char* text)
    {
        return {text, std::strlen(text)};
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

    const uec_transform transform{
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    if (result == UEC_RESULT_OK) result = api->get_default_world(context, &world);
    if (result == UEC_RESULT_OK && world == nullptr) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) {
        result = api->spawn_actor(world, View(classPathData), &transform, &actor);
    }
    if (result == UEC_RESULT_OK && actor == nullptr) result = UEC_RESULT_INTERNAL_ERROR;

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
