/* Level streaming and travel adapters. */
    uec_result UEC_CALL GetStreamingLevelCount(uec_world* rawWorld, uint32_t* outCount)
    {
        if (outCount != nullptr) *outCount = 0;
        if (outCount == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const TArray<ULevelStreaming*>& levels = world->GetStreamingLevels();
        if (static_cast<uint64>(levels.Num()) > UINT32_MAX) return UEC_RESULT_INTERNAL_ERROR;
        *outCount = static_cast<uint32_t>(levels.Num());
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetStreamingLevelAt(uec_world* rawWorld,
                                            uint32_t index,
                                            char* packageBuffer,
                                            size_t packageBufferSize,
                                            size_t* packageRequiredSize,
                                            uec_bool* outLoaded,
                                            uec_bool* outVisible)
    {
        if (packageRequiredSize != nullptr) *packageRequiredSize = 0;
        if (outLoaded != nullptr) *outLoaded = UEC_FALSE;
        if (outVisible != nullptr) *outVisible = UEC_FALSE;
        if (packageRequiredSize == nullptr || outLoaded == nullptr || outVisible == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const TArray<ULevelStreaming*>& levels = world->GetStreamingLevels();
        if (index >= static_cast<uint32_t>(levels.Num()) || levels[static_cast<int32>(index)] == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        ULevelStreaming* level = levels[static_cast<int32>(index)];
        *outLoaded = level->HasLoadedLevel() ? UEC_TRUE : UEC_FALSE;
        *outVisible = level->ShouldBeVisible() ? UEC_TRUE : UEC_FALSE;
        return CopyFStringToUtf8(level->GetWorldAssetPackageName(),
                                 packageBuffer, packageBufferSize, packageRequiredSize);
    }

    uec_result UEC_CALL SetStreamingLevelState(uec_world* rawWorld,
                                               uec_string_view packagePath,
                                               uec_bool shouldBeLoaded,
                                               uec_bool shouldBeVisible)
    {
        if (!IsValidStringView(packagePath) || packagePath.size == 0 ||
            !IsValidBool(shouldBeLoaded) || !IsValidBool(shouldBeVisible)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const FName targetPackage(*ToFString(packagePath));
        for (ULevelStreaming* level : world->GetStreamingLevels())
        {
            if (level == nullptr || FName(*level->GetWorldAssetPackageName()) != targetPackage) continue;
            level->SetShouldBeLoaded(shouldBeLoaded != UEC_FALSE);
            level->SetShouldBeVisible(shouldBeVisible != UEC_FALSE);
            return UEC_RESULT_OK;
        }
        return UEC_RESULT_NOT_INITIALIZED;
    }

    uec_result UEC_CALL GetWorldName(uec_world* rawWorld,
                                     char* buffer,
                                     size_t bufferSize,
                                     size_t* requiredSize)
    {
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *requiredSize = 0;
        auto* handle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = handle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        return CopyFStringToUtf8(world->GetMapName(), buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL TravelWorld(uec_world* rawWorld, uec_string_view levelPath)
    {
        if (!IsValidStringView(levelPath) || levelPath.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* handle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = handle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const FString path = ToFString(levelPath);
        if (path.IsEmpty()) return UEC_RESULT_INVALID_ARGUMENT;
        CancelTimersFor(world);
        CancelTickSubscriptionsFor(world);
        InvalidateWorldHandles(world);
        UGameplayStatics::OpenLevel(world, FName(*path));
        return UEC_RESULT_OK;
    }

    static bool TravelPathMatches(const FString& requestedPath, UWorld* world)
    {
        if (world == nullptr) return false;
        const FString mapName = UWorld::RemovePIEPrefix(world->GetMapName());
        const UPackage* package = world->GetOutermost();
        const FString packageName = package == nullptr
            ? FString() : UWorld::RemovePIEPrefix(package->GetName());
        return requestedPath == mapName || requestedPath == packageName ||
            (!packageName.IsEmpty() && requestedPath.StartsWith(packageName + TEXT(".")));
    }

    static int32 GetPIEInstanceForWorld(UWorld* world)
    {
        if (GEngine == nullptr || world == nullptr) return -1;
        for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
        {
            if (worldContext.World() == world) return worldContext.PIEInstance;
        }
        return -1;
    }

    static void HandlePostLoadMap(UWorld* world)
    {
        if (world == nullptr || IsShuttingDown()) return;
        TArray<TSharedPtr<FUECTravelRequest>> completed;
        {
            FScopeLock lock(&GHandleMutex);
            TArray<uint64> completedIds;
            for (const TPair<uint64, TSharedPtr<FUECTravelRequest>>& pair : GTravelRequests)
            {
                const TSharedPtr<FUECTravelRequest>& request = pair.Value;
                if (!request.IsValid() || request->Cancelled ||
                    request->PreviousWorld.Get() == world ||
                    !TravelPathMatches(request->LevelPath, world)) {
                    continue;
                }
                completedIds.Add(pair.Key);
                completed.Add(request);
            }
            for (uint64 requestId : completedIds) GTravelRequests.Remove(requestId);
        }
        for (const TSharedPtr<FUECTravelRequest>& request : completed)
        {
            if (!request.IsValid() || request->Cancelled || request->Callback == nullptr) continue;
            uec_world* rawWorld = nullptr;
            FUECWorld* worldHandle = MakeWorldHandle(
                world, world->WorldType, GetPIEInstanceForWorld(world));
            uec_result result = UEC_RESULT_OK;
            if (worldHandle == nullptr) {
                result = HandleCreationFailureResult();
            } else {
                rawWorld = reinterpret_cast<uec_world*>(worldHandle);
            }
            if (IsShuttingDown())
            {
                if (worldHandle != nullptr)
                {
                    TombstoneHandle(worldHandle->Header);
                    worldHandle->Value.Reset();
                }
                continue;
            }
            FUECCallbackScope callbackScope;
            request->Callback(request->Id, result, rawWorld, request->UserData);
        }
    }

    uec_result UEC_CALL TravelWorldAsync(uec_world* rawWorld,
                                         uec_string_view levelPath,
                                         uec_travel_callback callback,
                                         void* userData,
                                         uint64_t* outRequestId)
    {
        if (outRequestId != nullptr) *outRequestId = 0;
        if (callback == nullptr || outRequestId == nullptr || !IsValidStringView(levelPath) ||
            levelPath.size == 0) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        auto request = MakeShared<FUECTravelRequest>();
        request->PreviousWorld = world;
        request->LevelPath = ToFString(levelPath);
        request->Callback = callback;
        request->UserData = userData;
        {
            FScopeLock lock(&GHandleMutex);
            if (GShuttingDown) return UEC_RESULT_SHUTTING_DOWN;
            if (GTravelRequests.Num() >= MaxQueuedGameThreadRequests) return UEC_RESULT_QUEUE_FULL;
            if (!AllocateMonotonicId(GNextTravelRequestId, request->Id)) {
                return UEC_RESULT_INTERNAL_ERROR;
            }
            GTravelRequests.Add(request->Id, request);
        }
        *outRequestId = request->Id;
        CancelTimersFor(world);
        CancelTickSubscriptionsFor(world);
        InvalidateWorldHandles(world);
        UGameplayStatics::OpenLevel(world, FName(*request->LevelPath));
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL CancelTravelRequest(uec_context* rawContext, uint64_t requestId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        FScopeLock lock(&GHandleMutex);
        TSharedPtr<FUECTravelRequest>* request = GTravelRequests.Find(requestId);
        if (request == nullptr || !request->IsValid()) return UEC_RESULT_INVALID_ARGUMENT;
        (*request)->Cancelled = true;
        GTravelRequests.Remove(requestId);
        return UEC_RESULT_OK;
    }

    static void CancelAllTravelRequests()
    {
        FScopeLock lock(&GHandleMutex);
        for (TPair<uint64, TSharedPtr<FUECTravelRequest>>& pair : GTravelRequests)
        {
            if (pair.Value.IsValid()) pair.Value->Cancelled = true;
        }
        GTravelRequests.Empty();
    }


    struct FUECStreamingRequest final
    {
        uint64 Id = 0;
        TWeakObjectPtr<UWorld> World;
        FName Package;
        bool ShouldBeLoaded = false;
        bool ShouldBeVisible = false;
        uec_streaming_callback Callback = nullptr;
        void* UserData = nullptr;
        FTSTicker::FDelegateHandle Handle;
        bool Cancelled = false;
    };
    TMap<uint64, TSharedPtr<FUECStreamingRequest>> GStreamingRequests;
    uint64 GNextStreamingRequestId = 1;

    static ULevelStreaming* FindStreamingLevel(UWorld* world, const FName& package)
    {
        if (world == nullptr) return nullptr;
        for (ULevelStreaming* level : world->GetStreamingLevels())
        {
            if (level != nullptr && FName(*level->GetWorldAssetPackageName()) == package) return level;
        }
        return nullptr;
    }

    static bool IsStreamingRequestSatisfied(const FUECStreamingRequest& request,
                                            ULevelStreaming* level)
    {
        return level != nullptr && level->HasLoadedLevel() == request.ShouldBeLoaded &&
            level->IsLevelVisible() == request.ShouldBeVisible;
    }

    static void CompleteStreamingRequest(const TSharedPtr<FUECStreamingRequest>& request,
                                         uec_result result)
    {
        if (!request.IsValid() || request->Cancelled) return;
        ULevelStreaming* level = FindStreamingLevel(request->World.Get(), request->Package);
        const uec_bool loaded = level != nullptr && level->HasLoadedLevel() ? UEC_TRUE : UEC_FALSE;
        const uec_bool visible = level != nullptr && level->IsLevelVisible() ? UEC_TRUE : UEC_FALSE;
        request->Cancelled = true;
        {
            FScopeLock lock(&GHandleMutex);
            GStreamingRequests.Remove(request->Id);
        }
        if (IsShuttingDown() || request->Callback == nullptr) return;
        FUECCallbackScope callbackScope;
        request->Callback(request->Id, result, loaded, visible, request->UserData);
    }

    static bool PollStreamingRequest(const TSharedPtr<FUECStreamingRequest>& request)
    {
        if (!request.IsValid() || request->Cancelled || IsShuttingDown()) return false;
        ULevelStreaming* level = FindStreamingLevel(request->World.Get(), request->Package);
        if (level == nullptr)
        {
            CompleteStreamingRequest(request, UEC_RESULT_NOT_INITIALIZED);
            return false;
        }
        if (!IsStreamingRequestSatisfied(*request, level)) return true;
        CompleteStreamingRequest(request, UEC_RESULT_OK);
        return false;
    }

    static void CancelStreamingRequestsFor(UWorld* world)
    {
        if (world == nullptr) return;
        TArray<TSharedPtr<FUECStreamingRequest>> cancelled;
        {
            FScopeLock lock(&GHandleMutex);
            TArray<uint64> ids;
            for (const TPair<uint64, TSharedPtr<FUECStreamingRequest>>& pair : GStreamingRequests)
            {
                if (pair.Value.IsValid() && pair.Value->World.Get() == world) ids.Add(pair.Key);
            }
            for (uint64 id : ids)
            {
                TSharedPtr<FUECStreamingRequest>* request = GStreamingRequests.Find(id);
                if (request == nullptr || !request->IsValid()) continue;
                (*request)->Cancelled = true;
                cancelled.Add(*request);
                GStreamingRequests.Remove(id);
            }
        }
        for (const TSharedPtr<FUECStreamingRequest>& request : cancelled)
        {
            if (request.IsValid() && request->Handle.IsValid()) {
                FTSTicker::GetCoreTicker().RemoveTicker(request->Handle);
            }
        }
    }

    static void CancelAllStreamingRequests()
    {
        TArray<TSharedPtr<FUECStreamingRequest>> cancelled;
        {
            FScopeLock lock(&GHandleMutex);
            for (const TPair<uint64, TSharedPtr<FUECStreamingRequest>>& pair : GStreamingRequests)
            {
                if (pair.Value.IsValid()) {
                    pair.Value->Cancelled = true;
                    cancelled.Add(pair.Value);
                }
            }
            GStreamingRequests.Empty();
        }
        for (const TSharedPtr<FUECStreamingRequest>& request : cancelled)
        {
            if (request.IsValid() && request->Handle.IsValid()) {
                FTSTicker::GetCoreTicker().RemoveTicker(request->Handle);
            }
        }
    }

    uec_result UEC_CALL SetStreamingLevelStateAsync(uec_world* rawWorld,
                                                    uec_string_view packagePath,
                                                    uec_bool shouldBeLoaded,
                                                    uec_bool shouldBeVisible,
                                                    uec_streaming_callback callback,
                                                    void* userData,
                                                    uint64_t* outRequestId)
    {
        if (outRequestId != nullptr) *outRequestId = 0;
        if (outRequestId == nullptr || callback == nullptr || !IsValidStringView(packagePath) ||
            packagePath.size == 0 || !IsValidBool(shouldBeLoaded) || !IsValidBool(shouldBeVisible)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        const FName package(*ToFString(packagePath));
        ULevelStreaming* level = FindStreamingLevel(world, package);
        if (level == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        auto request = MakeShared<FUECStreamingRequest>();
        request->World = world;
        request->Package = package;
        request->ShouldBeLoaded = shouldBeLoaded != UEC_FALSE;
        request->ShouldBeVisible = shouldBeVisible != UEC_FALSE;
        request->Callback = callback;
        request->UserData = userData;
        {
            FScopeLock lock(&GHandleMutex);
            if (GShuttingDown) return UEC_RESULT_SHUTTING_DOWN;
            if (GStreamingRequests.Num() >= MaxQueuedGameThreadRequests) return UEC_RESULT_QUEUE_FULL;
            if (!AllocateMonotonicId(GNextStreamingRequestId, request->Id)) return UEC_RESULT_INTERNAL_ERROR;
            GStreamingRequests.Add(request->Id, request);
        }
        level->SetShouldBeLoaded(request->ShouldBeLoaded);
        level->SetShouldBeVisible(request->ShouldBeVisible);
        TWeakPtr<FUECStreamingRequest> weakRequest = request;
        request->Handle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([weakRequest](float)
            {
                return PollStreamingRequest(weakRequest.Pin());
            }));
        if (!request->Handle.IsValid())
        {
            request->Cancelled = true;
            GStreamingRequests.Remove(request->Id);
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outRequestId = request->Id;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL CancelStreamingLevelRequest(uec_context* rawContext,
                                                     uint64_t requestId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECStreamingRequest> request;
        {
            FScopeLock lock(&GHandleMutex);
            TSharedPtr<FUECStreamingRequest>* found = GStreamingRequests.Find(requestId);
            if (found == nullptr || !found->IsValid()) return UEC_RESULT_INVALID_ARGUMENT;
            request = *found;
            request->Cancelled = true;
            GStreamingRequests.Remove(requestId);
        }
        if (request->Handle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(request->Handle);
        return UEC_RESULT_OK;
    }

    /* Streaming requests are intentionally polled on the game thread. */
