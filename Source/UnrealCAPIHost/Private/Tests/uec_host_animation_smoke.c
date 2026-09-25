#include "uec_api.h"

#include <stdio.h>
#include <string.h>

typedef struct uec_animation_smoke_state {
    const uec_api* api;
    uec_context* context;
    uec_world* world;
    uec_actor* actor;
    uec_scene_component* component;
    uec_object* mesh;
    uec_object* animation;
    uec_object* wrong_object;
    uint64_t animation_subscription_id;
    uint64_t tick_subscription_id;
    uint32_t completion_count;
    double elapsed_seconds;
    uec_result callback_result;
    uec_bool running;
    uec_bool animation_playing;
} uec_animation_smoke_state;

static uec_animation_smoke_state g_animation_smoke;

static uec_string_view AnimationSmokeView(const char* text)
{
    return (uec_string_view){text, strlen(text)};
}

static uec_result ReleaseAnimationSmokeState(uec_animation_smoke_state* state)
{
    if (state == NULL) return UEC_RESULT_INVALID_ARGUMENT;
    const uec_api* api = state->api;
    uec_context* context = state->context;
    uec_result result = state->callback_result;
    if (api != NULL && context != NULL && state->tick_subscription_id != 0u) {
        const uec_result unsubscribeResult = api->unsubscribe_world_tick(
            context, state->tick_subscription_id);
        if (result == UEC_RESULT_OK && unsubscribeResult != UEC_RESULT_OK)
            result = unsubscribeResult;
        state->tick_subscription_id = 0u;
    }
    if (api != NULL && context != NULL && state->animation_subscription_id != 0u &&
        state->completion_count == 0u) {
        const uec_result unbindResult = api->unbind_animation_finished(
            context, state->animation_subscription_id);
        if (result == UEC_RESULT_OK && unbindResult != UEC_RESULT_OK)
            result = unbindResult;
        state->animation_subscription_id = 0u;
    }
    if (api != NULL && state->component != NULL) {
        if (state->animation_playing == UEC_TRUE) {
            const uec_result stopResult = api->stop_skeletal_animation(state->component);
            if (result == UEC_RESULT_OK && stopResult != UEC_RESULT_OK)
                result = stopResult;
            state->animation_playing = UEC_FALSE;
        }
        const uec_result releaseResult = api->release_scene_component(state->component);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
        state->component = NULL;
    }
    if (api != NULL && state->wrong_object != NULL) {
        const uec_result releaseResult = api->release_object(state->wrong_object);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
        state->wrong_object = NULL;
    }
    if (api != NULL && state->animation != NULL) {
        const uec_result releaseResult = api->release_object(state->animation);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
        state->animation = NULL;
    }
    if (api != NULL && state->mesh != NULL) {
        const uec_result releaseResult = api->release_object(state->mesh);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
        state->mesh = NULL;
    }
    if (api != NULL && state->actor != NULL) {
        const uec_result destroyResult = api->destroy_actor(state->actor);
        if (destroyResult != UEC_RESULT_OK) {
            const uec_result releaseResult = api->release_actor(state->actor);
            if (result == UEC_RESULT_OK)
                result = releaseResult == UEC_RESULT_OK ? destroyResult : releaseResult;
        }
        state->actor = NULL;
    }
    if (api != NULL && state->world != NULL) {
        const uec_result releaseResult = api->release_world(state->world);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
        state->world = NULL;
    }
    if (api != NULL && context != NULL) {
        const uec_result releaseResult = api->release_context(context);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
        state->context = NULL;
    }
    return result;
}

static void FinishAnimationSmoke(uec_result result)
{
    if (g_animation_smoke.running != UEC_TRUE) return;
    if (result == UEC_RESULT_OK) result = g_animation_smoke.callback_result;
    g_animation_smoke.callback_result = result;
    result = ReleaseAnimationSmokeState(&g_animation_smoke);
    if (result == UEC_RESULT_OK)
        fprintf(stderr, "C skeletal animation smoke completed\n");
    else
        fprintf(stderr, "C skeletal animation smoke failed with result %d\n", (int)result);
    memset(&g_animation_smoke, 0, sizeof(g_animation_smoke));
}

static void UEC_CALL AnimationFinished(uint64_t subscriptionId, void* userData)
{
    uec_animation_smoke_state* state = (uec_animation_smoke_state*)userData;
    if (state != &g_animation_smoke || state->running != UEC_TRUE) return;
    if (subscriptionId != state->animation_subscription_id)
        state->callback_result = UEC_RESULT_INTERNAL_ERROR;
    ++state->completion_count;
}

static void UEC_CALL AnimationSmokeTick(
    uint64_t subscriptionId, double deltaSeconds, void* userData)
{
    uec_animation_smoke_state* state = (uec_animation_smoke_state*)userData;
    if (state != &g_animation_smoke || state->running != UEC_TRUE) return;
    if (subscriptionId != state->tick_subscription_id || deltaSeconds < 0.0) {
        FinishAnimationSmoke(UEC_RESULT_INTERNAL_ERROR);
        return;
    }
    if (state->completion_count > 1u) {
        FinishAnimationSmoke(UEC_RESULT_INTERNAL_ERROR);
        return;
    }
    if (state->completion_count == 1u) {
        const uec_result stopResult = state->api->stop_skeletal_animation(state->component);
        FinishAnimationSmoke(stopResult);
        return;
    }
    state->elapsed_seconds += deltaSeconds;
    if (state->elapsed_seconds > 10.0)
        FinishAnimationSmoke(UEC_RESULT_INTERNAL_ERROR);
}

static uec_result AbortAnimationSmoke(uec_result result)
{
    g_animation_smoke.running = UEC_TRUE;
    g_animation_smoke.callback_result = result;
    result = ReleaseAnimationSmokeState(&g_animation_smoke);
    memset(&g_animation_smoke, 0, sizeof(g_animation_smoke));
    return result;
}

uec_result UEC_CALL uec_host_animation_smoke_start(void)
{
    static const char pawnClassPath[] =
        "/Script/UnrealCAPIHost.UECAPIHostPlayerFlowPawn";
    static const char skeletalMeshClassPath[] = "/Script/Engine.SkeletalMeshComponent";
    static const char cameraClassPath[] = "/Script/Engine.CameraComponent";
    static const char meshAssetPath[] =
        "/Engine/Tutorial/SubEditors/TutorialAssets/Character/TutorialTPP.TutorialTPP";
    static const char animationAssetPath[] =
        "/Engine/Tutorial/SubEditors/TutorialAssets/Character/Tutorial_Walk_Fwd.Tutorial_Walk_Fwd";
    static const char wrongObjectPath[] = "/Script/Engine.Actor";
    uec_animation_smoke_state* state = &g_animation_smoke;
    if (state->running == UEC_TRUE) return UEC_RESULT_INVALID_ARGUMENT;
    memset(state, 0, sizeof(*state));
    state->callback_result = UEC_RESULT_OK;

    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR,
                                    &state->api, &state->context);
    if (result != UEC_RESULT_OK) return result;
    const uec_api* api = state->api;
    if (api == NULL || state->context == NULL || api->get_default_world == NULL ||
        api->release_world == NULL || api->spawn_actor == NULL ||
        api->destroy_actor == NULL || api->release_actor == NULL ||
        api->get_actor_component_count_by_class == NULL ||
        api->get_actor_component_at_by_class == NULL ||
        api->release_scene_component == NULL || api->load_object == NULL ||
        api->release_object == NULL || api->set_skeletal_mesh == NULL ||
        api->play_skeletal_animation == NULL || api->stop_skeletal_animation == NULL ||
        api->bind_animation_finished == NULL || api->unbind_animation_finished == NULL ||
        api->subscribe_world_tick == NULL || api->unsubscribe_world_tick == NULL ||
        api->release_context == NULL) {
        return AbortAnimationSmoke(UEC_RESULT_UNSUPPORTED);
    }

    result = api->get_default_world(state->context, &state->world);
    if (result == UEC_RESULT_OK && state->world == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    const uec_transform transform = {
        {50000.0, 50000.0, 50000.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    if (result == UEC_RESULT_OK)
        result = api->spawn_actor(state->world, AnimationSmokeView(pawnClassPath),
                                  &transform, &state->actor);
    if (result == UEC_RESULT_OK && state->actor == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;

    uint32_t componentCount = 0u;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_component_count_by_class(
            state->actor, AnimationSmokeView(skeletalMeshClassPath), &componentCount);
    if (result == UEC_RESULT_OK && componentCount == 0u)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_component_at_by_class(
            state->actor, AnimationSmokeView(skeletalMeshClassPath), 0u, &state->component);
    if (result == UEC_RESULT_OK && state->component == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->load_object(state->context, AnimationSmokeView(meshAssetPath), &state->mesh);
    if (result == UEC_RESULT_OK && state->mesh == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->load_object(
            state->context, AnimationSmokeView(animationAssetPath), &state->animation);
    if (result == UEC_RESULT_OK && state->animation == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->load_object(
            state->context, AnimationSmokeView(wrongObjectPath), &state->wrong_object);
    if (result == UEC_RESULT_OK && state->wrong_object == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->set_skeletal_mesh(state->component, state->mesh, UEC_TRUE);

    if (result == UEC_RESULT_OK && api->play_skeletal_animation(
            state->component, state->wrong_object, UEC_FALSE) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    uint32_t cameraCount = 0u;
    uec_scene_component* camera = NULL;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_component_count_by_class(
            state->actor, AnimationSmokeView(cameraClassPath), &cameraCount);
    if (result == UEC_RESULT_OK && cameraCount == 0u)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_component_at_by_class(
            state->actor, AnimationSmokeView(cameraClassPath), 0u, &camera);
    if (result == UEC_RESULT_OK && camera == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK && api->play_skeletal_animation(
            camera, state->animation, UEC_FALSE) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (camera != NULL) {
        const uec_result releaseResult = api->release_scene_component(camera);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
    }
    if (result == UEC_RESULT_OK && api->play_skeletal_animation(
            state->component, state->animation, (uec_bool)2u) != UEC_RESULT_INVALID_ARGUMENT)
        result = UEC_RESULT_INTERNAL_ERROR;

    uint64_t unavailableSubscriptionId = UINT64_MAX;
    if (result == UEC_RESULT_OK && api->bind_animation_finished(
            state->component, AnimationFinished, state,
            &unavailableSubscriptionId) != UEC_RESULT_NOT_INITIALIZED)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK && unavailableSubscriptionId != 0u)
        result = UEC_RESULT_INTERNAL_ERROR;

    if (result == UEC_RESULT_OK) {
        result = api->play_skeletal_animation(state->component, state->animation, UEC_TRUE);
        if (result == UEC_RESULT_OK) state->animation_playing = UEC_TRUE;
    }
    uint64_t cancelledSubscriptionId = 0u;
    if (result == UEC_RESULT_OK)
        result = api->bind_animation_finished(
            state->component, AnimationFinished, state, &cancelledSubscriptionId);
    if (result == UEC_RESULT_OK && cancelledSubscriptionId == 0u)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->unbind_animation_finished(state->context, cancelledSubscriptionId);
    if (result == UEC_RESULT_OK && state->completion_count != 0u)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) {
        result = api->stop_skeletal_animation(state->component);
        if (result == UEC_RESULT_OK) state->animation_playing = UEC_FALSE;
    }

    if (result == UEC_RESULT_OK) {
        result = api->play_skeletal_animation(state->component, state->animation, UEC_FALSE);
        if (result == UEC_RESULT_OK) state->animation_playing = UEC_TRUE;
    }
    if (result == UEC_RESULT_OK)
        result = api->bind_animation_finished(
            state->component, AnimationFinished, state,
            &state->animation_subscription_id);
    if (result == UEC_RESULT_OK && state->animation_subscription_id == 0u)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->subscribe_world_tick(
            state->world, AnimationSmokeTick, state, &state->tick_subscription_id);
    if (result == UEC_RESULT_OK && state->tick_subscription_id == 0u)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result != UEC_RESULT_OK) return AbortAnimationSmoke(result);

    state->running = UEC_TRUE;
    return UEC_RESULT_OK;
}

void UEC_CALL uec_host_animation_smoke_cancel(void)
{
    if (g_animation_smoke.running != UEC_TRUE) return;
    FinishAnimationSmoke(UEC_RESULT_SHUTTING_DOWN);
}

uec_bool UEC_CALL uec_host_animation_smoke_is_running(void)
{
    return g_animation_smoke.running;
}
