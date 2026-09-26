#include "uec_api.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

uec_result UEC_CALL uec_host_widget_ui_smoke(void);

typedef struct uec_travel_smoke_state {
    const uec_api* api;
    uec_context* context;
    uec_world* old_world;
    uec_actor* controller;
    uec_actor* audio_actor;
    uec_object* player_state;
    uec_object* audio_component;
    uec_object* widget;
    uec_object* button;
    uec_object* editable_text_box;
    uec_object* slider;
    uec_object* combo_box;
    uint64_t request_id;
    uint64_t button_subscription_id;
    uint64_t audio_subscription_id;
    uint64_t audio_callback_count;
    uint32_t baseline_worlds;
    uint32_t baseline_pending_requests;
    uint32_t baseline_active_callbacks;
    uint32_t baseline_active_subscriptions;
    uec_result result;
    uec_bool submitted;
    uec_bool callback_received;
    uec_bool started;
    uec_bool complete;
} uec_travel_smoke_state;

static uec_travel_smoke_state g_travel_smoke_state;

static void UEC_CALL IgnoreTravelSmokeButtonClick(uint64_t subscriptionId,
                                                   void* userData)
{
    (void)subscriptionId;
    (void)userData;
}

static void UEC_CALL CountTravelSmokeAudioFinished(uint64_t subscriptionId,
                                                    void* userData)
{
    (void)subscriptionId;
    uec_travel_smoke_state* state = (uec_travel_smoke_state*)userData;
    if (state != NULL && state->audio_callback_count != UINT64_MAX) {
        state->audio_callback_count += 1u;
    }
}

static void FinishTravelSmoke(uec_travel_smoke_state* state,
                              uec_result result,
                              uec_bool cancel_request)
{
    if (state == NULL || state->complete == UEC_TRUE) return;
    if (cancel_request == UEC_TRUE && state->request_id != 0 &&
        state->callback_received != UEC_TRUE && state->api != NULL &&
        state->context != NULL && state->api->cancel_travel_request != NULL) {
        const uec_result cancelResult = state->api->cancel_travel_request(
            state->context, state->request_id);
        if (result == UEC_RESULT_OK && cancelResult != UEC_RESULT_OK) {
            result = cancelResult;
        }
    }
    state->request_id = 0;
    if (state->button_subscription_id != 0 && state->api != NULL &&
        state->context != NULL && state->api->unbind_button_clicked != NULL) {
        const uec_result unbindResult = state->api->unbind_button_clicked(
            state->context, state->button_subscription_id);
        const uec_result expectedResult = state->submitted == UEC_TRUE
            ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_OK;
        if (result == UEC_RESULT_OK && unbindResult != expectedResult) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    state->button_subscription_id = 0;
    if (state->audio_subscription_id != 0 && state->api != NULL &&
        state->context != NULL && state->api->unbind_audio_finished != NULL) {
        const uec_result unbindResult = state->api->unbind_audio_finished(
            state->context, state->audio_subscription_id);
        const uec_result expectedResult = state->submitted == UEC_TRUE
            ? UEC_RESULT_INVALID_ARGUMENT : UEC_RESULT_OK;
        if (result == UEC_RESULT_OK &&
            (unbindResult != expectedResult || state->audio_callback_count != 0u)) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    state->audio_subscription_id = 0;
    if (state->audio_component != NULL && state->api != NULL &&
        state->api->release_object != NULL) {
        const uec_result releaseResult = state->api->release_object(state->audio_component);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    state->audio_component = NULL;
    if (state->player_state != NULL && state->api != NULL &&
        state->api->release_object != NULL) {
        const uec_result releaseResult = state->api->release_object(state->player_state);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    state->player_state = NULL;
    if (state->controller != NULL && state->api != NULL &&
        state->api->release_actor != NULL) {
        const uec_result releaseResult = state->api->release_actor(state->controller);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    state->controller = NULL;
    if (state->audio_actor != NULL && state->api != NULL &&
        state->api->release_actor != NULL) {
        const uec_result releaseResult = state->api->release_actor(state->audio_actor);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    state->audio_actor = NULL;
    if (state->button != NULL && state->api != NULL &&
        state->api->release_object != NULL) {
        const uec_result releaseResult = state->api->release_object(state->button);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    state->button = NULL;
    if (state->editable_text_box != NULL && state->api != NULL &&
        state->api->release_object != NULL) {
        const uec_result releaseResult =
            state->api->release_object(state->editable_text_box);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    state->editable_text_box = NULL;
    if (state->slider != NULL && state->api != NULL &&
        state->api->release_object != NULL) {
        const uec_result releaseResult = state->api->release_object(state->slider);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    state->slider = NULL;
    if (state->combo_box != NULL && state->api != NULL &&
        state->api->release_object != NULL) {
        const uec_result releaseResult = state->api->release_object(state->combo_box);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    state->combo_box = NULL;
    if (state->widget != NULL && state->api != NULL &&
        state->api->release_object != NULL) {
        const uec_result releaseResult = state->api->release_object(state->widget);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
            result = releaseResult;
        }
    }
    state->widget = NULL;
    if (state->old_world != NULL && state->api != NULL) {
        if (state->api->release_world == NULL) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        } else {
            const uec_result releaseResult = state->api->release_world(state->old_world);
            const uec_bool alreadyInvalidated = state->submitted == UEC_TRUE &&
                releaseResult == UEC_RESULT_INVALID_HANDLE ? UEC_TRUE : UEC_FALSE;
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK &&
                alreadyInvalidated != UEC_TRUE) {
                result = releaseResult;
            }
        }
    }
    state->old_world = NULL;
    if (state->context != NULL && state->api != NULL) {
        if (state->api->release_context == NULL) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        } else {
            const uec_result releaseResult = state->api->release_context(state->context);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
        }
    }
    state->context = NULL;
    state->result = result;
    state->complete = UEC_TRUE;
}

static void UEC_CALL CompleteTravelSmoke(uint64_t requestId,
                                         uec_result result,
                                         uec_world* newWorld,
                                         void* userData)
{
    uec_travel_smoke_state* state = (uec_travel_smoke_state*)userData;
    if (state == NULL) return;
    if (state->complete == UEC_TRUE || state->callback_received == UEC_TRUE) {
        if (newWorld != NULL && state->api != NULL) {
            (void)state->api->release_world(newWorld);
        }
        return;
    }
    state->callback_received = UEC_TRUE;
    state->result = UEC_RESULT_INTERNAL_ERROR;
    if (requestId != state->request_id || result != UEC_RESULT_OK ||
        newWorld == NULL || state->api == NULL || state->context == NULL) {
        if (newWorld != NULL && state->api != NULL) {
            (void)state->api->release_world(newWorld);
        }
        return;
    }

    char mapName[256] = {0};
    size_t mapNameRequired = 0;
    const uec_result mapResult = state->api->get_world_name(
        newWorld, mapName, sizeof(mapName), &mapNameRequired);
    size_t staleWorldRequired = 1u;
    const uec_result staleWorldResult = state->api->get_world_name(
        state->old_world, NULL, 0, &staleWorldRequired);
    uec_runtime_stats stats = {0};
    stats.struct_size = sizeof(stats);
    const uec_result statsResult = state->api->get_runtime_stats(state->context, &stats);
    if (mapResult != UEC_RESULT_OK || mapNameRequired <= 1u || mapName[0] == '\0' ||
        staleWorldResult != UEC_RESULT_INVALID_HANDLE || staleWorldRequired != 0u ||
        statsResult != UEC_RESULT_OK ||
        state->baseline_active_callbacks == UINT32_MAX ||
        stats.active_callbacks != state->baseline_active_callbacks + 1u ||
        state->audio_callback_count != 0u ||
        stats.active_subscriptions != state->baseline_active_subscriptions ||
        stats.pending_requests != state->baseline_pending_requests ||
        stats.live_worlds != state->baseline_worlds + 1u) {
        (void)state->api->release_world(newWorld);
        return;
    }

    const uec_result releaseResult = state->api->release_world(newWorld);
    if (releaseResult != UEC_RESULT_OK) return;
    stats = (uec_runtime_stats){0};
    stats.struct_size = sizeof(stats);
    if (state->api->get_runtime_stats(state->context, &stats) != UEC_RESULT_OK ||
        stats.pending_requests != state->baseline_pending_requests ||
        stats.live_worlds != state->baseline_worlds ||
        stats.active_subscriptions != state->baseline_active_subscriptions) {
        return;
    }
    state->result = UEC_RESULT_OK;
}

uec_result UEC_CALL uec_host_travel_smoke_start(void)
{
    static const char targetMapPath[] = "/Engine/Maps/Templates/OpenWorld";
    uec_travel_smoke_state* state = &g_travel_smoke_state;
    if (state->started == UEC_TRUE) return UEC_RESULT_INVALID_ARGUMENT;
    const uec_result widgetResult = uec_host_widget_ui_smoke();
    if (widgetResult != UEC_RESULT_OK) return widgetResult;
    *state = (uec_travel_smoke_state){0};
    state->started = UEC_TRUE;
    state->result = UEC_RESULT_INTERNAL_ERROR;
    uec_runtime_stats baselineStats = {0};
    baselineStats.struct_size = sizeof(baselineStats);
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR,
                                    &state->api, &state->context);
    if (result != UEC_RESULT_OK) {
        FinishTravelSmoke(state, result, UEC_FALSE);
        return result;
    }
    if (state->api == NULL || state->context == NULL ||
        state->api->get_runtime_stats == NULL ||
        state->api->get_default_world == NULL ||
        state->api->get_first_player_controller == NULL ||
        state->api->get_controller_player_state == NULL ||
        state->api->object_is_a == NULL ||
        state->api->get_world_name == NULL || state->api->release_world == NULL ||
        state->api->create_widget == NULL || state->api->get_widget_child == NULL ||
        state->api->get_editable_text_box_text == NULL ||
        state->api->set_editable_text_box_text == NULL ||
        state->api->get_slider_value == NULL || state->api->set_slider_value == NULL ||
        state->api->get_combo_box_selected_option == NULL ||
        state->api->set_combo_box_selected_option == NULL ||
        state->api->get_combo_box_option_count == NULL ||
        state->api->get_combo_box_option_at == NULL ||
        state->api->add_combo_box_option == NULL ||
        state->api->remove_combo_box_option == NULL ||
        state->api->clear_combo_box_options == NULL ||
        state->api->add_widget_to_viewport == NULL ||
        state->api->spawn_actor == NULL || state->api->get_actor_property_object == NULL ||
        state->api->bind_audio_finished == NULL ||
        state->api->unbind_audio_finished == NULL ||
        state->api->get_audio_component_playing == NULL ||
        state->api->bind_button_clicked == NULL ||
        state->api->unbind_button_clicked == NULL ||
        state->api->release_actor == NULL || state->api->release_object == NULL ||
        state->api->travel_world_async == NULL ||
        state->api->cancel_travel_request == NULL ||
        state->api->release_context == NULL) {
        FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    result = state->api->get_runtime_stats(state->context, &baselineStats);
    if (result != UEC_RESULT_OK || baselineStats.live_worlds == UINT32_MAX ||
        baselineStats.pending_requests == UINT32_MAX ||
        baselineStats.active_subscriptions == UINT32_MAX) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        FinishTravelSmoke(state, result, UEC_FALSE);
        return result;
    }
    state->baseline_worlds = baselineStats.live_worlds;
    state->baseline_pending_requests = baselineStats.pending_requests;
    state->baseline_active_callbacks = baselineStats.active_callbacks;
    state->baseline_active_subscriptions = baselineStats.active_subscriptions;
    result = state->api->get_default_world(state->context, &state->old_world);
    if (result != UEC_RESULT_OK || state->old_world == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        FinishTravelSmoke(state, result, UEC_FALSE);
        return result;
    }
    result = state->api->get_first_player_controller(
        state->old_world, &state->controller);
    if (result == UEC_RESULT_OK) {
        result = state->api->get_controller_player_state(
            state->controller, &state->player_state);
    }
    static const char playerStateClassPathData[] = "/Script/Engine.PlayerState";
    const uec_string_view playerStateClassPath = {
        playerStateClassPathData, sizeof(playerStateClassPathData) - 1u};
    uec_bool isPlayerState = UEC_FALSE;
    if (result == UEC_RESULT_OK) {
        result = state->api->object_is_a(
            state->player_state, playerStateClassPath, &isPlayerState);
        if (result == UEC_RESULT_OK && isPlayerState != UEC_TRUE) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result != UEC_RESULT_OK || state->controller == NULL ||
        state->player_state == NULL) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_NOT_INITIALIZED;
        FinishTravelSmoke(state, result, UEC_FALSE);
        return result;
    }
    {
        static const char widgetClassPathData[] =
            "/Script/UnrealCAPIHost.ECAPIHostCleanupWidget";
        static const char buttonNameData[] = "CleanupButton";
        static const char editableTextBoxNameData[] = "CleanupEditableTextBox";
        static const char sliderNameData[] = "CleanupSlider";
        static const char comboBoxNameData[] = "CleanupComboBox";
        static const char comboOptionData[] = "High";
        static const char missingComboOptionData[] = "Ultra";
        static const char unicodeTextData[] = "Player – 世界 🌍";
        const uec_string_view widgetClassPath = {
            widgetClassPathData, sizeof(widgetClassPathData) - 1u};
        const uec_string_view buttonName = {
            buttonNameData, sizeof(buttonNameData) - 1u};
        const uec_string_view editableTextBoxName = {
            editableTextBoxNameData, sizeof(editableTextBoxNameData) - 1u};
        const uec_string_view sliderName = {
            sliderNameData, sizeof(sliderNameData) - 1u};
        const uec_string_view comboBoxName = {
            comboBoxNameData, sizeof(comboBoxNameData) - 1u};
        const uec_string_view comboOption = {
            comboOptionData, sizeof(comboOptionData) - 1u};
        const uec_string_view missingComboOption = {
            missingComboOptionData, sizeof(missingComboOptionData) - 1u};
        const uec_string_view unicodeText = {
            unicodeTextData, sizeof(unicodeTextData) - 1u};
        result = state->api->create_widget(
            state->old_world, widgetClassPath, &state->widget);
        if (result != UEC_RESULT_OK || state->widget == NULL) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        result = state->api->get_widget_child(state->widget, buttonName, &state->button);
        if (result != UEC_RESULT_OK || state->button == NULL) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        result = state->api->get_widget_child(
            state->widget, editableTextBoxName, &state->editable_text_box);
        if (result != UEC_RESULT_OK || state->editable_text_box == NULL) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        result = state->api->set_editable_text_box_text(
            state->editable_text_box, unicodeText);
        if (result != UEC_RESULT_OK) {
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        size_t requiredTextSize = 0u;
        result = state->api->get_editable_text_box_text(
            state->editable_text_box, NULL, 0u, &requiredTextSize);
        if (result != UEC_RESULT_BUFFER_TOO_SMALL ||
            requiredTextSize != sizeof(unicodeTextData)) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        char textReadback[64] = {0};
        result = state->api->get_editable_text_box_text(
            state->editable_text_box, textReadback, sizeof(textReadback),
            &requiredTextSize);
        if (result != UEC_RESULT_OK || requiredTextSize != sizeof(unicodeTextData) ||
            memcmp(textReadback, unicodeTextData, sizeof(unicodeTextData)) != 0) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        result = state->api->get_widget_child(state->widget, sliderName, &state->slider);
        if (result != UEC_RESULT_OK || state->slider == NULL) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        result = state->api->set_slider_value(state->slider, 0.0);
        if (result != UEC_RESULT_OK) {
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        double sliderValue = -1.0;
        result = state->api->get_slider_value(state->slider, &sliderValue);
        if (result != UEC_RESULT_OK || sliderValue != 0.0) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        result = state->api->set_slider_value(state->slider, 0.375);
        if (result != UEC_RESULT_OK) {
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        result = state->api->get_slider_value(state->slider, &sliderValue);
        if (result != UEC_RESULT_OK || sliderValue < 0.374999 || sliderValue > 0.375001) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        if (state->api->set_slider_value(state->slider, -0.01) !=
                UEC_RESULT_INVALID_ARGUMENT ||
            state->api->set_slider_value(state->slider, 1.01) !=
                UEC_RESULT_INVALID_ARGUMENT ||
            state->api->set_slider_value(state->slider, 1.0e300) !=
                UEC_RESULT_INVALID_ARGUMENT) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        result = state->api->get_slider_value(state->slider, &sliderValue);
        if (result != UEC_RESULT_OK || sliderValue < 0.374999 || sliderValue > 0.375001) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        sliderValue = -1.0;
        if (state->api->get_slider_value(state->editable_text_box, &sliderValue) !=
                UEC_RESULT_INVALID_ARGUMENT || sliderValue != 0.0) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        result = state->api->get_widget_child(
            state->widget, comboBoxName, &state->combo_box);
        if (result != UEC_RESULT_OK || state->combo_box == NULL) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        result = state->api->set_combo_box_selected_option(
            state->combo_box, comboOption);
        if (result != UEC_RESULT_OK) {
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        uint32_t comboOptionCount = 99u;
        result = state->api->get_combo_box_option_count(
            state->combo_box, &comboOptionCount);
        if (result != UEC_RESULT_OK || comboOptionCount != 2u) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        size_t optionRequired = 0u;
        result = state->api->get_combo_box_option_at(
            state->combo_box, 0u, NULL, 0u, &optionRequired);
        if (result != UEC_RESULT_BUFFER_TOO_SMALL ||
            optionRequired != sizeof("Low")) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        char firstOption[16] = {0};
        result = state->api->get_combo_box_option_at(
            state->combo_box, 0u, firstOption, sizeof(firstOption),
            &optionRequired);
        if (result != UEC_RESULT_OK || strcmp(firstOption, "Low") != 0) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        optionRequired = 99u;
        if (state->api->get_combo_box_option_at(
                state->combo_box, 2u, firstOption, sizeof(firstOption),
                &optionRequired) != UEC_RESULT_INVALID_ARGUMENT ||
            optionRequired != 0u) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        optionRequired = 99u;
        if (state->api->get_combo_box_option_at(
                state->editable_text_box, 0u, firstOption, sizeof(firstOption),
                &optionRequired) != UEC_RESULT_INVALID_ARGUMENT ||
            optionRequired != 0u) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        size_t selectedOptionRequired = 0u;
        result = state->api->get_combo_box_selected_option(
            state->combo_box, NULL, 0u, &selectedOptionRequired);
        if (result != UEC_RESULT_BUFFER_TOO_SMALL ||
            selectedOptionRequired != sizeof(comboOptionData)) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        char selectedOption[16] = {0};
        result = state->api->get_combo_box_selected_option(
            state->combo_box, selectedOption, sizeof(selectedOption),
            &selectedOptionRequired);
        if (result != UEC_RESULT_OK ||
            selectedOptionRequired != sizeof(comboOptionData) ||
            strcmp(selectedOption, comboOptionData) != 0) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        if (state->api->set_combo_box_selected_option(
                state->combo_box, missingComboOption) != UEC_RESULT_INVALID_ARGUMENT) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        selectedOptionRequired = 99u;
        if (state->api->get_combo_box_selected_option(
                state->editable_text_box, selectedOption, sizeof(selectedOption),
                &selectedOptionRequired) != UEC_RESULT_INVALID_ARGUMENT ||
            selectedOptionRequired != 0u) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        memset(selectedOption, 0, sizeof(selectedOption));
        selectedOptionRequired = 0u;
        result = state->api->get_combo_box_selected_option(
            state->combo_box, selectedOption, sizeof(selectedOption),
            &selectedOptionRequired);
        if (result != UEC_RESULT_OK || strcmp(selectedOption, comboOptionData) != 0) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        if (state->api->add_combo_box_option(
                state->combo_box, missingComboOption) != UEC_RESULT_OK ||
            state->api->add_combo_box_option(
                state->combo_box, missingComboOption) != UEC_RESULT_INVALID_ARGUMENT) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        uint32_t editedOptionCount = 0u;
        if (state->api->get_combo_box_option_count(
                state->combo_box, &editedOptionCount) != UEC_RESULT_OK ||
            editedOptionCount != 3u) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        size_t editedOptionRequired = 0u;
        if (state->api->get_combo_box_option_at(
                state->combo_box, 2u, selectedOption, sizeof(selectedOption),
                &editedOptionRequired) != UEC_RESULT_OK ||
            strcmp(selectedOption, missingComboOptionData) != 0 ||
            state->api->remove_combo_box_option(
                state->combo_box, missingComboOption) != UEC_RESULT_OK ||
            state->api->remove_combo_box_option(
                state->combo_box, missingComboOption) != UEC_RESULT_INVALID_ARGUMENT ||
            state->api->remove_combo_box_option(
                state->combo_box, comboOption) != UEC_RESULT_OK) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        selectedOptionRequired = 99u;
        if (state->api->get_combo_box_selected_option(
                state->combo_box, selectedOption, sizeof(selectedOption),
                &selectedOptionRequired) != UEC_RESULT_OK ||
            selectedOptionRequired != 1u || selectedOption[0] != '\0' ||
            state->api->add_combo_box_option(
                state->combo_box, comboOption) != UEC_RESULT_OK ||
            state->api->set_combo_box_selected_option(
                state->combo_box, comboOption) != UEC_RESULT_OK ||
            state->api->clear_combo_box_options(state->combo_box) != UEC_RESULT_OK) {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        editedOptionCount = 99u;
        editedOptionRequired = 99u;
        if (state->api->get_combo_box_option_count(
                state->combo_box, &editedOptionCount) != UEC_RESULT_OK ||
            editedOptionCount != 0u ||
            state->api->get_combo_box_option_at(
                state->combo_box, 0u, selectedOption, sizeof(selectedOption),
                &editedOptionRequired) != UEC_RESULT_INVALID_ARGUMENT ||
            editedOptionRequired != 0u ||
            state->api->get_combo_box_selected_option(
                state->combo_box, selectedOption, sizeof(selectedOption),
                &selectedOptionRequired) != UEC_RESULT_OK ||
            selectedOptionRequired != 1u || selectedOption[0] != '\0') {
            FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_FALSE);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        result = state->api->add_widget_to_viewport(state->widget, 0);
        if (result != UEC_RESULT_OK) {
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        result = state->api->bind_button_clicked(
            state->button, &IgnoreTravelSmokeButtonClick, NULL,
            &state->button_subscription_id);
        if (result != UEC_RESULT_OK || state->button_subscription_id == 0u) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        uec_runtime_stats boundStats = {0};
        boundStats.struct_size = sizeof(boundStats);
        result = state->api->get_runtime_stats(state->context, &boundStats);
        if (result != UEC_RESULT_OK ||
            boundStats.active_subscriptions !=
                state->baseline_active_subscriptions + 1u) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
    }
    {
        static const char actorClassPathData[] =
            "/Script/UnrealCAPIHost.UECAPIHostPlayerFlowPawn";
        static const char audioPropertyNameData[] = "FlowAudio";
        const uec_string_view actorClassPath = {
            actorClassPathData, sizeof(actorClassPathData) - 1u};
        const uec_string_view audioPropertyName = {
            audioPropertyNameData, sizeof(audioPropertyNameData) - 1u};
        const uec_transform transform = {
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
        result = state->api->spawn_actor(
            state->old_world, actorClassPath, &transform, &state->audio_actor);
        if (result != UEC_RESULT_OK || state->audio_actor == NULL) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        result = state->api->get_actor_property_object(
            state->audio_actor, audioPropertyName, &state->audio_component);
        if (result != UEC_RESULT_OK || state->audio_component == NULL) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        result = state->api->bind_audio_finished(
            state->audio_component, &CountTravelSmokeAudioFinished, state,
            &state->audio_subscription_id);
        if (result != UEC_RESULT_OK || state->audio_subscription_id == 0u) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
        uec_runtime_stats audioBoundStats = {0};
        audioBoundStats.struct_size = sizeof(audioBoundStats);
        result = state->api->get_runtime_stats(state->context, &audioBoundStats);
        if (result != UEC_RESULT_OK ||
            audioBoundStats.active_subscriptions !=
                state->baseline_active_subscriptions + 2u) {
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
            FinishTravelSmoke(state, result, UEC_FALSE);
            return result;
        }
    }
    const uec_string_view targetMap = {targetMapPath, sizeof(targetMapPath) - 1u};
    result = state->api->travel_world_async(state->old_world, targetMap,
                                            &CompleteTravelSmoke, state,
                                            &state->request_id);
    if (result != UEC_RESULT_OK || state->request_id == 0u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        FinishTravelSmoke(state, result, UEC_FALSE);
        return result;
    }
    state->submitted = UEC_TRUE;
    isPlayerState = UEC_TRUE;
    if (state->api->object_is_a(state->player_state, playerStateClassPath,
                                &isPlayerState) != UEC_RESULT_INVALID_HANDLE ||
        isPlayerState != UEC_FALSE) {
        FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_TRUE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    size_t staleWorldRequired = 1u;
    if (state->api->get_world_name(state->old_world, NULL, 0, &staleWorldRequired) !=
            UEC_RESULT_INVALID_HANDLE || staleWorldRequired != 0u) {
        FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_TRUE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    size_t staleTextRequired = 1u;
    if (state->api->get_editable_text_box_text(
            state->editable_text_box, NULL, 0u, &staleTextRequired) !=
            UEC_RESULT_INVALID_HANDLE || staleTextRequired != 0u) {
        FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_TRUE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    double staleSliderValue = -1.0;
    if (state->api->get_slider_value(state->slider, &staleSliderValue) !=
            UEC_RESULT_INVALID_HANDLE || staleSliderValue != 0.0) {
        FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_TRUE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    size_t staleComboOptionRequired = 99u;
    if (state->api->get_combo_box_selected_option(
            state->combo_box, NULL, 0u, &staleComboOptionRequired) !=
            UEC_RESULT_INVALID_HANDLE || staleComboOptionRequired != 0u) {
        FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_TRUE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    uint32_t staleComboOptionCount = 99u;
    if (state->api->get_combo_box_option_count(
            state->combo_box, &staleComboOptionCount) != UEC_RESULT_INVALID_HANDLE ||
        staleComboOptionCount != 0u) {
        FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_TRUE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    if (state->api->unbind_button_clicked(state->context,
            state->button_subscription_id) != UEC_RESULT_INVALID_ARGUMENT) {
        FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_TRUE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    state->button_subscription_id = 0;
    if (state->api->unbind_audio_finished(state->context,
            state->audio_subscription_id) != UEC_RESULT_INVALID_ARGUMENT ||
        state->audio_callback_count != 0u) {
        FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_TRUE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    state->audio_subscription_id = 0;
    uec_bool audioPlaying = UEC_TRUE;
    if (state->api->get_audio_component_playing(
            state->audio_component, &audioPlaying) != UEC_RESULT_INVALID_HANDLE ||
        audioPlaying != UEC_FALSE) {
        FinishTravelSmoke(state, UEC_RESULT_INTERNAL_ERROR, UEC_TRUE);
        return UEC_RESULT_INTERNAL_ERROR;
    }
    uec_runtime_stats observedStats = {0};
    observedStats.struct_size = sizeof(observedStats);
    result = state->api->get_runtime_stats(state->context, &observedStats);
    if (result != UEC_RESULT_OK ||
        observedStats.pending_requests != state->baseline_pending_requests + 1u ||
        observedStats.active_subscriptions != state->baseline_active_subscriptions ||
        observedStats.live_worlds != state->baseline_worlds) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        FinishTravelSmoke(state, result, UEC_TRUE);
        return result;
    }
    return UEC_RESULT_OK;
}

uec_bool UEC_CALL uec_host_travel_smoke_poll(uec_result* outResult)
{
    if (outResult == NULL) return UEC_FALSE;
    uec_travel_smoke_state* state = &g_travel_smoke_state;
    if (state->complete != UEC_TRUE && state->callback_received == UEC_TRUE) {
        FinishTravelSmoke(state, state->result, UEC_FALSE);
    }
    *outResult = state->complete == UEC_TRUE ? state->result : UEC_RESULT_NOT_INITIALIZED;
    return state->complete;
}

void UEC_CALL uec_host_travel_smoke_cancel(void)
{
    FinishTravelSmoke(&g_travel_smoke_state, UEC_RESULT_OK, UEC_TRUE);
}
