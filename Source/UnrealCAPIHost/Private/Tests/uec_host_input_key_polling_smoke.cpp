#include "CoreMinimal.h"

#include "uec_api.h"

extern "C" uec_result UEC_CALL uec_host_input_key_polling_smoke(
    const uec_api* api,
    uec_actor* controller,
    uec_actor* input_actor,
    uec_object* expected_player_state)
{
    if (api == nullptr || controller == nullptr || input_actor == nullptr ||
        expected_player_state == nullptr || api->get_input_key_down == nullptr ||
        api->get_input_key_value == nullptr || api->get_controller_player_state == nullptr)
        return UEC_RESULT_UNSUPPORTED;

    static constexpr char digitalKeyData[] = "SpaceBar";
    static constexpr char analogKeyData[] = "Gamepad_LeftX";
    const uec_string_view digitalKey{digitalKeyData, sizeof(digitalKeyData) - 1u};
    const uec_string_view analogKey{analogKeyData, sizeof(analogKeyData) - 1u};
    const uec_string_view emptyKey{nullptr, 0u};

    uec_bool isDown = UEC_TRUE;
    uec_result result = api->get_input_key_down(controller, digitalKey, &isDown);
    if (result != UEC_RESULT_OK || (isDown != UEC_FALSE && isDown != UEC_TRUE))
        return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
    double analogValue = 0.0;
    result = api->get_input_key_value(controller, analogKey, &analogValue);
    if (result != UEC_RESULT_OK || !FMath::IsFinite(analogValue))
        return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;

    isDown = UEC_TRUE;
    result = api->get_input_key_down(controller, emptyKey, &isDown);
    if (result != UEC_RESULT_INVALID_ARGUMENT || isDown != UEC_FALSE)
        return UEC_RESULT_INTERNAL_ERROR;
    analogValue = 1.0;
    result = api->get_input_key_value(controller, emptyKey, &analogValue);
    if (result != UEC_RESULT_INVALID_ARGUMENT || analogValue != 0.0)
        return UEC_RESULT_INTERNAL_ERROR;

    isDown = UEC_TRUE;
    result = api->get_input_key_down(input_actor, digitalKey, &isDown);
    if (result != UEC_RESULT_INVALID_ARGUMENT || isDown != UEC_FALSE)
        return UEC_RESULT_INTERNAL_ERROR;
    analogValue = 1.0;
    result = api->get_input_key_value(input_actor, analogKey, &analogValue);
    if (result != UEC_RESULT_INVALID_ARGUMENT || analogValue != 0.0)
        return UEC_RESULT_INTERNAL_ERROR;
    uec_object* unexpectedPlayerState = expected_player_state;
    result = api->get_controller_player_state(input_actor, &unexpectedPlayerState);
    if (result != UEC_RESULT_INVALID_ARGUMENT || unexpectedPlayerState != nullptr)
        return UEC_RESULT_INTERNAL_ERROR;
    return UEC_RESULT_OK;
}
