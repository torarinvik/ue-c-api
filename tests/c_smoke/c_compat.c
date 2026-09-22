#include "uec_api.h"

#include <stddef.h>

/* This fixture models a consumer compiled before the append-only extensions.
 * It requests the bootstrap prefix and never dereferences newer fields. */
enum { UEC_COMPAT_MINOR = 0u };

_Static_assert(offsetof(uec_api, get_capabilities) > offsetof(uec_api, abi_minor),
               "compatibility prefix must contain bootstrap fields");
_Static_assert(offsetof(uec_api, get_world_game_state) >
                   offsetof(uec_api, get_world_game_instance),
               "new fields must remain append-only");

int main(void)
{
    const uec_api* api = NULL;
    uec_context* context = NULL;
    if (uec_get_api(UEC_ABI_MAJOR, UEC_COMPAT_MINOR, &api, &context) != UEC_RESULT_OK) {
        return 1;
    }
    if (api == NULL || context == NULL || api->abi_major != UEC_ABI_MAJOR ||
        api->abi_minor < UEC_COMPAT_MINOR ||
        api->struct_size < offsetof(uec_api, get_capabilities) + sizeof(api->get_capabilities)) {
        return 2;
    }
    return 0;
}
