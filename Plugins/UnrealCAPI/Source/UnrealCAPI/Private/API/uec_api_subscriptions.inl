    /* Timer, tick, UI, audio, and animation subscription lifetimes. */
    uec_result UEC_CALL SetTimer(uec_world* rawWorld,
                                 double intervalSeconds,
                                 uec_bool looping,
                                 uec_timer_callback callback,
                                 void* userData,
                                 uint64_t* outTimerId)
    {
        if (outTimerId != nullptr) *outTimerId = 0;
        if (outTimerId == nullptr || callback == nullptr || !IsValidBool(looping)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        if (!IsRepresentableFloat(intervalSeconds) || intervalSeconds <= 0.0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (GTimers.Num() >= MaxSubscriptions) return UEC_RESULT_QUEUE_FULL;

        uint64 timerId = 0;
        if (!AllocateMonotonicId(GNextTimerId, timerId)) return UEC_RESULT_INTERNAL_ERROR;
        auto state = MakeShared<FUECTimerState>();
        state->Id = timerId;
        state->World = world;
        state->Callback = callback;
        state->UserData = userData;
        state->Looping = looping != UEC_FALSE;
        TWeakPtr<FUECTimerState> weakState = state;
        FTimerDelegate delegate;
        delegate.BindLambda([weakState]()
        {
            TSharedPtr<FUECTimerState> current = weakState.Pin();
            if (!current.IsValid() || current->Cancelled || current->Callback == nullptr ||
                IsShuttingDown()) return;
            if (current->World.Get() == nullptr)
            {
                GTimers.Remove(current->Id);
                return;
            }
            FUECCallbackScope callbackScope;
            current->Callback(current->Id, current->UserData);
            if (!current->Looping)
            {
                GTimers.Remove(current->Id);
            }
        });
        world->GetTimerManager().SetTimer(state->Handle, delegate, intervalSeconds, state->Looping);
        if (IsShuttingDown())
        {
            world->GetTimerManager().ClearTimer(state->Handle);
            state->Cancelled = true;
            return UEC_RESULT_SHUTTING_DOWN;
        }
        GTimers.Add(timerId, state);
        *outTimerId = timerId;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL ClearTimer(uec_world* rawWorld, uint64_t timerId)
    {
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECTimerState>* statePtr = GTimers.Find(timerId);
        if (statePtr == nullptr || !statePtr->IsValid()) return UEC_RESULT_INVALID_ARGUMENT;
        TSharedPtr<FUECTimerState> state = *statePtr;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (state->World.Get() != world) return UEC_RESULT_INVALID_ARGUMENT;
        world->GetTimerManager().ClearTimer(state->Handle);
        state->Cancelled = true;
        GTimers.Remove(timerId);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SubscribeWorldTick(uec_world* rawWorld,
                                           uec_tick_callback callback,
                                           void* userData,
                                           uint64_t* outSubscriptionId)
    {
        if (outSubscriptionId != nullptr) *outSubscriptionId = 0;
        if (callback == nullptr || outSubscriptionId == nullptr)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (GTickSubscriptions.Num() >= MaxSubscriptions) return UEC_RESULT_QUEUE_FULL;

        uint64 subscriptionId = 0;
        if (!AllocateMonotonicId(GNextTickSubscriptionId, subscriptionId)) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        auto subscription = MakeShared<FUECTickSubscription>();
        subscription->Id = subscriptionId;
        subscription->World = world;
        subscription->Callback = callback;
        subscription->UserData = userData;
        TWeakPtr<FUECTickSubscription> weakSubscription = subscription;
        subscription->Handle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([weakSubscription](float deltaSeconds)
            {
                TSharedPtr<FUECTickSubscription> current = weakSubscription.Pin();
                if (!current.IsValid() || current->Cancelled || IsShuttingDown())
                {
                    return false;
                }
                if (current->World.Get() == nullptr)
                {
                    GTickSubscriptions.Remove(current->Id);
                    return false;
                }
                current->InCallback = true;
                FUECCallbackScope callbackScope;
                current->Callback(current->Id, static_cast<double>(deltaSeconds), current->UserData);
                current->InCallback = false;
                if (current->Cancelled || IsShuttingDown())
                {
                    GTickSubscriptions.Remove(current->Id);
                    return false;
                }
                return true;
            }),
            0.0f);
        if (IsShuttingDown())
        {
            FTSTicker::RemoveTicker(subscription->Handle);
            subscription->Cancelled = true;
            return UEC_RESULT_SHUTTING_DOWN;
        }
        GTickSubscriptions.Add(subscriptionId, subscription);
        *outSubscriptionId = subscriptionId;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL UnsubscribeWorldTick(uec_context* rawContext,
                                             uint64_t subscriptionId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECTickSubscription>* subscriptionPtr = GTickSubscriptions.Find(subscriptionId);
        if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid())
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        TSharedPtr<FUECTickSubscription> subscription = *subscriptionPtr;
        subscription->Cancelled = true;
        if (!subscription->InCallback)
        {
            FTSTicker::RemoveTicker(subscription->Handle);
        }
        GTickSubscriptions.Remove(subscriptionId);
        return UEC_RESULT_OK;
    }

    static void ClearAllTimers()
    {
        for (const TPair<uint64, TSharedPtr<FUECTimerState>>& pair : GTimers)
        {
            if (pair.Value.IsValid())
            {
                if (UWorld* world = pair.Value->World.Get())
                {
                    world->GetTimerManager().ClearTimer(pair.Value->Handle);
                }
                pair.Value->Cancelled = true;
            }
        }
        GTimers.Empty();
    }

    static void ClearAllTickSubscriptions()
    {
        for (const TPair<uint64, TSharedPtr<FUECTickSubscription>>& pair : GTickSubscriptions)
        {
            if (pair.Value.IsValid())
            {
                pair.Value->Cancelled = true;
                FTSTicker::RemoveTicker(pair.Value->Handle);
            }
        }
        GTickSubscriptions.Empty();
    }

    /* Button click callback subscriptions. */
    static void RemoveWidgetSubscription(const TSharedPtr<FUECWidgetSubscription>& subscription)
    {
        if (!subscription.IsValid()) return;
        if (UButton* button = subscription->Button.Get())
        {
            if (UECButtonClickBridge* bridge = subscription->Bridge.Get())
            {
                button->OnClicked.RemoveDynamic(
                    bridge, &UECButtonClickBridge::HandleButtonClicked);
            }
        }
        if (UECButtonClickBridge* bridge = subscription->Bridge.Get())
        {
            bridge->GetNativeClickedEvent().Remove(subscription->Handle);
        }
        subscription->Bridge.Reset();
    }

    uec_result UEC_CALL BindButtonClicked(uec_object* rawButton,
                                          uec_widget_event_callback callback,
                                          void* userData,
                                          uint64_t* outSubscriptionId)
    {
        if (outSubscriptionId != nullptr) *outSubscriptionId = 0;
        if (callback == nullptr || outSubscriptionId == nullptr)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* buttonHandle = reinterpret_cast<FUECObject*>(rawButton);
        if (!IsValidObject(buttonHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UButton* button = Cast<UButton>(buttonHandle->Value.Get());
        if (button == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (GWidgetSubscriptions.Num() >= MaxSubscriptions) return UEC_RESULT_QUEUE_FULL;

        uint64 subscriptionId = 0;
        if (!AllocateMonotonicId(GNextWidgetSubscriptionId, subscriptionId)) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        auto subscription = MakeShared<FUECWidgetSubscription>();
        subscription->Id = subscriptionId;
        subscription->Button = button;
        subscription->Callback = callback;
        subscription->UserData = userData;
        UECButtonClickBridge* bridge = NewObject<UECButtonClickBridge>(button);
        if (bridge == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        subscription->Bridge.Reset(bridge);
        button->OnClicked.AddDynamic(bridge, &UECButtonClickBridge::HandleButtonClicked);
        TWeakPtr<FUECWidgetSubscription> weakSubscription = subscription;
        subscription->Handle = bridge->GetNativeClickedEvent().AddLambda(
            [weakSubscription]()
            {
                TSharedPtr<FUECWidgetSubscription> current = weakSubscription.Pin();
                if (!current.IsValid() || current->Cancelled || IsShuttingDown()) return;
                current->InCallback = true;
                FUECCallbackScope callbackScope;
                current->Callback(current->Id, current->UserData);
                current->InCallback = false;
                current->Cancelled = true;
                RemoveWidgetSubscription(current);
                GWidgetSubscriptions.Remove(current->Id);
            });
        if (IsShuttingDown())
        {
            RemoveWidgetSubscription(subscription);
            subscription->Cancelled = true;
            return UEC_RESULT_SHUTTING_DOWN;
        }
        GWidgetSubscriptions.Add(subscriptionId, subscription);
        *outSubscriptionId = subscriptionId;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL UnbindButtonClicked(uec_context* rawContext,
                                            uint64_t subscriptionId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECWidgetSubscription>* subscriptionPtr = GWidgetSubscriptions.Find(subscriptionId);
        if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid())
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        TSharedPtr<FUECWidgetSubscription> subscription = *subscriptionPtr;
        subscription->Cancelled = true;
        if (!subscription->InCallback)
        {
            RemoveWidgetSubscription(subscription);
        }
        GWidgetSubscriptions.Remove(subscriptionId);
        return UEC_RESULT_OK;
    }


    /* Audio-finished callback subscriptions. */
    uec_result UEC_CALL BindAudioFinished(uec_object* rawAudioComponent,
                                          uec_audio_finished_callback callback,
                                          void* userData,
                                          uint64_t* outSubscriptionId)
    {
        if (outSubscriptionId != nullptr) *outSubscriptionId = 0;
        if (callback == nullptr || outSubscriptionId == nullptr)
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* audioHandle = reinterpret_cast<FUECObject*>(rawAudioComponent);
        if (!IsValidObject(audioHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UAudioComponent* audio = Cast<UAudioComponent>(audioHandle->Value.Get());
        if (audio == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (GAudioSubscriptions.Num() >= MaxSubscriptions) return UEC_RESULT_QUEUE_FULL;

        uint64 subscriptionId = 0;
        if (!AllocateMonotonicId(GNextAudioSubscriptionId, subscriptionId)) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        auto subscription = MakeShared<FUECAudioSubscription>();
        subscription->Id = subscriptionId;
        subscription->Component = audio;
        subscription->Callback = callback;
        subscription->UserData = userData;
        TWeakPtr<FUECAudioSubscription> weakSubscription = subscription;
        subscription->Handle = audio->OnAudioFinishedNative.AddLambda(
            [weakSubscription](UAudioComponent*)
            {
                TSharedPtr<FUECAudioSubscription> current = weakSubscription.Pin();
                if (!current.IsValid() || current->Cancelled || IsShuttingDown()) return;
                current->InCallback = true;
                FUECCallbackScope callbackScope;
                current->Callback(current->Id, current->UserData);
                current->InCallback = false;
                current->Cancelled = true;
                if (UAudioComponent* finishedAudio = current->Component.Get())
                {
                    finishedAudio->OnAudioFinishedNative.Remove(current->Handle);
                }
                GAudioSubscriptions.Remove(current->Id);
            });
        if (IsShuttingDown())
        {
            audio->OnAudioFinishedNative.Remove(subscription->Handle);
            subscription->Cancelled = true;
            return UEC_RESULT_SHUTTING_DOWN;
        }
        GAudioSubscriptions.Add(subscriptionId, subscription);
        *outSubscriptionId = subscriptionId;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL UnbindAudioFinished(uec_context* rawContext,
                                            uint64_t subscriptionId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECAudioSubscription>* subscriptionPtr = GAudioSubscriptions.Find(subscriptionId);
        if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid())
        {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        TSharedPtr<FUECAudioSubscription> subscription = *subscriptionPtr;
        subscription->Cancelled = true;
        if (!subscription->InCallback)
        {
            if (UAudioComponent* audio = subscription->Component.Get())
            {
                audio->OnAudioFinishedNative.Remove(subscription->Handle);
            }
        }
        GAudioSubscriptions.Remove(subscriptionId);
        return UEC_RESULT_OK;
    }

    static void CancelAudioSubscriptionIds(const TArray<uint64>& subscriptionIds)
    {
        for (uint64 subscriptionId : subscriptionIds)
        {
            TSharedPtr<FUECAudioSubscription>* subscriptionPtr =
                GAudioSubscriptions.Find(subscriptionId);
            if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid()) continue;
            TSharedPtr<FUECAudioSubscription> subscription = *subscriptionPtr;
            subscription->Cancelled = true;
            if (!subscription->InCallback)
            {
                if (UAudioComponent* audio = subscription->Component.Get())
                    audio->OnAudioFinishedNative.Remove(subscription->Handle);
            }
            GAudioSubscriptions.Remove(subscriptionId);
        }
    }

    static void CancelAudioSubscriptionsFor(UAudioComponent* audio)
    {
        if (audio == nullptr) return;
        TArray<uint64> subscriptionIds;
        for (const TPair<uint64, TSharedPtr<FUECAudioSubscription>>& pair : GAudioSubscriptions)
        {
            if (pair.Value.IsValid() && pair.Value->Component.Get() == audio)
            {
                subscriptionIds.Add(pair.Key);
            }
        }
        CancelAudioSubscriptionIds(subscriptionIds);
    }

    static void CancelAudioSubscriptionsForActor(AActor* actor)
    {
        if (actor == nullptr) return;
        TArray<uint64> subscriptionIds;
        for (const TPair<uint64, TSharedPtr<FUECAudioSubscription>>& pair : GAudioSubscriptions)
        {
            UAudioComponent* audio = pair.Value.IsValid() ? pair.Value->Component.Get() : nullptr;
            if (audio != nullptr && audio->GetOwner() == actor) subscriptionIds.Add(pair.Key);
        }
        CancelAudioSubscriptionIds(subscriptionIds);
    }

    static void CancelAudioSubscriptionsForWorld(UWorld* world)
    {
        if (world == nullptr) return;
        TArray<uint64> subscriptionIds;
        for (const TPair<uint64, TSharedPtr<FUECAudioSubscription>>& pair : GAudioSubscriptions)
        {
            UAudioComponent* audio = pair.Value.IsValid() ? pair.Value->Component.Get() : nullptr;
            if (audio != nullptr && audio->GetWorld() == world) subscriptionIds.Add(pair.Key);
        }
        CancelAudioSubscriptionIds(subscriptionIds);
    }


    /* Subscription registry shutdown cleanup. */
    static void ClearAllAudioSubscriptions()
    {
        for (const TPair<uint64, TSharedPtr<FUECAudioSubscription>>& pair : GAudioSubscriptions)
        {
            if (!pair.Value.IsValid()) continue;
            pair.Value->Cancelled = true;
            if (UAudioComponent* audio = pair.Value->Component.Get())
            {
                audio->OnAudioFinishedNative.Remove(pair.Value->Handle);
            }
        }
        GAudioSubscriptions.Empty();
    }

    static void ClearAllWidgetSubscriptions()
    {
        for (const TPair<uint64, TSharedPtr<FUECWidgetSubscription>>& pair : GWidgetSubscriptions)
        {
            if (!pair.Value.IsValid()) continue;
            pair.Value->Cancelled = true;
            RemoveWidgetSubscription(pair.Value);
        }
        GWidgetSubscriptions.Empty();
    }

    static void CancelWidgetSubscriptionsForWorld(UWorld* world)
    {
        if (world == nullptr) return;
        TArray<uint64> subscriptionIds;
        for (const TPair<uint64, TSharedPtr<FUECWidgetSubscription>>& pair : GWidgetSubscriptions)
        {
            UButton* button = pair.Value.IsValid() ? pair.Value->Button.Get() : nullptr;
            if (button != nullptr && button->GetWorld() == world) subscriptionIds.Add(pair.Key);
        }
        for (uint64 subscriptionId : subscriptionIds)
        {
            TSharedPtr<FUECWidgetSubscription>* subscriptionPtr =
                GWidgetSubscriptions.Find(subscriptionId);
            if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid()) continue;
            TSharedPtr<FUECWidgetSubscription> subscription = *subscriptionPtr;
            subscription->Cancelled = true;
            if (!subscription->InCallback) RemoveWidgetSubscription(subscription);
            GWidgetSubscriptions.Remove(subscriptionId);
        }
    }


    /* Poll the documented single-animation state so completion works without
     * requiring a generated native bridge component or Blueprint delegate. */
    uec_result UEC_CALL BindAnimationFinished(uec_scene_component* rawComponent,
                                              uec_animation_finished_callback callback,
                                              void* userData,
                                              uint64_t* outSubscriptionId)
    {
        if (outSubscriptionId != nullptr) *outSubscriptionId = 0;
        if (callback == nullptr || outSubscriptionId == nullptr) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USkeletalMeshComponent* component = Cast<USkeletalMeshComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        if (!component->IsPlaying()) return UEC_RESULT_NOT_INITIALIZED;
        if (GAnimationSubscriptions.Num() >= MaxSubscriptions) return UEC_RESULT_QUEUE_FULL;

        uint64 subscriptionId = 0;
        if (!AllocateMonotonicId(GNextAnimationSubscriptionId, subscriptionId)) {
            return UEC_RESULT_INTERNAL_ERROR;
        }
        auto subscription = MakeShared<FUECAnimationSubscription>();
        subscription->Id = subscriptionId;
        subscription->Component = component;
        subscription->Callback = callback;
        subscription->UserData = userData;
        subscription->WasPlaying = true;
        TWeakPtr<FUECAnimationSubscription> weakSubscription = subscription;
        subscription->Handle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([weakSubscription](float)
            {
                TSharedPtr<FUECAnimationSubscription> current = weakSubscription.Pin();
                if (!current.IsValid() || current->Cancelled || IsShuttingDown()) return false;
                USkeletalMeshComponent* component = current->Component.Get();
                if (component == nullptr)
                {
                    GAnimationSubscriptions.Remove(current->Id);
                    return false;
                }
                const bool isPlaying = component->IsPlaying();
                if (current->WasPlaying && !isPlaying)
                {
                    current->InCallback = true;
                    FUECCallbackScope callbackScope;
                    current->Callback(current->Id, current->UserData);
                    current->InCallback = false;
                    current->Cancelled = true;
                    GAnimationSubscriptions.Remove(current->Id);
                    return false;
                }
                current->WasPlaying = isPlaying;
                return true;
            }),
            0.0f);
        if (IsShuttingDown())
        {
            FTSTicker::RemoveTicker(subscription->Handle);
            subscription->Cancelled = true;
            return UEC_RESULT_SHUTTING_DOWN;
        }
        GAnimationSubscriptions.Add(subscriptionId, subscription);
        *outSubscriptionId = subscriptionId;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL UnbindAnimationFinished(uec_context* rawContext,
                                                uint64_t subscriptionId)
    {
        if (!IsValidContext(rawContext)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        TSharedPtr<FUECAnimationSubscription>* subscriptionPtr = GAnimationSubscriptions.Find(subscriptionId);
        if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid()) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        TSharedPtr<FUECAnimationSubscription> subscription = *subscriptionPtr;
        subscription->Cancelled = true;
        if (!subscription->InCallback) {
            FTSTicker::RemoveTicker(subscription->Handle);
        }
        GAnimationSubscriptions.Remove(subscriptionId);
        return UEC_RESULT_OK;
    }

    static void ClearAllAnimationSubscriptions()
    {
        for (const TPair<uint64, TSharedPtr<FUECAnimationSubscription>>& pair : GAnimationSubscriptions)
        {
            if (!pair.Value.IsValid()) continue;
            pair.Value->Cancelled = true;
            FTSTicker::RemoveTicker(pair.Value->Handle);
        }
        GAnimationSubscriptions.Empty();
    }

    static void CancelAnimationSubscriptionIds(const TArray<uint64>& subscriptionIds)
    {
        for (uint64 subscriptionId : subscriptionIds)
        {
            TSharedPtr<FUECAnimationSubscription>* subscriptionPtr =
                GAnimationSubscriptions.Find(subscriptionId);
            if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid()) continue;
            TSharedPtr<FUECAnimationSubscription> subscription = *subscriptionPtr;
            subscription->Cancelled = true;
            if (!subscription->InCallback) FTSTicker::RemoveTicker(subscription->Handle);
            GAnimationSubscriptions.Remove(subscriptionId);
        }
    }

    static void CancelAnimationSubscriptionsForActor(AActor* actor)
    {
        if (actor == nullptr) return;
        TArray<uint64> subscriptionIds;
        for (const TPair<uint64, TSharedPtr<FUECAnimationSubscription>>& pair : GAnimationSubscriptions)
        {
            USkeletalMeshComponent* component =
                pair.Value.IsValid() ? pair.Value->Component.Get() : nullptr;
            if (component != nullptr && component->GetOwner() == actor) subscriptionIds.Add(pair.Key);
        }
        CancelAnimationSubscriptionIds(subscriptionIds);
    }

    static void CancelAnimationSubscriptionsForWorld(UWorld* world)
    {
        if (world == nullptr) return;
        TArray<uint64> subscriptionIds;
        for (const TPair<uint64, TSharedPtr<FUECAnimationSubscription>>& pair : GAnimationSubscriptions)
        {
            USkeletalMeshComponent* component =
                pair.Value.IsValid() ? pair.Value->Component.Get() : nullptr;
            if (component != nullptr && component->GetWorld() == world)
                subscriptionIds.Add(pair.Key);
        }
        CancelAnimationSubscriptionIds(subscriptionIds);
    }
