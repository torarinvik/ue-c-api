#include "uec_api.h"

#include <string.h>

uec_result UEC_CALL uec_host_reflection_metadata_smoke(void)
{
    static const char actorClassPath[] = "/Script/Engine.Actor";
    static const char targetFunctionName[] = "K2_GetActorLocation";
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_class* actorClass = NULL;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    uint32_t functionCount = 0u;
    uint32_t targetFunctionIndex = UINT32_MAX;
    uec_bool targetHasReturnValue = UEC_FALSE;
    uec_bool targetIsLatent = UEC_TRUE;
    uint32_t targetParameterCount = 0u;

    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL || api->find_class == NULL ||
        api->release_class == NULL || api->release_context == NULL ||
        api->get_class_function_count == NULL || api->get_class_function_at == NULL ||
        api->get_class_function_flags == NULL ||
        api->get_class_function_parameter_at == NULL) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    result = api->find_class(context,
        (uec_string_view){actorClassPath, sizeof(actorClassPath) - 1u}, &actorClass);
    if (result != UEC_RESULT_OK || actorClass == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }
    result = api->get_class_function_count(actorClass, &functionCount);
    if (result != UEC_RESULT_OK || functionCount == 0u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    for (uint32_t index = 0u; index < functionCount; ++index) {
        char functionName[256] = {0};
        size_t requiredSize = 0u;
        uint32_t parameterCount = 0u;
        uec_bool hasReturnValue = UEC_FALSE;
        uec_bool isLatent = UEC_TRUE;
        result = api->get_class_function_at(actorClass, index, functionName,
            sizeof(functionName), &requiredSize, &parameterCount,
            &hasReturnValue, &isLatent);
        if (result != UEC_RESULT_OK) goto cleanup;
        if (strcmp(functionName, targetFunctionName) != 0) continue;
        targetFunctionIndex = index;
        targetHasReturnValue = hasReturnValue;
        targetIsLatent = isLatent;
        targetParameterCount = parameterCount;
        break;
    }
    if (targetFunctionIndex == UINT32_MAX || targetHasReturnValue != UEC_TRUE ||
        targetIsLatent != UEC_FALSE) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    {
        uint32_t functionFlags = 0u;
        result = api->get_class_function_flags(actorClass, targetFunctionIndex,
                                                &functionFlags);
        if (result != UEC_RESULT_OK ||
            (functionFlags & UEC_FUNCTION_FLAG_NATIVE) == 0u) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
    }

    /* get_class_function_at excludes ReturnValue from parameter_count, while
     * get_class_function_parameter_at follows reflected property order. */
    {
        const uint32_t reflectedPropertyCount = targetParameterCount +
            (targetHasReturnValue == UEC_TRUE ? 1u : 0u);
        uec_bool foundReturnValue = UEC_FALSE;
        for (uint32_t index = 0u; index < reflectedPropertyCount; ++index) {
            char parameterName[128] = {0};
            size_t requiredSize = 0u;
            uec_property_kind kind = UEC_PROPERTY_UNKNOWN;
            uint32_t flags = 0u;
            result = api->get_class_function_parameter_at(actorClass,
                targetFunctionIndex, index, parameterName, sizeof(parameterName),
                &requiredSize, &kind, &flags);
            if (result != UEC_RESULT_OK) goto cleanup;
            if ((flags & UEC_FUNCTION_PARAMETER_RETURN) != 0u) {
                foundReturnValue = kind == UEC_PROPERTY_STRUCT ? UEC_TRUE : UEC_FALSE;
                break;
            }
        }
        if (foundReturnValue != UEC_TRUE) {
            result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
    }
    result = UEC_RESULT_OK;

cleanup:
    if (actorClass != NULL && api != NULL && api->release_class != NULL) {
        const uec_result releaseResult = api->release_class(actorClass);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    if (context != NULL && api != NULL && api->release_context != NULL) {
        const uec_result releaseResult = api->release_context(context);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    return result;
}
