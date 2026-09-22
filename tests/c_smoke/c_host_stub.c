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

#include "c_host_stub_reflection.inl"
