#include "c_widget_ui.h"

#include <stddef.h>
#include <string.h>

static int gLookupCalls;
static int gSetCalls;
static int gGetCalls;
static int gSliderGetCalls;
static int gSliderSetCalls;
static int gComboBoxGetCalls;
static int gComboBoxSetCalls;
static int gComboBoxCountCalls;
static int gComboBoxOptionCalls;
static int gComboBoxAddCalls;
static int gComboBoxRemoveCalls;
static int gComboBoxClearCalls;
static int gButtonBindCalls;
static int gButtonUnbindCalls;
static int gButtonCallbackCalls;
static int gReleaseCalls;
static int gMismatch;
static uec_object* gExpectedWidget;
static uec_object* gExpectedChild;
static uec_string_view gExpectedName;
static uec_string_view gExpectedText;
static uec_result gLookupResult;
static uec_result gSetResult;
static uec_result gGetResult;
static uec_result gReleaseResult;
static uec_result gSliderGetResult;
static uec_result gSliderSetResult;
static uec_result gComboBoxGetResult;
static uec_result gComboBoxSetResult;
static uec_result gComboBoxCountResult;
static uec_result gComboBoxOptionResult;
static uec_result gComboBoxAddResult;
static uec_result gComboBoxRemoveResult;
static uec_result gComboBoxClearResult;
static uec_result gCheckBoxGetResult;
static uec_result gCheckBoxSetResult;
static uec_result gProgressBarGetResult;
static uec_result gProgressBarSetResult;
static uec_result gButtonBindResult;
static uec_result gButtonUnbindResult;
static double gSliderValue;
static double gProgressBarPercent;
static uec_checkbox_state gCheckBoxState;
static uec_context* gExpectedContext;
static void* gExpectedButtonUserData;
static uec_widget_event_callback gBoundButtonCallback;
static void* gBoundButtonUserData;

static void UEC_CALL MockWidgetCallback(uint64_t subscriptionId, void* userData)
{
    ++gButtonCallbackCalls;
    if (subscriptionId != UINT64_C(73) || userData != gExpectedButtonUserData) {
        gMismatch = 1;
    }
}

static uec_result UEC_CALL MockGetWidgetChild(uec_object* userWidget,
                                               uec_string_view childName,
                                               uec_object** outChild)
{
    ++gLookupCalls;
    if (outChild != NULL) *outChild = NULL;
    if (outChild == NULL || userWidget != gExpectedWidget ||
        childName.data != gExpectedName.data || childName.size != gExpectedName.size) {
        gMismatch = 1;
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    if (gLookupResult == UEC_RESULT_OK) *outChild = gExpectedChild;
    return gLookupResult;
}

static uec_result UEC_CALL MockSetTextBlockText(uec_object* textBlock, uec_string_view text)
{
    ++gSetCalls;
    if (textBlock != gExpectedChild || text.data != gExpectedText.data ||
        text.size != gExpectedText.size) gMismatch = 1;
    return gSetResult;
}

static uec_result UEC_CALL MockSetEditableTextBoxText(uec_object* textBox,
                                                      uec_string_view text)
{
    return MockSetTextBlockText(textBox, text);
}

static uec_result UEC_CALL MockGetEditableTextBoxText(uec_object* textBox,
                                                      char* buffer,
                                                      size_t bufferSize,
                                                      size_t* requiredSize)
{
    ++gGetCalls;
    if (requiredSize != NULL) *requiredSize = 0u;
    if (requiredSize == NULL || textBox != gExpectedChild) {
        gMismatch = 1;
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    *requiredSize = gExpectedText.size + 1u;
    if (gGetResult != UEC_RESULT_OK) return gGetResult;
    if (buffer == NULL || bufferSize < *requiredSize) return UEC_RESULT_BUFFER_TOO_SMALL;
    memcpy(buffer, gExpectedText.data, gExpectedText.size);
    buffer[gExpectedText.size] = '\0';
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockGetSliderValue(uec_object* slider, double* outValue)
{
    ++gSliderGetCalls;
    if (outValue != NULL) *outValue = 0.0;
    if (outValue == NULL || slider != gExpectedChild) {
        gMismatch = 1;
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    if (gSliderGetResult != UEC_RESULT_OK) return gSliderGetResult;
    *outValue = gSliderValue;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockSetSliderValue(uec_object* slider, double value)
{
    ++gSliderSetCalls;
    if (slider != gExpectedChild || value != 0.625) gMismatch = 1;
    if (gSliderSetResult != UEC_RESULT_OK) return gSliderSetResult;
    gSliderValue = value;
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockGetComboBoxSelectedOption(
    uec_object* comboBox, char* buffer, size_t bufferSize, size_t* requiredSize)
{
    ++gComboBoxGetCalls;
    if (requiredSize != NULL) *requiredSize = 0u;
    if (requiredSize == NULL || comboBox != gExpectedChild) {
        gMismatch = 1;
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    if (gComboBoxGetResult != UEC_RESULT_OK) return gComboBoxGetResult;
    *requiredSize = gExpectedText.size + 1u;
    if (buffer == NULL || bufferSize < *requiredSize) return UEC_RESULT_BUFFER_TOO_SMALL;
    memcpy(buffer, gExpectedText.data, gExpectedText.size);
    buffer[gExpectedText.size] = '\0';
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockSetComboBoxSelectedOption(
    uec_object* comboBox, uec_string_view option)
{
    ++gComboBoxSetCalls;
    if (comboBox != gExpectedChild || option.data != gExpectedText.data ||
        option.size != gExpectedText.size) gMismatch = 1;
    return gComboBoxSetResult;
}

static uec_result UEC_CALL MockGetComboBoxOptionCount(
    uec_object* comboBox, uint32_t* outCount)
{
    ++gComboBoxCountCalls;
    if (outCount != NULL) *outCount = 0u;
    if (outCount == NULL || comboBox != gExpectedChild) {
        gMismatch = 1;
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    if (gComboBoxCountResult == UEC_RESULT_OK) *outCount = 2u;
    return gComboBoxCountResult;
}

static uec_result UEC_CALL MockGetComboBoxOptionAt(
    uec_object* comboBox, uint32_t index, char* buffer, size_t bufferSize,
    size_t* requiredSize)
{
    ++gComboBoxOptionCalls;
    if (requiredSize != NULL) *requiredSize = 0u;
    if (requiredSize == NULL || comboBox != gExpectedChild) {
        gMismatch = 1;
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    if (index != 0u) return UEC_RESULT_INVALID_ARGUMENT;
    if (gComboBoxOptionResult != UEC_RESULT_OK) return gComboBoxOptionResult;
    *requiredSize = gExpectedText.size + 1u;
    if (buffer == NULL || bufferSize < *requiredSize) return UEC_RESULT_BUFFER_TOO_SMALL;
    memcpy(buffer, gExpectedText.data, gExpectedText.size);
    buffer[gExpectedText.size] = '\0';
    return UEC_RESULT_OK;
}

static uec_result UEC_CALL MockAddComboBoxOption(
    uec_object* comboBox, uec_string_view option)
{
    ++gComboBoxAddCalls;
    if (comboBox != gExpectedChild || option.data != gExpectedText.data ||
        option.size != gExpectedText.size) gMismatch = 1;
    return gComboBoxAddResult;
}

static uec_result UEC_CALL MockRemoveComboBoxOption(
    uec_object* comboBox, uec_string_view option)
{
    ++gComboBoxRemoveCalls;
    if (comboBox != gExpectedChild || option.data != gExpectedText.data ||
        option.size != gExpectedText.size) gMismatch = 1;
    return gComboBoxRemoveResult;
}

static uec_result UEC_CALL MockClearComboBoxOptions(uec_object* comboBox)
{
    ++gComboBoxClearCalls;
    if (comboBox != gExpectedChild) gMismatch = 1;
    return gComboBoxClearResult;
}

static uec_result UEC_CALL MockGetCheckBoxState(
    uec_object* checkBox, uec_checkbox_state* outState)
{
    if (outState != NULL) *outState = UEC_CHECKBOX_UNCHECKED;
    if (outState == NULL || checkBox != gExpectedChild) {
        gMismatch = 1;
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    if (gCheckBoxGetResult == UEC_RESULT_OK) *outState = gCheckBoxState;
    return gCheckBoxGetResult;
}

static uec_result UEC_CALL MockSetCheckBoxState(
    uec_object* checkBox, uec_checkbox_state state)
{
    if (checkBox != gExpectedChild || state != UEC_CHECKBOX_CHECKED) {
        gMismatch = 1;
    }
    return gCheckBoxSetResult;
}

static uec_result UEC_CALL MockGetProgressBarPercent(
    uec_object* progressBar, double* outPercent)
{
    if (outPercent != NULL) *outPercent = 0.0;
    if (outPercent == NULL || progressBar != gExpectedChild) {
        gMismatch = 1;
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    if (gProgressBarGetResult == UEC_RESULT_OK) *outPercent = gProgressBarPercent;
    return gProgressBarGetResult;
}

static uec_result UEC_CALL MockSetProgressBarPercent(
    uec_object* progressBar, double percent)
{
    if (progressBar != gExpectedChild || percent != 0.375) gMismatch = 1;
    return gProgressBarSetResult;
}

static uec_result UEC_CALL MockBindButtonClicked(
    uec_object* button,
    uec_widget_event_callback callback,
    void* userData,
    uint64_t* outSubscriptionId)
{
    ++gButtonBindCalls;
    if (button != gExpectedChild || callback != &MockWidgetCallback ||
        userData != gExpectedButtonUserData || outSubscriptionId == NULL) {
        gMismatch = 1;
    }
    gBoundButtonCallback = callback;
    gBoundButtonUserData = userData;
    if (outSubscriptionId != NULL) *outSubscriptionId = UINT64_C(73);
    return gButtonBindResult;
}

static uec_result UEC_CALL MockUnbindButtonClicked(
    uec_context* context, uint64_t subscriptionId)
{
    ++gButtonUnbindCalls;
    if (context != gExpectedContext || subscriptionId != UINT64_C(73)) {
        gMismatch = 1;
    }
    return gButtonUnbindResult;
}

static uec_result UEC_CALL MockReleaseObject(uec_object* object)
{
    ++gReleaseCalls;
    if (object != gExpectedChild) gMismatch = 1;
    return gReleaseResult;
}

static void ResetMocks(void)
{
    gLookupCalls = 0;
    gSetCalls = 0;
    gGetCalls = 0;
    gSliderGetCalls = 0;
    gSliderSetCalls = 0;
    gComboBoxGetCalls = 0;
    gComboBoxSetCalls = 0;
    gComboBoxCountCalls = 0;
    gComboBoxOptionCalls = 0;
    gComboBoxAddCalls = 0;
    gComboBoxRemoveCalls = 0;
    gComboBoxClearCalls = 0;
    gButtonBindCalls = 0;
    gButtonUnbindCalls = 0;
    gButtonCallbackCalls = 0;
    gReleaseCalls = 0;
    gMismatch = 0;
    gLookupResult = UEC_RESULT_OK;
    gSetResult = UEC_RESULT_OK;
    gGetResult = UEC_RESULT_OK;
    gReleaseResult = UEC_RESULT_OK;
    gSliderGetResult = UEC_RESULT_OK;
    gSliderSetResult = UEC_RESULT_OK;
    gComboBoxGetResult = UEC_RESULT_OK;
    gComboBoxSetResult = UEC_RESULT_OK;
    gComboBoxCountResult = UEC_RESULT_OK;
    gComboBoxOptionResult = UEC_RESULT_OK;
    gComboBoxAddResult = UEC_RESULT_OK;
    gComboBoxRemoveResult = UEC_RESULT_OK;
    gComboBoxClearResult = UEC_RESULT_OK;
    gCheckBoxGetResult = UEC_RESULT_OK;
    gCheckBoxSetResult = UEC_RESULT_OK;
    gProgressBarGetResult = UEC_RESULT_OK;
    gProgressBarSetResult = UEC_RESULT_OK;
    gButtonBindResult = UEC_RESULT_OK;
    gButtonUnbindResult = UEC_RESULT_OK;
    gBoundButtonCallback = NULL;
    gBoundButtonUserData = NULL;
    gSliderValue = 0.0;
    gProgressBarPercent = 0.625;
    gCheckBoxState = UEC_CHECKBOX_UNDETERMINED;
}

int uec_widget_ui_smoke_test(void)
{
    static char widgetStorage;
    static char childStorage;
    static char contextStorage;
    static int buttonUserData;
    static const char childName[] = "StatusText";
    static const char text[] = "Ready";
    const uec_string_view nameView = {childName, sizeof(childName) - 1u};
    const uec_string_view textView = {text, sizeof(text) - 1u};
    uec_api api = {0};
    api.struct_size = (uint32_t)sizeof(api);
    api.get_widget_child = &MockGetWidgetChild;
    api.set_text_block_text = &MockSetTextBlockText;
    api.get_editable_text_box_text = &MockGetEditableTextBoxText;
    api.set_editable_text_box_text = &MockSetEditableTextBoxText;
    api.get_slider_value = &MockGetSliderValue;
    api.set_slider_value = &MockSetSliderValue;
    api.get_combo_box_selected_option = &MockGetComboBoxSelectedOption;
    api.set_combo_box_selected_option = &MockSetComboBoxSelectedOption;
    api.get_combo_box_option_count = &MockGetComboBoxOptionCount;
    api.get_combo_box_option_at = &MockGetComboBoxOptionAt;
    api.add_combo_box_option = &MockAddComboBoxOption;
    api.remove_combo_box_option = &MockRemoveComboBoxOption;
    api.clear_combo_box_options = &MockClearComboBoxOptions;
    api.get_checkbox_state = &MockGetCheckBoxState;
    api.set_checkbox_state = &MockSetCheckBoxState;
    api.get_progress_bar_percent = &MockGetProgressBarPercent;
    api.set_progress_bar_percent = &MockSetProgressBarPercent;
    api.bind_button_clicked = &MockBindButtonClicked;
    api.unbind_button_clicked = &MockUnbindButtonClicked;
    api.release_object = &MockReleaseObject;
    gExpectedWidget = (uec_object*)&widgetStorage;
    gExpectedChild = (uec_object*)&childStorage;
    gExpectedContext = (uec_context*)&contextStorage;
    gExpectedButtonUserData = &buttonUserData;
    gExpectedName = nameView;
    gExpectedText = textView;

    ResetMocks();
    if (uec_widget_set_text_child(&api, gExpectedWidget, nameView, textView) != UEC_RESULT_OK ||
        gMismatch != 0 || gLookupCalls != 1 || gSetCalls != 1 || gReleaseCalls != 1) return 1;

    ResetMocks();
    gSetResult = UEC_RESULT_UNSUPPORTED;
    if (uec_widget_set_text_child(&api, gExpectedWidget, nameView, textView) !=
            UEC_RESULT_UNSUPPORTED || gMismatch != 0 || gLookupCalls != 1 ||
        gSetCalls != 1 || gReleaseCalls != 1) return 2;

    ResetMocks();
    gReleaseResult = UEC_RESULT_INTERNAL_ERROR;
    if (uec_widget_set_text_child(&api, gExpectedWidget, nameView, textView) !=
            UEC_RESULT_INTERNAL_ERROR || gMismatch != 0 || gLookupCalls != 1 ||
        gSetCalls != 1 || gReleaseCalls != 1) return 3;

    ResetMocks();
    gLookupResult = UEC_RESULT_NOT_INITIALIZED;
    if (uec_widget_set_text_child(&api, gExpectedWidget, nameView, textView) !=
            UEC_RESULT_NOT_INITIALIZED || gMismatch != 0 || gLookupCalls != 1 ||
        gSetCalls != 0 || gReleaseCalls != 0) return 4;

    ResetMocks();
    api.struct_size = (uint32_t)offsetof(uec_api, get_widget_child);
    if (uec_widget_set_text_child(&api, gExpectedWidget, nameView, textView) !=
            UEC_RESULT_UNSUPPORTED || gLookupCalls != 0 || gSetCalls != 0 ||
        gReleaseCalls != 0) return 5;

    ResetMocks();
    api.struct_size = (uint32_t)sizeof(api);
    api.release_object = NULL;
    if (uec_widget_set_text_child(&api, gExpectedWidget, nameView, textView) !=
            UEC_RESULT_UNSUPPORTED || gLookupCalls != 0 || gSetCalls != 0 ||
        gReleaseCalls != 0) return 6;

    ResetMocks();
    api.struct_size = (uint32_t)sizeof(api);
    api.release_object = &MockReleaseObject;
    if (uec_widget_set_editable_text_child(
            &api, gExpectedWidget, nameView, textView) != UEC_RESULT_OK ||
        gMismatch != 0 || gLookupCalls != 1 || gSetCalls != 1 ||
        gReleaseCalls != 1) return 7;

    ResetMocks();
    gSetResult = UEC_RESULT_UNSUPPORTED;
    if (uec_widget_set_editable_text_child(
            &api, gExpectedWidget, nameView, textView) != UEC_RESULT_UNSUPPORTED ||
        gMismatch != 0 || gLookupCalls != 1 || gSetCalls != 1 ||
        gReleaseCalls != 1) return 8;

    ResetMocks();
    {
        char buffer[16] = {0};
        size_t requiredSize = 0u;
        if (uec_widget_get_editable_text_child(
                &api, gExpectedWidget, nameView, buffer, sizeof(buffer),
                &requiredSize) != UEC_RESULT_OK ||
            requiredSize != sizeof(text) || strcmp(buffer, text) != 0 ||
            gMismatch != 0 || gLookupCalls != 1 || gGetCalls != 1 ||
            gReleaseCalls != 1) return 9;
    }

    ResetMocks();
    {
        char buffer[2] = {0};
        size_t requiredSize = 99u;
        if (uec_widget_get_editable_text_child(
                &api, gExpectedWidget, nameView, buffer, sizeof(buffer),
                &requiredSize) != UEC_RESULT_BUFFER_TOO_SMALL ||
            requiredSize != sizeof(text) || gMismatch != 0 ||
            gLookupCalls != 1 || gGetCalls != 1 || gReleaseCalls != 1) return 10;
    }

    ResetMocks();
    gLookupResult = UEC_RESULT_NOT_INITIALIZED;
    {
        size_t requiredSize = 99u;
        if (uec_widget_get_editable_text_child(
                &api, gExpectedWidget, nameView, NULL, 0u,
                &requiredSize) != UEC_RESULT_NOT_INITIALIZED ||
            requiredSize != 0u || gMismatch != 0 || gLookupCalls != 1 ||
            gGetCalls != 0 || gReleaseCalls != 0) return 11;
    }

    ResetMocks();
    api.struct_size = (uint32_t)offsetof(uec_api, get_editable_text_box_text);
    {
        size_t requiredSize = 99u;
        if (uec_widget_get_editable_text_child(
                &api, gExpectedWidget, nameView, NULL, 0u,
                &requiredSize) != UEC_RESULT_UNSUPPORTED ||
            requiredSize != 0u || gLookupCalls != 0 || gGetCalls != 0 ||
            gReleaseCalls != 0) return 12;
    }

    ResetMocks();
    api.struct_size = (uint32_t)sizeof(api);
    api.release_object = &MockReleaseObject;
    if (uec_widget_set_slider_child(
            &api, gExpectedWidget, nameView, 0.625) != UEC_RESULT_OK ||
        gSliderValue != 0.625 || gMismatch != 0 || gLookupCalls != 1 ||
        gSliderSetCalls != 1 || gReleaseCalls != 1) return 13;

    ResetMocks();
    {
        double value = -1.0;
        gSliderValue = 0.625;
        if (uec_widget_get_slider_child(
                &api, gExpectedWidget, nameView, &value) != UEC_RESULT_OK ||
            value != 0.625 || gMismatch != 0 || gLookupCalls != 1 ||
            gSliderGetCalls != 1 || gReleaseCalls != 1) return 14;
    }

    ResetMocks();
    gSliderSetResult = UEC_RESULT_UNSUPPORTED;
    if (uec_widget_set_slider_child(
            &api, gExpectedWidget, nameView, 0.625) != UEC_RESULT_UNSUPPORTED ||
        gMismatch != 0 || gLookupCalls != 1 || gSliderSetCalls != 1 ||
        gReleaseCalls != 1) return 15;

    ResetMocks();
    gSliderGetResult = UEC_RESULT_UNSUPPORTED;
    {
        double value = -1.0;
        if (uec_widget_get_slider_child(
                &api, gExpectedWidget, nameView, &value) != UEC_RESULT_UNSUPPORTED ||
            value != 0.0 || gMismatch != 0 || gLookupCalls != 1 ||
            gSliderGetCalls != 1 || gReleaseCalls != 1) return 16;
    }

    ResetMocks();
    api.struct_size = (uint32_t)offsetof(uec_api, get_slider_value);
    {
        double value = -1.0;
        if (uec_widget_get_slider_child(
                &api, gExpectedWidget, nameView, &value) != UEC_RESULT_UNSUPPORTED ||
            value != 0.0 || gLookupCalls != 0 || gSliderGetCalls != 0 ||
            gReleaseCalls != 0) return 17;
    }

    ResetMocks();
    api.struct_size = (uint32_t)sizeof(api);
    if (uec_widget_set_combo_box_selected_option_child(
            &api, gExpectedWidget, nameView, textView) != UEC_RESULT_OK ||
        gMismatch != 0 || gLookupCalls != 1 || gComboBoxSetCalls != 1 ||
        gReleaseCalls != 1) return 18;

    ResetMocks();
    {
        char buffer[16] = {0};
        size_t requiredSize = 0u;
        if (uec_widget_get_combo_box_selected_option_child(
                &api, gExpectedWidget, nameView, buffer, sizeof(buffer),
                &requiredSize) != UEC_RESULT_OK ||
            requiredSize != sizeof(text) || strcmp(buffer, text) != 0 ||
            gMismatch != 0 || gLookupCalls != 1 || gComboBoxGetCalls != 1 ||
            gReleaseCalls != 1) return 19;
    }

    ResetMocks();
    gComboBoxSetResult = UEC_RESULT_INVALID_ARGUMENT;
    if (uec_widget_set_combo_box_selected_option_child(
            &api, gExpectedWidget, nameView, textView) != UEC_RESULT_INVALID_ARGUMENT ||
        gMismatch != 0 || gLookupCalls != 1 || gComboBoxSetCalls != 1 ||
        gReleaseCalls != 1) return 20;

    ResetMocks();
    gComboBoxGetResult = UEC_RESULT_UNSUPPORTED;
    {
        char buffer[16] = {0};
        size_t requiredSize = 99u;
        if (uec_widget_get_combo_box_selected_option_child(
                &api, gExpectedWidget, nameView, buffer, sizeof(buffer),
                &requiredSize) != UEC_RESULT_UNSUPPORTED || requiredSize != 0u ||
            gMismatch != 0 || gLookupCalls != 1 || gComboBoxGetCalls != 1 ||
            gReleaseCalls != 1) return 21;
    }

    ResetMocks();
    api.struct_size = (uint32_t)offsetof(uec_api, get_combo_box_selected_option);
    {
        char buffer[16] = {0};
        size_t requiredSize = 99u;
        if (uec_widget_get_combo_box_selected_option_child(
                &api, gExpectedWidget, nameView, buffer, sizeof(buffer),
                &requiredSize) != UEC_RESULT_UNSUPPORTED || requiredSize != 0u ||
            gLookupCalls != 0 || gComboBoxGetCalls != 0 || gReleaseCalls != 0) {
            return 22;
        }
    }

    ResetMocks();
    api.struct_size = (uint32_t)sizeof(api);
    {
        uint32_t optionCount = 99u;
        if (uec_widget_get_combo_box_option_count_child(
                &api, gExpectedWidget, nameView, &optionCount) != UEC_RESULT_OK ||
            optionCount != 2u || gMismatch != 0 || gLookupCalls != 1 ||
            gComboBoxCountCalls != 1 || gReleaseCalls != 1) return 23;
    }

    ResetMocks();
    gComboBoxCountResult = UEC_RESULT_UNSUPPORTED;
    {
        uint32_t optionCount = 99u;
        if (uec_widget_get_combo_box_option_count_child(
                &api, gExpectedWidget, nameView, &optionCount) != UEC_RESULT_UNSUPPORTED ||
            optionCount != 0u || gMismatch != 0 || gLookupCalls != 1 ||
            gComboBoxCountCalls != 1 || gReleaseCalls != 1) return 24;
    }

    ResetMocks();
    api.struct_size = (uint32_t)sizeof(api);
    {
        char buffer[16] = {0};
        size_t requiredSize = 0u;
        if (uec_widget_get_combo_box_option_child(
                &api, gExpectedWidget, nameView, 0u, buffer, sizeof(buffer),
                &requiredSize) != UEC_RESULT_OK ||
            requiredSize != sizeof(text) || strcmp(buffer, text) != 0 ||
            gMismatch != 0 || gLookupCalls != 1 || gComboBoxOptionCalls != 1 ||
            gReleaseCalls != 1) return 25;
    }

    ResetMocks();
    gComboBoxOptionResult = UEC_RESULT_INVALID_ARGUMENT;
    {
        char buffer[16] = {0};
        size_t requiredSize = 99u;
        if (uec_widget_get_combo_box_option_child(
                &api, gExpectedWidget, nameView, 99u, buffer, sizeof(buffer),
                &requiredSize) != UEC_RESULT_INVALID_ARGUMENT || requiredSize != 0u ||
            gMismatch != 0 || gLookupCalls != 1 || gComboBoxOptionCalls != 1 ||
            gReleaseCalls != 1) {
            return 26;
        }
    }

    ResetMocks();
    api.struct_size = (uint32_t)sizeof(api);
    if (uec_widget_add_combo_box_option_child(
            &api, gExpectedWidget, nameView, textView) != UEC_RESULT_OK ||
        gMismatch != 0 || gLookupCalls != 1 || gComboBoxAddCalls != 1 ||
        gReleaseCalls != 1) return 27;

    ResetMocks();
    gComboBoxAddResult = UEC_RESULT_INVALID_ARGUMENT;
    if (uec_widget_add_combo_box_option_child(
            &api, gExpectedWidget, nameView, textView) != UEC_RESULT_INVALID_ARGUMENT ||
        gMismatch != 0 || gLookupCalls != 1 || gComboBoxAddCalls != 1 ||
        gReleaseCalls != 1) return 28;

    ResetMocks();
    gComboBoxRemoveResult = UEC_RESULT_INVALID_ARGUMENT;
    if (uec_widget_remove_combo_box_option_child(
            &api, gExpectedWidget, nameView, textView) != UEC_RESULT_INVALID_ARGUMENT ||
        gMismatch != 0 || gLookupCalls != 1 || gComboBoxRemoveCalls != 1 ||
        gReleaseCalls != 1) return 29;

    ResetMocks();
    if (uec_widget_clear_combo_box_options_child(
            &api, gExpectedWidget, nameView) != UEC_RESULT_OK ||
        gMismatch != 0 || gLookupCalls != 1 || gComboBoxClearCalls != 1 ||
        gReleaseCalls != 1) return 30;

    ResetMocks();
    api.struct_size = (uint32_t)offsetof(uec_api, add_combo_box_option);
    if (uec_widget_add_combo_box_option_child(
            &api, gExpectedWidget, nameView, textView) != UEC_RESULT_UNSUPPORTED ||
        gLookupCalls != 0 || gComboBoxAddCalls != 0 || gReleaseCalls != 0) return 31;

    ResetMocks();
    api.struct_size = (uint32_t)sizeof(api);
    if (uec_widget_remove_combo_box_option_child(
            &api, gExpectedWidget, nameView, textView) != UEC_RESULT_OK ||
        gMismatch != 0 || gLookupCalls != 1 || gComboBoxRemoveCalls != 1 ||
        gReleaseCalls != 1) return 32;

    ResetMocks();
    gComboBoxClearResult = UEC_RESULT_UNSUPPORTED;
    if (uec_widget_clear_combo_box_options_child(
            &api, gExpectedWidget, nameView) != UEC_RESULT_UNSUPPORTED ||
        gMismatch != 0 || gLookupCalls != 1 || gComboBoxClearCalls != 1 ||
        gReleaseCalls != 1) return 33;

    ResetMocks();
    if (uec_widget_set_checkbox_child(
            &api, gExpectedWidget, nameView, UEC_CHECKBOX_CHECKED) != UEC_RESULT_OK ||
        gMismatch != 0 || gLookupCalls != 1 || gReleaseCalls != 1) return 34;

    ResetMocks();
    {
        uec_checkbox_state state = UEC_CHECKBOX_UNCHECKED;
        if (uec_widget_get_checkbox_child(
                &api, gExpectedWidget, nameView, &state) != UEC_RESULT_OK ||
            state != UEC_CHECKBOX_UNDETERMINED || gMismatch != 0 ||
            gLookupCalls != 1 || gReleaseCalls != 1) return 35;
    }

    ResetMocks();
    gCheckBoxGetResult = UEC_RESULT_UNSUPPORTED;
    {
        uec_checkbox_state state = UEC_CHECKBOX_CHECKED;
        if (uec_widget_get_checkbox_child(
                &api, gExpectedWidget, nameView, &state) != UEC_RESULT_UNSUPPORTED ||
            state != UEC_CHECKBOX_UNCHECKED || gMismatch != 0 ||
            gLookupCalls != 1 || gReleaseCalls != 1) return 36;
    }

    ResetMocks();
    gCheckBoxSetResult = UEC_RESULT_INVALID_ARGUMENT;
    if (uec_widget_set_checkbox_child(
            &api, gExpectedWidget, nameView, UEC_CHECKBOX_CHECKED) !=
            UEC_RESULT_INVALID_ARGUMENT || gMismatch != 0 || gLookupCalls != 1 ||
        gReleaseCalls != 1) return 37;

    ResetMocks();
    if (uec_widget_set_progress_bar_child(
            &api, gExpectedWidget, nameView, 0.375) != UEC_RESULT_OK ||
        gMismatch != 0 || gLookupCalls != 1 || gReleaseCalls != 1) return 38;

    ResetMocks();
    {
        double percent = -1.0;
        if (uec_widget_get_progress_bar_child(
                &api, gExpectedWidget, nameView, &percent) != UEC_RESULT_OK ||
            percent != 0.625 || gMismatch != 0 || gLookupCalls != 1 ||
            gReleaseCalls != 1) return 39;
    }

    ResetMocks();
    gProgressBarGetResult = UEC_RESULT_UNSUPPORTED;
    {
        double percent = -1.0;
        if (uec_widget_get_progress_bar_child(
                &api, gExpectedWidget, nameView, &percent) != UEC_RESULT_UNSUPPORTED ||
            percent != 0.0 || gMismatch != 0 || gLookupCalls != 1 ||
            gReleaseCalls != 1) return 40;
    }

    ResetMocks();
    {
        uint64_t subscriptionId = 0u;
        if (uec_widget_bind_button_clicked_child(
                &api, gExpectedWidget, nameView, &MockWidgetCallback,
                gExpectedButtonUserData, &subscriptionId) != UEC_RESULT_OK ||
            subscriptionId != UINT64_C(73) || gMismatch != 0 ||
            gLookupCalls != 1 || gButtonBindCalls != 1 || gReleaseCalls != 1 ||
            gBoundButtonCallback != &MockWidgetCallback ||
            gBoundButtonUserData != gExpectedButtonUserData) return 41;
        gBoundButtonCallback(subscriptionId, gBoundButtonUserData);
        if (gButtonCallbackCalls != 1 || gMismatch != 0) return 42;
    }

    ResetMocks();
    gButtonBindResult = UEC_RESULT_QUEUE_FULL;
    {
        uint64_t subscriptionId = UINT64_C(91);
        if (uec_widget_bind_button_clicked_child(
                &api, gExpectedWidget, nameView, &MockWidgetCallback,
                gExpectedButtonUserData, &subscriptionId) != UEC_RESULT_QUEUE_FULL ||
            subscriptionId != 0u || gMismatch != 0 || gLookupCalls != 1 ||
            gButtonBindCalls != 1 || gReleaseCalls != 1) return 43;
    }

    ResetMocks();
    {
        uint64_t subscriptionId = UINT64_C(91);
        if (uec_widget_bind_button_clicked_child(
                &api, gExpectedWidget, nameView, NULL, gExpectedButtonUserData,
                &subscriptionId) != UEC_RESULT_INVALID_ARGUMENT ||
            subscriptionId != 0u || gLookupCalls != 0 ||
            gButtonBindCalls != 0 || gReleaseCalls != 0) return 44;
    }

    ResetMocks();
    api.struct_size = (uint32_t)offsetof(uec_api, get_widget_child);
    {
        uint64_t subscriptionId = UINT64_C(91);
        if (uec_widget_bind_button_clicked_child(
                &api, gExpectedWidget, nameView, &MockWidgetCallback,
                gExpectedButtonUserData, &subscriptionId) != UEC_RESULT_UNSUPPORTED ||
            subscriptionId != 0u || gLookupCalls != 0 ||
            gButtonBindCalls != 0 || gReleaseCalls != 0) return 45;
    }
    api.struct_size = (uint32_t)sizeof(api);

    ResetMocks();
    if (uec_widget_unbind_button_clicked(
            &api, gExpectedContext, UINT64_C(73)) != UEC_RESULT_OK ||
        gMismatch != 0 || gButtonUnbindCalls != 1) return 46;

    ResetMocks();
    api.struct_size = (uint32_t)offsetof(uec_api, unbind_button_clicked);
    if (uec_widget_unbind_button_clicked(
            &api, gExpectedContext, UINT64_C(73)) != UEC_RESULT_UNSUPPORTED ||
        gButtonUnbindCalls != 0) return 47;

    ResetMocks();
    api.struct_size = (uint32_t)sizeof(api);
    gButtonUnbindResult = UEC_RESULT_INVALID_ARGUMENT;
    if (uec_widget_unbind_button_clicked(
            &api, gExpectedContext, UINT64_C(73)) != UEC_RESULT_INVALID_ARGUMENT ||
        gMismatch != 0 || gButtonUnbindCalls != 1) return 48;
    return 0;
}
