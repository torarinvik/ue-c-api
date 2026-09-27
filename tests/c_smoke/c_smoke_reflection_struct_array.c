#include "uec_api.h"

#include <string.h>

int uec_test_property_struct_array_stubs(const uec_api* api)
{
    if (api == NULL || api->get_actor_property_array_struct_value == NULL ||
        api->get_object_property_array_struct_value == NULL ||
        api->set_actor_property_array_struct_value == NULL ||
        api->set_object_property_array_struct_value == NULL ||
        api->get_actor_property_map_struct_value == NULL ||
        api->get_object_property_map_struct_value == NULL ||
        api->set_actor_property_map_struct_value == NULL ||
        api->set_object_property_map_struct_value == NULL ||
        api->get_actor_property_set_element_struct_value == NULL ||
        api->get_object_property_set_element_struct_value == NULL ||
        api->set_actor_property_set_element_struct_value == NULL ||
        api->set_object_property_set_element_struct_value == NULL) return 1;

    const uec_string_view emptyName = {NULL, 0u};
    uec_property_struct_value output;
    memset(&output, 0x7f, sizeof(output));
    output.struct_size = sizeof(output);
    if (api->get_actor_property_array_struct_value(
            NULL, emptyName, 0u, &output) != UEC_RESULT_UNSUPPORTED ||
        output.kind != UEC_PROPERTY_STRUCT_NONE || output.value.vector3.x != 0.0 ||
        output.value.vector3.y != 0.0 || output.value.vector3.z != 0.0) return 2;

    output.struct_size = sizeof(output);
    output.kind = UEC_PROPERTY_STRUCT_VECTOR3;
    output.value.vector3.x = 1.0;
    output.value.vector3.y = 2.0;
    output.value.vector3.z = 3.0;
    if (api->get_object_property_array_struct_value(
            NULL, emptyName, 0u, &output) != UEC_RESULT_UNSUPPORTED ||
        output.kind != UEC_PROPERTY_STRUCT_NONE || output.value.vector3.x != 0.0 ||
        output.value.vector3.y != 0.0 || output.value.vector3.z != 0.0) return 3;

    output.struct_size = sizeof(output);
    output.kind = UEC_PROPERTY_STRUCT_VECTOR3;
    if (api->get_actor_property_map_struct_value(
            NULL, emptyName, 0u, &output) != UEC_RESULT_UNSUPPORTED ||
        output.kind != UEC_PROPERTY_STRUCT_NONE || output.value.vector3.x != 0.0 ||
        output.value.vector3.y != 0.0 || output.value.vector3.z != 0.0) return 4;
    output.struct_size = sizeof(output);
    output.kind = UEC_PROPERTY_STRUCT_VECTOR3;
    if (api->get_object_property_map_struct_value(
            NULL, emptyName, 0u, &output) != UEC_RESULT_UNSUPPORTED ||
        output.kind != UEC_PROPERTY_STRUCT_NONE || output.value.vector3.x != 0.0 ||
        output.value.vector3.y != 0.0 || output.value.vector3.z != 0.0) return 5;
    output.struct_size = sizeof(output);
    output.kind = UEC_PROPERTY_STRUCT_VECTOR3;
    if (api->get_actor_property_set_element_struct_value(
            NULL, emptyName, 0u, &output) != UEC_RESULT_UNSUPPORTED ||
        output.kind != UEC_PROPERTY_STRUCT_NONE || output.value.vector3.x != 0.0 ||
        output.value.vector3.y != 0.0 || output.value.vector3.z != 0.0) return 6;
    output.struct_size = sizeof(output);
    output.kind = UEC_PROPERTY_STRUCT_VECTOR3;
    if (api->get_object_property_set_element_struct_value(
            NULL, emptyName, 0u, &output) != UEC_RESULT_UNSUPPORTED ||
        output.kind != UEC_PROPERTY_STRUCT_NONE || output.value.vector3.x != 0.0 ||
        output.value.vector3.y != 0.0 || output.value.vector3.z != 0.0) return 7;

    uec_property_struct_value input;
    memset(&input, 0, sizeof(input));
    input.struct_size = sizeof(input);
    input.kind = UEC_PROPERTY_STRUCT_VECTOR3;
    input.value.vector3.x = 1.0;
    input.value.vector3.y = 2.0;
    input.value.vector3.z = 3.0;
    if (api->set_actor_property_array_struct_value(
            NULL, emptyName, 0u, &input) != UEC_RESULT_UNSUPPORTED ||
        api->set_object_property_array_struct_value(
            NULL, emptyName, 0u, &input) != UEC_RESULT_UNSUPPORTED ||
        api->set_actor_property_array_struct_value(
            NULL, emptyName, 0u, NULL) != UEC_RESULT_INVALID_ARGUMENT ||
        api->set_actor_property_map_struct_value(
            NULL, emptyName, 0u, &input) != UEC_RESULT_UNSUPPORTED ||
        api->set_object_property_map_struct_value(
            NULL, emptyName, 0u, &input) != UEC_RESULT_UNSUPPORTED ||
        api->set_actor_property_set_element_struct_value(
            NULL, emptyName, 0u, &input) != UEC_RESULT_UNSUPPORTED ||
        api->set_object_property_set_element_struct_value(
            NULL, emptyName, 0u, &input) != UEC_RESULT_UNSUPPORTED)
        return 8;
    return 0;
}
