#include "CoreMinimal.h"
#include "Misc/Guid.h"

#include <string>

#include "c_playable.h"
#include "uec_api.h"

extern "C" int UEC_CALL uec_host_cleanup_widget_click_playable_button(int32 buttonIndex);

namespace
{
    constexpr uint32 PlayableSubscriptionCount = 5u;

    struct FPlayableSmokeState
    {
        const uec_api* Api = nullptr;
        uec_context* Context = nullptr;
        uec_world* World = nullptr;
        uec_actor* Pawn = nullptr;
        uec_scene_component* Camera = nullptr;
        uec_object* Widget = nullptr;
        std::string Slot;
        uec_playable_sample_state Sample{};
        uec_runtime_stats Baseline{};
        bool BaselineValid = false;
        bool Started = false;
    };

    FPlayableSmokeState GPlayableSmoke;

    bool SameLifetimeStats(const uec_runtime_stats& left,
                           const uec_runtime_stats& right)
    {
        return left.pending_requests == right.pending_requests &&
            left.active_callbacks == right.active_callbacks &&
            left.live_contexts == right.live_contexts &&
            left.live_worlds == right.live_worlds &&
            left.live_actors == right.live_actors &&
            left.live_components == right.live_components &&
            left.live_classes == right.live_classes &&
            left.live_objects == right.live_objects;
    }

    bool SameStatsWithFixtureHandles(const uec_runtime_stats& before,
                                     const uec_runtime_stats& after)
    {
        return before.pending_requests == after.pending_requests &&
            before.active_callbacks == after.active_callbacks &&
            before.live_contexts == after.live_contexts &&
            before.live_actors == after.live_actors &&
            before.live_classes == after.live_classes &&
            before.live_components < UINT32_MAX &&
            after.live_components == before.live_components + 1u &&
            before.live_objects < UINT32_MAX &&
            after.live_objects == before.live_objects + 1u;
    }

    uec_result ReadStats(FPlayableSmokeState& state, uec_runtime_stats& stats)
    {
        if (state.Api == nullptr || state.Context == nullptr ||
            state.Api->get_runtime_stats == nullptr) return UEC_RESULT_UNSUPPORTED;
        stats = {};
        stats.struct_size = sizeof(stats);
        return state.Api->get_runtime_stats(state.Context, &stats);
    }

    uec_result ReleaseFixture(FPlayableSmokeState& state)
    {
        uec_result result = UEC_RESULT_OK;
        if (state.Sample.started && state.Sample.done != UEC_TRUE)
            uec_playable_sample_cancel(&state.Sample);
        if (state.Sample.last_result != UEC_RESULT_OK) result = state.Sample.last_result;
        if (state.Widget != nullptr && state.Api != nullptr &&
            state.Api->release_object != nullptr) {
            const uec_result releaseResult = state.Api->release_object(state.Widget);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
                result = releaseResult;
            state.Widget = nullptr;
        }
        if (state.Camera != nullptr && state.Api != nullptr &&
            state.Api->release_scene_component != nullptr) {
            const uec_result releaseResult = state.Api->release_scene_component(state.Camera);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
                result = releaseResult;
            state.Camera = nullptr;
        }
        state.Started = false;
        return result;
    }

    uec_result ReadWidgetStatus(FPlayableSmokeState& state, const char* expected)
    {
        static constexpr char childNameData[] = "StatusText";
        const uec_string_view childName{childNameData, sizeof(childNameData) - 1u};
        uec_object* child = nullptr;
        uec_result result = state.Api->get_widget_child(state.Widget, childName, &child);
        char text[64] = {0};
        size_t requiredSize = 0u;
        if (result == UEC_RESULT_OK && child == nullptr) result = UEC_RESULT_INTERNAL_ERROR;
        if (result == UEC_RESULT_OK)
            result = state.Api->get_text_block_text(child, text, sizeof(text), &requiredSize);
        if (result == UEC_RESULT_OK && FCStringAnsi::Strcmp(text, expected) != 0)
            result = UEC_RESULT_INTERNAL_ERROR;
        if (child != nullptr) {
            const uec_result releaseResult = state.Api->release_object(child);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
                result = releaseResult;
        }
        return result;
    }

    uec_result VerifyProgress(FPlayableSmokeState& state)
    {
        static constexpr char childNameData[] = "DistanceProgress";
        const uec_string_view childName{childNameData, sizeof(childNameData) - 1u};
        uec_object* child = nullptr;
        double progress = 0.0;
        uec_result result = state.Api->get_widget_child(state.Widget, childName, &child);
        if (result == UEC_RESULT_OK && child == nullptr) result = UEC_RESULT_INTERNAL_ERROR;
        if (result == UEC_RESULT_OK)
            result = state.Api->get_progress_bar_percent(child, &progress);
        if (result == UEC_RESULT_OK && !(progress > 0.0 && progress <= 1.0))
            result = UEC_RESULT_INTERNAL_ERROR;
        if (child != nullptr) {
            const uec_result releaseResult = state.Api->release_object(child);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
                result = releaseResult;
        }
        return result;
    }

    uec_result VerifyMovementFeedback(FPlayableSmokeState& state)
    {
        static constexpr char childNameData[] = "StatusText";
        const uec_string_view childName{childNameData, sizeof(childNameData) - 1u};
        uec_object* child = nullptr;
        uec_result result = state.Api->get_widget_child(state.Widget, childName, &child);
        char text[64] = {0};
        size_t requiredSize = 0u;
        if (result == UEC_RESULT_OK && child == nullptr) result = UEC_RESULT_INTERNAL_ERROR;
        if (result == UEC_RESULT_OK)
            result = state.Api->get_text_block_text(child, text, sizeof(text), &requiredSize);
        if (result == UEC_RESULT_OK &&
            (FCStringAnsi::Strncmp(text, "Move ", 5) != 0 ||
             (FCStringAnsi::Strstr(text, " | clear") == nullptr &&
              FCStringAnsi::Strstr(text, " | blocked") == nullptr)))
            result = UEC_RESULT_INTERNAL_ERROR;
        if (child != nullptr) {
            const uec_result releaseResult = state.Api->release_object(child);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK)
                result = releaseResult;
        }
        return result;
    }
}

extern "C" uec_result UEC_CALL uec_host_playable_smoke_start(
    const uec_api* api,
    uec_context* context,
    uec_world* world,
    uec_actor* controller,
    uec_actor* pawn,
    uec_object* mapping_context,
    uec_object* move_action)
{
    FPlayableSmokeState& state = GPlayableSmoke;
    if (state.Started) return UEC_RESULT_INVALID_ARGUMENT;
    state = {};
    state.Api = api;
    state.Context = context;
    state.World = world;
    state.Pawn = pawn;
    if (api == nullptr || context == nullptr || world == nullptr ||
        controller == nullptr || pawn == nullptr ||
        mapping_context == nullptr || move_action == nullptr ||
        api->get_actor_component_count_by_class == nullptr ||
        api->get_actor_component_at_by_class == nullptr || api->get_camera_field_of_view == nullptr ||
        api->create_widget == nullptr || api->get_runtime_stats == nullptr ||
        api->get_widget_child == nullptr || api->get_text_block_text == nullptr ||
        api->get_progress_bar_percent == nullptr) return UEC_RESULT_UNSUPPORTED;

    static constexpr char cameraClassData[] = "/Script/Engine.CameraComponent";
    static constexpr char widgetClassData[] = "/Script/UnrealCAPIHost.ECAPIHostCleanupWidget";
    const uec_string_view cameraClass{cameraClassData, sizeof(cameraClassData) - 1u};
    const uec_string_view widgetClass{widgetClassData, sizeof(widgetClassData) - 1u};
    uec_result result = ReadStats(state, state.Baseline);
    if (result == UEC_RESULT_OK) state.BaselineValid = true;
    uint32 cameraCount = 0u;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_component_count_by_class(pawn, cameraClass, &cameraCount);
    if (result == UEC_RESULT_OK && cameraCount != 1u) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->get_actor_component_at_by_class(pawn, cameraClass, 0u, &state.Camera);
    if (result == UEC_RESULT_OK && state.Camera == nullptr) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK)
        result = api->create_widget(world, widgetClass, &state.Widget);
    if (result == UEC_RESULT_OK && state.Widget == nullptr) result = UEC_RESULT_INTERNAL_ERROR;
    const FString slotName = FString::Printf(
        TEXT("UEC-Playable-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    if (result == UEC_RESULT_OK) {
        FTCHARToUTF8 slotUtf8(*slotName);
        state.Slot.assign(slotUtf8.Get(), static_cast<size_t>(slotUtf8.Length()));
        const uec_string_view slot{state.Slot.data(), state.Slot.size()};
        result = uec_playable_sample_start(api, context, controller, pawn, mapping_context,
            move_action, state.Camera, state.Widget, slot, 12.0, 37, &state.Sample);
    }
    if (result == UEC_RESULT_OK) {
        state.Started = true;
        uec_runtime_stats observed{};
        result = ReadStats(state, observed);
        if (result == UEC_RESULT_OK &&
            (state.Baseline.active_subscriptions > UINT32_MAX - PlayableSubscriptionCount ||
             observed.active_subscriptions != state.Baseline.active_subscriptions +
                 PlayableSubscriptionCount ||
             state.Baseline.live_worlds == UINT32_MAX ||
             observed.live_worlds != state.Baseline.live_worlds + 1u ||
             !SameStatsWithFixtureHandles(state.Baseline, observed)))
            result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result != UEC_RESULT_OK) {
        (void)ReleaseFixture(state);
        return result;
    }
    return UEC_RESULT_OK;
}

extern "C" uec_result UEC_CALL uec_host_playable_smoke_verify(void)
{
    FPlayableSmokeState& state = GPlayableSmoke;
    if (!state.Started || state.Sample.last_result != UEC_RESULT_OK ||
        state.Sample.input_event_count == 0u || state.Sample.movement_step_count == 0u ||
        state.Sample.save_button_subscription_id == 0u ||
        state.Sample.load_button_subscription_id == 0u)
        return UEC_RESULT_INTERNAL_ERROR;

    double fov = 0.0;
    uec_result result = state.Api->get_camera_field_of_view(state.Camera, &fov);
    if (result == UEC_RESULT_OK && !(fov > state.Sample.default_camera_fov))
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) result = VerifyMovementFeedback(state);
    uec_transform saved{};
    if (result == UEC_RESULT_OK) result = state.Api->get_actor_transform(state.Pawn, &saved);
    if (result == UEC_RESULT_OK && uec_host_cleanup_widget_click_playable_button(0) != 1)
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK && (state.Sample.save_count != 1u ||
        state.Sample.save_button_subscription_id != 0u)) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) result = ReadWidgetStatus(state, "Saved current position");

    uec_transform moved = saved;
    moved.translation.x += 700.0;
    moved.translation.y -= 400.0;
    if (result == UEC_RESULT_OK) result = state.Api->set_actor_transform(state.Pawn, &moved, UEC_FALSE);
    if (result == UEC_RESULT_OK && uec_host_cleanup_widget_click_playable_button(1) != 1)
        result = UEC_RESULT_INTERNAL_ERROR;
    uec_transform loaded{};
    if (result == UEC_RESULT_OK) result = state.Api->get_actor_transform(state.Pawn, &loaded);
    if (result == UEC_RESULT_OK &&
        (!FMath::IsNearlyEqual(static_cast<float>(saved.translation.x),
                               static_cast<float>(loaded.translation.x)) ||
         !FMath::IsNearlyEqual(static_cast<float>(saved.translation.y),
                               static_cast<float>(loaded.translation.y)) ||
         !FMath::IsNearlyEqual(static_cast<float>(saved.translation.z),
                               static_cast<float>(loaded.translation.z)) ||
         state.Sample.load_count != 1u || state.Sample.load_button_subscription_id != 0u))
        result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) result = ReadWidgetStatus(state, "Loaded saved position");
    if (result == UEC_RESULT_OK) result = VerifyProgress(state);

    uec_runtime_stats observed{};
    if (result == UEC_RESULT_OK) result = ReadStats(state, observed);
    if (result == UEC_RESULT_OK &&
        (observed.active_subscriptions != state.Baseline.active_subscriptions + 6u ||
         observed.live_worlds != state.Baseline.live_worlds + 1u ||
         !SameStatsWithFixtureHandles(state.Baseline, observed)))
        result = UEC_RESULT_INTERNAL_ERROR;
    return result;
}

extern "C" uec_result UEC_CALL uec_host_playable_smoke_cancel(void)
{
    FPlayableSmokeState& state = GPlayableSmoke;
    if (state.Api == nullptr) return UEC_RESULT_OK;
    uec_result result = ReleaseFixture(state);
    if (state.Context != nullptr && state.BaselineValid) {
        uec_runtime_stats observed{};
        if (ReadStats(state, observed) != UEC_RESULT_OK ||
            observed.active_subscriptions != state.Baseline.active_subscriptions ||
            !SameLifetimeStats(state.Baseline, observed)) {
            UE_LOG(LogTemp, Error, TEXT("Playable sample smoke cleanup leaked API state"));
            if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    state = {};
    return result;
}
