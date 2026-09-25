#include "CoreMinimal.h"
#include "UObject/GarbageCollection.h"

#include "uec_api.h"

namespace
{
    uec_result CheckObjectType(const uec_api* api, uec_object* object,
                               uec_result expectedResult)
    {
        const char classPathData[] = "/Script/Engine.SaveGame";
        const uec_string_view classPath{classPathData, sizeof(classPathData) - 1u};
        uec_bool isSaveGame = UEC_TRUE;
        const uec_result result = api->object_is_a(object, classPath, &isSaveGame);
        if (result != expectedResult) return UEC_RESULT_INTERNAL_ERROR;
        if (expectedResult == UEC_RESULT_OK && isSaveGame != UEC_TRUE) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        if (expectedResult != UEC_RESULT_OK && isSaveGame != UEC_FALSE) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        return UEC_RESULT_OK;
    }
}

extern "C" uec_result UEC_CALL uec_host_gc_smoke(void)
{
    const uec_api* api = nullptr;
    uec_context* context = nullptr;
    uec_object* weakObject = nullptr;
    uec_object* retainedObject = nullptr;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;

    if (api == nullptr || context == nullptr || api->create_save_game == nullptr ||
        api->retain_object == nullptr || api->object_is_a == nullptr ||
        api->release_object == nullptr || api->release_context == nullptr) {
        result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    {
        const char classPathData[] = "/Script/UnrealCAPIHost.UECAPIHostSaveGame";
        const uec_string_view classPath{classPathData, sizeof(classPathData) - 1u};
        result = api->create_save_game(context, classPath, &weakObject);
        if (result != UEC_RESULT_OK || weakObject == nullptr) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            goto cleanup;
        }
    }

    result = api->retain_object(weakObject, &retainedObject);
    if (result != UEC_RESULT_OK || retainedObject == nullptr) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        goto cleanup;
    }

    CollectGarbage(RF_NoFlags, true);
    result = CheckObjectType(api, weakObject, UEC_RESULT_OK);
    if (result != UEC_RESULT_OK) goto cleanup;
    result = CheckObjectType(api, retainedObject, UEC_RESULT_OK);
    if (result != UEC_RESULT_OK) goto cleanup;

    result = api->release_object(retainedObject);
    retainedObject = nullptr;
    if (result != UEC_RESULT_OK) goto cleanup;

    CollectGarbage(RF_NoFlags, true);
    result = CheckObjectType(api, weakObject, UEC_RESULT_INVALID_HANDLE);

cleanup:
    if (retainedObject != nullptr && api != nullptr && api->release_object != nullptr) {
        const uec_result releaseResult = api->release_object(retainedObject);
        if (result == UEC_RESULT_OK) result = releaseResult;
    }
    if (weakObject != nullptr && api != nullptr && api->release_object != nullptr) {
        const uec_result releaseResult = api->release_object(weakObject);
        if (result == UEC_RESULT_OK) result = releaseResult;
    }
    if (context != nullptr && api != nullptr && api->release_context != nullptr) {
        const uec_result releaseResult = api->release_context(context);
        if (result == UEC_RESULT_OK) result = releaseResult;
    }
    return result;
}
