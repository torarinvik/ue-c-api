#include "uec_api.h"

#include <string.h>

#include "c_host_stub_bootstrap.inl"

static uec_result UEC_CALL StubSetComponentCollisionEnabled(
    uec_scene_component* component, uec_collision_enabled enabled)
{
    (void)component;
    if (enabled < UEC_COLLISION_DISABLED || enabled > UEC_COLLISION_QUERY_AND_PHYSICS)
        return UEC_RESULT_INVALID_ARGUMENT;
    return UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubBindInputAction(
    uec_actor* actor, uec_object* action, uec_input_trigger_event triggerEvent,
    uec_input_action_callback callback, void* userData, uint64_t* outBindingId)
{
    (void)actor; (void)action; (void)userData;
    if (outBindingId != NULL) *outBindingId = 0;
    if (outBindingId == NULL || callback == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    if (triggerEvent < UEC_INPUT_TRIGGER_STARTED ||
        triggerEvent > UEC_INPUT_TRIGGER_COMPLETED) return UEC_RESULT_INVALID_ARGUMENT;
    return UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubInjectInputActionValue(
    uec_actor* controller, uec_object* action, const uec_input_action_value* value)
{
    (void)controller; (void)action;
    if (value == NULL || value->struct_size < sizeof(uec_input_action_value))
        return UEC_RESULT_INVALID_ARGUMENT;
    if (value->kind < UEC_INPUT_ACTION_VALUE_BOOLEAN ||
        value->kind > UEC_INPUT_ACTION_VALUE_AXIS_3D) return UEC_RESULT_INVALID_ARGUMENT;
    return UEC_RESULT_INVALID_HANDLE;
}

static uec_result UEC_CALL StubGetCheckBoxState(uec_object* checkBox,
                                                uec_checkbox_state* outState)
{
    if (outState != NULL) *outState = UEC_CHECKBOX_UNCHECKED;
    if (outState == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    return checkBox == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetCheckBoxState(uec_object* checkBox,
                                                uec_checkbox_state state)
{
    if (state < UEC_CHECKBOX_UNCHECKED || state > UEC_CHECKBOX_UNDETERMINED)
        return UEC_RESULT_INVALID_ARGUMENT;
    return checkBox == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubReleaseObject(uec_object* object)
{
    return object == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubSetTextBlockText(uec_object* textBlock, uec_string_view text)
{
    (void)text;
    return textBlock == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

static uec_result UEC_CALL StubGetWidgetChild(uec_object* userWidget,
                                              uec_string_view childName,
                                              uec_object** outChild)
{
    if (outChild != NULL) *outChild = NULL;
    if (outChild == NULL || childName.data == NULL || childName.size == 0u ||
        memchr(childName.data, '\0', childName.size) != NULL) {
        return UEC_RESULT_INVALID_ARGUMENT;
    }
    return userWidget == NULL ? UEC_RESULT_INVALID_HANDLE : UEC_RESULT_UNSUPPORTED;
}

#include "c_host_stub_reflection.inl"
