#include "uec_api.h"

uec_result UEC_CALL uec_host_smoke_bootstrap(void)
{
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;
    if (api == NULL || context == NULL ||
        api->struct_size < offsetof(uec_api, release_context) + sizeof(api->release_context) ||
        api->abi_major != UEC_ABI_MAJOR) {
        return UEC_RESULT_INTERNAL_ERROR;
    }

    uec_capabilities capabilities = 0;
    result = api->get_capabilities(context, &capabilities);
    if (result == UEC_RESULT_OK && (capabilities & UEC_CAPABILITY_BOOTSTRAP) == 0) {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK && api->struct_size >=
            offsetof(uec_api, log) + sizeof(api->log)) {
        const char message[] = "UnrealCAPI C host bootstrap reached the bridge";
        const uec_string_view view = {message, sizeof(message) - 1};
        result = api->log(context, view);
    }

    const uec_result release_result = api->release_context(context);
    return result == UEC_RESULT_OK ? release_result : result;
}
