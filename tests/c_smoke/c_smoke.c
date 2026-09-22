#include "uec_api.h"

#include <stdio.h>
#include <stddef.h>

_Static_assert(sizeof(uec_vector3) == 24, "uec_vector3 ABI changed");
_Static_assert(sizeof(uec_quaternion) == 32, "uec_quaternion ABI changed");
_Static_assert(sizeof(uec_transform) == 80, "uec_transform ABI changed");
_Static_assert(offsetof(uec_api, get_capabilities) > offsetof(uec_api, abi_minor),
               "uec_api function table ordering changed");

int main(void)
{
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK || api == NULL || context == NULL)
    {
        return 1;
    }

    uec_capabilities capabilities = 0;
    result = api->get_capabilities(context, &capabilities);
    if (result != UEC_RESULT_OK || (capabilities & UEC_CAPABILITY_BOOTSTRAP) == 0 ||
        (capabilities & UEC_CAPABILITY_ACTORS) == 0 ||
        (capabilities & UEC_CAPABILITY_REFLECTION) != 0)
    {
        api->release_context(context);
        return 5;
    }

    const char message[] = "C ABI smoke test";
    result = api->log(context, (uec_string_view){message, sizeof(message) - 1u});
    if (result != UEC_RESULT_OK)
    {
        api->release_context(context);
        return 2;
    }

    char error[32];
    size_t required = 0;
    result = api->get_last_error(context, error, sizeof(error), &required);
    if (result != UEC_RESULT_OK || required == 0)
    {
        api->release_context(context);
        return 3;
    }

    if (api->struct_size < sizeof(uec_api) || api->abi_major != UEC_ABI_MAJOR)
    {
        api->release_context(context);
        return 6;
    }

    api->release_context(context);
    puts("uec C ABI smoke test passed");
    return 0;
}
