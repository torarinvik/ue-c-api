#include "uec_api.h"

int uec_physics_simulation_smoke_test(void)
{
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK || api == NULL || context == NULL) return 1;

    uec_bool simulating = UEC_TRUE;
    if (api->get_component_simulating_physics(NULL, &simulating) !=
            UEC_RESULT_INVALID_HANDLE || simulating != UEC_FALSE ||
        api->get_component_simulating_physics(NULL, NULL) != UEC_RESULT_INVALID_ARGUMENT ||
        api->set_component_simulating_physics(NULL, 2u) != UEC_RESULT_INVALID_ARGUMENT ||
        api->set_component_simulating_physics(NULL, UEC_FALSE) != UEC_RESULT_INVALID_HANDLE ||
        api->set_component_simulating_physics(NULL, UEC_TRUE) != UEC_RESULT_INVALID_HANDLE) {
        (void)api->release_context(context);
        return 2;
    }

    return api->release_context(context) == UEC_RESULT_OK ? 0 : 3;
}
