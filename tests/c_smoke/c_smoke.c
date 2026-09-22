#include "uec_api.h"

#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
#define UEC_TEST_ASSERT static_assert
#else
#define UEC_TEST_ASSERT _Static_assert
#endif

UEC_TEST_ASSERT(sizeof(uec_vector3) == 24, "uec_vector3 ABI changed");
UEC_TEST_ASSERT(sizeof(uec_quaternion) == 32, "uec_quaternion ABI changed");
UEC_TEST_ASSERT(sizeof(uec_transform) == 80, "uec_transform ABI changed");
UEC_TEST_ASSERT(sizeof(uec_property_value) == 32, "uec_property_value ABI changed");
UEC_TEST_ASSERT(offsetof(uec_api, get_capabilities) > offsetof(uec_api, abi_minor),
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
        (capabilities & UEC_CAPABILITY_REFLECTION) == 0 ||
        (capabilities & UEC_CAPABILITY_CLASS_METADATA) == 0)
    {
        api->release_context(context);
        return 5;
    }

    const char message[] = "C ABI smoke test";
    const uec_string_view message_view = {message, sizeof(message) - 1u};
    result = api->log(context, message_view);
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
