#include "c_gameplay.h"

#include <stddef.h>

int uec_gameplay_example_table_smoke(void)
{
    static char contextStorage;
    static const char classPath[] = "/Script/Engine.Actor";
    const uec_string_view path = {classPath, sizeof(classPath) - 1u};
    const uec_transform transform = {0};
    uec_gameplay_example_state state = {0};
    uec_api api = {0};

    api.struct_size = (uint32_t)offsetof(uec_api, emit_actor_event_bridge);
    if (uec_gameplay_example_start(&api, (uec_context*)&contextStorage, path,
                                   &transform, &state) != UEC_RESULT_UNSUPPORTED ||
        state.done != UEC_TRUE || state.last_result != UEC_RESULT_UNSUPPORTED) {
        return 1;
    }
    api.struct_size = (uint32_t)sizeof(api);
    if (uec_gameplay_example_start(&api, (uec_context*)&contextStorage, path,
                                   &transform, &state) != UEC_RESULT_UNSUPPORTED ||
        state.done != UEC_TRUE || state.last_result != UEC_RESULT_UNSUPPORTED) {
        return 2;
    }
    if (uec_gameplay_example_start(NULL, (uec_context*)&contextStorage, path,
                                   &transform, &state) != UEC_RESULT_INVALID_ARGUMENT) {
        return 3;
    }
    return 0;
}
