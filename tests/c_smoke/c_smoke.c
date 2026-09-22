#include "uec_api.h"

#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
#define UEC_TEST_ASSERT static_assert
#else
#define UEC_TEST_ASSERT _Static_assert
#endif

UEC_TEST_ASSERT(sizeof(uec_vector3) == 24, "uec_vector3 ABI changed");
UEC_TEST_ASSERT(sizeof(uec_quaternion) == 32, "uec_quaternion ABI changed");
UEC_TEST_ASSERT(sizeof(uec_transform) == 80, "uec_transform ABI changed");
UEC_TEST_ASSERT(sizeof(uec_property_value) == 32, "uec_property_value ABI changed");
UEC_TEST_ASSERT(sizeof(uec_collision_shape) == 56, "uec_collision_shape ABI changed");
UEC_TEST_ASSERT(sizeof(uec_input_action_value) == 40, "uec_input_action_value ABI changed");
UEC_TEST_ASSERT(UEC_RESULT_QUEUE_FULL == 9, "queue-full result code changed");
UEC_TEST_ASSERT(offsetof(uec_api, get_capabilities) > offsetof(uec_api, abi_minor),
               "uec_api function table ordering changed");
UEC_TEST_ASSERT(offsetof(uec_api, sweep_trace) > offsetof(uec_api, cancel_object_load),
               "collision query functions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, run_on_game_thread) > offsetof(uec_api, delete_game_slot),
               "thread dispatch functions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, async_save_game_to_slot) >
                   offsetof(uec_api, set_object_property_object),
               "async save functions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_actor_count_by_class) >
                   offsetof(uec_api, cancel_save_game_request),
               "actor query functions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, bind_input_action) >
                   offsetof(uec_api, destroy_audio_component),
               "input binding functions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_player_controller) >
                   offsetof(uec_api, unbind_input_action),
               "context access functions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, invoke_actor_function_text) >
                   offsetof(uec_api, get_world_game_instance),
               "text-marshaled invocation must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, subscribe_world_tick) >
                   offsetof(uec_api, invoke_actor_function_text),
               "tick subscriptions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, bind_audio_finished) >
                   offsetof(uec_api, unsubscribe_world_tick),
               "audio subscriptions must append to uec_api");
UEC_TEST_ASSERT(offsetof(uec_api, get_object_path) >
                   offsetof(uec_api, unbind_audio_finished),
               "object identity queries must append to uec_api");

int main(void)
{
    const uec_api* api = NULL;
    uec_context* context = NULL;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK || api == NULL || context == NULL)
    {
        return 1;
    }

    uec_capabilities capabilities = 0;
    result = api->get_capabilities(context, &capabilities);
    if (result != UEC_RESULT_OK || (capabilities & UEC_CAPABILITY_BOOTSTRAP) == 0 ||
        (capabilities & UEC_CAPABILITY_ACTORS) == 0 ||
        (capabilities & UEC_CAPABILITY_REFLECTION) == 0 ||
        (capabilities & UEC_CAPABILITY_CLASS_METADATA) == 0)
    {
        api->release_context(context);
        return 5;
    }

    const char message[] = "C ABI smoke test";
    const uec_string_view message_view = {message, sizeof(message) - 1u};
    result = api->log(context, message_view);
    if (result != UEC_RESULT_OK)
    {
        api->release_context(context);
        return 2;
    }

    char error[32];
    size_t required = 0;
    result = api->get_last_error(context, error, sizeof(error), &required);
    if (result != UEC_RESULT_OK || required == 0)
    {
        api->release_context(context);
        return 3;
    }

    if (api->struct_size < sizeof(uec_api) || api->abi_major != UEC_ABI_MAJOR)
    {
        api->release_context(context);
        return 6;
    }

    api->release_context(context);
    puts("uec C ABI smoke test passed");
    return 0;
}
