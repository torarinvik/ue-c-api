/* Typed scalar and math-struct function invocation values. */
    static uec_result SetInvocationPropertyValue(FProperty* property,
                                                 void* container,
                                                 const uec_property_value& value)
    {
        if (property == nullptr || container == nullptr || value.struct_size < sizeof(value)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (FBoolProperty* boolProperty = CastField<FBoolProperty>(property))
        {
            if (value.kind != UEC_PROPERTY_BOOL || !IsValidBool(value.bool_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            boolProperty->SetPropertyValue_InContainer(container, value.bool_value != UEC_FALSE);
            return UEC_RESULT_OK;
        }
        if (FEnumProperty* enumProperty = CastField<FEnumProperty>(property))
        {
            if ((value.kind != UEC_PROPERTY_INTEGER && value.kind != UEC_PROPERTY_ENUM) ||
                !IsIntegerValueInRange(enumProperty->GetUnderlyingProperty(), value.integer_value) ||
                !IsValidEnumValue(property, value.integer_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            void* enumValue = enumProperty->ContainerPtrToValuePtr<void>(container);
            enumProperty->GetUnderlyingProperty()->SetNumericPropertyValueFromString(
                enumValue, *LexToString(value.integer_value));
            return UEC_RESULT_OK;
        }
        if (FNumericProperty* numericProperty = CastField<FNumericProperty>(property))
        {
            if (numericProperty->IsFloatingPoint())
            {
                if ((value.kind != UEC_PROPERTY_FLOAT && value.kind != UEC_PROPERTY_DOUBLE) ||
                    !FMath::IsFinite(value.real_value) ||
                    (CastField<FFloatProperty>(property) && !IsRepresentableFloat(value.real_value))) {
                    return UEC_RESULT_INVALID_ARGUMENT;
                }
                numericProperty->SetFloatingPointPropertyValue(
                    numericProperty->ContainerPtrToValuePtr<void>(container), value.real_value);
                return UEC_RESULT_OK;
            }
            if (numericProperty->IsInteger())
            {
                if ((value.kind != UEC_PROPERTY_INTEGER && value.kind != UEC_PROPERTY_ENUM) ||
                    (numericProperty->IsEnum() && !IsValidEnumValue(property, value.integer_value)) ||
                    !IsIntegerValueInRange(numericProperty, value.integer_value)) {
                    return UEC_RESULT_INVALID_ARGUMENT;
                }
                numericProperty->SetNumericPropertyValueFromString_InContainer(
                    container, *LexToString(value.integer_value));
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_UNSUPPORTED;
    }

    static uec_result ReadInvocationPropertyValue(FProperty* property,
                                                  const void* container,
                                                  uec_property_value* outValue)
    {
        if (property == nullptr || container == nullptr || outValue == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        outValue->kind = UEC_PROPERTY_UNKNOWN;
        outValue->bool_value = UEC_FALSE;
        outValue->integer_value = 0;
        outValue->real_value = 0.0;
        if (const FBoolProperty* boolProperty = CastField<FBoolProperty>(property))
        {
            outValue->kind = UEC_PROPERTY_BOOL;
            outValue->bool_value = boolProperty->GetPropertyValue_InContainer(container)
                ? UEC_TRUE : UEC_FALSE;
            return UEC_RESULT_OK;
        }
        if (const FEnumProperty* enumProperty = CastField<FEnumProperty>(property))
        {
            outValue->kind = UEC_PROPERTY_ENUM;
            const void* enumValue = enumProperty->ContainerPtrToValuePtr<void>(container);
            return TryReadIntegerPropertyValue(enumProperty->GetUnderlyingProperty(),
                                               enumValue, outValue->integer_value)
                ? UEC_RESULT_OK : UEC_RESULT_UNSUPPORTED;
        }
        if (const FNumericProperty* numericProperty = CastField<FNumericProperty>(property))
        {
            if (numericProperty->IsFloatingPoint())
            {
                outValue->kind = CastField<FFloatProperty>(property)
                    ? UEC_PROPERTY_FLOAT : UEC_PROPERTY_DOUBLE;
                outValue->real_value = numericProperty->GetFloatingPointPropertyValue(
                    numericProperty->ContainerPtrToValuePtr<void>(container));
                return UEC_RESULT_OK;
            }
            if (numericProperty->IsInteger())
            {
                outValue->kind = UEC_PROPERTY_INTEGER;
                return TryReadIntegerProperty(numericProperty, container,
                                              outValue->integer_value)
                    ? UEC_RESULT_OK : UEC_RESULT_UNSUPPORTED;
            }
        }
        return UEC_RESULT_UNSUPPORTED;
    }

    static bool IsFiniteInvocationVector3(const uec_vector3& value)
    {
        return FMath::IsFinite(value.x) && FMath::IsFinite(value.y) &&
            FMath::IsFinite(value.z);
    }

    static bool IsFiniteInvocationRotator(const uec_rotator& value)
    {
        return FMath::IsFinite(value.pitch) && FMath::IsFinite(value.yaw) &&
            FMath::IsFinite(value.roll);
    }

    static bool IsRepresentableInvocationLinearColor(const uec_linear_color& value)
    {
        return IsRepresentableFloat(value.r) && IsRepresentableFloat(value.g) &&
            IsRepresentableFloat(value.b) && IsRepresentableFloat(value.a);
    }

    static bool IsFiniteInvocationVector2(const uec_vector2& value)
    {
        return FMath::IsFinite(value.x) && FMath::IsFinite(value.y);
    }

    static bool IsFiniteInvocationVector4(const uec_vector4& value)
    {
        return FMath::IsFinite(value.x) && FMath::IsFinite(value.y) &&
            FMath::IsFinite(value.z) && FMath::IsFinite(value.w);
    }

    static bool IsValidInvocationQuaternion(const uec_quaternion& value)
    {
        if (!FMath::IsFinite(value.x) || !FMath::IsFinite(value.y) ||
            !FMath::IsFinite(value.z) || !FMath::IsFinite(value.w)) return false;
        const double lengthSquared = value.x * value.x + value.y * value.y +
            value.z * value.z + value.w * value.w;
        return FMath::IsFinite(lengthSquared) && lengthSquared > SMALL_NUMBER;
    }

    static bool IsValidInvocationTransform(const uec_transform& value)
    {
        return IsFiniteInvocationVector3(value.translation) &&
            IsValidInvocationQuaternion(value.rotation) &&
            IsFiniteInvocationVector3(value.scale);
    }

    static UScriptStruct* GetInvocationTimespanStruct()
    {
        // UE 5.8 reflects FTimespan but does not expose a TBaseStructure accessor.
        static UScriptStruct* const TimespanStruct =
            FindObject<UScriptStruct>(nullptr, TEXT("/Script/CoreUObject.Timespan"));
        return TimespanStruct;
    }

    static uec_function_struct_kind GetInvocationStructKind(FProperty* property)
    {
        const FStructProperty* structProperty = CastField<FStructProperty>(property);
        if (structProperty == nullptr || structProperty->Struct == nullptr) {
            return UEC_FUNCTION_STRUCT_NONE;
        }
        if (structProperty->Struct == TBaseStructure<FVector>::Get()) {
            return UEC_FUNCTION_STRUCT_VECTOR3;
        }
        if (structProperty->Struct == TBaseStructure<FQuat>::Get()) {
            return UEC_FUNCTION_STRUCT_QUATERNION;
        }
        if (structProperty->Struct == TBaseStructure<FTransform>::Get()) {
            return UEC_FUNCTION_STRUCT_TRANSFORM;
        }
        if (structProperty->Struct == TBaseStructure<FRotator>::Get()) {
            return UEC_FUNCTION_STRUCT_ROTATOR;
        }
        if (structProperty->Struct == TBaseStructure<FLinearColor>::Get()) {
            return UEC_FUNCTION_STRUCT_LINEAR_COLOR;
        }
        if (structProperty->Struct == TBaseStructure<FVector2D>::Get()) {
            return UEC_FUNCTION_STRUCT_VECTOR2;
        }
        if (structProperty->Struct == TBaseStructure<FVector4>::Get()) {
            return UEC_FUNCTION_STRUCT_VECTOR4;
        }
        if (structProperty->Struct == TBaseStructure<FColor>::Get()) {
            return UEC_FUNCTION_STRUCT_COLOR;
        }
        if (structProperty->Struct == TBaseStructure<FIntPoint>::Get()) {
            return UEC_FUNCTION_STRUCT_INT_POINT;
        }
        if (structProperty->Struct == TBaseStructure<FIntVector>::Get()) {
            return UEC_FUNCTION_STRUCT_INT_VECTOR;
        }
        if (structProperty->Struct == TBaseStructure<FGuid>::Get()) {
            return UEC_FUNCTION_STRUCT_GUID;
        }
        if (structProperty->Struct == TBaseStructure<FDateTime>::Get()) {
            return UEC_FUNCTION_STRUCT_DATETIME;
        }
        if (structProperty->Struct == GetInvocationTimespanStruct()) {
            return UEC_FUNCTION_STRUCT_TIMESPAN;
        }
        return UEC_FUNCTION_STRUCT_NONE;
    }

    static bool IsSupportedInvocationStructKind(uec_function_struct_kind kind)
    {
        return kind == UEC_FUNCTION_STRUCT_NONE ||
            kind == UEC_FUNCTION_STRUCT_VECTOR3 ||
            kind == UEC_FUNCTION_STRUCT_QUATERNION ||
            kind == UEC_FUNCTION_STRUCT_TRANSFORM ||
            kind == UEC_FUNCTION_STRUCT_ROTATOR ||
            kind == UEC_FUNCTION_STRUCT_LINEAR_COLOR ||
            kind == UEC_FUNCTION_STRUCT_VECTOR2 ||
            kind == UEC_FUNCTION_STRUCT_VECTOR4 ||
            kind == UEC_FUNCTION_STRUCT_COLOR ||
            kind == UEC_FUNCTION_STRUCT_INT_POINT ||
            kind == UEC_FUNCTION_STRUCT_INT_VECTOR ||
            kind == UEC_FUNCTION_STRUCT_GUID ||
            kind == UEC_FUNCTION_STRUCT_DATETIME ||
            kind == UEC_FUNCTION_STRUCT_TIMESPAN;
    }

    static uec_result SetInvocationStructValue(
        FProperty* property,
        void* container,
        const uec_function_struct_value& value)
    {
        FStructProperty* structProperty = CastField<FStructProperty>(property);
        if (structProperty == nullptr || container == nullptr ||
            GetInvocationStructKind(property) != value.kind) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        switch (value.kind)
        {
        case UEC_FUNCTION_STRUCT_VECTOR3:
        {
            const uec_vector3& input = value.value.vector3;
            if (!IsFiniteInvocationVector3(input)) return UEC_RESULT_INVALID_ARGUMENT;
            *structProperty->ContainerPtrToValuePtr<FVector>(container) =
                FVector(input.x, input.y, input.z);
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_QUATERNION:
        {
            const uec_quaternion& input = value.value.quaternion;
            if (!IsValidInvocationQuaternion(input)) return UEC_RESULT_INVALID_ARGUMENT;
            *structProperty->ContainerPtrToValuePtr<FQuat>(container) =
                FQuat(input.x, input.y, input.z, input.w);
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_TRANSFORM:
        {
            const uec_transform& input = value.value.transform;
            if (!IsValidInvocationTransform(input)) return UEC_RESULT_INVALID_ARGUMENT;
            *structProperty->ContainerPtrToValuePtr<FTransform>(container) =
                ToFTransform(input);
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_ROTATOR:
        {
            const uec_rotator& input = value.value.rotator;
            if (!IsFiniteInvocationRotator(input)) return UEC_RESULT_INVALID_ARGUMENT;
            *structProperty->ContainerPtrToValuePtr<FRotator>(container) =
                FRotator(input.pitch, input.yaw, input.roll);
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_LINEAR_COLOR:
        {
            const uec_linear_color& input = value.value.linear_color;
            if (!IsRepresentableInvocationLinearColor(input)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            *structProperty->ContainerPtrToValuePtr<FLinearColor>(container) =
                FLinearColor(static_cast<float>(input.r), static_cast<float>(input.g),
                             static_cast<float>(input.b), static_cast<float>(input.a));
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_VECTOR2:
        {
            const uec_vector2& input = value.value.vector2;
            if (!IsFiniteInvocationVector2(input)) return UEC_RESULT_INVALID_ARGUMENT;
            *structProperty->ContainerPtrToValuePtr<FVector2D>(container) =
                FVector2D(input.x, input.y);
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_VECTOR4:
        {
            const uec_vector4& input = value.value.vector4;
            if (!IsFiniteInvocationVector4(input)) return UEC_RESULT_INVALID_ARGUMENT;
            *structProperty->ContainerPtrToValuePtr<FVector4>(container) =
                FVector4(input.x, input.y, input.z, input.w);
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_COLOR:
        {
            const uec_color& input = value.value.color;
            *structProperty->ContainerPtrToValuePtr<FColor>(container) =
                FColor(input.r, input.g, input.b, input.a);
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_INT_POINT:
        {
            const uec_int_point& input = value.value.int_point;
            *structProperty->ContainerPtrToValuePtr<FIntPoint>(container) =
                FIntPoint(input.x, input.y);
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_INT_VECTOR:
        {
            const uec_int_vector& input = value.value.int_vector;
            *structProperty->ContainerPtrToValuePtr<FIntVector>(container) =
                FIntVector(input.x, input.y, input.z);
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_GUID:
        {
            const uec_guid& input = value.value.guid;
            *structProperty->ContainerPtrToValuePtr<FGuid>(container) =
                FGuid(input.a, input.b, input.c, input.d);
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_DATETIME:
        {
            const int64_t ticks = value.value.datetime.ticks;
            if (ticks < FDateTime::MinValue().GetTicks() ||
                ticks > FDateTime::MaxValue().GetTicks()) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            *structProperty->ContainerPtrToValuePtr<FDateTime>(container) = FDateTime(ticks);
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_TIMESPAN:
        {
            *structProperty->ContainerPtrToValuePtr<FTimespan>(container) =
                FTimespan(value.value.timespan.ticks);
            return UEC_RESULT_OK;
        }
        default:
            return UEC_RESULT_INVALID_ARGUMENT;
        }
    }

    static uec_result ReadInvocationStructValue(
        FProperty* property,
        const void* container,
        uec_function_struct_value* outValue)
    {
        if (property == nullptr || container == nullptr || outValue == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        *outValue = {};
        outValue->kind = GetInvocationStructKind(property);
        switch (outValue->kind)
        {
        case UEC_FUNCTION_STRUCT_VECTOR3:
        {
            const FVector& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FVector>(container);
            outValue->value.vector3 = {value.X, value.Y, value.Z};
            return IsFiniteInvocationVector3(outValue->value.vector3)
                ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
        }
        case UEC_FUNCTION_STRUCT_QUATERNION:
        {
            const FQuat& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FQuat>(container);
            outValue->value.quaternion = {value.X, value.Y, value.Z, value.W};
            return IsValidInvocationQuaternion(outValue->value.quaternion)
                ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
        }
        case UEC_FUNCTION_STRUCT_TRANSFORM:
        {
            const FTransform& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FTransform>(container);
            outValue->value.transform = FromFTransform(value);
            return IsValidInvocationTransform(outValue->value.transform)
                ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
        }
        case UEC_FUNCTION_STRUCT_ROTATOR:
        {
            const FRotator& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FRotator>(container);
            outValue->value.rotator = {value.Pitch, value.Yaw, value.Roll};
            return IsFiniteInvocationRotator(outValue->value.rotator)
                ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
        }
        case UEC_FUNCTION_STRUCT_LINEAR_COLOR:
        {
            const FLinearColor& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FLinearColor>(container);
            outValue->value.linear_color = {value.R, value.G, value.B, value.A};
            return IsRepresentableInvocationLinearColor(outValue->value.linear_color)
                ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
        }
        case UEC_FUNCTION_STRUCT_VECTOR2:
        {
            const FVector2D& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FVector2D>(container);
            outValue->value.vector2 = {value.X, value.Y};
            return IsFiniteInvocationVector2(outValue->value.vector2)
                ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
        }
        case UEC_FUNCTION_STRUCT_VECTOR4:
        {
            const FVector4& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FVector4>(container);
            outValue->value.vector4 = {value.X, value.Y, value.Z, value.W};
            return IsFiniteInvocationVector4(outValue->value.vector4)
                ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
        }
        case UEC_FUNCTION_STRUCT_COLOR:
        {
            const FColor& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FColor>(container);
            outValue->value.color = {value.R, value.G, value.B, value.A};
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_INT_POINT:
        {
            const FIntPoint& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FIntPoint>(container);
            outValue->value.int_point = {value.X, value.Y};
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_INT_VECTOR:
        {
            const FIntVector& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FIntVector>(container);
            outValue->value.int_vector = {value.X, value.Y, value.Z};
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_GUID:
        {
            const FGuid& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FGuid>(container);
            outValue->value.guid = {value.A, value.B, value.C, value.D};
            return UEC_RESULT_OK;
        }
        case UEC_FUNCTION_STRUCT_DATETIME:
        {
            const FDateTime& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FDateTime>(container);
            outValue->value.datetime.ticks = value.GetTicks();
            return outValue->value.datetime.ticks >= FDateTime::MinValue().GetTicks() &&
                outValue->value.datetime.ticks <= FDateTime::MaxValue().GetTicks()
                ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
        }
        case UEC_FUNCTION_STRUCT_TIMESPAN:
        {
            const FTimespan& value = *CastFieldChecked<FStructProperty>(property)
                ->ContainerPtrToValuePtr<FTimespan>(container);
            outValue->value.timespan.ticks = value.GetTicks();
            return UEC_RESULT_OK;
        }
        default:
            outValue->kind = UEC_FUNCTION_STRUCT_NONE;
            return UEC_RESULT_UNSUPPORTED;
        }
    }

    uec_result UEC_CALL InvokeActorFunctionValue(
        uec_actor* rawActor,
        uec_string_view functionName,
        const uec_property_value* argumentValues,
        uint32_t argumentCount,
        uec_property_value* outReturnValue)
    {
        if (outReturnValue != nullptr)
        {
            if (outReturnValue->struct_size < sizeof(uec_property_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            outReturnValue->kind = UEC_PROPERTY_UNKNOWN;
            outReturnValue->bool_value = UEC_FALSE;
            outReturnValue->integer_value = 0;
            outReturnValue->real_value = 0.0;
        }
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(functionName) || functionName.size == 0 ||
            (argumentCount != 0 && argumentValues == nullptr)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UFunction* function = actor->FindFunction(FName(*ToFString(functionName)));
        if (function == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (IsLatentFunction(function) || function->HasAnyFunctionFlags(FUNC_Net) ||
            (function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly) &&
             actor->GetWorld() != nullptr && actor->GetWorld()->GetNetMode() == NM_Client)) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (!function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_Native | FUNC_BlueprintEvent)) {
            return UEC_RESULT_UNSUPPORTED;
        }

        TArray<FProperty*> inputParameters;
        TArray<FProperty*> outputParameters;
        FProperty* outputProperty = nullptr;
        for (TFieldIterator<FProperty> iterator(function); iterator; ++iterator)
        {
            FProperty* parameter = *iterator;
            if (!parameter->HasAnyPropertyFlags(CPF_Parm)) continue;
            if (parameter->HasAnyPropertyFlags(CPF_ReturnParm)) {
                outputProperty = parameter;
                continue;
            }
            if (parameter->HasAnyPropertyFlags(CPF_OutParm)) outputParameters.Add(parameter);
            if (!parameter->HasAnyPropertyFlags(CPF_OutParm) ||
                parameter->HasAnyPropertyFlags(CPF_ReferenceParm)) inputParameters.Add(parameter);
        }
        if (argumentCount != static_cast<uint32_t>(inputParameters.Num())) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (outputProperty == nullptr && outputParameters.Num() != 0) outputProperty = outputParameters[0];
        if (outputProperty != nullptr && outReturnValue == nullptr) return UEC_RESULT_INVALID_ARGUMENT;

        FStructOnScope parameters(function);
        uint8* parameterMemory = parameters.GetStructMemory();
        for (uint32_t index = 0; index < argumentCount; ++index)
        {
            const uec_result result = SetInvocationPropertyValue(
                inputParameters[static_cast<int32>(index)], parameterMemory, argumentValues[index]);
            if (result != UEC_RESULT_OK) return result;
        }
        actor->ProcessEvent(function, parameterMemory);
        if (outputProperty == nullptr) return UEC_RESULT_OK;
        return ReadInvocationPropertyValue(outputProperty, parameterMemory, outReturnValue);
    }

    uec_result UEC_CALL InvokeActorFunctionValues(
        uec_actor* rawActor,
        uec_string_view functionName,
        const uec_property_value* argumentValues,
        uint32_t argumentCount,
        uec_property_value* outValues,
        uint32_t outCapacity,
        uint32_t* outCount)
    {
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outCount = 0;
        if (outCapacity != 0 && outValues == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* actorHandle = reinterpret_cast<FUECActor*>(rawActor);
        if (!IsValidActor(actorHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(functionName) || functionName.size == 0 ||
            (argumentCount != 0 && argumentValues == nullptr)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        AActor* actor = actorHandle->Value.Get();
        if (actor == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UFunction* function = actor->FindFunction(FName(*ToFString(functionName)));
        if (function == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (IsLatentFunction(function) || function->HasAnyFunctionFlags(FUNC_Net) ||
            (function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly) &&
             actor->GetWorld() != nullptr && actor->GetWorld()->GetNetMode() == NM_Client)) {
            return UEC_RESULT_UNSUPPORTED;
        }
        if (!function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_Native | FUNC_BlueprintEvent)) {
            return UEC_RESULT_UNSUPPORTED;
        }

        TArray<FProperty*> inputParameters;
        TArray<FProperty*> outputParameters;
        FProperty* returnProperty = nullptr;
        for (TFieldIterator<FProperty> iterator(function); iterator; ++iterator)
        {
            FProperty* parameter = *iterator;
            if (!parameter->HasAnyPropertyFlags(CPF_Parm)) continue;
            if (parameter->HasAnyPropertyFlags(CPF_ReturnParm)) {
                returnProperty = parameter;
                continue;
            }
            if (parameter->HasAnyPropertyFlags(CPF_OutParm)) outputParameters.Add(parameter);
            if (!parameter->HasAnyPropertyFlags(CPF_OutParm) ||
                parameter->HasAnyPropertyFlags(CPF_ReferenceParm)) inputParameters.Add(parameter);
        }
        if (argumentCount != static_cast<uint32_t>(inputParameters.Num())) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }

        TArray<FProperty*> outputProperties;
        if (returnProperty != nullptr) outputProperties.Add(returnProperty);
        outputProperties.Append(outputParameters);
        if (static_cast<uint64>(outputProperties.Num()) > UINT32_MAX) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outCount = static_cast<uint32_t>(outputProperties.Num());
        if (static_cast<uint64>(outputProperties.Num()) > outCapacity) {
            return UEC_RESULT_BUFFER_TOO_SMALL;
        }
        for (uint32_t index = 0; index < *outCount; ++index)
        {
            if (outValues[index].struct_size < sizeof(uec_property_value)) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
        }

        FStructOnScope parameters(function);
        uint8* parameterMemory = parameters.GetStructMemory();
        for (uint32_t index = 0; index < argumentCount; ++index)
        {
            const uec_result result = SetInvocationPropertyValue(
                inputParameters[static_cast<int32>(index)], parameterMemory, argumentValues[index]);
            if (result != UEC_RESULT_OK) return result;
        }
        actor->ProcessEvent(function, parameterMemory);
        for (uint32_t index = 0; index < *outCount; ++index)
        {
            const uec_result result = ReadInvocationPropertyValue(
                outputProperties[static_cast<int32>(index)], parameterMemory, &outValues[index]);
            if (result != UEC_RESULT_OK) return result;
        }
        return UEC_RESULT_OK;
    }
