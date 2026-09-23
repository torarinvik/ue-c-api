#include "c_widget_ui.h"

static int gLookupCalls;
static int gSetCalls;
static int gReleaseCalls;
static int gMismatch;
static uec_object* gExpectedWidget;
static uec_object* gExpectedChild;
static uec_string_view gExpectedName;
static uec_string_view gExpectedText;
static uec_result gLookupResult;
static uec_result gSetResult;
static uec_result gReleaseResult;

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
    gReleaseCalls = 0;
    gMismatch = 0;
    gLookupResult = UEC_RESULT_OK;
    gSetResult = UEC_RESULT_OK;
    gReleaseResult = UEC_RESULT_OK;
}

int uec_widget_ui_smoke_test(void)
{
    static char widgetStorage;
    static char childStorage;
    static const char childName[] = "StatusText";
    static const char text[] = "Ready";
    const uec_string_view nameView = {childName, sizeof(childName) - 1u};
    const uec_string_view textView = {text, sizeof(text) - 1u};
    uec_api api = {0};
    api.get_widget_child = &MockGetWidgetChild;
    api.set_text_block_text = &MockSetTextBlockText;
    api.release_object = &MockReleaseObject;
    gExpectedWidget = (uec_object*)&widgetStorage;
    gExpectedChild = (uec_object*)&childStorage;
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
    api.release_object = NULL;
    if (uec_widget_set_text_child(&api, gExpectedWidget, nameView, textView) !=
            UEC_RESULT_UNSUPPORTED || gLookupCalls != 0 || gSetCalls != 0 ||
        gReleaseCalls != 0) return 5;
    return 0;
}
