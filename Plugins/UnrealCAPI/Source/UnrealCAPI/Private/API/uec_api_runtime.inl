/* Shared runtime validation, conversion, lifecycle, and diagnostics helpers. */
    static void CancelActorSubscriptions(AActor* actor);
    static void CancelActorSubscriptionsForWorld(UWorld* world);
    static void RemoveActorDestroyedHandler(UWorld* world);
    static void RemoveAllActorDestroyedHandlers(); static void ClearAllActorDestroyedSubscriptions();
    static void HandleWorldCleanup(UWorld* world, bool sessionEnded, bool cleanupResources);
    static void HandlePostLoadMap(UWorld* world); static void CancelAllTravelRequests(); static void CancelAllStreamingRequests(); static void CancelStreamingRequestsFor(UWorld* world);
    static bool AllocateMonotonicId(uint64& nextId, uint64& outId)
    {
        if (nextId == 0) return false;
        outId = nextId++;
        return true;
    }
    struct FUECCallbackScope final
    {
        FUECCallbackScope() { ++GActiveCallbacks; }
        ~FUECCallbackScope() { --GActiveCallbacks; }
    };
    constexpr int32 MaxQueuedObjectLoads = 1024;
    constexpr int32 MaxQueuedGameThreadRequests = 1024;
    constexpr int32 MaxSubscriptions = 1024;
    static constexpr size_t MaxLastErrorBytes = 512;
    static thread_local char GLastErrorMessage[MaxLastErrorBytes] = "No error";
    static thread_local size_t GLastErrorRequiredSize = sizeof("No error");
    static void SetLastErrorMessage(const TCHAR* message)
    {
        const TCHAR* source = message == nullptr ? TEXT("Unknown error") : message;
        FTCHARToUTF8 utf8(source);
        size_t length = static_cast<size_t>(utf8.Length());
        if (length >= MaxLastErrorBytes) length = MaxLastErrorBytes - 1;
        FMemory::Memcpy(GLastErrorMessage, utf8.Get(), length);
        GLastErrorMessage[length] = '\0';
        GLastErrorRequiredSize = length + 1;
    }
    static uec_result CopyLastErrorMessage(char* buffer, size_t bufferSize,
                                           size_t* requiredSize)
    {
        if (requiredSize == nullptr)
        {
            SetLastErrorMessage(TEXT("Required-size output is null"));
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        *requiredSize = GLastErrorRequiredSize;
        if (buffer == nullptr || bufferSize < GLastErrorRequiredSize)
        {
            return UEC_RESULT_BUFFER_TOO_SMALL;
        }
        FMemory::Memcpy(buffer, GLastErrorMessage, GLastErrorRequiredSize);
        return UEC_RESULT_OK;
    }
    static uint64 AllocateHandleGeneration()
    {
        FScopeLock lock(&GHandleMutex);
        if (GNextHandleGeneration == 0) return 0;
        return GNextHandleGeneration++;
    }
    static bool InitializeHandle(FUECHandleHeader& header, EUECHandleKind kind)
    {
        const uint64 generation = AllocateHandleGeneration();
        if (generation == 0) return false;
        header.Kind = kind;
        header.Generation = generation;
        header.bReleased = false;
        return true;
    }
    static void TombstoneHandle(FUECHandleHeader& header)
    {
        FScopeLock lock(&GHandleMutex);
        header.bReleased = true;
    }
    static bool IsShuttingDown()
    {
        FScopeLock lock(&GHandleMutex);
        return GShuttingDown;
    }
    static uec_result HandleCreationFailureResult()
    {
        return IsShuttingDown() ? UEC_RESULT_SHUTTING_DOWN : UEC_RESULT_INTERNAL_ERROR;
    }
    static bool IsValidContextNoLock(const FUECContext* context)
    {
        return context != nullptr && GContexts.Contains(context) &&
            context->Header.Kind == EUECHandleKind::Context &&
            context->Header.Generation != 0 && !context->Header.bReleased;
    }
    static bool IsValidContext(uec_context* rawContext)
    {
        const auto* context = reinterpret_cast<const FUECContext*>(rawContext);
        FScopeLock lock(&GHandleMutex);
        const bool valid = !GShuttingDown && IsValidContextNoLock(context);
        if (!valid) SetLastErrorMessage(TEXT("Invalid or stale context handle"));
        return valid;
    }
    // Release checks registry identity without requiring the weak UObject to remain valid.
    template <typename THandle, typename TRegistry>
    static bool IsRegisteredHandle(const THandle* handle, const TRegistry& registry,
                                   EUECHandleKind kind, const TCHAR* invalidMessage)
    {
        FScopeLock lock(&GHandleMutex);
        const bool valid = handle != nullptr && !GShuttingDown && registry.Contains(handle) &&
            handle->Header.Kind == kind && handle->Header.Generation != 0 &&
            !handle->Header.bReleased;
        if (!valid) SetLastErrorMessage(invalidMessage);
        return valid;
    }
    static bool IsValidWorld(const FUECWorld* world)
    {
        FScopeLock lock(&GHandleMutex);
        const bool valid = world != nullptr && !GShuttingDown && GWorlds.Contains(world) &&
            world->Header.Kind == EUECHandleKind::World && world->Header.Generation != 0 &&
            !world->Header.bReleased && world->Value.IsValid();
        if (!valid) SetLastErrorMessage(TEXT("Invalid or stale world handle"));
        return valid;
    }
    static bool IsValidActor(const FUECActor* actor)
    {
        FScopeLock lock(&GHandleMutex);
        const bool valid = actor != nullptr && !GShuttingDown && GActors.Contains(actor) &&
            actor->Header.Kind == EUECHandleKind::Actor && actor->Header.Generation != 0 &&
            !actor->Header.bReleased && actor->Value.IsValid();
        if (!valid) SetLastErrorMessage(TEXT("Invalid or stale actor handle"));
        return valid;
    }
    static bool IsValidComponent(const FUECSceneComponent* component)
    {
        FScopeLock lock(&GHandleMutex);
        const bool valid = component != nullptr && !GShuttingDown && GComponents.Contains(component) &&
            component->Header.Kind == EUECHandleKind::SceneComponent &&
            component->Header.Generation != 0 && !component->Header.bReleased &&
            component->Value.IsValid();
        if (!valid) SetLastErrorMessage(TEXT("Invalid or stale scene-component handle"));
        return valid;
    }
    static bool IsValidClass(const FUECClass* klass)
    {
        FScopeLock lock(&GHandleMutex);
        const bool valid = klass != nullptr && !GShuttingDown && GClasses.Contains(klass) &&
            klass->Header.Kind == EUECHandleKind::Class && klass->Header.Generation != 0 &&
            !klass->Header.bReleased && klass->Value.IsValid();
        if (!valid) SetLastErrorMessage(TEXT("Invalid or stale class handle"));
        return valid;
    }
    static bool IsValidObject(const FUECObject* object)
    {
        FScopeLock lock(&GHandleMutex);
        const bool valid = object != nullptr && !GShuttingDown && GObjects.Contains(object) &&
            object->Header.Kind == EUECHandleKind::Object && object->Header.Generation != 0 &&
            !object->Header.bReleased && object->Value.IsValid();
        if (!valid) SetLastErrorMessage(TEXT("Invalid or stale object handle"));
        return valid;
    }
    static uec_property_kind GetPropertyKind(const FProperty* property)
    {
        if (CastField<FBoolProperty>(property)) return UEC_PROPERTY_BOOL;
        if (const FByteProperty* byteProperty = CastField<FByteProperty>(property); byteProperty != nullptr && byteProperty->GetIntPropertyEnum() != nullptr) return UEC_PROPERTY_ENUM;
        if (CastField<FIntProperty>(property) || CastField<FInt64Property>(property) ||
            CastField<FUInt32Property>(property) || CastField<FUInt64Property>(property) ||
            CastField<FByteProperty>(property) || CastField<FInt16Property>(property) ||
            CastField<FUInt16Property>(property) || CastField<FInt8Property>(property) ||
            CastField<FUInt8Property>(property)) return UEC_PROPERTY_INTEGER;
        if (CastField<FFloatProperty>(property)) return UEC_PROPERTY_FLOAT; if (CastField<FDoubleProperty>(property)) return UEC_PROPERTY_DOUBLE;
        if (CastField<FEnumProperty>(property)) return UEC_PROPERTY_ENUM;
        if (CastField<FStrProperty>(property)) return UEC_PROPERTY_STRING;
        if (CastField<FNameProperty>(property)) return UEC_PROPERTY_NAME;
        if (CastField<FTextProperty>(property)) return UEC_PROPERTY_TEXT;
        if (CastField<FSoftClassProperty>(property)) return UEC_PROPERTY_SOFT_CLASS;
        if (CastField<FSoftObjectProperty>(property)) return UEC_PROPERTY_SOFT_OBJECT;
        if (CastField<FClassProperty>(property)) return UEC_PROPERTY_CLASS;
        if (CastField<FObjectPropertyBase>(property)) return UEC_PROPERTY_OBJECT;
        if (CastField<FStructProperty>(property)) return UEC_PROPERTY_STRUCT;
        if (CastField<FArrayProperty>(property)) return UEC_PROPERTY_ARRAY;
        if (CastField<FMapProperty>(property)) return UEC_PROPERTY_MAP;
        if (CastField<FSetProperty>(property)) return UEC_PROPERTY_SET;
        return UEC_PROPERTY_UNKNOWN;
    }
    static bool IsWritableProperty(const FProperty* property)
    {
        return property != nullptr &&
            !property->HasAnyPropertyFlags(
                CPF_EditConst | CPF_BlueprintReadOnly | CPF_ConstParm | CPF_ReturnParm);
    }
    static bool IsUnsignedIntegerProperty(const FProperty* property)
    {
        return CastField<FByteProperty>(property) || CastField<FUInt8Property>(property) ||
            CastField<FUInt16Property>(property) || CastField<FUInt32Property>(property) ||
            CastField<FUInt64Property>(property);
    }
    static bool TryReadIntegerProperty(const FNumericProperty* property,
                                       const void* container, int64& outValue)
    {
        if (property == nullptr || container == nullptr) return false;
        if (!IsUnsignedIntegerProperty(property))
        {
            outValue = property->GetSignedIntPropertyValue_InContainer(container);
            return true;
        }
        const uint64 value = property->GetUnsignedIntPropertyValue_InContainer(container);
        if (value > static_cast<uint64>(TNumericLimits<int64>::Max())) return false;
        outValue = static_cast<int64>(value);
        return true;
    }
    static bool IsIntegerValueInRange(const FNumericProperty* property, int64 value)
    {
        if (property == nullptr) return false;
        if (IsUnsignedIntegerProperty(property))
        {
            if (value < 0) return false;
            const uint64 unsignedValue = static_cast<uint64>(value);
            if (CastField<FByteProperty>(property) || CastField<FUInt8Property>(property))
                return unsignedValue <= TNumericLimits<uint8>::Max();
            if (CastField<FUInt16Property>(property))
                return unsignedValue <= TNumericLimits<uint16>::Max();
            if (CastField<FUInt32Property>(property))
                return unsignedValue <= TNumericLimits<uint32>::Max();
            return unsignedValue <= static_cast<uint64>(TNumericLimits<int64>::Max());
        }
        if (CastField<FInt8Property>(property))
            return value >= TNumericLimits<int8>::Lowest() && value <= TNumericLimits<int8>::Max();
        if (CastField<FInt16Property>(property))
            return value >= TNumericLimits<int16>::Lowest() && value <= TNumericLimits<int16>::Max();
        if (CastField<FIntProperty>(property))
            return value >= TNumericLimits<int32>::Lowest() && value <= TNumericLimits<int32>::Max();
        return true;
    }
    static uec_result CopyFStringToUtf8(const FString& value, char* buffer,
                                        size_t bufferSize, size_t* requiredSize)
    {
        if (requiredSize == nullptr)
        {
            SetLastErrorMessage(TEXT("Required-size output is null"));
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        FTCHARToUTF8 utf8(*value);
        const size_t required = static_cast<size_t>(utf8.Length()) + 1;
        *requiredSize = required;
        if (buffer == nullptr || bufferSize < required)
        {
            SetLastErrorMessage(TEXT("Output buffer is null or too small"));
            return UEC_RESULT_BUFFER_TOO_SMALL;
        }
        FMemory::Memcpy(buffer, utf8.Get(), required - 1);
        buffer[required - 1] = '\0';
        return UEC_RESULT_OK;
    }
    static bool IsValidStringView(uec_string_view value);
    static FString ToFString(uec_string_view value)
    {
        if (!IsValidStringView(value) || value.size == 0) return FString();
        FUTF8ToTCHAR converter(value.data, static_cast<int32>(value.size));
        return FString(converter.Length(), converter.Get());
    }
    static bool IsValidUtf8(uec_string_view value)
    {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(value.data);
        size_t index = 0;
        while (index < value.size)
        {
            const uint8_t first = bytes[index++];
            if (first == 0u) return false;
            if (first <= 0x7Fu) continue;
            uint32 codePoint = 0;
            size_t continuationCount = 0;
            uint32 minimum = 0;
            if (first >= 0xC2u && first <= 0xDFu)
            {
                codePoint = first & 0x1Fu;
                continuationCount = 1;
                minimum = 0x80u;
            }
            else if (first >= 0xE0u && first <= 0xEFu)
            {
                codePoint = first & 0x0Fu;
                continuationCount = 2;
                minimum = 0x800u;
            }
            else if (first >= 0xF0u && first <= 0xF4u)
            {
                codePoint = first & 0x07u;
                continuationCount = 3;
                minimum = 0x10000u;
            }
            else
            {
                return false;
            }
            if (index + continuationCount > value.size) return false;
            for (size_t continuation = 0; continuation < continuationCount; ++continuation)
            {
                const uint8_t next = bytes[index++];
                if ((next & 0xC0u) != 0x80u) return false;
                codePoint = (codePoint << 6u) | (next & 0x3Fu);
            }
            if (codePoint < minimum || codePoint > 0x10FFFFu ||
                (codePoint >= 0xD800u && codePoint <= 0xDFFFu))
            {
                return false;
            }
        }
        return true;
    }
    static bool IsValidStringView(uec_string_view value)
    {
        const bool valid = (value.data != nullptr || value.size == 0) &&
            value.size <= static_cast<size_t>(INT32_MAX) &&
            (value.size == 0 || IsValidUtf8(value));
        if (!valid) SetLastErrorMessage(TEXT("Invalid UTF-8 string view"));
        return valid;
    }
    static bool IsFiniteVector(const uec_vector3& value)
    {
        const double maximum = static_cast<double>(TNumericLimits<float>::Max());
        return FMath::IsFinite(value.x) && FMath::Abs(value.x) <= maximum &&
            FMath::IsFinite(value.y) && FMath::Abs(value.y) <= maximum &&
            FMath::IsFinite(value.z) && FMath::Abs(value.z) <= maximum;
    }
    static bool IsRepresentableFloat(double value)
    {
        return FMath::IsFinite(value) &&
            FMath::Abs(value) <= static_cast<double>(TNumericLimits<float>::Max());
    }
    static bool IsValidBool(uec_bool value)
    {
        return value == UEC_FALSE || value == UEC_TRUE;
    }
    static uec_result RequireWorldAuthority(const UWorld* world)
    {
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return world->GetNetMode() == NM_Client ? UEC_RESULT_UNSUPPORTED : UEC_RESULT_OK;
    }
    static bool IsFiniteTransform(const uec_transform& value)
    {
        const double rotationLengthSquared = value.rotation.x * value.rotation.x +
            value.rotation.y * value.rotation.y + value.rotation.z * value.rotation.z +
            value.rotation.w * value.rotation.w;
        return IsFiniteVector(value.translation) && IsFiniteVector(value.scale) &&
            IsRepresentableFloat(value.rotation.x) && IsRepresentableFloat(value.rotation.y) &&
            IsRepresentableFloat(value.rotation.z) && IsRepresentableFloat(value.rotation.w) &&
            rotationLengthSquared > 0.0;
    }
    static bool WriteInputActionValue(const FInputActionValue& value,
                                      uec_input_action_value& outValue)
    {
        outValue.struct_size = sizeof(uec_input_action_value);
        outValue.kind = UEC_INPUT_ACTION_VALUE_BOOLEAN;
        outValue.bool_value = UEC_FALSE;
        outValue.reserved[0] = 0;
        outValue.reserved[1] = 0;
        outValue.reserved[2] = 0;
        outValue.axis = {0.0, 0.0, 0.0};
        switch (value.GetValueType())
        {
        case EInputActionValueType::Boolean:
            outValue.bool_value = value.Get<bool>() ? UEC_TRUE : UEC_FALSE;
            return true;
        case EInputActionValueType::Axis1D:
            outValue.kind = UEC_INPUT_ACTION_VALUE_AXIS_1D;
            outValue.axis.x = static_cast<double>(value.Get<float>());
            return true;
        case EInputActionValueType::Axis2D:
        {
            outValue.kind = UEC_INPUT_ACTION_VALUE_AXIS_2D;
            const FVector2D axis = value.Get<FVector2D>();
            outValue.axis.x = axis.X;
            outValue.axis.y = axis.Y;
            return true;
        }
        case EInputActionValueType::Axis3D:
        {
            outValue.kind = UEC_INPUT_ACTION_VALUE_AXIS_3D;
            const FVector axis = value.Get<FVector>();
            outValue.axis.x = axis.X;
            outValue.axis.y = axis.Y;
            outValue.axis.z = axis.Z;
            return true;
        }
        default:
            return false;
        }
    }
    static FTransform ToFTransform(const uec_transform& value)
    {
        return FTransform(
            FQuat(value.rotation.x, value.rotation.y, value.rotation.z, value.rotation.w),
            FVector(value.translation.x, value.translation.y, value.translation.z),
            FVector(value.scale.x, value.scale.y, value.scale.z));
    }
    static uec_transform FromFTransform(const FTransform& value)
    {
        const FVector translation = value.GetTranslation();
        const FQuat rotation = value.GetRotation();
        const FVector scale = value.GetScale3D();
        return {{translation.X, translation.Y, translation.Z},
                {rotation.X, rotation.Y, rotation.Z, rotation.W},
                {scale.X, scale.Y, scale.Z}};
    }
    static uec_world_kind ToWorldKind(EWorldType::Type type)
    {
        switch (type)
        {
        case EWorldType::Game: return UEC_WORLD_KIND_GAME;
        case EWorldType::PIE: return UEC_WORLD_KIND_PIE;
        case EWorldType::Editor: return UEC_WORLD_KIND_EDITOR;
        case EWorldType::GamePreview: return UEC_WORLD_KIND_GAME_PREVIEW;
        case EWorldType::Inactive: return UEC_WORLD_KIND_INACTIVE;
        default: return UEC_WORLD_KIND_UNKNOWN;
        }
    }
    static bool ToCollisionChannel(uec_trace_channel channel, ECollisionChannel& outChannel)
    {
        switch (channel)
        {
        case UEC_TRACE_VISIBILITY: outChannel = ECC_Visibility; return true;
        case UEC_TRACE_CAMERA: outChannel = ECC_Camera; return true;
        case UEC_TRACE_WORLD_STATIC: outChannel = ECC_WorldStatic; return true;
        case UEC_TRACE_WORLD_DYNAMIC: outChannel = ECC_WorldDynamic; return true;
        case UEC_TRACE_PAWN: outChannel = ECC_Pawn; return true;
        case UEC_TRACE_PHYSICS_BODY: outChannel = ECC_PhysicsBody; return true;
        default: return false;
        }
    }
    static uec_result MakeCollisionShape(const uec_collision_shape* descriptor,
                                         FCollisionShape& outShape)
    {
        if (descriptor == nullptr || descriptor->struct_size < sizeof(uec_collision_shape))
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        switch (descriptor->kind)
        {
        case UEC_COLLISION_SHAPE_SPHERE:
            if (!IsRepresentableFloat(descriptor->radius) || descriptor->radius <= 0.0) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            outShape = FCollisionShape::MakeSphere(static_cast<float>(descriptor->radius));
            return UEC_RESULT_OK;
        case UEC_COLLISION_SHAPE_BOX:
            if (!IsRepresentableFloat(descriptor->half_extents.x) ||
                !IsRepresentableFloat(descriptor->half_extents.y) ||
                !IsRepresentableFloat(descriptor->half_extents.z) ||
                descriptor->half_extents.x <= 0.0 || descriptor->half_extents.y <= 0.0 ||
                descriptor->half_extents.z <= 0.0) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            outShape = FCollisionShape::MakeBox(FVector(
                descriptor->half_extents.x, descriptor->half_extents.y, descriptor->half_extents.z));
            return UEC_RESULT_OK;
        case UEC_COLLISION_SHAPE_CAPSULE:
            if (!IsRepresentableFloat(descriptor->radius) ||
                !IsRepresentableFloat(descriptor->half_height) ||
                descriptor->radius <= 0.0 || descriptor->half_height < descriptor->radius) {
                return UEC_RESULT_INVALID_ARGUMENT;
            }
            outShape = FCollisionShape::MakeCapsule(
                static_cast<float>(descriptor->radius), static_cast<float>(descriptor->half_height));
            return UEC_RESULT_OK;
        default:
            return UEC_RESULT_INVALID_ARGUMENT;
        }
    }
    static void LogOutstandingResources()
    {
        FScopeLock lock(&GHandleMutex);
        UE_LOG(LogTemp, Verbose,
            TEXT("%s shutdown resources: handles context=%d world=%d actor=%d component=%d class=%d object=%d; timers=%d tick=%d audio=%d widget=%d animation=%d collision=%d input=%d loads=%d game_thread=%d travel=%d saves=%d"),
            UTF8_TO_TCHAR(kModuleName), GContexts.Num(), GWorlds.Num(), GActors.Num(),
            GComponents.Num(), GClasses.Num(), GObjects.Num(), GTimers.Num(),
            GTickSubscriptions.Num(), GAudioSubscriptions.Num(), GWidgetSubscriptions.Num(),
            GAnimationSubscriptions.Num(), GCollisionSubscriptions.Num(), GInputBindings.Num(),
            GObjectLoadRequests.Num(), GGameThreadRequests.Num(), GTravelRequests.Num(),
            GSaveGameRequests.Num());
    }
