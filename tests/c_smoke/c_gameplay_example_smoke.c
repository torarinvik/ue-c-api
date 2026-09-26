#include "c_gameplay.h"

#include <stddef.h>

static uec_actor* gExpectedController;
static uec_object* gExpectedPlayerState;
static int gPlayerStateLookupCalls;

static uec_result UEC_CALL MockGetControllerPlayerState(
    uec_actor* controller, uec_object** outPlayerState)
{
    ++gPlayerStateLookupCalls;
    if (outPlayerState == NULL || controller != gExpectedController) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    *outPlayerState = gExpectedPlayerState;
    return UEC_RESULT_OK;
}

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
    {
        static char controllerStorage;
        static char playerStateStorage;
        uec_actor* controller = (uec_actor*)&controllerStorage;
        uec_object* expectedPlayerState = (uec_object*)&playerStateStorage;
        uec_object* playerState = expectedPlayerState;
        api.struct_size = (uint32_t)offsetof(uec_api, get_controller_player_state);
        if (uec_gameplay_get_player_state(&api, controller, &playerState) !=
                UEC_RESULT_UNSUPPORTED || playerState != NULL ||
            gPlayerStateLookupCalls != 0) return 4;

        api.struct_size = (uint32_t)sizeof(api);
        playerState = expectedPlayerState;
        if (uec_gameplay_get_player_state(&api, controller, &playerState) !=
                UEC_RESULT_UNSUPPORTED || playerState != NULL ||
            gPlayerStateLookupCalls != 0) return 5;

        gExpectedController = controller;
        gExpectedPlayerState = expectedPlayerState;
        api.get_controller_player_state = &MockGetControllerPlayerState;
        if (uec_gameplay_get_player_state(&api, controller, &playerState) !=
                UEC_RESULT_OK || playerState != expectedPlayerState ||
            gPlayerStateLookupCalls != 1) return 6;
    }
    return 0;
}
