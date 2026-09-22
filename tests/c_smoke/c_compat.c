#include "legacy/uec_api_abi_135.h"

#include <string.h>

#ifdef __cplusplus
static_assert(UEC_ABI_MINOR == 135u, "legacy fixture must remain ABI 1.135");
#else
_Static_assert(UEC_ABI_MINOR == 135u, "legacy fixture must remain ABI 1.135");
#endif

int main(void)
{
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    int status = 0;
    uec_capabilities capabilities = 0;
    const char slotName[] = "legacy-abi-135";
    const uec_string_view slot = {slotName, sizeof(slotName) - 1u};
    const uint8_t expected[] = {0x31u, 0x33u, 0x35u};
    uint8_t actual[sizeof(expected)] = {0u};
    uec_bool saved = UEC_FALSE;
    uint32_t schemaVersion = 0u;
    size_t requiredSize = 0u;
    if (result != UEC_RESULT_OK) return 1;
    if (api == NULL || context == NULL || api->struct_size < sizeof(*api) ||
        api->abi_major != UEC_ABI_MAJOR || api->abi_minor < UEC_ABI_MINOR ||
        api->get_capabilities == NULL || api->release_context == NULL ||
        api->save_versioned_application_data == NULL ||
        api->load_versioned_application_data == NULL) {
        return 2;
    }

    if (api->get_capabilities(context, &capabilities) != UEC_RESULT_OK ||
        (capabilities & UEC_CAPABILITY_BOOTSTRAP) == 0 ||
        (capabilities & UEC_CAPABILITY_SAVE_DATA) == 0) {
        status = 3;
        goto cleanup;
    }

    if (api->save_versioned_application_data(context, slot, 0, 135u,
            expected, sizeof(expected), &saved) != UEC_RESULT_OK || saved != UEC_TRUE) {
        status = 4;
        goto cleanup;
    }

    result = api->load_versioned_application_data(context, slot, 0,
        &schemaVersion, NULL, 0u, &requiredSize);
    if (result != UEC_RESULT_BUFFER_TOO_SMALL || schemaVersion != 135u ||
        requiredSize != sizeof(expected)) {
        status = 5;
        goto cleanup;
    }
    result = api->load_versioned_application_data(context, slot, 0,
        &schemaVersion, actual, sizeof(actual), &requiredSize);
    if (result != UEC_RESULT_OK || schemaVersion != 135u ||
        requiredSize != sizeof(expected) || memcmp(actual, expected, sizeof(expected)) != 0) {
        status = 6;
    }

cleanup:
    api->release_context(context);
    return status;
}
