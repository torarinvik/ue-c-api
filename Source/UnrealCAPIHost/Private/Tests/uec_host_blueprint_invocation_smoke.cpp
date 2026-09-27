#include "uec_api.h"

#include "UECAPIHostReflectionSmokeActor.h"

#include <cmath>
#include <cstring>

#if WITH_EDITOR
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#endif

namespace
{
    uec_string_view View(const char* text)
    {
        return {text, std::strlen(text)};
    }

    bool Near(double actual, double expected)
    {
        return std::fabs(actual - expected) <= 0.0001;
    }

    uec_result InvokeTypedStructEcho(const uec_api* api,
                                     uec_actor* actor,
                                     const char* functionName,
                                     const uec_function_argument& argument,
                                     uec_function_struct_kind requestedKind,
                                     uec_function_output& output)
    {
        output = {};
        output.struct_size = sizeof(output);
        output.struct_value.kind = requestedKind;
        uint32_t outputCount = 0u;
        const uec_result result = api->invoke_actor_function_arguments(
            actor, View(functionName), &argument, 1u, &output, 1u, &outputCount);
        if (result != UEC_RESULT_OK) return result;
        return outputCount == 1u && output.kind == UEC_PROPERTY_STRUCT &&
            output.struct_value.kind == requestedKind
            ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
    }

    uec_property_struct_value IntegerStructValue(
        uec_function_struct_kind kind, int32_t x, int32_t y, int32_t z = 0)
    {
        uec_property_struct_value value{};
        value.struct_size = sizeof(value);
        value.kind = kind;
        if (kind == UEC_FUNCTION_STRUCT_INT_POINT) value.value.int_point = {x, y};
        else value.value.int_vector = {x, y, z};
        return value;
    }

    bool MatchesIntegerStructValue(const uec_property_struct_value& value,
                                   uec_function_struct_kind kind,
                                   int32_t x, int32_t y, int32_t z)
    {
        if (value.kind != kind) return false;
        if (kind == UEC_FUNCTION_STRUCT_INT_POINT)
            return value.value.int_point.x == x && value.value.int_point.y == y;
        return value.value.int_vector.x == x && value.value.int_vector.y == y &&
            value.value.int_vector.z == z;
    }

    uec_result VerifyIntegerStructProperty(
        const uec_api* api, uec_actor* actor, uec_object* object,
        const char* name, const uec_property_struct_value& first,
        const uec_property_struct_value& second)
    {
        uec_result result = api->set_actor_property_struct_value(actor, View(name), &first);
        uec_property_struct_value observed{};
        observed.struct_size = sizeof(observed);
        if (result == UEC_RESULT_OK)
            result = api->get_object_property_struct_value(object, View(name), &observed);
        const bool firstMatches = first.kind == UEC_FUNCTION_STRUCT_INT_POINT
            ? MatchesIntegerStructValue(observed, first.kind, first.value.int_point.x,
                                        first.value.int_point.y, 0)
            : MatchesIntegerStructValue(observed, first.kind, first.value.int_vector.x,
                                        first.value.int_vector.y, first.value.int_vector.z);
        if (result == UEC_RESULT_OK && !firstMatches) result = UEC_RESULT_INTERNAL_ERROR;
        if (result == UEC_RESULT_OK)
            result = api->set_object_property_struct_value(object, View(name), &second);
        observed = {};
        observed.struct_size = sizeof(observed);
        if (result == UEC_RESULT_OK)
            result = api->get_actor_property_struct_value(actor, View(name), &observed);
        const bool secondMatches = second.kind == UEC_FUNCTION_STRUCT_INT_POINT
            ? MatchesIntegerStructValue(observed, second.kind, second.value.int_point.x,
                                        second.value.int_point.y, 0)
            : MatchesIntegerStructValue(observed, second.kind, second.value.int_vector.x,
                                        second.value.int_vector.y, second.value.int_vector.z);
        return result == UEC_RESULT_OK && !secondMatches
            ? UEC_RESULT_INTERNAL_ERROR : result;
    }

    uec_result VerifyTypedMathStructInvocation(const uec_api* api, uec_world* world)
    {
        static constexpr char classPathData[] =
            "/Script/UnrealCAPIHost.UECAPIHostReflectionSmokeActor";
        uec_actor* actor = nullptr;
        const uec_transform transform{
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
        uec_result result = api->spawn_actor(
            world, View(classPathData), &transform, &actor);
        if (result == UEC_RESULT_OK && actor == nullptr) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }

        uec_function_argument argument{};
        argument.struct_size = sizeof(argument);
        argument.kind = UEC_PROPERTY_STRUCT;
        uec_function_output output{};

        if (result == UEC_RESULT_OK) {
            argument.struct_value.kind = UEC_FUNCTION_STRUCT_VECTOR2;
            argument.struct_value.value.vector2 = {2.5, -4.0};
            result = InvokeTypedStructEcho(api, actor, "EchoVector2D", argument,
                                          UEC_FUNCTION_STRUCT_VECTOR2, output);
        }
        if (result == UEC_RESULT_OK &&
            (!Near(output.struct_value.value.vector2.x, 2.5) ||
             !Near(output.struct_value.value.vector2.y, -4.0))) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            argument.struct_value.kind = UEC_FUNCTION_STRUCT_VECTOR4;
            argument.struct_value.value.vector4 = {0.5, -1.5, 2.5, -3.5};
            result = InvokeTypedStructEcho(api, actor, "EchoVector4", argument,
                                          UEC_FUNCTION_STRUCT_VECTOR4, output);
        }
        if (result == UEC_RESULT_OK &&
            (!Near(output.struct_value.value.vector4.x, 0.5) ||
             !Near(output.struct_value.value.vector4.y, -1.5) ||
             !Near(output.struct_value.value.vector4.z, 2.5) ||
             !Near(output.struct_value.value.vector4.w, -3.5))) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            argument.struct_value.kind = UEC_FUNCTION_STRUCT_ROTATOR;
            argument.struct_value.value.rotator = {2.0, -30.0, 45.0};
            result = InvokeTypedStructEcho(api, actor, "EchoRotator", argument,
                                          UEC_FUNCTION_STRUCT_ROTATOR, output);
        }
        if (result == UEC_RESULT_OK &&
            (!Near(output.struct_value.value.rotator.pitch, 2.0) ||
             !Near(output.struct_value.value.rotator.yaw, -30.0) ||
             !Near(output.struct_value.value.rotator.roll, 45.0))) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            argument.struct_value.kind = UEC_FUNCTION_STRUCT_LINEAR_COLOR;
            argument.struct_value.value.linear_color = {2.0, 0.25, 1.5, 0.5};
            result = InvokeTypedStructEcho(api, actor, "EchoLinearColor", argument,
                                          UEC_FUNCTION_STRUCT_LINEAR_COLOR, output);
        }
        if (result == UEC_RESULT_OK &&
            (!Near(output.struct_value.value.linear_color.r, 2.0) ||
             !Near(output.struct_value.value.linear_color.g, 0.25) ||
             !Near(output.struct_value.value.linear_color.b, 1.5) ||
             !Near(output.struct_value.value.linear_color.a, 0.5))) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            argument.struct_value.kind = UEC_FUNCTION_STRUCT_COLOR;
            argument.struct_value.value.color = {17u, 65u, 129u, 255u};
            result = InvokeTypedStructEcho(api, actor, "EchoColor", argument,
                                          UEC_FUNCTION_STRUCT_COLOR, output);
        }
        if (result == UEC_RESULT_OK &&
            (output.struct_value.value.color.r != 17u ||
             output.struct_value.value.color.g != 65u ||
             output.struct_value.value.color.b != 129u ||
             output.struct_value.value.color.a != 255u)) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            argument.struct_value.kind = UEC_FUNCTION_STRUCT_INT_POINT;
            argument.struct_value.value.int_point = {INT32_MIN, 2026};
            result = InvokeTypedStructEcho(api, actor, "EchoIntPoint", argument,
                UEC_FUNCTION_STRUCT_INT_POINT, output);
            if (result == UEC_RESULT_OK &&
                (output.struct_value.value.int_point.x != INT32_MIN ||
                 output.struct_value.value.int_point.y != 2026))
                result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            argument.struct_value.kind = UEC_FUNCTION_STRUCT_INT_VECTOR;
            argument.struct_value.value.int_vector = {INT32_MAX, -2000000000, 42};
            result = InvokeTypedStructEcho(api, actor, "EchoIntVector", argument,
                UEC_FUNCTION_STRUCT_INT_VECTOR, output);
            if (result == UEC_RESULT_OK &&
                (output.struct_value.value.int_vector.x != INT32_MAX ||
                 output.struct_value.value.int_vector.y != -2000000000 ||
                 output.struct_value.value.int_vector.z != 42))
                result = UEC_RESULT_INTERNAL_ERROR;
        }

        if (result == UEC_RESULT_OK) {
            uec_function_argument input{};
            input.struct_size = sizeof(input);
            input.kind = UEC_PROPERTY_STRUCT;
            input.struct_value.kind = UEC_FUNCTION_STRUCT_VECTOR2;
            input.struct_value.value.vector2 = {7.5, -8.5};

            uec_function_output outputs[2]{};
            outputs[0].struct_size = sizeof(outputs[0]);
            outputs[0].struct_value.kind = UEC_FUNCTION_STRUCT_VECTOR4;
            outputs[1].struct_size = sizeof(outputs[1]);
            outputs[1].struct_value.kind = UEC_FUNCTION_STRUCT_COLOR;
            uint32_t outputCount = 0u;
            const uec_result shortResult = api->invoke_actor_function_arguments(
                actor, View("EchoVector2DWithColor"), &input, 1u,
                outputs, 1u, &outputCount);
            if (shortResult != UEC_RESULT_BUFFER_TOO_SMALL || outputCount != 2u) {
                UE_LOG(LogTemp, Error,
                    TEXT("Typed return/out short-buffer check returned %d with %u outputs"),
                    static_cast<int32>(shortResult), outputCount);
                result = UEC_RESULT_INTERNAL_ERROR;
            }
            if (result == UEC_RESULT_OK) {
                outputCount = 0u;
                result = api->invoke_actor_function_arguments(
                    actor, View("EchoVector2DWithColor"), &input, 1u,
                    outputs, 2u, &outputCount);
                if (result != UEC_RESULT_OK) {
                    UE_LOG(LogTemp, Error,
                        TEXT("Typed return/out invocation returned %d"),
                        static_cast<int32>(result));
                }
            }
            if (result == UEC_RESULT_OK &&
                (outputCount != 2u || outputs[0].kind != UEC_PROPERTY_STRUCT ||
                 outputs[0].struct_value.kind != UEC_FUNCTION_STRUCT_VECTOR4 ||
                 !Near(outputs[0].struct_value.value.vector4.x, 7.5) ||
                 !Near(outputs[0].struct_value.value.vector4.y, -8.5) ||
                 !Near(outputs[0].struct_value.value.vector4.z, 101.0) ||
                 !Near(outputs[0].struct_value.value.vector4.w, 104.0) ||
                 outputs[1].kind != UEC_PROPERTY_STRUCT ||
                 outputs[1].struct_value.kind != UEC_FUNCTION_STRUCT_COLOR ||
                 outputs[1].struct_value.value.color.r != 101u ||
                 outputs[1].struct_value.value.color.g != 102u ||
                 outputs[1].struct_value.value.color.b != 103u ||
                 outputs[1].struct_value.value.color.a != 104u)) {
                UE_LOG(LogTemp, Error,
                    TEXT("Typed return/out mismatch: count=%u return-kind=%d out-kind=%d"),
                    outputCount, static_cast<int32>(outputs[0].struct_value.kind),
                    static_cast<int32>(outputs[1].struct_value.kind));
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }

        if (result == UEC_RESULT_OK) {
            uec_property_struct_value observed{};
            observed.struct_size = sizeof(observed);
            result = api->get_actor_property_struct_value(
                actor, View("PackedTint"), &observed);
            if (result == UEC_RESULT_OK &&
                (observed.kind != UEC_PROPERTY_STRUCT_COLOR ||
                 observed.value.color.r != 32u || observed.value.color.g != 64u ||
                 observed.value.color.b != 128u || observed.value.color.a != 255u)) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
            uec_property_struct_value colorValue{};
            colorValue.struct_size = sizeof(colorValue);
            colorValue.kind = UEC_PROPERTY_STRUCT_COLOR;
            colorValue.value.color = {201u, 77u, 9u, 128u};
            if (result == UEC_RESULT_OK) {
                result = api->set_actor_property_struct_value(
                    actor, View("PackedTint"), &colorValue);
            }
            uec_object* selfObject = nullptr;
            if (result == UEC_RESULT_OK) {
                result = api->get_actor_property_object(
                    actor, View("SelfObject"), &selfObject);
            }
            if (result == UEC_RESULT_OK && selfObject == nullptr) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
            if (result == UEC_RESULT_OK) {
                result = VerifyIntegerStructProperty(api, actor, selfObject, "GridCell",
                    IntegerStructValue(UEC_FUNCTION_STRUCT_INT_POINT, INT32_MIN, 2026),
                    IntegerStructValue(UEC_FUNCTION_STRUCT_INT_POINT, INT32_MAX, -11));
            }
            if (result == UEC_RESULT_OK) {
                result = VerifyIntegerStructProperty(api, actor, selfObject, "VoxelCell",
                    IntegerStructValue(UEC_FUNCTION_STRUCT_INT_VECTOR,
                                       INT32_MAX, -2000000000, 42),
                    IntegerStructValue(UEC_FUNCTION_STRUCT_INT_VECTOR,
                                       INT32_MIN, 700, -800));
            }
            if (result == UEC_RESULT_OK) {
                observed = {};
                observed.struct_size = sizeof(observed);
                result = api->get_object_property_struct_value(
                    selfObject, View("PackedTint"), &observed);
            }
            if (result == UEC_RESULT_OK &&
                (observed.kind != UEC_PROPERTY_STRUCT_COLOR ||
                 observed.value.color.r != 201u || observed.value.color.g != 77u ||
                 observed.value.color.b != 9u || observed.value.color.a != 128u)) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
            if (result == UEC_RESULT_OK) {
                colorValue.value.color = {1u, 3u, 5u, 7u};
                result = api->set_object_property_struct_value(
                    selfObject, View("PackedTint"), &colorValue);
            }
            if (selfObject != nullptr) {
                const uec_result releaseResult = api->release_object(selfObject);
                if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                    result = releaseResult;
                }
            }
            if (result == UEC_RESULT_OK) {
                observed = {};
                observed.struct_size = sizeof(observed);
                result = api->get_actor_property_struct_value(
                    actor, View("PackedTint"), &observed);
            }
            if (result == UEC_RESULT_OK &&
                (observed.kind != UEC_PROPERTY_STRUCT_COLOR ||
                 observed.value.color.r != 1u || observed.value.color.g != 3u ||
                 observed.value.color.b != 5u || observed.value.color.a != 7u)) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
            uec_property_struct_value wrongKind{};
            wrongKind.struct_size = sizeof(wrongKind);
            wrongKind.kind = UEC_PROPERTY_STRUCT_VECTOR2;
            wrongKind.value.vector2 = {1.0, 2.0};
            if (result == UEC_RESULT_OK &&
                api->set_actor_property_struct_value(
                    actor, View("PackedTint"), &wrongKind) !=
                    UEC_RESULT_INVALID_ARGUMENT) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }

        if (result == UEC_RESULT_OK) {
            argument.struct_value.kind = UEC_FUNCTION_STRUCT_VECTOR2;
            argument.struct_value.value.vector2 = {NAN, 1.0};
            output = {};
            output.struct_size = sizeof(output);
            output.struct_value.kind = UEC_FUNCTION_STRUCT_VECTOR2;
            uint32_t outputCount = 0u;
            if (api->invoke_actor_function_arguments(
                    actor, View("EchoVector2D"), &argument, 1u,
                    &output, 1u, &outputCount) != UEC_RESULT_INVALID_ARGUMENT) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }
        if (result == UEC_RESULT_OK) {
            argument.struct_value.kind = UEC_FUNCTION_STRUCT_VECTOR2;
            argument.struct_value.value.vector2 = {1.0, 2.0};
            output = {};
            output.struct_size = sizeof(output);
            output.struct_value.kind = UEC_FUNCTION_STRUCT_ROTATOR;
            uint32_t outputCount = 0u;
            if (api->invoke_actor_function_arguments(
                    actor, View("EchoRotator"), &argument, 1u,
                    &output, 1u, &outputCount) != UEC_RESULT_INVALID_ARGUMENT) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }

        if (actor != nullptr) {
            const uec_result destroyResult = api->destroy_actor(actor);
            if (destroyResult != UEC_RESULT_OK) {
                const uec_result releaseResult = api->release_actor(actor);
                if (result == UEC_RESULT_OK) {
                    result = releaseResult == UEC_RESULT_OK
                        ? destroyResult : releaseResult;
                }
            }
        }
        return result;
    }

    uec_result VerifyBlueprintFunctionMetadata(
        const uec_api* api, uec_context* context)
    {
        static constexpr char classPathData[] =
            "/Game/Tests/BP_UECAPIHostFunctionSmoke.BP_UECAPIHostFunctionSmoke_C";
        static constexpr char functionNameData[] = "BlueprintGeneratedSmokeCall";
        static constexpr char inputNameData[] = "Value";
        static constexpr char returnNameData[] = "ReturnValue";

        if (api->find_class == nullptr || api->release_class == nullptr ||
            api->get_class_function_count == nullptr ||
            api->get_class_function_at == nullptr ||
            api->get_class_function_flags == nullptr ||
            api->get_class_function_parameter_at == nullptr)
        {
            return UEC_RESULT_INTERNAL_ERROR;
        }

        uec_class* blueprintClass = nullptr;
        uec_result result = api->find_class(
            context, View(classPathData), &blueprintClass);
        if (result == UEC_RESULT_OK && blueprintClass == nullptr) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }

        uint32_t functionIndex = UINT32_MAX;
        uint32_t parameterCount = 0u;
        uec_bool hasReturnValue = UEC_FALSE;
        uec_bool isLatent = UEC_TRUE;
        if (result == UEC_RESULT_OK) {
            uint32_t classFunctionCount = 0u;
            result = api->get_class_function_count(blueprintClass, &classFunctionCount);
            if (result == UEC_RESULT_OK && classFunctionCount == 0u) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
            for (uint32_t index = 0u;
                 result == UEC_RESULT_OK && index < classFunctionCount; ++index)
            {
                char functionName[128] = {};
                size_t requiredSize = 0u;
                result = api->get_class_function_at(
                    blueprintClass, index, functionName, sizeof(functionName),
                    &requiredSize, &parameterCount, &hasReturnValue, &isLatent);
                if (result == UEC_RESULT_OK &&
                    std::strcmp(functionName, functionNameData) == 0)
                {
                    functionIndex = index;
                    break;
                }
            }
        }

        if (result == UEC_RESULT_OK &&
            (functionIndex == UINT32_MAX || parameterCount != 1u ||
             hasReturnValue != UEC_TRUE || isLatent != UEC_FALSE))
        {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            uint32_t functionFlags = 0u;
            result = api->get_class_function_flags(
                blueprintClass, functionIndex, &functionFlags);
            const uint32_t requiredFlags = UEC_FUNCTION_FLAG_BLUEPRINT_CALLABLE |
                UEC_FUNCTION_FLAG_EVENT;
            if (result == UEC_RESULT_OK &&
                (functionFlags & requiredFlags) != requiredFlags)
            {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }

        if (result == UEC_RESULT_OK) {
            bool foundInput = false;
            bool foundReturn = false;
            for (uint32_t index = 0u; result == UEC_RESULT_OK && index < 2u; ++index) {
                char parameterName[64] = {};
                size_t requiredSize = 0u;
                uec_property_kind kind = UEC_PROPERTY_UNKNOWN;
                uint32_t flags = 0u;
                result = api->get_class_function_parameter_at(
                    blueprintClass, functionIndex, index, parameterName,
                    sizeof(parameterName), &requiredSize, &kind, &flags);
                if (result == UEC_RESULT_OK) {
                    const bool isInput =
                        (flags & UEC_FUNCTION_PARAMETER_INPUT) != 0u;
                    const bool isReturn =
                        (flags & UEC_FUNCTION_PARAMETER_RETURN) != 0u;
                    if (kind != UEC_PROPERTY_INTEGER || isInput == isReturn ||
                        (isInput && std::strcmp(parameterName, inputNameData) != 0) ||
                        (isReturn && std::strcmp(parameterName, returnNameData) != 0))
                    {
                        result = UEC_RESULT_INTERNAL_ERROR;
                    }
                    foundInput |= isInput;
                    foundReturn |= isReturn;
                }
            }
            if (result == UEC_RESULT_OK && (!foundInput || !foundReturn)) {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }

        if (blueprintClass != nullptr) {
            const uec_result releaseResult = api->release_class(blueprintClass);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
        }
        return result;
    }

    uec_result VerifyBlueprintReinstancing(
        const uec_api* api, uec_context* context, uec_world* world)
    {
#if WITH_EDITOR
        static constexpr char classPathData[] =
            "/Game/Tests/BP_UECAPIHostFunctionSmoke.BP_UECAPIHostFunctionSmoke_C";
        uec_class* staleClass = nullptr;
        uec_result result = api->find_class(context, View(classPathData), &staleClass);
        UBlueprint* blueprint = LoadObject<UBlueprint>(
            nullptr, TEXT("/Game/Tests/BP_UECAPIHostFunctionSmoke"));
        UClass* previousGeneratedClass =
            blueprint == nullptr ? nullptr : blueprint->GeneratedClass;
        const FName temporaryVariableName(TEXT("UECReinstancingSmokeTemp"));
        bool addedTemporaryVariable = false;
        if (result == UEC_RESULT_OK &&
            (staleClass == nullptr || previousGeneratedClass == nullptr))
        {
            result = UEC_RESULT_INTERNAL_ERROR;
        }

        uec_actor* actorBeforeCompile = nullptr;
        bool spawnedActorForCompile = false;
        const uec_transform transform{
            {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
        if (result == UEC_RESULT_OK) {
            result = api->spawn_actor(world, View(classPathData), &transform,
                                      &actorBeforeCompile);
        }
        if (result == UEC_RESULT_OK && actorBeforeCompile == nullptr) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (actorBeforeCompile != nullptr) {
            spawnedActorForCompile = true;
            const uec_result releaseResult = api->release_actor(actorBeforeCompile);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
            actorBeforeCompile = nullptr;
        }

        uint32_t oldPropertyCount = 0u;
        if (result == UEC_RESULT_OK) {
            result = api->get_class_property_count(staleClass, &oldPropertyCount);
        }
        if (result == UEC_RESULT_OK && oldPropertyCount == 0u) {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            FEdGraphPinType temporaryVariableType;
            temporaryVariableType.PinCategory = FName(TEXT("int"));
            addedTemporaryVariable = FBlueprintEditorUtils::AddMemberVariable(
                blueprint, temporaryVariableName, temporaryVariableType);
            if (!addedTemporaryVariable) result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (result == UEC_RESULT_OK) {
            const EBlueprintCompileOptions compileOptions =
                EBlueprintCompileOptions::SkipSave |
                EBlueprintCompileOptions::SkipGarbageCollection;
            FKismetEditorUtilities::CompileBlueprint(blueprint, compileOptions);
            if (blueprint->GeneratedClass == nullptr ||
                blueprint->GeneratedClass->FindPropertyByName(temporaryVariableName) == nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("Blueprint compile did not add the temporary property"));
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }

        if (result == UEC_RESULT_OK) {
            const bool classWasReplaced =
                blueprint->GeneratedClass != previousGeneratedClass;
            uint32_t updatedPropertyCount = UINT32_MAX;
            const uec_result metadataResult = api->get_class_property_count(
                staleClass, &updatedPropertyCount);
            const bool expectedInvalidation = classWasReplaced &&
                metadataResult == UEC_RESULT_INVALID_HANDLE && updatedPropertyCount == 0u;
            const bool expectedInPlaceUpdate = !classWasReplaced &&
                metadataResult == UEC_RESULT_OK && updatedPropertyCount > oldPropertyCount;
            if (!expectedInvalidation && !expectedInPlaceUpdate) {
                UE_LOG(LogTemp, Error,
                    TEXT("Blueprint class metadata after compile returned %d with %u properties; replaced=%s"),
                    static_cast<int32>(metadataResult), updatedPropertyCount,
                    classWasReplaced ? TEXT("yes") : TEXT("no"));
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }
        if (staleClass != nullptr) {
            const uec_result releaseResult = api->release_class(staleClass);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
        }

        if (result == UEC_RESULT_OK && addedTemporaryVariable) {
            uec_class* recompiledClass = nullptr;
            result = api->find_class(context, View(classPathData), &recompiledClass);
            uint32_t propertyCount = 0u;
            if (result == UEC_RESULT_OK) {
                result = api->get_class_property_count(recompiledClass, &propertyCount);
            }
            bool foundTemporaryProperty = false;
            for (uint32_t index = 0u; result == UEC_RESULT_OK && index < propertyCount; ++index) {
                char propertyName[128] = {};
                size_t requiredSize = 0u;
                uec_property_kind propertyKind = UEC_PROPERTY_UNKNOWN;
                result = api->get_class_property_at(
                    recompiledClass, index, propertyName, sizeof(propertyName),
                    &requiredSize, &propertyKind);
                if (result == UEC_RESULT_OK &&
                    std::strcmp(propertyName, "UECReinstancingSmokeTemp") == 0)
                {
                    foundTemporaryProperty = propertyKind == UEC_PROPERTY_INTEGER;
                }
            }
            if (result == UEC_RESULT_OK && !foundTemporaryProperty) {
                UE_LOG(LogTemp, Error,
                    TEXT("C API did not reflect the temporary Blueprint property"));
                result = UEC_RESULT_INTERNAL_ERROR;
            }
            if (recompiledClass != nullptr) {
                const uec_result releaseResult = api->release_class(recompiledClass);
                if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                    result = releaseResult;
                }
            }
        }

        if (addedTemporaryVariable) {
            FBlueprintEditorUtils::RemoveMemberVariable(blueprint, temporaryVariableName);
            const EBlueprintCompileOptions compileOptions =
                EBlueprintCompileOptions::SkipSave |
                EBlueprintCompileOptions::SkipGarbageCollection;
            FKismetEditorUtilities::CompileBlueprint(blueprint, compileOptions);
            if (blueprint->GeneratedClass == nullptr ||
                blueprint->GeneratedClass->FindPropertyByName(temporaryVariableName) != nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("Temporary Blueprint variable was not removed"));
                result = UEC_RESULT_INTERNAL_ERROR;
            }
        }

        if (spawnedActorForCompile) {
            uint32_t actorCount = 0u;
            const uec_result countResult = api->get_actor_count_by_class(
                world, View(classPathData), &actorCount);
            if (result == UEC_RESULT_OK &&
                (countResult != UEC_RESULT_OK || actorCount != 1u))
            {
                result = UEC_RESULT_INTERNAL_ERROR;
            }
            if (countResult == UEC_RESULT_OK && actorCount > 0u) {
                uec_actor* reinstancedActor = nullptr;
                const uec_result getActorResult = api->get_actor_at_by_class(
                    world, View(classPathData), 0u, &reinstancedActor);
                if (getActorResult != UEC_RESULT_OK || reinstancedActor == nullptr) {
                    if (result == UEC_RESULT_OK) result = UEC_RESULT_INTERNAL_ERROR;
                } else {
                    const uec_result destroyResult = api->destroy_actor(reinstancedActor);
                    if (destroyResult != UEC_RESULT_OK) {
                        if (result == UEC_RESULT_OK) result = destroyResult;
                        const uec_result releaseResult = api->release_actor(reinstancedActor);
                        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                            result = releaseResult;
                        }
                    }
                }
            }
        }

        uec_class* currentClass = nullptr;
        if (result == UEC_RESULT_OK) {
            result = api->find_class(context, View(classPathData), &currentClass);
            if (result != UEC_RESULT_OK) {
                UE_LOG(LogTemp, Error, TEXT("Finding Blueprint class after compile returned %d"),
                    static_cast<int32>(result));
            }
        }
        uint32_t currentPropertyCount = 0u;
        if (result == UEC_RESULT_OK) {
            result = api->get_class_property_count(currentClass, &currentPropertyCount);
        }
        if (result == UEC_RESULT_OK && currentPropertyCount != oldPropertyCount) {
            UE_LOG(LogTemp, Error, TEXT("Blueprint property count did not return to its original value"));
            result = UEC_RESULT_INTERNAL_ERROR;
        }
        if (currentClass != nullptr) {
            const uec_result releaseResult = api->release_class(currentClass);
            if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) {
                result = releaseResult;
            }
        }
        return result;
#else
        static_cast<void>(api);
        static_cast<void>(context);
        static_cast<void>(world);
        return UEC_RESULT_OK;
#endif
    }
}

extern "C" uec_result UEC_CALL uec_host_blueprint_invocation_smoke(void)
{
    static constexpr char classPathData[] =
        "/Game/Tests/BP_UECAPIHostFunctionSmoke.BP_UECAPIHostFunctionSmoke_C";
    static constexpr char functionNameData[] = "BlueprintGeneratedSmokeCall";

    const uec_api* api = nullptr;
    uec_context* context = nullptr;
    uec_world* world = nullptr;
    uec_actor* actor = nullptr;
    uec_result result = uec_get_api(UEC_ABI_MAJOR, UEC_ABI_MINOR, &api, &context);
    if (result != UEC_RESULT_OK) return result;

    if (api == nullptr || context == nullptr) return UEC_RESULT_INTERNAL_ERROR;

    if (api->get_default_world == nullptr ||
        api->release_world == nullptr || api->release_context == nullptr ||
        api->spawn_actor == nullptr || api->destroy_actor == nullptr ||
        api->release_actor == nullptr || api->get_actor_count_by_class == nullptr ||
        api->get_actor_at_by_class == nullptr ||
        api->get_class_property_count == nullptr || api->get_class_property_at == nullptr ||
        api->invoke_actor_function_value == nullptr ||
        api->invoke_actor_function_arguments == nullptr ||
        api->get_actor_property_struct_value == nullptr ||
        api->set_actor_property_struct_value == nullptr ||
        api->get_object_property_struct_value == nullptr ||
        api->set_object_property_struct_value == nullptr ||
        api->get_actor_property_object == nullptr ||
        api->release_object == nullptr)
    {
        if (api->release_context != nullptr) api->release_context(context);
        return UEC_RESULT_INTERNAL_ERROR;
    }

    result = VerifyBlueprintFunctionMetadata(api, context);
    if (result == UEC_RESULT_OK) {
        result = api->get_default_world(context, &world);
    }
    if (result == UEC_RESULT_OK && world == nullptr) result = UEC_RESULT_INTERNAL_ERROR;
    if (result == UEC_RESULT_OK) {
        result = VerifyBlueprintReinstancing(api, context, world);
    }

    const uec_transform transform{
        {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 1.0}};
    if (result == UEC_RESULT_OK) {
        result = api->spawn_actor(world, View(classPathData), &transform, &actor);
    }
    if (result == UEC_RESULT_OK && actor == nullptr) result = UEC_RESULT_INTERNAL_ERROR;

    const uec_property_value mismatchedArgument{
        sizeof(uec_property_value), UEC_PROPERTY_DOUBLE, UEC_FALSE, {0u, 0u, 0u},
        0, 1.5};
    uec_property_value rejectedReturn{};
    rejectedReturn.struct_size = sizeof(rejectedReturn);
    rejectedReturn.kind = UEC_PROPERTY_INTEGER;
    rejectedReturn.integer_value = 99;
    if (result == UEC_RESULT_OK) {
        const uec_result rejectedResult = api->invoke_actor_function_value(
            actor, View(functionNameData), &mismatchedArgument, 1u, &rejectedReturn);
        if (rejectedResult != UEC_RESULT_INVALID_ARGUMENT ||
            rejectedReturn.kind != UEC_PROPERTY_UNKNOWN ||
            rejectedReturn.bool_value != UEC_FALSE ||
            rejectedReturn.integer_value != 0 || rejectedReturn.real_value != 0.0)
        {
            result = UEC_RESULT_INTERNAL_ERROR;
        }
    }

    const uec_property_value argument{
        sizeof(uec_property_value), UEC_PROPERTY_INTEGER, UEC_FALSE, {0u, 0u, 0u},
        867, 0.0};
    uec_property_value returnValue{};
    returnValue.struct_size = sizeof(returnValue);
    if (result == UEC_RESULT_OK) {
        result = api->invoke_actor_function_value(
            actor, View(functionNameData), &argument, 1u, &returnValue);
    }
    if (result == UEC_RESULT_OK &&
        (returnValue.struct_size < sizeof(returnValue) ||
         returnValue.kind != UEC_PROPERTY_INTEGER || returnValue.integer_value != 868))
    {
        result = UEC_RESULT_INTERNAL_ERROR;
    }
    if (result == UEC_RESULT_OK) {
        result = VerifyTypedMathStructInvocation(api, world);
    }

    if (actor != nullptr) {
        const uec_result destroyResult = api->destroy_actor(actor);
        if (destroyResult != UEC_RESULT_OK) {
            const uec_result releaseResult = api->release_actor(actor);
            if (result == UEC_RESULT_OK) {
                result = releaseResult == UEC_RESULT_OK ? destroyResult : releaseResult;
            }
        }
    }
    if (world != nullptr) {
        const uec_result releaseResult = api->release_world(world);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    if (context != nullptr) {
        const uec_result releaseResult = api->release_context(context);
        if (result == UEC_RESULT_OK && releaseResult != UEC_RESULT_OK) result = releaseResult;
    }
    return result;
}
