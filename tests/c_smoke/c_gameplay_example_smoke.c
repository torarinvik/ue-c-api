#include "c_gameplay.h"

#include <stddef.h>

static uec_actor* gExpectedController;
static uec_object* gExpectedPlayerState;
static uec_actor* gExpectedPawn;
static uec_actor* gExpectedCharacter;
static uec_vector3 gExpectedMovementDirection;
static double gExpectedMovementScale;
static uec_bool gExpectedMovementForce;
static int gPlayerStateLookupCalls;
static int gMovementInputCalls;
static int gJumpCalls;
static int gStopJumpCalls;

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

static uec_result UEC_CALL MockAddPawnMovementInput(
    uec_actor* pawn, uec_vector3 direction, double scale, uec_bool force)
{
    ++gMovementInputCalls;
    if (pawn != gExpectedPawn || direction.x != gExpectedMovementDirection.x ||
        direction.y != gExpectedMovementDirection.y ||
        direction.z != gExpectedMovementDirection.z || scale != gExpectedMovementScale ||
        force != gExpectedMovementForce) return UEC_RESULT_INVALID_ARGUMENT;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockJumpCharacter(uec_actor* character)
{
    ++gJumpCalls;
    return character == gExpectedCharacter ? UEC_RESULT_OK : UEC_RESULT_INVALID_ARGUMENT;
}

static uec_result UEC_CALL MockStopCharacterJumping(uec_actor* character)
{
    ++gStopJumpCalls;
    return character == gExpectedCharacter ? UEC_RESULT_OK : UEC_RESULT_INVALID_ARGUMENT;
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
    {
        static char pawnStorage;
        static char characterStorage;
        uec_actor* pawn = (uec_actor*)&pawnStorage;
        uec_actor* character = (uec_actor*)&characterStorage;
        const uec_vector3 direction = {0.25, -1.0, 0.5};
        api.struct_size = (uint32_t)offsetof(uec_api, add_pawn_movement_input);
        if (uec_gameplay_apply_pawn_movement_input(
                &api, pawn, direction, -0.75, UEC_TRUE) != UEC_RESULT_UNSUPPORTED ||
            gMovementInputCalls != 0) return 7;

        api.struct_size = (uint32_t)sizeof(api);
        gExpectedPawn = pawn;
        gExpectedMovementDirection = direction;
        gExpectedMovementScale = -0.75;
        gExpectedMovementForce = UEC_TRUE;
        api.add_pawn_movement_input = &MockAddPawnMovementInput;
        if (uec_gameplay_apply_pawn_movement_input(
                &api, pawn, direction, -0.75, (uec_bool)2) !=
                UEC_RESULT_INVALID_ARGUMENT || gMovementInputCalls != 0) return 8;
        if (uec_gameplay_apply_pawn_movement_input(
                &api, pawn, direction, -0.75, UEC_TRUE) != UEC_RESULT_OK ||
            gMovementInputCalls != 1) return 9;

        gExpectedCharacter = character;
        api.jump_character = &MockJumpCharacter;
        api.stop_character_jumping = &MockStopCharacterJumping;
        api.struct_size = (uint32_t)offsetof(uec_api, stop_character_jumping);
        if (uec_gameplay_set_character_jump_pressed(
                &api, character, UEC_TRUE) != UEC_RESULT_UNSUPPORTED ||
            gJumpCalls != 0 || gStopJumpCalls != 0) return 10;

        api.struct_size = (uint32_t)sizeof(api);
        if (uec_gameplay_set_character_jump_pressed(
                &api, character, (uec_bool)2) != UEC_RESULT_INVALID_ARGUMENT ||
            gJumpCalls != 0 || gStopJumpCalls != 0) return 11;
        if (uec_gameplay_set_character_jump_pressed(
                &api, character, UEC_TRUE) != UEC_RESULT_OK || gJumpCalls != 1)
            return 12;
        if (uec_gameplay_set_character_jump_pressed(
                &api, character, UEC_FALSE) != UEC_RESULT_OK || gStopJumpCalls != 1)
            return 13;
    }
    return 0;
}
