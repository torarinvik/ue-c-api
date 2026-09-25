#include "uec_api.h"

#include <stdio.h>
#include <string.h>

typedef struct uec_animation_smoke_state {
    const uec_api* api;
    uec_context* context;
    uec_world* world;
    uec_actor* actor;
    uec_scene_component* component;
    uec_object* audio_component;
    uec_object* destroyable_audio_component;
    uec_object* stoppable_audio_component;
    uec_object* completion_audio_component;
    uec_object* sound;
    uec_object* mesh;
    uec_object* animation;
    uec_object* wrong_object;
    uint64_t animation_subscription_id;
    uint64_t audio_subscription_id;
    uint64_t destroyable_audio_subscription_id;
    uint64_t stopped_audio_subscription_id;
    uint64_t completion_audio_subscription_id;
    uint64_t tick_subscription_id;
    uint32_t completion_count;
    uint32_t audio_callback_count;
    uint32_t stopped_audio_callback_count;
    uint32_t completion_audio_callback_count;
    uint32_t reentrant_unbind_count;
    uint32_t callbacks_observed_in_flight;
    uint32_t audio_callbacks_observed_in_flight;
    uint32_t baseline_subscriptions;
    double elapsed_seconds;
    uec_result callback_result;
    uec_bool running;
    uec_bool animation_playing;
    uec_bool audio_callbacks_verified;
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
    if (api != NULL && context != NULL && state->audio_subscription_id != 0u) {
        const uec_result unbindResult = api->unbind_audio_finished(
            context, state->audio_subscription_id);
        if (result == UEC_RESULT_OK && unbindResult != UEC_RESULT_OK)
            result = unbindResult;
        state->audio_subscription_id = 0u;
    }
    if (api != NULL && context != NULL && state->stopped_audio_subscription_id != 0u) {
        const uec_result unbindResult = api->unbind_audio_finished(
            context, state->stopped_audio_subscription_id);
        if (result == UEC_RESULT_OK && unbindResult != UEC_RESULT_OK)
            result = unbindResult;
        state->stopped_audio_subscription_id = 0u;
    }
    if (api != NULL && context != NULL && state->completion_audio_subscription_id != 0u) {
        const uec_result unbindResult = api->unbind_audio_finished(
            context, state->completion_audio_subscription_id);
        if (result == UEC_RESULT_OK && unbindResult != UEC_RESULT_OK)
            result = unbindResult;
        state->completion_audio_subscription_id = 0u;
    }
    if (api != NULL && context != NULL &&
        state->destroyable_audio_subscription_id != 0u) {
        const uec_result unbindResult = api->unbind_audio_finished(
            context, state->destroyable_audio_subscription_id);
        if (result == UEC_RESULT_OK && unbindResult != UEC_RESULT_OK)
            result = unbindResult;
        state->destroyable_audio_subscription_id = 0u;
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
    if (api != NULL && state->audio_component != NULL) {
        const uec_result releaseResult = api->release_object(state->audio_component);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
        state->audio_component = NULL;
    }
    if (api != NULL && state->destroyable_audio_component != NULL) {
        const uec_result releaseResult = api->release_object(
            state->destroyable_audio_component);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
        state->destroyable_audio_component = NULL;
    }
    if (api != NULL && state->stoppable_audio_component != NULL) {
        const uec_result releaseResult = api->release_object(
            state->stoppable_audio_component);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
        state->stoppable_audio_component = NULL;
    }
    if (api != NULL && state->completion_audio_component != NULL) {
        const uec_result releaseResult = api->release_object(
            state->completion_audio_component);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
        state->completion_audio_component = NULL;
    }
    if (api != NULL && state->sound != NULL) {
        const uec_result releaseResult = api->release_object(state->sound);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
            result = releaseResult;
        state->sound = NULL;
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

static void UEC_CALL AnimationFinished(uint64_t subscriptionId, void* userData);

static uec_result AnimationCleanupStepFailed(const char* step, uec_result result)
{
    fprintf(stderr, "Animation actor-cleanup step %s failed with result %d\n",
            step, (int)result);
    return result;
}

static void UEC_CALL AudioFinished(uint64_t subscriptionId, void* userData)
{
    uec_animation_smoke_state* state = (uec_animation_smoke_state*)userData;
    if (state != &g_animation_smoke || state->running != UEC_TRUE) return;
    if (subscriptionId == state->stopped_audio_subscription_id ||
        subscriptionId == state->completion_audio_subscription_id) {
        uec_runtime_stats stats = {0};
        stats.struct_size = sizeof(stats);
        const uec_result result = state->api->get_runtime_stats(state->context, &stats);
        if (result != UEC_RESULT_OK || stats.active_callbacks != 1u) {
            state->callback_result = result == UEC_RESULT_OK ?
                UEC_RESULT_INTERNAL_ERROR : result;
        } else {
            ++state->audio_callbacks_observed_in_flight;
        }
        if (subscriptionId == state->stopped_audio_subscription_id)
            ++state->stopped_audio_callback_count;
        else
            ++state->completion_audio_callback_count;
        return;
    }
    if (subscriptionId != state->audio_subscription_id &&
        subscriptionId != state->destroyable_audio_subscription_id) {
        state->callback_result = UEC_RESULT_INTERNAL_ERROR;
    }
    ++state->audio_callback_count;
}

static uec_result BeginAudioPlaybackSmoke(uec_animation_smoke_state* state)
{
    uec_result result = state->api->get_actor_property_object(
        state->actor, AnimationSmokeView("CookedTestSound"), &state->sound);
    if (result != UEC_RESULT_OK || state->sound == NULL)
        return AnimationCleanupStepFailed(
            "sound-fixture", result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result);

    uec_bool isSound = UEC_FALSE;
    result = state->api->object_is_a(
        state->sound, AnimationSmokeView("/Script/Engine.SoundBase"), &isSound);
    if (result != UEC_RESULT_OK || isSound != UEC_TRUE)
        return AnimationCleanupStepFailed(
            "sound-type", result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result);

    const uec_vector3 location = {50000.0, 50000.0, 50000.0};
    result = state->api->play_sound_at_location(
        state->world, state->sound, location, 0.01, 1.0);
    if (result != UEC_RESULT_OK)
        return AnimationCleanupStepFailed("play-at-location", result);
    if (state->api->play_sound_at_location(
            state->world, state->wrong_object, location, 1.0, 1.0) !=
        UEC_RESULT_INVALID_ARGUMENT ||
        state->api->play_sound_at_location(
            state->world, state->sound, location, 1.0, 0.0) !=
        UEC_RESULT_INVALID_ARGUMENT) {
        return AnimationCleanupStepFailed(
            "play-at-location-validation", UEC_RESULT_INTERNAL_ERROR);
    }

    uec_object* rejectedAudio = state->sound;
    if (state->api->spawn_sound_attached(
            state->component, state->sound, AnimationSmokeView(""), 1.0, 0.0,
            &rejectedAudio) != UEC_RESULT_INVALID_ARGUMENT || rejectedAudio != NULL) {
        return AnimationCleanupStepFailed(
            "attached-playback-validation", UEC_RESULT_INTERNAL_ERROR);
    }
    result = state->api->spawn_sound_attached(
        state->component, state->sound, AnimationSmokeView(""), 0.01, 1.0,
        &state->stoppable_audio_component);
    if (result != UEC_RESULT_OK || state->stoppable_audio_component == NULL)
        return AnimationCleanupStepFailed(
            "attached-playback", result == UEC_RESULT_OK ?
                UEC_RESULT_INTERNAL_ERROR : result);

    uec_bool isPlaying = UEC_FALSE;
    result = state->api->get_audio_component_playing(
        state->stoppable_audio_component, &isPlaying);
    if (result != UEC_RESULT_OK || isPlaying != UEC_TRUE)
        return AnimationCleanupStepFailed(
            "attached-playing-state", result == UEC_RESULT_OK ?
                UEC_RESULT_INTERNAL_ERROR : result);
    result = state->api->bind_audio_finished(
        state->stoppable_audio_component, AudioFinished, state,
        &state->stopped_audio_subscription_id);
    if (result != UEC_RESULT_OK || state->stopped_audio_subscription_id == 0u)
        return AnimationCleanupStepFailed(
            "stopped-audio-bind", result == UEC_RESULT_OK ?
                UEC_RESULT_INTERNAL_ERROR : result);
    result = state->api->stop_audio_component(state->stoppable_audio_component);
    if (result != UEC_RESULT_OK)
        return AnimationCleanupStepFailed("stop-audio-component", result);
    isPlaying = UEC_TRUE;
    result = state->api->get_audio_component_playing(
        state->stoppable_audio_component, &isPlaying);
    if (result != UEC_RESULT_OK || isPlaying != UEC_FALSE)
        return AnimationCleanupStepFailed(
            "stopped-playing-state", result == UEC_RESULT_OK ?
                UEC_RESULT_INTERNAL_ERROR : result);

    result = state->api->spawn_sound_attached(
        state->component, state->sound, AnimationSmokeView(""), 0.01, 1.0,
        &state->completion_audio_component);
    if (result != UEC_RESULT_OK || state->completion_audio_component == NULL)
        return AnimationCleanupStepFailed(
            "completion-playback", result == UEC_RESULT_OK ?
                UEC_RESULT_INTERNAL_ERROR : result);
    result = state->api->bind_audio_finished(
        state->completion_audio_component, AudioFinished, state,
        &state->completion_audio_subscription_id);
    if (result != UEC_RESULT_OK || state->completion_audio_subscription_id == 0u)
        return AnimationCleanupStepFailed(
            "completion-audio-bind", result == UEC_RESULT_OK ?
                UEC_RESULT_INTERNAL_ERROR : result);
    return UEC_RESULT_OK;
}

static uec_result VerifyAudioPlaybackCompletions(uec_animation_smoke_state* state)
{
    if (state->stopped_audio_callback_count != 1u ||
        state->completion_audio_callback_count != 1u ||
        state->audio_callbacks_observed_in_flight != 2u) {
        return UEC_RESULT_INTERNAL_ERROR;
    }
    uec_bool isPlaying = UEC_TRUE;
    uec_result result = state->api->get_audio_component_playing(
        state->stoppable_audio_component, &isPlaying);
    if (result != UEC_RESULT_OK || isPlaying != UEC_FALSE)
        return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
    result = state->api->get_audio_component_playing(
        state->completion_audio_component, &isPlaying);
    if (result != UEC_RESULT_OK || isPlaying != UEC_FALSE)
        return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;

    result = state->api->unbind_audio_finished(
        state->context, state->stopped_audio_subscription_id);
    if (result != UEC_RESULT_INVALID_ARGUMENT) return UEC_RESULT_INTERNAL_ERROR;
    state->stopped_audio_subscription_id = 0u;
    result = state->api->unbind_audio_finished(
        state->context, state->completion_audio_subscription_id);
    if (result != UEC_RESULT_INVALID_ARGUMENT) return UEC_RESULT_INTERNAL_ERROR;
    state->completion_audio_subscription_id = 0u;
    state->audio_callbacks_verified = UEC_TRUE;
    return UEC_RESULT_OK;
}

static uec_result VerifyAnimationSubscriptionActorCleanup(
    uec_animation_smoke_state* state)
{
    uec_result result = state->api->stop_skeletal_animation(state->component);
    if (result != UEC_RESULT_OK) return AnimationCleanupStepFailed("stop", result);
    state->animation_playing = UEC_FALSE;
    result = state->api->play_skeletal_animation(
        state->component, state->animation, UEC_TRUE);
    if (result != UEC_RESULT_OK) return AnimationCleanupStepFailed("replay", result);
    state->animation_playing = UEC_TRUE;
    uint64_t cancelledSubscriptionId = 0u;
    result = state->api->bind_animation_finished(
        state->component, AnimationFinished, state, &cancelledSubscriptionId);
    if (result != UEC_RESULT_OK || cancelledSubscriptionId == 0u)
        return AnimationCleanupStepFailed(
            "bind", result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result);
    state->animation_subscription_id = cancelledSubscriptionId;

    result = state->api->bind_audio_finished(
        state->destroyable_audio_component, AudioFinished, state,
        &state->destroyable_audio_subscription_id);
    if (result != UEC_RESULT_OK || state->destroyable_audio_subscription_id == 0u)
        return AnimationCleanupStepFailed(
            "explicit-audio-bind",
            result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result);

    uec_runtime_stats stats = {0};
    stats.struct_size = sizeof(stats);
    result = state->api->get_runtime_stats(state->context, &stats);
    if (result != UEC_RESULT_OK ||
        stats.active_subscriptions != state->baseline_subscriptions + 3u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        return AnimationCleanupStepFailed("explicit-audio-pre-destroy-stats", result);
    }
    result = state->api->destroy_audio_component(state->destroyable_audio_component);
    if (result != UEC_RESULT_OK)
        return AnimationCleanupStepFailed("destroy-audio-component", result);
    uec_bool audioPlaying = UEC_TRUE;
    if (state->api->get_audio_component_playing(
            state->destroyable_audio_component, &audioPlaying) !=
            UEC_RESULT_INVALID_HANDLE || audioPlaying != UEC_FALSE) {
        return AnimationCleanupStepFailed(
            "destroyed-audio-handle", UEC_RESULT_INTERNAL_ERROR);
    }
    const uec_result explicitAudioUnbindResult = state->api->unbind_audio_finished(
        state->context, state->destroyable_audio_subscription_id);
    if (explicitAudioUnbindResult != UEC_RESULT_INVALID_ARGUMENT)
        return AnimationCleanupStepFailed("explicit-audio-token",
            explicitAudioUnbindResult == UEC_RESULT_OK ?
                UEC_RESULT_INTERNAL_ERROR : explicitAudioUnbindResult);
    state->destroyable_audio_subscription_id = 0u;
    if (state->audio_callback_count != 0u)
        return AnimationCleanupStepFailed("explicit-audio-callback",
                                          UEC_RESULT_INTERNAL_ERROR);
    if (state->api->release_object(state->destroyable_audio_component) !=
        UEC_RESULT_INVALID_HANDLE) {
        return AnimationCleanupStepFailed(
            "released-destroyed-audio", UEC_RESULT_INTERNAL_ERROR);
    }
    state->destroyable_audio_component = NULL;
    stats = (uec_runtime_stats){0};
    stats.struct_size = sizeof(stats);
    result = state->api->get_runtime_stats(state->context, &stats);
    if (result != UEC_RESULT_OK ||
        stats.active_subscriptions != state->baseline_subscriptions + 2u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        return AnimationCleanupStepFailed("explicit-audio-post-destroy-stats", result);
    }

    result = state->api->bind_audio_finished(
        (uec_object*)state->audio_component, AudioFinished, state,
        &state->audio_subscription_id);
    if (result != UEC_RESULT_OK || state->audio_subscription_id == 0u)
        return AnimationCleanupStepFailed(
            "audio-bind", result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result);

    stats = (uec_runtime_stats){0};
    stats.struct_size = sizeof(stats);
    result = state->api->get_runtime_stats(state->context, &stats);
    if (result != UEC_RESULT_OK ||
        stats.active_subscriptions != state->baseline_subscriptions + 3u) {
        if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        return AnimationCleanupStepFailed(
            "pre-destroy-stats", result == UEC_RESULT_OK ?
                UEC_RESULT_INTERNAL_ERROR : result);
    }

    uec_actor* destroyedActor = state->actor;
    result = state->api->destroy_actor(destroyedActor);
    if (result != UEC_RESULT_OK) return AnimationCleanupStepFailed("destroy", result);
    state->animation_playing = UEC_FALSE;
    state->actor = NULL;
    uec_transform ignoredTransform = {0};
    result = state->api->get_actor_transform(destroyedActor, &ignoredTransform);
    if (result != UEC_RESULT_INVALID_HANDLE)
        return AnimationCleanupStepFailed(
            "destroyed-actor-handle", result == UEC_RESULT_OK ?
                UEC_RESULT_INTERNAL_ERROR : result);
    result = state->api->release_scene_component(state->component);
    if (result != UEC_RESULT_OK)
        return AnimationCleanupStepFailed("release-component", result);
    state->component = NULL;
    result = state->api->release_object(state->audio_component);
    if (result != UEC_RESULT_OK)
        return AnimationCleanupStepFailed("release-audio-component", result);
    state->audio_component = NULL;

    const uec_result unbindResult = state->api->unbind_animation_finished(
        state->context, cancelledSubscriptionId);
    if (unbindResult == UEC_RESULT_OK)
        return AnimationCleanupStepFailed("subscription-still-present",
                                          UEC_RESULT_INTERNAL_ERROR);
    if (unbindResult != UEC_RESULT_INVALID_ARGUMENT)
        return AnimationCleanupStepFailed("cancelled-token", unbindResult);
    state->animation_subscription_id = 0u;
    if (state->completion_count != 1u) return UEC_RESULT_INTERNAL_ERROR;
    const uec_result audioUnbindResult = state->api->unbind_audio_finished(
        state->context, state->audio_subscription_id);
    if (audioUnbindResult == UEC_RESULT_OK)
        return AnimationCleanupStepFailed("audio-subscription-still-present",
                                          UEC_RESULT_INTERNAL_ERROR);
    if (audioUnbindResult != UEC_RESULT_INVALID_ARGUMENT)
        return AnimationCleanupStepFailed("audio-cancelled-token", audioUnbindResult);
    state->audio_subscription_id = 0u;
    if (state->audio_callback_count != 0u)
        return AnimationCleanupStepFailed("audio-callback-not-suppressed",
                                          UEC_RESULT_INTERNAL_ERROR);
    stats = (uec_runtime_stats){0};
    stats.struct_size = sizeof(stats);
    result = state->api->get_runtime_stats(state->context, &stats);
    if (result != UEC_RESULT_OK)
        return AnimationCleanupStepFailed("post-destroy-stats", result);
    if (stats.active_subscriptions != state->baseline_subscriptions + 1u)
        return AnimationCleanupStepFailed("subscription-count",
                                          UEC_RESULT_INTERNAL_ERROR);
    return UEC_RESULT_OK;
}

static void UEC_CALL AnimationFinished(uint64_t subscriptionId, void* userData)
{
    uec_animation_smoke_state* state = (uec_animation_smoke_state*)userData;
    if (state != &g_animation_smoke || state->running != UEC_TRUE) return;
    if (subscriptionId != state->animation_subscription_id)
        state->callback_result = UEC_RESULT_INTERNAL_ERROR;
    uec_runtime_stats stats = {0};
    stats.struct_size = sizeof(stats);
    const uec_result statsResult = state->api->get_runtime_stats(
        state->context, &stats);
    if (statsResult != UEC_RESULT_OK || stats.active_callbacks != 1u) {
        state->callback_result = statsResult == UEC_RESULT_OK ?
            UEC_RESULT_INTERNAL_ERROR : statsResult;
    } else {
        state->callbacks_observed_in_flight = stats.active_callbacks;
    }
    const uec_result unbindResult = state->api->unbind_animation_finished(
        state->context, subscriptionId);
    if (unbindResult != UEC_RESULT_OK) {
        if (state->callback_result == UEC_RESULT_OK)
            state->callback_result = unbindResult;
    } else {
        ++state->reentrant_unbind_count;
        state->animation_subscription_id = 0u;
    }
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
    if (state->completion_count > 1u || state->stopped_audio_callback_count > 1u ||
        state->completion_audio_callback_count > 1u) {
        FinishAnimationSmoke(UEC_RESULT_INTERNAL_ERROR);
        return;
    }
    if (state->audio_callbacks_verified != UEC_TRUE &&
        state->stopped_audio_callback_count == 1u &&
        state->completion_audio_callback_count == 1u) {
        const uec_result audioResult = VerifyAudioPlaybackCompletions(state);
        if (audioResult != UEC_RESULT_OK) {
            FinishAnimationSmoke(audioResult);
            return;
        }
    }
    if (state->completion_count == 1u && state->audio_callbacks_verified == UEC_TRUE) {
        if (state->reentrant_unbind_count != 1u ||
            state->callbacks_observed_in_flight != 1u) {
            FinishAnimationSmoke(UEC_RESULT_INTERNAL_ERROR);
            return;
        }
        FinishAnimationSmoke(VerifyAnimationSubscriptionActorCleanup(state));
        return;
    }
    state->elapsed_seconds += deltaSeconds;
    /* Allow for large editor hitches while asynchronous assets finish loading in PIE. */
    if (state->elapsed_seconds > 60.0)
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
        api->get_runtime_stats == NULL ||
        api->release_world == NULL || api->spawn_actor == NULL ||
        api->destroy_actor == NULL || api->release_actor == NULL ||
        api->get_actor_transform == NULL ||
        api->get_actor_component_count_by_class == NULL ||
        api->get_actor_component_at_by_class == NULL ||
        api->get_actor_property_object == NULL ||
        api->object_is_a == NULL || api->play_sound_at_location == NULL ||
        api->spawn_sound_attached == NULL || api->stop_audio_component == NULL ||
        api->bind_audio_finished == NULL || api->unbind_audio_finished == NULL ||
        api->destroy_audio_component == NULL ||
        api->get_audio_component_playing == NULL ||
        api->release_scene_component == NULL || api->load_object == NULL ||
        api->release_object == NULL || api->set_skeletal_mesh == NULL ||
        api->play_skeletal_animation == NULL || api->stop_skeletal_animation == NULL ||
        api->bind_animation_finished == NULL || api->unbind_animation_finished == NULL ||
        api->subscribe_world_tick == NULL || api->unsubscribe_world_tick == NULL ||
        api->release_context == NULL) {
        return AbortAnimationSmoke(UEC_RESULT_UNSUPPORTED);
    }

    uec_runtime_stats baseline = {0};
    baseline.struct_size = sizeof(baseline);
    result = api->get_runtime_stats(state->context, &baseline);
    if (result != UEC_RESULT_OK) return AbortAnimationSmoke(result);
    if (baseline.active_subscriptions > UINT32_MAX - 3u)
        return AbortAnimationSmoke(UEC_RESULT_INTERNAL_ERROR);
    state->baseline_subscriptions = baseline.active_subscriptions;

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

    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_object(
            state->actor, AnimationSmokeView("FlowAudio"), &state->audio_component);
    if (result == UEC_RESULT_OK && state->audio_component == NULL)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_property_object(
            state->actor, AnimationSmokeView("FlowAudioForDestroySmoke"),
            &state->destroyable_audio_component);
    if (result == UEC_RESULT_OK && state->destroyable_audio_component == NULL)
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
    result = BeginAudioPlaybackSmoke(state);
    if (result != UEC_RESULT_OK) return AbortAnimationSmoke(result);
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
