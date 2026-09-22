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

#include "c_host_stub_reflection.inl"
