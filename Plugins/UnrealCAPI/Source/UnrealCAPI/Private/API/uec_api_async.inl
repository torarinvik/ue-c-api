    static uec_result RegisterObjectLoadRequest(const TSharedPtr<FUECObjectLoadRequest>& request)
    {
        FScopeLock lock(&GHandleMutex);
        if (GShuttingDown) return UEC_RESULT_SHUTTING_DOWN;
        if (GObjectLoadRequests.Num() >= MaxQueuedObjectLoads) return UEC_RESULT_QUEUE_FULL;
        if (!AllocateMonotonicId(GNextObjectLoadRequestId, request->Id)) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        GObjectLoadRequests.Add(request->Id, request);
        return UEC_RESULT_OK;
    }

    static uec_result RegisterSaveGameRequest(const TSharedPtr<FUECSaveGameRequest>& request)
    {
        FScopeLock lock(&GHandleMutex);
        if (GShuttingDown) return UEC_RESULT_SHUTTING_DOWN;
        if (GSaveGameRequests.Num() >= MaxQueuedGameThreadRequests) return UEC_RESULT_QUEUE_FULL;
        if (!AllocateMonotonicId(GNextSaveGameRequestId, request->Id)) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        GSaveGameRequests.Add(request->Id, request);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL LoadObjectHandle(uec_context* rawContext,
                                         uec_string_view objectPath,
                                         uec_object** outObject)
    {
        if (outObject != nullptr) *outObject = nullptr;
        if (outObject == nullptr || !IsValidStringView(objectPath) || objectPath.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = LoadObject<UObject>(nullptr, *ToFString(objectPath));
        if (object == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = MakeObjectHandle(object);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outObject = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL FindObjectHandle(uec_context* rawContext,
                                         uec_string_view objectPath,
                                         uec_object** outObject)
    {
        if (outObject != nullptr) *outObject = nullptr;
        if (outObject == nullptr || !IsValidStringView(objectPath) || objectPath.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = FindObject<UObject>(nullptr, *ToFString(objectPath));
        if (object == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        auto* handle = MakeObjectHandle(object);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outObject = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ReleaseObject(uec_object* rawObject)
    {
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsRegisteredHandle(handle, GObjects, EUECHandleKind::Object,
                                TEXT("Invalid or stale object handle"))) {
            return UEC_RESULT_INVALID_HANDLE;
        }
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TombstoneHandle(handle->Header);
        handle->Value.Reset();
        handle->StrongValue.Reset();
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetObjectName(uec_object* rawObject,
                                      char* buffer,
                                      size_t bufferSize,
                                      size_t* requiredSize)
    {
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0;
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return CopyFStringToUtf8(object->GetName(), buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL GetObjectPath(uec_object* rawObject,
                                      char* buffer,
                                      size_t bufferSize,
                                      size_t* requiredSize)
    {
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0;
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return CopyFStringToUtf8(object->GetPathName(), buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL GetObjectClassName(uec_object* rawObject,
                                           char* buffer,
                                           size_t bufferSize,
                                           size_t* requiredSize)
    {
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0;
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        if (object == nullptr || object->GetClass() == nullptr)
        {
            return UEC_RESULT_INVALID_HANDLE;
        }
        return CopyFStringToUtf8(object->GetClass()->GetPathName(), buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL ObjectIsA(uec_object* rawObject,
                                  uec_string_view classPath,
                                  uec_bool* outIsA)
    {
        if (outIsA != nullptr) *outIsA = UEC_FALSE;
        if (outIsA == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidStringView(classPath) || classPath.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UObject* object = handle->Value.Get();
        if (object == nullptr) return UEC_RESULT_INVALID_HANDLE;
        UClass* klass = LoadClass<UObject>(nullptr, *ToFString(classPath));
        if (klass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outIsA = object->IsA(klass) ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL RequestObjectLoad(uec_context* rawContext,
                                           uec_string_view objectPath,
                                           uec_object_load_callback callback,
                                           void* userData,
                                           uint64_t* outRequestId)
    {
        if (outRequestId != nullptr) *outRequestId = 0;
        if (callback == nullptr || outRequestId == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(objectPath) || objectPath.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        const FString pathString = ToFString(objectPath);
        const FSoftObjectPath path(pathString);
        if (!path.IsValid()) return UEC_RESULT_INVALID_ARGUMENT;

        auto request = MakeShared<FUECObjectLoadRequest>();
        request->Path = path;
        request->Callback = callback;
        request->UserData = userData;
        const uec_result registrationResult = RegisterObjectLoadRequest(request);
        if (registrationResult != UEC_RESULT_OK) return registrationResult;
        TWeakPtr<FUECObjectLoadRequest> weakRequest = request;
        FStreamableDelegate completed = FStreamableDelegate::CreateLambda([weakRequest]()
        {
            TSharedPtr<FUECObjectLoadRequest> current = weakRequest.Pin();
            if (!current.IsValid() || current->Cancelled || current->Callback == nullptr ||
                IsShuttingDown()) return;
            UObject* loadedObject = current->Path.ResolveObject();
            uec_object* objectHandle = nullptr;
            uec_result result = loadedObject == nullptr ? UEC_RESULT_INTERNAL_ERROR : UEC_RESULT_OK;
            if (loadedObject != nullptr)
            {
                FUECObject* handle = MakeObjectHandle(loadedObject);
                if (handle == nullptr)
                {
                    GObjectLoadRequests.Remove(current->Id);
                    if (!IsShuttingDown())
                    {
                        FUECCallbackScope callbackScope;
                        current->Callback(current->Id, UEC_RESULT_INTERNAL_ERROR, nullptr,
                                          current->UserData);
                    }
                    return;
                }
                objectHandle = reinterpret_cast<uec_object*>(handle);
            }
            GObjectLoadRequests.Remove(current->Id);
            if (IsShuttingDown())
            {
                if (FUECObject* handle = reinterpret_cast<FUECObject*>(objectHandle))
                {
                    TombstoneHandle(handle->Header);
                    handle->Value.Reset();
                    handle->StrongValue.Reset();
                }
                return;
            }
            FUECCallbackScope callbackScope;
            current->Callback(current->Id, result, objectHandle, current->UserData);
        });
        request->Handle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(path, completed);
        if (!request->Handle.IsValid())
        {
            GObjectLoadRequests.Remove(request->Id);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outRequestId = request->Id;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL CancelObjectLoad(uec_context* rawContext, uint64_t requestId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECObjectLoadRequest>* requestPtr = GObjectLoadRequests.Find(requestId);
        if (requestPtr == nullptr || !requestPtr->IsValid()) return UEC_RESULT_INVALID_ARGUMENT;
        TSharedPtr<FUECObjectLoadRequest> request = *requestPtr;
        request->Cancelled = true;
        if (request->Handle.IsValid()) request->Handle->CancelHandle();
        GObjectLoadRequests.Remove(requestId);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL CreateSaveGame(uec_context* rawContext,
                                       uec_string_view classPath,
                                       uec_object** outSaveGame)
    {
        if (outSaveGame != nullptr) *outSaveGame = nullptr;
        if (outSaveGame == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(classPath) || classPath.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        UClass* saveClass = LoadClass<USaveGame>(nullptr, *ToFString(classPath));
        if (saveClass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        USaveGame* saveGame = UGameplayStatics::CreateSaveGameObject(saveClass);
        FUECObject* handle = MakeObjectHandle(saveGame);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outSaveGame = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SaveGameToSlot(uec_object* rawSaveGame,
                                       uec_string_view slotName,
                                       int32_t userIndex,
                                       uec_bool* outSaved)
    {
        if (outSaved != nullptr) *outSaved = UEC_FALSE;
        if (outSaved == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* saveHandle = reinterpret_cast<FUECObject*>(rawSaveGame);
        if (!IsValidObject(saveHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(slotName) || userIndex < 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        USaveGame* saveGame = Cast<USaveGame>(saveHandle->Value.Get());
        if (saveGame == nullptr || slotName.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        *outSaved = UGameplayStatics::SaveGameToSlot(
            saveGame, ToFString(slotName), userIndex) ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL LoadGameFromSlot(uec_context* rawContext,
                                         uec_string_view classPath,
                                         uec_string_view slotName,
                                         int32_t userIndex,
                                         uec_object** outSaveGame)
    {
        if (outSaveGame != nullptr) *outSaveGame = nullptr;
        if (outSaveGame == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(classPath) || !IsValidStringView(slotName) ||
            classPath.size == 0 || slotName.size == 0 || userIndex < 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UClass* saveClass = LoadClass<USaveGame>(nullptr, *ToFString(classPath));
        if (saveClass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        USaveGame* saveGame = UGameplayStatics::LoadGameFromSlot(ToFString(slotName), userIndex);
        if (saveGame == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        if (!saveGame->IsA(saveClass)) return UEC_RESULT_INVALID_ARGUMENT;
        FUECObject* handle = MakeObjectHandle(saveGame);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outSaveGame = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL DeleteGameSlot(uec_context* rawContext,
                                       uec_string_view slotName,
                                       int32_t userIndex,
                                       uec_bool* outDeleted)
    {
        if (outDeleted != nullptr) *outDeleted = UEC_FALSE;
        if (outDeleted == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(slotName) || slotName.size == 0 || userIndex < 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        *outDeleted = UGameplayStatics::DeleteGameInSlot(ToFString(slotName), userIndex)
            ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SaveVersionedApplicationData(uec_context* rawContext,
                                                       uec_string_view slotName,
                                                       int32_t userIndex,
                                                       uint32_t schemaVersion,
                                                       const uint8_t* data,
                                                       size_t dataSize,
                                                       uec_bool* outSaved)
    {
        constexpr size_t kMaxPayloadSize = 16u * 1024u * 1024u;
        if (outSaved != nullptr) *outSaved = UEC_FALSE;
        if (outSaved == nullptr || schemaVersion == 0 || dataSize > kMaxPayloadSize ||
            (data == nullptr && dataSize != 0)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(slotName) || slotName.size == 0 || userIndex < 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }

        auto* saveGame = Cast<UECVersionedDataSaveGame>(
            UGameplayStatics::CreateSaveGameObject(UECVersionedDataSaveGame::StaticClass()));
        if (saveGame == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        saveGame->SchemaVersion = schemaVersion;
        saveGame->Payload.SetNumUninitialized(static_cast<int32>(dataSize));
        if (dataSize != 0) FMemory::Memcpy(saveGame->Payload.GetData(), data, dataSize);
        *outSaved = UGameplayStatics::SaveGameToSlot(
            saveGame, ToFString(slotName), userIndex) ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL LoadVersionedApplicationData(uec_context* rawContext,
                                                       uec_string_view slotName,
                                                       int32_t userIndex,
                                                       uint32_t* outSchemaVersion,
                                                       uint8_t* buffer,
                                                       size_t bufferCapacity,
                                                       size_t* outRequiredSize)
    {
        constexpr size_t kMaxPayloadSize = 16u * 1024u * 1024u;
        if (outSchemaVersion != nullptr) *outSchemaVersion = 0;
        if (outRequiredSize != nullptr) *outRequiredSize = 0;
        if (outSchemaVersion == nullptr || outRequiredSize == nullptr ||
            (buffer == nullptr && bufferCapacity != 0)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(slotName) || slotName.size == 0 || userIndex < 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }

        USaveGame* loaded = UGameplayStatics::LoadGameFromSlot(ToFString(slotName), userIndex);
        if (loaded == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        UECVersionedDataSaveGame* saveGame = Cast<UECVersionedDataSaveGame>(loaded);
        if (saveGame == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        const size_t payloadSize = static_cast<size_t>(saveGame->Payload.Num());
        if (saveGame->SchemaVersion == 0 || payloadSize > kMaxPayloadSize) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outSchemaVersion = saveGame->SchemaVersion;
        *outRequiredSize = payloadSize;
        if (bufferCapacity < payloadSize) return UEC_RESULT_BUFFER_TOO_SMALL;
        if (payloadSize != 0) FMemory::Memcpy(buffer, saveGame->Payload.GetData(), payloadSize);
        return UEC_RESULT_OK;
    }

    constexpr int32 MaxGameThreadCallbacksPerTick = 64;

    static void CompactGameThreadRequestOrder()
    {
        if (GGameThreadRequestOrderHead >= GGameThreadRequestOrder.Num())
        {
            GGameThreadRequestOrder.Reset();
            GGameThreadRequestOrderHead = 0;
        }
        else if (GGameThreadRequestOrderHead >= 256 &&
                 GGameThreadRequestOrderHead * 2 >= GGameThreadRequestOrder.Num())
        {
            GGameThreadRequestOrder.RemoveAt(
                0, GGameThreadRequestOrderHead, EAllowShrinking::No);
            GGameThreadRequestOrderHead = 0;
        }
    }

    bool DispatchGameThreadRequests(float)
    {
        int32 dispatched = 0;
        while (dispatched < MaxGameThreadCallbacksPerTick)
        {
            TSharedPtr<FUECGameThreadRequest> request;
            {
                FScopeLock lock(&GHandleMutex);
                if (GShuttingDown) return true;
                if (GGameThreadRequestOrderHead >= GGameThreadRequestOrder.Num()) break;

                const uint64 requestId =
                    GGameThreadRequestOrder[GGameThreadRequestOrderHead++];
                TSharedPtr<FUECGameThreadRequest>* requestPtr =
                    GGameThreadRequests.Find(requestId);
                if (requestPtr != nullptr && requestPtr->IsValid())
                {
                    request = *requestPtr;
                    GGameThreadRequests.Remove(requestId);
                }
                CompactGameThreadRequestOrder();
            }
            if (!request.IsValid()) continue;
            if (request->Cancelled || IsShuttingDown()) continue;

            FUECCallbackScope callbackScope;
            request->Callback(request->UserData);
            ++dispatched;
        }
        return true;
    }

    uec_result UEC_CALL RunOnGameThread(uec_context* rawContext,
                                        uec_game_thread_callback callback,
                                        void* userData,
                                        uint64_t* outRequestId)
    {
        if (outRequestId != nullptr) *outRequestId = 0;
        if (callback == nullptr || outRequestId == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto request = MakeShared<FUECGameThreadRequest>();
        request->Callback = callback;
        request->UserData = userData;
        const auto* context = reinterpret_cast<const FUECContext*>(rawContext);
        {
            FScopeLock lock(&GHandleMutex);
            if (GShuttingDown) return UEC_RESULT_SHUTTING_DOWN;
            if (!IsValidContextNoLock(context)) {
                SetLastErrorMessage(TEXT("Invalid or stale context handle"));
                return UEC_RESULT_INVALID_HANDLE;
            }
            if (GGameThreadRequests.Num() >= MaxQueuedGameThreadRequests)
            {
                return UEC_RESULT_QUEUE_FULL;
            }
            if (!AllocateMonotonicId(GNextGameThreadRequestId, request->Id)) {
                return UEC_RESULT_INTERNAL_ERROR;
            }
            GGameThreadRequests.Add(request->Id, request);
            GGameThreadRequestOrder.Add(request->Id);
        }
        *outRequestId = request->Id;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL CancelGameThreadRequest(uec_context* rawContext, uint64_t requestId)
    {
        const auto* context = reinterpret_cast<const FUECContext*>(rawContext);
        FScopeLock lock(&GHandleMutex);
        if (GShuttingDown) return UEC_RESULT_SHUTTING_DOWN;
        if (!IsValidContextNoLock(context)) {
            SetLastErrorMessage(TEXT("Invalid or stale context handle"));
            return UEC_RESULT_INVALID_HANDLE;
        }
        TSharedPtr<FUECGameThreadRequest>* requestPtr = GGameThreadRequests.Find(requestId);
        if (requestPtr == nullptr || !requestPtr->IsValid()) return UEC_RESULT_INVALID_ARGUMENT;
        int32 queueIndex = INDEX_NONE;
        for (int32 index = GGameThreadRequestOrderHead;
             index < GGameThreadRequestOrder.Num(); ++index) {
            if (GGameThreadRequestOrder[index] == requestId) {
                queueIndex = index;
                break;
            }
        }
        if (queueIndex == INDEX_NONE) return UEC_RESULT_INTERNAL_ERROR;
        (*requestPtr)->Cancelled = true;
        GGameThreadRequests.Remove(requestId);
        GGameThreadRequestOrder.RemoveAt(queueIndex, 1, EAllowShrinking::No);
        CompactGameThreadRequestOrder();
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetRuntimeStats(uec_context* rawContext,
                                        uec_runtime_stats* outStats)
    {
        constexpr size_t baseSize = offsetof(uec_runtime_stats, live_contexts);
        if (outStats == nullptr || outStats->struct_size < baseSize) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        outStats->active_subscriptions = 0;
        outStats->pending_requests = 0;
        outStats->active_callbacks = 0;
        if (outStats->struct_size >= sizeof(uec_runtime_stats)) {
            outStats->live_contexts = 0;
            outStats->live_worlds = 0;
            outStats->live_actors = 0;
            outStats->live_components = 0;
            outStats->live_classes = 0;
            outStats->live_objects = 0;
        }
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        FScopeLock lock(&GHandleMutex);
        const uint64 subscriptions = static_cast<uint64>(GTimers.Num()) +
            static_cast<uint64>(GTickSubscriptions.Num()) +
            static_cast<uint64>(GAudioSubscriptions.Num()) +
            static_cast<uint64>(GWidgetSubscriptions.Num()) +
            static_cast<uint64>(GAnimationSubscriptions.Num()) +
            static_cast<uint64>(GCollisionSubscriptions.Num()) +
            static_cast<uint64>(GEventBridgeSubscriptions.Num()) +
            static_cast<uint64>(GInputBindings.Num()) +
            static_cast<uint64>(GActorDestroyedSubscriptions.Num());
        const uint64 requests = static_cast<uint64>(GObjectLoadRequests.Num()) +
            static_cast<uint64>(GGameThreadRequests.Num()) +
            static_cast<uint64>(GTravelRequests.Num()) +
            static_cast<uint64>(GSaveGameRequests.Num()) +
            static_cast<uint64>(GStreamingRequests.Num()) +
            static_cast<uint64>(GLatentFunctionRequests.Num());
        if (subscriptions > UINT32_MAX || requests > UINT32_MAX || GActiveCallbacks < 0) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        outStats->active_subscriptions = static_cast<uint32>(subscriptions);
        outStats->pending_requests = static_cast<uint32>(requests);
        outStats->active_callbacks = static_cast<uint32>(GActiveCallbacks);
        if (outStats->struct_size >= sizeof(uec_runtime_stats)) {
            const auto countLiveHandles = [](const auto& registry) -> uint64
            {
                uint64 count = 0;
                for (const auto* handle : registry)
                {
                    if (handle != nullptr && handle->Header.Generation != 0 &&
                        !handle->Header.bReleased) ++count;
                }
                return count;
            };
            const uint64 contexts = countLiveHandles(GContexts);
            const uint64 worlds = countLiveHandles(GWorlds);
            const uint64 actors = countLiveHandles(GActors);
            const uint64 components = countLiveHandles(GComponents);
            const uint64 classes = countLiveHandles(GClasses);
            const uint64 objects = countLiveHandles(GObjects);
            if (contexts > UINT32_MAX || worlds > UINT32_MAX || actors > UINT32_MAX ||
                components > UINT32_MAX || classes > UINT32_MAX || objects > UINT32_MAX) {
                return UEC_RESULT_INTERNAL_ERROR;
            }
            outStats->live_contexts = static_cast<uint32>(contexts);
            outStats->live_worlds = static_cast<uint32>(worlds);
            outStats->live_actors = static_cast<uint32>(actors);
            outStats->live_components = static_cast<uint32>(components);
            outStats->live_classes = static_cast<uint32>(classes);
            outStats->live_objects = static_cast<uint32>(objects);
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL IsObjectPathLoaded(uec_context* rawContext,
                                           uec_string_view objectPath,
                                           uec_bool* outLoaded)
    {
        if (outLoaded != nullptr) *outLoaded = UEC_FALSE;
        if (outLoaded == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(objectPath) || objectPath.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        const FSoftObjectPath path(ToFString(objectPath));
        *outLoaded = path.ResolveObject() != nullptr ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL AsyncSaveGameToSlot(uec_object* rawSaveGame,
                                            uec_string_view slotName,
                                            int32_t userIndex,
                                            uec_save_game_callback callback,
                                            void* userData,
                                            uint64_t* outRequestId)
    {
        if (outRequestId != nullptr) *outRequestId = 0;
        if (callback == nullptr || outRequestId == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* saveHandle = reinterpret_cast<FUECObject*>(rawSaveGame);
        if (!IsValidObject(saveHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(slotName) || slotName.size == 0 || userIndex < 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        USaveGame* saveGame = Cast<USaveGame>(saveHandle->Value.Get());
        if (saveGame == nullptr) return UEC_RESULT_INVALID_ARGUMENT;

        auto request = MakeShared<FUECSaveGameRequest>();
        request->Callback = callback;
        request->UserData = userData;
        const uec_result registrationResult = RegisterSaveGameRequest(request);
        if (registrationResult != UEC_RESULT_OK) return registrationResult;
        TWeakPtr<FUECSaveGameRequest> weakRequest = request;
        UGameplayStatics::AsyncSaveGameToSlot(
            saveGame,
            ToFString(slotName),
            userIndex,
            FAsyncSaveGameToSlotDelegate::CreateLambda(
                [weakRequest](const FString&, const int32, bool success)
                {
                    TSharedPtr<FUECSaveGameRequest> current = weakRequest.Pin();
                    if (!current.IsValid() || current->Cancelled || current->Callback == nullptr ||
                        IsShuttingDown()) return;
                    GSaveGameRequests.Remove(current->Id);
                    if (!IsShuttingDown())
                    {
                        FUECCallbackScope callbackScope;
                        current->Callback(current->Id,
                                          success ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR,
                                          nullptr,
                                          success ? UEC_TRUE : UEC_FALSE,
                                          current->UserData);
                    }
                }));
        *outRequestId = request->Id;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL AsyncLoadGameFromSlot(uec_context* rawContext,
                                              uec_string_view slotName,
                                              int32_t userIndex,
                                              uec_save_game_callback callback,
                                              void* userData,
                                              uint64_t* outRequestId)
    {
        if (outRequestId != nullptr) *outRequestId = 0;
        if (callback == nullptr || outRequestId == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(slotName) || slotName.size == 0 || userIndex < 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }

        auto request = MakeShared<FUECSaveGameRequest>();
        request->Callback = callback;
        request->UserData = userData;
        const uec_result registrationResult = RegisterSaveGameRequest(request);
        if (registrationResult != UEC_RESULT_OK) return registrationResult;
        TWeakPtr<FUECSaveGameRequest> weakRequest = request;
        UGameplayStatics::AsyncLoadGameFromSlot(
            ToFString(slotName),
            userIndex,
            FAsyncLoadGameFromSlotDelegate::CreateLambda(
                [weakRequest](const FString&, const int32, USaveGame* saveGame)
                {
                    TSharedPtr<FUECSaveGameRequest> current = weakRequest.Pin();
                    if (!current.IsValid() || current->Cancelled || current->Callback == nullptr ||
                        IsShuttingDown()) return;
                    uec_object* objectHandle = nullptr;
                    if (saveGame != nullptr)
                    {
                        FUECObject* handle = MakeObjectHandle(saveGame);
                        if (handle != nullptr) objectHandle = reinterpret_cast<uec_object*>(handle);
                    }
                    const bool success = saveGame != nullptr && objectHandle != nullptr;
                    GSaveGameRequests.Remove(current->Id);
                    if (IsShuttingDown())
                    {
                        if (FUECObject* handle = reinterpret_cast<FUECObject*>(objectHandle))
                        {
                            TombstoneHandle(handle->Header);
                            handle->Value.Reset();
                            handle->StrongValue.Reset();
                        }
                        return;
                    }
                    FUECCallbackScope callbackScope;
                    current->Callback(current->Id,
                                      success ? UEC_RESULT_OK : UEC_RESULT_NOT_INITIALIZED,
                                      objectHandle,
                                      success ? UEC_TRUE : UEC_FALSE,
                                      current->UserData);
                }));
        *outRequestId = request->Id;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL CancelSaveGameRequest(uec_context* rawContext, uint64_t requestId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECSaveGameRequest>* requestPtr = GSaveGameRequests.Find(requestId);
        if (requestPtr == nullptr || !requestPtr->IsValid()) return UEC_RESULT_INVALID_ARGUMENT;
        (*requestPtr)->Cancelled = true;
        GSaveGameRequests.Remove(requestId);
        return UEC_RESULT_OK;
    }

    static void CancelAllObjectLoads()
    {
        TArray<TSharedPtr<FUECObjectLoadRequest>> requests;
        requests.Reserve(GObjectLoadRequests.Num());
        for (const TPair<uint64, TSharedPtr<FUECObjectLoadRequest>>& pair : GObjectLoadRequests)
        {
            if (pair.Value.IsValid()) requests.Add(pair.Value);
        }
        for (const TSharedPtr<FUECObjectLoadRequest>& request : requests)
        {
            request->Cancelled = true;
            if (request->Handle.IsValid()) request->Handle->CancelHandle();
        }
        GObjectLoadRequests.Empty();
    }

    static void CancelAllGameThreadRequests()
    {
        FScopeLock lock(&GHandleMutex);
        for (const TPair<uint64, TSharedPtr<FUECGameThreadRequest>>& pair : GGameThreadRequests)
        {
            if (pair.Value.IsValid()) pair.Value->Cancelled = true;
        }
        GGameThreadRequests.Empty();
        GGameThreadRequestOrder.Empty();
        GGameThreadRequestOrderHead = 0;
    }

    static void CancelAllSaveGameRequests()
    {
        for (const TPair<uint64, TSharedPtr<FUECSaveGameRequest>>& pair : GSaveGameRequests)
        {
            if (pair.Value.IsValid()) pair.Value->Cancelled = true;
        }
        GSaveGameRequests.Empty();
    }

    static void CancelAllInputBindings()
    {
        for (const TPair<uint64, TSharedPtr<FUECInputBinding>>& pair : GInputBindings)
        {
            if (!pair.Value.IsValid()) continue;
            pair.Value->Cancelled = true;
            if (UEnhancedInputComponent* component = pair.Value->Component.Get())
            {
                component->RemoveBindingByHandle(pair.Value->EngineHandle);
            }
        }
        GInputBindings.Empty();
    }
