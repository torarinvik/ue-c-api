#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "uec_api.h"

namespace
{
    enum class EInputSmokeStage : uint8
    {
        WaitingForInput,
        WaitingForSuppressedCallback
    };

    struct FInputSmokeState
    {
        const uec_api* Api = nullptr;
        uec_context* Context = nullptr;
        uec_world* World = nullptr;
        uec_actor* Controller = nullptr;
        uec_actor* Actor = nullptr;
        uec_object* Action = nullptr;
        uec_object* MappingContext = nullptr;
        uec_object* Subsystem = nullptr;
        uint64 BindingId = 0;
        uint32 CallbackCount = 0;
        uint32 CallbackCountAfterUnbind = 0;
        uint32 SuppressionPolls = 0;
        uec_input_action_value ObservedValue{};
        EInputSmokeStage Stage = EInputSmokeStage::WaitingForInput;
        uec_result Result = UEC_RESULT_NOT_INITIALIZED;
        bool Started = false;
        bool Complete = false;
        bool MappingAdded = false;
        bool CallbackInvalid = false;
    };

    FInputSmokeState GInputSmokeState;

    void FinishInputSmoke(FInputSmokeState& state, uec_result result)
    {
        if (state.Complete) return;
        if (state.BindingId != 0 && state.Api != nullptr && state.Context != nullptr &&
            state.Api->unbind_input_action != nullptr) {
            const uec_result unbindResult = state.Api->unbind_input_action(
                state.Context, state.BindingId);
            if (result == UEC_RESULT_OK && unbindResult != UEC_RESULT_OK) {
                result = unbindResult;
            }
            state.BindingId = 0;
        }
        if (state.MappingAdded && state.Api != nullptr && state.Controller != nullptr &&
            state.MappingContext != nullptr && state.Api->remove_input_mapping_context != nullptr) {
            const uec_result removeResult = state.Api->remove_input_mapping_context(
                state.Controller, state.MappingContext);
            if (result == UEC_RESULT_OK && removeResult != UEC_RESULT_OK) {
                result = removeResult;
            }
            state.MappingAdded = false;
        }
        if (state.Subsystem != nullptr && state.Api != nullptr &&
            state.Api->release_object != nullptr) {
            const uec_result releaseResult = state.Api->release_object(state.Subsystem);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
            state.Subsystem = nullptr;
        }
        if (state.MappingContext != nullptr && state.Api != nullptr &&
            state.Api->release_object != nullptr) {
            const uec_result releaseResult = state.Api->release_object(state.MappingContext);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
            state.MappingContext = nullptr;
        }
        if (state.Action != nullptr && state.Api != nullptr &&
            state.Api->release_object != nullptr) {
            const uec_result releaseResult = state.Api->release_object(state.Action);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
            state.Action = nullptr;
        }
        if (state.Actor != nullptr && state.Api != nullptr) {
            uec_result destroyResult = UEC_RESULT_INVALID_HANDLE;
            if (state.Api->destroy_actor != nullptr) {
                destroyResult = state.Api->destroy_actor(state.Actor);
                if (result == UEC_RESULT_OK && destroyResult != UEC_RESULT_OK) {
                    result = destroyResult;
                }
            }
            if (destroyResult != UEC_RESULT_OK && state.Api->release_actor != nullptr) {
                const uec_result releaseResult = state.Api->release_actor(state.Actor);
                if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                    result = releaseResult;
                }
            }
            state.Actor = nullptr;
        }
        if (state.Controller != nullptr && state.Api != nullptr &&
            state.Api->release_actor != nullptr) {
            const uec_result releaseResult = state.Api->release_actor(state.Controller);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
            state.Controller = nullptr;
        }
        if (state.World != nullptr && state.Api != nullptr &&
            state.Api->release_world != nullptr) {
            const uec_result releaseResult = state.Api->release_world(state.World);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
            state.World = nullptr;
        }
        if (state.Context != nullptr && state.Api != nullptr &&
            state.Api->release_context != nullptr) {
            const uec_result releaseResult = state.Api->release_context(state.Context);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
            state.Context = nullptr;
        }
        state.Result = result;
        state.Complete = true;
    }

    uec_result FailInputSmokeStart(FInputSmokeState& state, uec_result result)
    {
        FinishInputSmoke(state, result);
        return result;
    }

    bool IsExpectedInputValue(const uec_input_action_value& value)
    {
        return value.struct_size >= sizeof(uec_input_action_value) &&
            value.kind == UEC_INPUT_ACTION_VALUE_AXIS_1D &&
            FMath::IsNearlyEqual(static_cast<float>(value.axis.x), 0.75f) &&
            FMath::IsNearlyZero(static_cast<float>(value.axis.y)) &&
            FMath::IsNearlyZero(static_cast<float>(value.axis.z));
    }

    void UEC_CALL OnInputSmokeAction(uint64_t bindingId,
                                    uec_input_action_value value,
                                    void* userData)
    {
        auto* state = static_cast<FInputSmokeState*>(userData);
        if (state == nullptr || state != &GInputSmokeState) return;
        if (bindingId != state->BindingId || !IsExpectedInputValue(value)) {
            state->CallbackInvalid = true;
        }
        ++state->CallbackCount;
        state->ObservedValue = value;
    }

    uec_result InjectExpectedValue(FInputSmokeState& state)
    {
        uec_input_action_value value{};
        value.struct_size = sizeof(value);
        value.kind = UEC_INPUT_ACTION_VALUE_AXIS_1D;
        value.axis.x = 0.75;
        return state.Api->inject_input_action_value(
            state.Controller, state.Action, &value);
    }
}

extern "C" uec_result UEC_CALL uec_host_input_smoke_start(void)
{
    FInputSmokeState& state = GInputSmokeState;
    if (state.Started && !state.Complete) return UEC_RESULT_INVALID_ARGUMENT;
    state = {};
    state.Started = true;

    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR,
                                    &state.Api, &state.Context);
    if (result != UEC_RESULT_OK) return FailInputSmokeStart(state, result);
    if (state.Api == nullptr || state.Context == nullptr ||
        state.Api->release_context == nullptr || state.Api->get_default_world == nullptr ||
        state.Api->release_world == nullptr || state.Api->get_player_controller == nullptr ||
        state.Api->release_actor == nullptr || state.Api->spawn_actor == nullptr ||
        state.Api->destroy_actor == nullptr || state.Api->get_actor_property_object == nullptr ||
        state.Api->release_object == nullptr || state.Api->object_is_a == nullptr ||
        state.Api->get_controller_enhanced_input_subsystem == nullptr ||
        state.Api->add_input_mapping_context == nullptr ||
        state.Api->remove_input_mapping_context == nullptr ||
        state.Api->get_input_action_value == nullptr ||
        state.Api->inject_input_action_value == nullptr ||
        state.Api->bind_input_action == nullptr || state.Api->unbind_input_action == nullptr) {
        return FailInputSmokeStart(state, UEC_RESULT_INTERNAL_ERROR);
    }

    result = state.Api->get_default_world(state.Context, &state.World);
    if (result == UEC_RESULT_OK) {
        result = state.Api->get_player_controller(state.World, 0u, &state.Controller);
    }
    static constexpr char actorClassPathData[] =
        "/Script/UnrealCAPIHost.UECAPIHostInputSmokeActor";
    static constexpr char actionPropertyData[] = "SmokeAction";
    static constexpr char mappingPropertyData[] = "SmokeMappingContext";
    static constexpr char subsystemClassPathData[] =
        "/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem";
    const uec_string_view actorClassPath{
        actorClassPathData, sizeof(actorClassPathData) - 1u};
    const uec_string_view actionProperty{
        actionPropertyData, sizeof(actionPropertyData) - 1u};
    const uec_string_view mappingProperty{
        mappingPropertyData, sizeof(mappingPropertyData) - 1u};
    const uec_string_view subsystemClassPath{
        subsystemClassPathData, sizeof(subsystemClassPathData) - 1u};
    const uec_transform transform{
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    if (result == UEC_RESULT_OK) {
        result = state.Api->spawn_actor(
            state.World, actorClassPath, &transform, &state.Actor);
    }
    if (result == UEC_RESULT_OK) {
        result = state.Api->get_actor_property_object(
            state.Actor, actionProperty, &state.Action);
    }
    if (result == UEC_RESULT_OK) {
        result = state.Api->get_actor_property_object(
            state.Actor, mappingProperty, &state.MappingContext);
    }
    if (result != UEC_RESULT_OK || state.Action == nullptr ||
        state.MappingContext == nullptr) {
        return FailInputSmokeStart(
            state, result == UEC_RESULT_OK ? UEC_RESULT_NOT_INITIALIZED : result);
    }

    result = state.Api->get_controller_enhanced_input_subsystem(
        state.Controller, &state.Subsystem);
    uec_bool isSubsystem = UEC_FALSE;
    if (result == UEC_RESULT_OK) {
        result = state.Api->object_is_a(
            state.Subsystem, subsystemClassPath, &isSubsystem);
        if (result == UEC_RESULT_OK && isSubsystem != UEC_TRUE) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }
    if (result != UEC_RESULT_OK) return FailInputSmokeStart(state, result);

    result = state.Api->add_input_mapping_context(
        state.Controller, state.MappingContext, 37);
    if (result != UEC_RESULT_OK) return FailInputSmokeStart(state, result);
    state.MappingAdded = true;

    uec_input_action_value initialValue{};
    initialValue.struct_size = sizeof(initialValue);
    result = state.Api->get_input_action_value(
        state.Controller, state.Action, &initialValue);
    if (result != UEC_RESULT_OK ||
        initialValue.kind != UEC_INPUT_ACTION_VALUE_AXIS_1D ||
        !FMath::IsNearlyZero(static_cast<float>(initialValue.axis.x))) {
        return FailInputSmokeStart(
            state, result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result);
    }

    result = state.Api->bind_input_action(
        state.Actor, state.Action, UEC_INPUT_TRIGGER_TRIGGERED,
        &OnInputSmokeAction, &state, &state.BindingId);
    if (result != UEC_RESULT_OK || state.BindingId == 0u) {
        return FailInputSmokeStart(
            state, result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result);
    }
    result = InjectExpectedValue(state);
    if (result != UEC_RESULT_OK) return FailInputSmokeStart(state, result);
    return UEC_RESULT_OK;
}

extern "C" uec_bool UEC_CALL uec_host_input_smoke_poll(uec_result* outResult)
{
    if (outResult == nullptr) return UEC_FALSE;
    FInputSmokeState& state = GInputSmokeState;
    if (!state.Started || state.Complete) {
        *outResult = state.Complete ? state.Result : UEC_RESULT_NOT_INITIALIZED;
        return state.Complete ? UEC_TRUE : UEC_FALSE;
    }

    if (state.Stage == EInputSmokeStage::WaitingForInput) {
        if (state.CallbackCount == 0u) {
            *outResult = UEC_RESULT_NOT_INITIALIZED;
            return UEC_FALSE;
        }
        if (state.CallbackInvalid || !IsExpectedInputValue(state.ObservedValue)) {
            FinishInputSmoke(state, UEC_RESULT_INTERNAL_ERROR);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        uec_result result = state.Api->unbind_input_action(
            state.Context, state.BindingId);
        if (result != UEC_RESULT_OK) {
            FinishInputSmoke(state, result);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        state.BindingId = 0u;
        state.CallbackCountAfterUnbind = state.CallbackCount;
        result = InjectExpectedValue(state);
        if (result != UEC_RESULT_OK) {
            FinishInputSmoke(state, result);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        state.Stage = EInputSmokeStage::WaitingForSuppressedCallback;
        *outResult = UEC_RESULT_NOT_INITIALIZED;
        return UEC_FALSE;
    }

    ++state.SuppressionPolls;
    if (state.SuppressionPolls < 3u) {
        *outResult = UEC_RESULT_NOT_INITIALIZED;
        return UEC_FALSE;
    }
    if (state.CallbackCount != state.CallbackCountAfterUnbind) {
        FinishInputSmoke(state, UEC_RESULT_INTERNAL_ERROR);
        *outResult = state.Result;
        return UEC_TRUE;
    }

    uec_result result = state.Api->remove_input_mapping_context(
        state.Controller, state.MappingContext);
    if (result == UEC_RESULT_OK) {
        result = state.Api->remove_input_mapping_context(
            state.Controller, state.MappingContext);
    }
    if (result == UEC_RESULT_OK) state.MappingAdded = false;
    FinishInputSmoke(state, result);
    *outResult = state.Result;
    return UEC_TRUE;
}

extern "C" void UEC_CALL uec_host_input_smoke_cancel(void)
{
    FinishInputSmoke(GInputSmokeState, UEC_RESULT_INTERNAL_ERROR);
}
