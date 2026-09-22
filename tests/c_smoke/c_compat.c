#include "uec_api.h"

#include <stddef.h>

/* Model a 1.132 consumer checking compatibility with the 1.133 bridge. It
 * requests the last published minor and never dereferences newer fields. */
enum { UEC_COMPAT_MINOR = 132u };

/* Keep this prefix deliberately independent from the current uec_api layout.
 * It represents the fields an old consumer needs to bootstrap and release a
 * context without naming any later extension. */
typedef struct uec_api_compat_prefix {
    uint32_t struct_size;
    uint32_t abi_major;
    uint32_t abi_minor;
    uec_result (UEC_CALL *get_capabilities)(uec_context* context,
                                             uec_capabilities* out_capabilities);
    uec_result (UEC_CALL *get_last_error)(uec_context* context,
                                           char* buffer,
                                           size_t buffer_size,
                                           size_t* required_size);
    uec_result (UEC_CALL *log)(uec_context* context, uec_string_view message);
    uec_result (UEC_CALL *release_context)(uec_context* context);
} uec_api_compat_prefix;

_Static_assert(offsetof(uec_api_compat_prefix, get_capabilities) >
                   offsetof(uec_api_compat_prefix, abi_minor),
               "compatibility prefix must contain bootstrap fields");

int main(void)
{
    const uec_api* api = NULL;
    uec_context* context = NULL;
    if (uec_get_api(UEC_ABI_MAJOR, UEC_COMPAT_MINOR, &api, &context) != UEC_RESULT_OK) {
        return 1;
    }
    if (api == NULL || context == NULL) {
        return 2;
    }
    const uec_api_compat_prefix* prefix = (const uec_api_compat_prefix*)api;
    if (prefix->abi_major != UEC_ABI_MAJOR || prefix->abi_minor < UEC_COMPAT_MINOR ||
        prefix->struct_size < offsetof(uec_api_compat_prefix, get_capabilities) +
            sizeof(prefix->get_capabilities) || prefix->get_capabilities == NULL ||
        prefix->release_context == NULL) {
        return 3;
    }
    uec_capabilities capabilities = 0;
    if (prefix->get_capabilities(context, &capabilities) != UEC_RESULT_OK ||
        (capabilities & UEC_CAPABILITY_BOOTSTRAP) == 0) {
        prefix->release_context(context);
        return 4;
    }
    return prefix->release_context(context) == UEC_RESULT_OK ? 0 : 5;
}
