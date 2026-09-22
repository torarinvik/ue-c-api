    /* UMG creation, viewport state, and interaction callbacks. */
    uec_result UEC_CALL CreateWidget(uec_world* rawWorld,
                                     uec_string_view widgetClassPath,
                                     uec_object** outWidget)
    {
        if (outWidget != nullptr) *outWidget = nullptr;
        if (outWidget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* worldHandle = reinterpret_cast<FUECWorld*>(rawWorld);
        if (!IsValidWorld(worldHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWorld* world = worldHandle->Value.Get();
        if (world == nullptr) return UEC_RESULT_INVALID_HANDLE;
        if (!IsValidStringView(widgetClassPath) || widgetClassPath.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        APlayerController* controller = world->GetFirstPlayerController();
        if (controller == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        UClass* widgetClass = LoadClass<UUserWidget>(nullptr, *ToFString(widgetClassPath));
        if (widgetClass == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        UUserWidget* widget = CreateWidget<UUserWidget>(controller, widgetClass);
        FUECObject* handle = MakeObjectHandle(widget);
        if (handle == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        *outWidget = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL AddWidgetToViewport(uec_object* rawWidget, int32_t zOrder)
    {
        auto* widgetHandle = reinterpret_cast<FUECObject*>(rawWidget);
        if (!IsValidObject(widgetHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UUserWidget* widget = Cast<UUserWidget>(widgetHandle->Value.Get());
        if (widget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        widget->AddToViewport(zOrder);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL RemoveWidgetFromParent(uec_object* rawWidget)
    {
        auto* widgetHandle = reinterpret_cast<FUECObject*>(rawWidget);
        if (!IsValidObject(widgetHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UUserWidget* widget = Cast<UUserWidget>(widgetHandle->Value.Get());
        if (widget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        widget->RemoveFromParent();
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetWidgetVisibility(uec_object* rawWidget,
                                            uec_widget_visibility visibility)
    {
        ESlateVisibility engineVisibility;
        switch (visibility)
        {
        case UEC_WIDGET_VISIBLE: engineVisibility = ESlateVisibility::Visible; break;
        case UEC_WIDGET_COLLAPSED: engineVisibility = ESlateVisibility::Collapsed; break;
        case UEC_WIDGET_HIDDEN: engineVisibility = ESlateVisibility::Hidden; break;
        case UEC_WIDGET_HIT_TEST_INVISIBLE:
            engineVisibility = ESlateVisibility::HitTestInvisible;
            break;
        case UEC_WIDGET_SELF_HIT_TEST_INVISIBLE:
            engineVisibility = ESlateVisibility::SelfHitTestInvisible;
            break;
        default: return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* widgetHandle = reinterpret_cast<FUECObject*>(rawWidget);
        if (!IsValidObject(widgetHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWidget* widget = Cast<UWidget>(widgetHandle->Value.Get());
        if (widget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        widget->SetVisibility(engineVisibility);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetTextBlockText(uec_object* rawWidget,
                                         uec_string_view text)
    {
        auto* widgetHandle = reinterpret_cast<FUECObject*>(rawWidget);
        if (!IsValidObject(widgetHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(text)) return UEC_RESULT_INVALID_ARGUMENT;
        UTextBlock* textBlock = Cast<UTextBlock>(widgetHandle->Value.Get());
        if (textBlock == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        textBlock->SetText(FText::FromString(ToFString(text)));
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetWidgetVisibility(uec_object* rawWidget,
                                            uec_widget_visibility* outVisibility)
    {
        if (outVisibility != nullptr) *outVisibility = UEC_WIDGET_VISIBLE;
        if (outVisibility == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* widgetHandle = reinterpret_cast<FUECObject*>(rawWidget);
        if (!IsValidObject(widgetHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWidget* widget = Cast<UWidget>(widgetHandle->Value.Get());
        if (widget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        switch (widget->GetVisibility())
        {
        case ESlateVisibility::Visible: *outVisibility = UEC_WIDGET_VISIBLE; return UEC_RESULT_OK;
        case ESlateVisibility::Collapsed: *outVisibility = UEC_WIDGET_COLLAPSED; return UEC_RESULT_OK;
        case ESlateVisibility::Hidden: *outVisibility = UEC_WIDGET_HIDDEN; return UEC_RESULT_OK;
        case ESlateVisibility::HitTestInvisible:
            *outVisibility = UEC_WIDGET_HIT_TEST_INVISIBLE;
            return UEC_RESULT_OK;
        case ESlateVisibility::SelfHitTestInvisible:
            *outVisibility = UEC_WIDGET_SELF_HIT_TEST_INVISIBLE;
            return UEC_RESULT_OK;
        default: return UEC_RESULT_UNSUPPORTED;
        }
    }

    uec_result UEC_CALL GetTextBlockText(uec_object* rawWidget,
                                         char* buffer,
                                         size_t bufferSize,
                                         size_t* requiredSize)
    {
        if (requiredSize != nullptr) *requiredSize = 0;
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* widgetHandle = reinterpret_cast<FUECObject*>(rawWidget);
        if (!IsValidObject(widgetHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UTextBlock* textBlock = Cast<UTextBlock>(widgetHandle->Value.Get());
        if (textBlock == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        return CopyFStringToUtf8(textBlock->GetText().ToString(), buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL GetProgressBarPercent(uec_object* rawProgressBar,
                                              double* outPercent)
    {
        if (outPercent != nullptr) *outPercent = 0.0;
        if (outPercent == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECObject*>(rawProgressBar);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UProgressBar* progressBar = Cast<UProgressBar>(handle->Value.Get());
        if (progressBar == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        const float percent = progressBar->GetPercent();
        if (!FMath::IsFinite(percent)) return UEC_RESULT_INTERNAL_ERROR;
        *outPercent = static_cast<double>(percent);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetProgressBarPercent(uec_object* rawProgressBar,
                                              double percent)
    {
        if (!IsRepresentableFloat(percent) || percent < 0.0 || percent > 1.0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* handle = reinterpret_cast<FUECObject*>(rawProgressBar);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UProgressBar* progressBar = Cast<UProgressBar>(handle->Value.Get());
        if (progressBar == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        progressBar->SetPercent(static_cast<float>(percent));
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetWidgetEnabled(uec_object* rawWidget, uec_bool* outEnabled)
    {
        if (outEnabled != nullptr) *outEnabled = UEC_FALSE;
        if (outEnabled == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECObject*>(rawWidget);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWidget* widget = Cast<UWidget>(handle->Value.Get());
        if (widget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outEnabled = widget->GetIsEnabled() ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetWidgetEnabled(uec_object* rawWidget, uec_bool enabled)
    {
        if (!IsValidBool(enabled)) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECObject*>(rawWidget);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UWidget* widget = Cast<UWidget>(handle->Value.Get());
        if (widget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        widget->SetIsEnabled(enabled != UEC_FALSE);
        return UEC_RESULT_OK;
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
        TWeakPtr<FUECWidgetSubscription> weakSubscription = subscription;
        subscription->Handle = button->OnClicked.AddLambda(
            [weakSubscription]()
            {
                TSharedPtr<FUECWidgetSubscription> current = weakSubscription.Pin();
                if (!current.IsValid() || current->Cancelled || IsShuttingDown()) return;
                current->InCallback = true;
                FUECCallbackScope callbackScope;
                current->Callback(current->Id, current->UserData);
                current->InCallback = false;
                current->Cancelled = true;
                if (UButton* button = current->Button.Get())
                {
                    button->OnClicked.Remove(current->Handle);
                }
                GWidgetSubscriptions.Remove(current->Id);
            });
        if (IsShuttingDown())
        {
            button->OnClicked.Remove(subscription->Handle);
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
            if (UButton* button = subscription->Button.Get())
            {
                button->OnClicked.Remove(subscription->Handle);
            }
        }
        GWidgetSubscriptions.Remove(subscriptionId);
        return UEC_RESULT_OK;
    }

    /* Camera component properties. */
    uec_result UEC_CALL GetCameraFieldOfView(uec_scene_component* rawComponent,
                                             double* outDegrees)
    {
        if (outDegrees != nullptr) *outDegrees = 0.0;
        if (outDegrees == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UCameraComponent* camera = Cast<UCameraComponent>(componentHandle->Value.Get());
        if (camera == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outDegrees = static_cast<double>(camera->FieldOfView);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetCameraFieldOfView(uec_scene_component* rawComponent,
                                             double degrees)
    {
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsRepresentableFloat(degrees) || degrees <= 0.0 || degrees >= 360.0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UCameraComponent* camera = Cast<UCameraComponent>(componentHandle->Value.Get());
        if (camera == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        camera->SetFieldOfView(static_cast<float>(degrees));
        return UEC_RESULT_OK;
    }

    /* Attached audio components and completion subscriptions. */
    uec_result UEC_CALL SpawnSoundAttached(uec_scene_component* rawAttachTo,
                                           uec_object* rawSound,
                                           uec_string_view socketName,
                                           double volumeMultiplier,
                                           double pitchMultiplier,
                                           uec_object** outAudioComponent)
    {
        if (outAudioComponent != nullptr) *outAudioComponent = nullptr;
        if (outAudioComponent == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawAttachTo);
        auto* soundHandle = reinterpret_cast<FUECObject*>(rawSound);
        if (!IsValidComponent(componentHandle) || !IsValidObject(soundHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(socketName) ||
            !IsRepresentableFloat(volumeMultiplier) || !IsRepresentableFloat(pitchMultiplier) ||
            volumeMultiplier < 0.0 || pitchMultiplier <= 0.0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        USceneComponent* attachTo = componentHandle->Value.Get();
        USoundBase* sound = Cast<USoundBase>(soundHandle->Value.Get());
        if (attachTo == nullptr || sound == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        UAudioComponent* audio = UGameplayStatics::SpawnSoundAttached(
            sound,
            attachTo,
            FName(*ToFString(socketName)),
            FVector::ZeroVector,
            EAttachLocation::KeepRelativeOffset,
            true,
            static_cast<float>(volumeMultiplier),
            static_cast<float>(pitchMultiplier),
            0.0f,
            nullptr,
            nullptr,
            false);
        if (audio == nullptr) return UEC_RESULT_INTERNAL_ERROR;
        FUECObject* handle = MakeObjectHandle(audio);
        if (handle == nullptr)
        {
            audio->DestroyComponent();
            return UEC_RESULT_INTERNAL_ERROR;
        }
        *outAudioComponent = reinterpret_cast<uec_object*>(handle);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL StopAudioComponent(uec_object* rawAudioComponent)
    {
        auto* audioHandle = reinterpret_cast<FUECObject*>(rawAudioComponent);
        if (!IsValidObject(audioHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UAudioComponent* audio = Cast<UAudioComponent>(audioHandle->Value.Get());
        if (audio == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        audio->Stop();
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetAudioComponentPlaying(uec_object* rawAudioComponent,
                                                uec_bool* outPlaying)
    {
        if (outPlaying != nullptr) *outPlaying = UEC_FALSE;
        if (outPlaying == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* audioHandle = reinterpret_cast<FUECObject*>(rawAudioComponent);
        if (!IsValidObject(audioHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UAudioComponent* audio = Cast<UAudioComponent>(audioHandle->Value.Get());
        if (audio == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        *outPlaying = audio->IsPlaying() ? UEC_TRUE : UEC_FALSE;
        return UEC_RESULT_OK;
    }

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
        for (uint64 subscriptionId : subscriptionIds)
        {
            TSharedPtr<FUECAudioSubscription>* subscriptionPtr = GAudioSubscriptions.Find(subscriptionId);
            if (subscriptionPtr == nullptr || !subscriptionPtr->IsValid()) continue;
            (*subscriptionPtr)->Cancelled = true;
            audio->OnAudioFinishedNative.Remove((*subscriptionPtr)->Handle);
            GAudioSubscriptions.Remove(subscriptionId);
        }
    }

    uec_result UEC_CALL DestroyAudioComponent(uec_object* rawAudioComponent)
    {
        auto* audioHandle = reinterpret_cast<FUECObject*>(rawAudioComponent);
        if (!IsValidObject(audioHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UAudioComponent* audio = Cast<UAudioComponent>(audioHandle->Value.Get());
        if (audio == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        CancelAudioSubscriptionsFor(audio);
        TombstoneHandle(audioHandle->Header);
        audioHandle->Value.Reset();
        audioHandle->StrongValue.Reset();
        audio->Stop();
        audio->DestroyComponent();
        return UEC_RESULT_OK;
    }

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
            if (UButton* button = pair.Value->Button.Get())
            {
                button->OnClicked.Remove(pair.Value->Handle);
            }
        }
        GWidgetSubscriptions.Empty();
    }

    /* Static and skeletal mesh assignment and transient animation playback. */
    uec_result UEC_CALL SetStaticMesh(uec_scene_component* rawComponent, uec_object* rawMesh)
    {
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        auto* meshHandle = reinterpret_cast<FUECObject*>(rawMesh);
        if (!IsValidComponent(componentHandle) || !IsValidObject(meshHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UStaticMeshComponent* component = Cast<UStaticMeshComponent>(componentHandle->Value.Get());
        UStaticMesh* mesh = Cast<UStaticMesh>(meshHandle->Value.Get());
        if (component == nullptr || mesh == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        return component->SetStaticMesh(mesh) ? UEC_RESULT_OK : UEC_RESULT_INTERNAL_ERROR;
    }

    uec_result UEC_CALL SetSkeletalMesh(uec_scene_component* rawComponent,
                                        uec_object* rawMesh,
                                        uec_bool reinitializePose)
    {
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        auto* meshHandle = reinterpret_cast<FUECObject*>(rawMesh);
        if (!IsValidComponent(componentHandle) || !IsValidObject(meshHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USkeletalMeshComponent* component = Cast<USkeletalMeshComponent>(componentHandle->Value.Get());
        USkeletalMesh* mesh = Cast<USkeletalMesh>(meshHandle->Value.Get());
        if (component == nullptr || mesh == nullptr || !IsValidBool(reinitializePose)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        component->SetSkeletalMesh(mesh, reinitializePose != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL PlaySkeletalAnimation(uec_scene_component* rawComponent,
                                              uec_object* rawAnimation,
                                              uec_bool looping)
    {
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        auto* animationHandle = reinterpret_cast<FUECObject*>(rawAnimation);
        if (!IsValidComponent(componentHandle) || !IsValidObject(animationHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USkeletalMeshComponent* component = Cast<USkeletalMeshComponent>(componentHandle->Value.Get());
        UAnimationAsset* animation = Cast<UAnimationAsset>(animationHandle->Value.Get());
        if (component == nullptr || animation == nullptr || !IsValidBool(looping)) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        component->PlayAnimation(animation, looping != UEC_FALSE);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL StopSkeletalAnimation(uec_scene_component* rawComponent)
    {
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USkeletalMeshComponent* component = Cast<USkeletalMeshComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        component->Stop();
        return UEC_RESULT_OK;
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
