    uec_result UEC_CALL LoadObjectHandle(uec_context* rawContext,
                                         uec_string_view objectPath,
                                         uec_object** outObject)
    {
        if (outObject == nullptr || !IsValidStringView(objectPath) || objectPath.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outObject = nullptr;
        UObject* object = LoadObject<UObject>(nullptr, *ToFString(objectPath));
        if (object == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = MakeObjectHandle(object);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outObject = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ReleaseObject(uec_object* rawObject)
    {
        auto* handle = reinterpret_cast<FUECObject*>(rawObject);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
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
        if (callback == nullptr || outRequestId == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(objectPath) || objectPath.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        const FString pathString = ToFString(objectPath);
        if (GObjectLoadRequests.Num() >= MaxQueuedObjectLoads) return UEC_RESULT_QUEUE_FULL;
        const FSoftObjectPath path(pathString);
        if (!path.IsValid()) return UEC_RESULT_INVALID_ARGUMENT;

        const uint64 requestId = GNextObjectLoadRequestId++;
        auto request = MakeShared<FUECObjectLoadRequest>();
        request->Id = requestId;
        request->Path = path;
        request->Callback = callback;
        request->UserData = userData;
        GObjectLoadRequests.Add(requestId, request);
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
                auto* handle = new FUECObject();
                if (!InitializeHandle(handle->Header, EUECHandleKind::Object))
                {
                    delete handle;
                    GObjectLoadRequests.Remove(current->Id);
                    if (!IsShuttingDown())
                    {
                        current->Callback(current->Id, UEC_RESULT_INTERNAL_ERROR, nullptr,
                                          current->UserData);
                    }
                    return;
                }
                handle->Value = loadedObject;
                {
                    FScopeLock lock(&GHandleMutex);
                    GObjects.Add(handle);
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
            current->Callback(current->Id, result, objectHandle, current->UserData);
        });
        request->Handle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(path, completed);
        if (!request->Handle.IsValid())
        {
            GObjectLoadRequests.Remove(requestId);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outRequestId = requestId;
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
        if (outSaveGame == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outSaveGame = nullptr;
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
        if (outSaveGame == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        *outSaveGame = nullptr;
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

    uec_result UEC_CALL RunOnGameThread(uec_context* rawContext,
                                        uec_game_thread_callback callback,
                                        void* userData,
                                        uint64_t* outRequestId)
    {
        if (callback == nullptr || outRequestId == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        auto request = MakeShared<FUECGameThreadRequest>();
        request->Callback = callback;
        request->UserData = userData;
        {
            FScopeLock lock(&GHandleMutex);
            if (GGameThreadRequests.Num() >= MaxQueuedGameThreadRequests)
            {
                return UEC_RESULT_QUEUE_FULL;
            }
            request->Id = GNextGameThreadRequestId++;
            GGameThreadRequests.Add(request->Id, request);
        }
        *outRequestId = request->Id;
        AsyncTask(ENamedThreads::GameThread, [request]()
        {
            uec_game_thread_callback callbackToRun = nullptr;
            void* userDataToRun = nullptr;
            {
                FScopeLock lock(&GHandleMutex);
                if (request->Cancelled)
                {
                    GGameThreadRequests.Remove(request->Id);
                    return;
                }
                callbackToRun = request->Callback;
                userDataToRun = request->UserData;
                GGameThreadRequests.Remove(request->Id);
            }
            if (!IsShuttingDown()) callbackToRun(userDataToRun);
        });
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL CancelGameThreadRequest(uec_context* rawContext, uint64_t requestId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        FScopeLock lock(&GHandleMutex);
        TSharedPtr<FUECGameThreadRequest>* requestPtr = GGameThreadRequests.Find(requestId);
        if (requestPtr == nullptr || !requestPtr->IsValid()) return UEC_RESULT_INVALID_ARGUMENT;
        (*requestPtr)->Cancelled = true;
        GGameThreadRequests.Remove(requestId);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL IsObjectPathLoaded(uec_context* rawContext,
                                           uec_string_view objectPath,
                                           uec_bool* outLoaded)
    {
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
        if (callback == nullptr || outRequestId == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* saveHandle = reinterpret_cast<FUECObject*>(rawSaveGame);
        if (!IsValidObject(saveHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(slotName) || slotName.size == 0 || userIndex < 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        USaveGame* saveGame = Cast<USaveGame>(saveHandle->Value.Get());
        if (saveGame == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (GSaveGameRequests.Num() >= MaxQueuedGameThreadRequests) return UEC_RESULT_QUEUE_FULL;

        auto request = MakeShared<FUECSaveGameRequest>();
        request->Id = GNextSaveGameRequestId++;
        request->Callback = callback;
        request->UserData = userData;
        GSaveGameRequests.Add(request->Id, request);
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
        if (callback == nullptr || outRequestId == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(slotName) || slotName.size == 0 || userIndex < 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (GSaveGameRequests.Num() >= MaxQueuedGameThreadRequests) return UEC_RESULT_QUEUE_FULL;

        auto request = MakeShared<FUECSaveGameRequest>();
        request->Id = GNextSaveGameRequestId++;
        request->Callback = callback;
        request->UserData = userData;
        GSaveGameRequests.Add(request->Id, request);
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
        for (const TPair<uint64, TSharedPtr<FUECObjectLoadRequest>>& pair : GObjectLoadRequests)
        {
            if (pair.Value.IsValid())
            {
                pair.Value->Cancelled = true;
                if (pair.Value->Handle.IsValid()) pair.Value->Handle->CancelHandle();
            }
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
