#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "uec_api.h"

namespace
{
    constexpr uint32 ActionValueCount = 4u;

    enum class EInputSmokeStage : uint8
    {
        WaitingForInput,
        WaitingForCompletion,
        WaitingForHoldOngoing,
        WaitingForHoldCanceled,
        WaitingForSuppressedCallback
    };

    struct FInputSmokeState
    {
        const uec_api* Api = nullptr;
        uec_context* Context = nullptr;
        uec_world* World = nullptr;
        uec_actor* Controller = nullptr;
        uec_actor* Actor = nullptr;
        uec_object* Actions[ActionValueCount]{};
        uec_object* HoldAction = nullptr;
        uec_object* ActiveAction = nullptr;
        uec_object* MappingContext = nullptr;
        uec_object* Subsystem = nullptr;
        uec_input_action_value ExpectedValue{};
        uint32 ActionIndex = 0u;
        uint64 StartedBindingId = 0u;
        uint64 BindingId = 0;
        uint64 CompletedBindingId = 0u;
        uint64 OngoingBindingId = 0u;
        uint64 CanceledBindingId = 0u;
        uint32 StartedCallbackCount = 0u;
        uint32 CallbackCount = 0;
        uint32 CompletedCallbackCount = 0u;
        uint32 OngoingCallbackCount = 0u;
        uint32 CanceledCallbackCount = 0u;
        uint32 StartedCallbackCountAfterUnbind = 0u;
        uint32 CallbackCountAfterUnbind = 0;
        uint32 CompletedCallbackCountAfterUnbind = 0u;
        uint32 OngoingCallbackCountAfterUnbind = 0u;
        uint32 CanceledCallbackCountAfterUnbind = 0u;
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
        if (state.Api != nullptr && state.Context != nullptr &&
            state.Api->unbind_input_action != nullptr) {
            uint64_t* bindingIds[] = {
                &state.StartedBindingId, &state.BindingId, &state.CompletedBindingId,
                &state.OngoingBindingId, &state.CanceledBindingId};
            for (uint64_t* bindingId : bindingIds) {
                if (*bindingId == 0u) continue;
                const uec_result unbindResult = state.Api->unbind_input_action(
                    state.Context, *bindingId);
                if (result == UEC_RESULT_OK && unbindResult != UEC_RESULT_OK) {
                    result = unbindResult;
                }
                *bindingId = 0u;
            }
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
        for (uec_object*& action : state.Actions) {
            if (action != nullptr && state.Api != nullptr &&
                state.Api->release_object != nullptr) {
                const uec_result releaseResult = state.Api->release_object(action);
                if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                    result = releaseResult;
                }
                action = nullptr;
            }
        }
        if (state.HoldAction != nullptr && state.Api != nullptr &&
            state.Api->release_object != nullptr) {
            const uec_result releaseResult = state.Api->release_object(state.HoldAction);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
            state.HoldAction = nullptr;
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

    uec_input_action_value ExpectedInputValue(uint32 actionIndex)
    {
        uec_input_action_value value{};
        value.struct_size = sizeof(value);
        switch (actionIndex)
        {
        case 0u:
            value.kind = UEC_INPUT_ACTION_VALUE_BOOLEAN;
            value.bool_value = UEC_TRUE;
            break;
        case 1u:
            value.kind = UEC_INPUT_ACTION_VALUE_AXIS_1D;
            value.axis.x = 0.75;
            break;
        case 2u:
            value.kind = UEC_INPUT_ACTION_VALUE_AXIS_2D;
            value.axis.x = 0.25;
            value.axis.y = -0.5;
            break;
        default:
            value.kind = UEC_INPUT_ACTION_VALUE_AXIS_3D;
            value.axis = {0.125, -0.25, 0.75};
            break;
        }
        return value;
    }

    bool IsExpectedInputValue(const uec_input_action_value& value,
                              const uec_input_action_value& expected)
    {
        return value.struct_size >= sizeof(uec_input_action_value) &&
            value.kind == expected.kind && value.bool_value == expected.bool_value &&
            FMath::IsNearlyEqual(static_cast<float>(value.axis.x),
                                 static_cast<float>(expected.axis.x)) &&
            FMath::IsNearlyEqual(static_cast<float>(value.axis.y),
                                 static_cast<float>(expected.axis.y)) &&
            FMath::IsNearlyEqual(static_cast<float>(value.axis.z),
                                 static_cast<float>(expected.axis.z));
    }

    bool IsDefaultInputValue(const uec_input_action_value& value,
                             uec_input_action_value_kind expectedKind)
    {
        uec_input_action_value expected{};
        expected.struct_size = sizeof(expected);
        expected.kind = expectedKind;
        return IsExpectedInputValue(value, expected);
    }

    void UEC_CALL OnInputSmokeAction(uint64_t bindingId,
                                    uec_input_action_value value,
                                    void* userData)
    {
        auto* state = static_cast<FInputSmokeState*>(userData);
        if (state == nullptr || state != &GInputSmokeState) return;
        if (bindingId == state->StartedBindingId) {
            if (!IsExpectedInputValue(value, state->ExpectedValue)) {
                state->CallbackInvalid = true;
            }
            ++state->StartedCallbackCount;
        } else if (bindingId == state->BindingId) {
            if (!IsExpectedInputValue(value, state->ExpectedValue)) {
                state->CallbackInvalid = true;
            }
            ++state->CallbackCount;
            state->ObservedValue = value;
        } else if (bindingId == state->CompletedBindingId) {
            if (value.struct_size < sizeof(uec_input_action_value) ||
                value.kind != state->ExpectedValue.kind) {
                state->CallbackInvalid = true;
            }
            ++state->CompletedCallbackCount;
        } else if (bindingId == state->OngoingBindingId) {
            if (!IsExpectedInputValue(value, state->ExpectedValue)) {
                state->CallbackInvalid = true;
            }
            ++state->OngoingCallbackCount;
        } else if (bindingId == state->CanceledBindingId) {
            if (value.struct_size < sizeof(uec_input_action_value) ||
                value.kind != state->ExpectedValue.kind) {
                state->CallbackInvalid = true;
            }
            ++state->CanceledCallbackCount;
        } else {
            state->CallbackInvalid = true;
        }
    }

    uec_result InjectExpectedValue(FInputSmokeState& state)
    {
        return state.Api->inject_input_action_value(
            state.Controller, state.ActiveAction, &state.ExpectedValue);
    }

    uec_result BeginActionValueCheck(FInputSmokeState& state, uint32 actionIndex)
    {
        if (actionIndex >= ActionValueCount || state.Actions[actionIndex] == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        state.ActionIndex = actionIndex;
        state.ActiveAction = state.Actions[actionIndex];
        state.ExpectedValue = ExpectedInputValue(actionIndex);
        state.StartedCallbackCount = 0u;
        state.CallbackCount = 0u;
        state.CompletedCallbackCount = 0u;
        state.OngoingCallbackCount = 0u;
        state.CanceledCallbackCount = 0u;
        state.StartedCallbackCountAfterUnbind = 0u;
        state.CallbackCountAfterUnbind = 0u;
        state.CompletedCallbackCountAfterUnbind = 0u;
        state.OngoingCallbackCountAfterUnbind = 0u;
        state.CanceledCallbackCountAfterUnbind = 0u;
        state.SuppressionPolls = 0u;
        state.CallbackInvalid = false;
        uec_result result = state.Api->bind_input_action(
            state.Actor, state.Actions[actionIndex], UEC_INPUT_TRIGGER_STARTED,
            &OnInputSmokeAction, &state, &state.StartedBindingId);
        if (result != UEC_RESULT_OK || state.StartedBindingId == 0u) {
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }
        result = state.Api->bind_input_action(
            state.Actor, state.Actions[actionIndex], UEC_INPUT_TRIGGER_TRIGGERED,
            &OnInputSmokeAction, &state, &state.BindingId);
        if (result != UEC_RESULT_OK || state.BindingId == 0u) {
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }
        result = state.Api->bind_input_action(
            state.Actor, state.Actions[actionIndex], UEC_INPUT_TRIGGER_COMPLETED,
            &OnInputSmokeAction, &state, &state.CompletedBindingId);
        if (result != UEC_RESULT_OK || state.CompletedBindingId == 0u) {
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }
        result = InjectExpectedValue(state);
        return result;
    }

    uec_result InjectReleasedValue(FInputSmokeState& state)
    {
        uec_input_action_value value{};
        value.struct_size = sizeof(value);
        value.kind = state.ExpectedValue.kind;
        return state.Api->inject_input_action_value(
            state.Controller, state.ActiveAction, &value);
    }

    uec_result UnbindInputSmokeActions(FInputSmokeState& state)
    {
        uint64_t* bindingIds[] = {
            &state.StartedBindingId, &state.BindingId, &state.CompletedBindingId,
            &state.OngoingBindingId, &state.CanceledBindingId};
        for (uint64_t* bindingId : bindingIds) {
            if (*bindingId == 0u) continue;
            const uec_result result = state.Api->unbind_input_action(
                state.Context, *bindingId);
            if (result != UEC_RESULT_OK) return result;
            *bindingId = 0u;
        }
        return UEC_RESULT_OK;
    }

    uec_result BeginHoldPhaseCheck(FInputSmokeState& state)
    {
        if (state.HoldAction == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        state.ActionIndex = ActionValueCount;
        state.ActiveAction = state.HoldAction;
        state.ExpectedValue = {};
        state.ExpectedValue.struct_size = sizeof(state.ExpectedValue);
        state.ExpectedValue.kind = UEC_INPUT_ACTION_VALUE_AXIS_1D;
        state.ExpectedValue.axis.x = 0.75;
        state.StartedCallbackCount = 0u;
        state.CallbackCount = 0u;
        state.CompletedCallbackCount = 0u;
        state.OngoingCallbackCount = 0u;
        state.CanceledCallbackCount = 0u;
        state.StartedCallbackCountAfterUnbind = 0u;
        state.CallbackCountAfterUnbind = 0u;
        state.CompletedCallbackCountAfterUnbind = 0u;
        state.OngoingCallbackCountAfterUnbind = 0u;
        state.CanceledCallbackCountAfterUnbind = 0u;
        state.SuppressionPolls = 0u;
        state.CallbackInvalid = false;
        uec_result result = state.Api->bind_input_action(
            state.Actor, state.ActiveAction, UEC_INPUT_TRIGGER_ONGOING,
            &OnInputSmokeAction, &state, &state.OngoingBindingId);
        if (result != UEC_RESULT_OK || state.OngoingBindingId == 0u) {
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }
        result = state.Api->bind_input_action(
            state.Actor, state.ActiveAction, UEC_INPUT_TRIGGER_CANCELED,
            &OnInputSmokeAction, &state, &state.CanceledBindingId);
        if (result != UEC_RESULT_OK || state.CanceledBindingId == 0u) {
            return result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result;
        }
        state.Stage = EInputSmokeStage::WaitingForHoldOngoing;
        return InjectExpectedValue(state);
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
    static constexpr char booleanActionPropertyData[] = "SmokeBooleanAction";
    static constexpr char axis2DActionPropertyData[] = "SmokeAxis2DAction";
    static constexpr char axis3DActionPropertyData[] = "SmokeAxis3DAction";
    static constexpr char holdActionPropertyData[] = "SmokeHoldAction";
    static constexpr char mappingPropertyData[] = "SmokeMappingContext";
    static constexpr char subsystemClassPathData[] =
        "/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem";
    const uec_string_view actorClassPath{
        actorClassPathData, sizeof(actorClassPathData) - 1u};
    const uec_string_view actionProperty{
        actionPropertyData, sizeof(actionPropertyData) - 1u};
    const uec_string_view actionProperties[ActionValueCount] = {
        {booleanActionPropertyData, sizeof(booleanActionPropertyData) - 1u},
        actionProperty,
        {axis2DActionPropertyData, sizeof(axis2DActionPropertyData) - 1u},
        {axis3DActionPropertyData, sizeof(axis3DActionPropertyData) - 1u}};
    const uec_string_view mappingProperty{
        mappingPropertyData, sizeof(mappingPropertyData) - 1u};
    const uec_string_view holdActionProperty{
        holdActionPropertyData, sizeof(holdActionPropertyData) - 1u};
    const uec_string_view subsystemClassPath{
        subsystemClassPathData, sizeof(subsystemClassPathData) - 1u};
    const uec_transform transform{
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    if (result == UEC_RESULT_OK) {
        result = state.Api->spawn_actor(
            state.World, actorClassPath, &transform, &state.Actor);
    }
    for (uint32 index = 0u; result == UEC_RESULT_OK && index < ActionValueCount; ++index) {
        result = state.Api->get_actor_property_object(
            state.Actor, actionProperties[index], &state.Actions[index]);
    }
    if (result == UEC_RESULT_OK) {
        result = state.Api->get_actor_property_object(
            state.Actor, holdActionProperty, &state.HoldAction);
    }
    if (result == UEC_RESULT_OK) {
        result = state.Api->get_actor_property_object(
            state.Actor, mappingProperty, &state.MappingContext);
    }
    bool allActionsFound = true;
    for (const uec_object* action : state.Actions) allActionsFound &= action != nullptr;
    if (result != UEC_RESULT_OK || !allActionsFound || state.HoldAction == nullptr ||
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

    for (uint32 index = 0u; index < ActionValueCount; ++index) {
        uec_input_action_value initialValue{};
        initialValue.struct_size = sizeof(initialValue);
        result = state.Api->get_input_action_value(
            state.Controller, state.Actions[index], &initialValue);
        const uec_input_action_value_kind inputKind = ExpectedInputValue(index).kind;
        if (result != UEC_RESULT_OK || !IsDefaultInputValue(initialValue, inputKind)) {
            return FailInputSmokeStart(
                state, result == UEC_RESULT_OK ? UEC_RESULT_INTERNAL_ERROR : result);
        }
    }
    result = BeginActionValueCheck(state, 0u);
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
        if (state.StartedCallbackCount == 0u || state.CallbackCount == 0u) {
            *outResult = UEC_RESULT_NOT_INITIALIZED;
            return UEC_FALSE;
        }
        if (state.CallbackInvalid ||
            !IsExpectedInputValue(state.ObservedValue, state.ExpectedValue)) {
            FinishInputSmoke(state, UEC_RESULT_INTERNAL_ERROR);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        const uec_result result = InjectReleasedValue(state);
        if (result != UEC_RESULT_OK) {
            FinishInputSmoke(state, result);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        state.Stage = EInputSmokeStage::WaitingForCompletion;
        *outResult = UEC_RESULT_NOT_INITIALIZED;
        return UEC_FALSE;
    }

    if (state.Stage == EInputSmokeStage::WaitingForCompletion) {
        if (state.CompletedCallbackCount == 0u) {
            *outResult = UEC_RESULT_NOT_INITIALIZED;
            return UEC_FALSE;
        }
        if (state.CallbackInvalid) {
            FinishInputSmoke(state, UEC_RESULT_INTERNAL_ERROR);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        const uec_result result = UnbindInputSmokeActions(state);
        if (result != UEC_RESULT_OK) {
            FinishInputSmoke(state, result);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        state.StartedCallbackCountAfterUnbind = state.StartedCallbackCount;
        state.CallbackCountAfterUnbind = state.CallbackCount;
        state.CompletedCallbackCountAfterUnbind = state.CompletedCallbackCount;
        const uec_result reinjectResult = InjectExpectedValue(state);
        if (reinjectResult != UEC_RESULT_OK) {
            FinishInputSmoke(state, reinjectResult);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        state.Stage = EInputSmokeStage::WaitingForSuppressedCallback;
        *outResult = UEC_RESULT_NOT_INITIALIZED;
        return UEC_FALSE;
    }

    if (state.Stage == EInputSmokeStage::WaitingForHoldOngoing) {
        if (state.OngoingCallbackCount == 0u) {
            const uec_result result = InjectExpectedValue(state);
            if (result != UEC_RESULT_OK) {
                FinishInputSmoke(state, result);
                *outResult = state.Result;
                return UEC_TRUE;
            }
            *outResult = UEC_RESULT_NOT_INITIALIZED;
            return UEC_FALSE;
        }
        if (state.CallbackInvalid) {
            FinishInputSmoke(state, UEC_RESULT_INTERNAL_ERROR);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        const uec_result result = InjectReleasedValue(state);
        if (result != UEC_RESULT_OK) {
            FinishInputSmoke(state, result);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        state.Stage = EInputSmokeStage::WaitingForHoldCanceled;
        *outResult = UEC_RESULT_NOT_INITIALIZED;
        return UEC_FALSE;
    }

    if (state.Stage == EInputSmokeStage::WaitingForHoldCanceled) {
        if (state.CanceledCallbackCount == 0u) {
            *outResult = UEC_RESULT_NOT_INITIALIZED;
            return UEC_FALSE;
        }
        if (state.CallbackInvalid) {
            FinishInputSmoke(state, UEC_RESULT_INTERNAL_ERROR);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        const uec_result result = UnbindInputSmokeActions(state);
        if (result != UEC_RESULT_OK) {
            FinishInputSmoke(state, result);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        state.StartedCallbackCountAfterUnbind = state.StartedCallbackCount;
        state.CallbackCountAfterUnbind = state.CallbackCount;
        state.CompletedCallbackCountAfterUnbind = state.CompletedCallbackCount;
        state.OngoingCallbackCountAfterUnbind = state.OngoingCallbackCount;
        state.CanceledCallbackCountAfterUnbind = state.CanceledCallbackCount;
        const uec_result reinjectResult = InjectExpectedValue(state);
        if (reinjectResult != UEC_RESULT_OK) {
            FinishInputSmoke(state, reinjectResult);
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
    if (state.StartedCallbackCount != state.StartedCallbackCountAfterUnbind ||
        state.CallbackCount != state.CallbackCountAfterUnbind ||
        state.CompletedCallbackCount != state.CompletedCallbackCountAfterUnbind ||
        state.OngoingCallbackCount != state.OngoingCallbackCountAfterUnbind ||
        state.CanceledCallbackCount != state.CanceledCallbackCountAfterUnbind) {
        FinishInputSmoke(state, UEC_RESULT_INTERNAL_ERROR);
        *outResult = state.Result;
        return UEC_TRUE;
    }

    if (state.ActionIndex < ActionValueCount) {
        if (state.ActionIndex + 1u < ActionValueCount) {
            const uec_result result = BeginActionValueCheck(state, state.ActionIndex + 1u);
            if (result != UEC_RESULT_OK) {
                FinishInputSmoke(state, result);
                *outResult = state.Result;
                return UEC_TRUE;
            }
            state.Stage = EInputSmokeStage::WaitingForInput;
            *outResult = UEC_RESULT_NOT_INITIALIZED;
            return UEC_FALSE;
        }
        const uec_result result = BeginHoldPhaseCheck(state);
        if (result != UEC_RESULT_OK) {
            FinishInputSmoke(state, result);
            *outResult = state.Result;
            return UEC_TRUE;
        }
        *outResult = UEC_RESULT_NOT_INITIALIZED;
        return UEC_FALSE;
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
    const FInputSmokeState& state = GInputSmokeState;
    UE_LOG(LogTemp, Error,
        TEXT("Input smoke timed out: stage=%d action=%u callbacks=%u/%u/%u/%u/%u invalid=%d"),
        static_cast<int32>(state.Stage), state.ActionIndex,
        state.StartedCallbackCount, state.CallbackCount,
        state.CompletedCallbackCount, state.OngoingCallbackCount,
        state.CanceledCallbackCount, state.CallbackInvalid);
    FinishInputSmoke(GInputSmokeState, UEC_RESULT_INTERNAL_ERROR);
}
