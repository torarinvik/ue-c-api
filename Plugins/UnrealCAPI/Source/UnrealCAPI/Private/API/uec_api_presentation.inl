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
        if (handle == nullptr) return HandleCreationFailureResult();
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

    uec_result UEC_CALL GetEditableTextBoxText(uec_object* rawEditableTextBox,
                                               char* buffer,
                                               size_t bufferSize,
                                               size_t* requiredSize)
    {
        if (requiredSize != nullptr) *requiredSize = 0u;
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECObject*>(rawEditableTextBox);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UEditableTextBox* editableTextBox = Cast<UEditableTextBox>(handle->Value.Get());
        if (editableTextBox == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        return CopyFStringToUtf8(
            editableTextBox->GetText().ToString(), buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL SetEditableTextBoxText(uec_object* rawEditableTextBox,
                                               uec_string_view text)
    {
        auto* handle = reinterpret_cast<FUECObject*>(rawEditableTextBox);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(text)) return UEC_RESULT_INVALID_ARGUMENT;
        UEditableTextBox* editableTextBox = Cast<UEditableTextBox>(handle->Value.Get());
        if (editableTextBox == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        editableTextBox->SetText(FText::FromString(ToFString(text)));
        return UEC_RESULT_OK;
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

    uec_result UEC_CALL GetSliderValue(uec_object* rawSlider, double* outValue)
    {
        if (outValue != nullptr) *outValue = 0.0;
        if (outValue == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECObject*>(rawSlider);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USlider* slider = Cast<USlider>(handle->Value.Get());
        if (slider == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        const float value = slider->GetValue();
        if (!FMath::IsFinite(value)) return UEC_RESULT_INTERNAL_ERROR;
        *outValue = static_cast<double>(value);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetSliderValue(uec_object* rawSlider, double value)
    {
        if (!IsRepresentableFloat(value) || value < 0.0 || value > 1.0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* handle = reinterpret_cast<FUECObject*>(rawSlider);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        USlider* slider = Cast<USlider>(handle->Value.Get());
        if (slider == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        slider->SetValue(static_cast<float>(value));
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetComboBoxSelectedOption(uec_object* rawComboBox,
                                                  char* buffer,
                                                  size_t bufferSize,
                                                  size_t* requiredSize)
    {
        if (requiredSize != nullptr) *requiredSize = 0u;
        if (requiredSize == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECObject*>(rawComboBox);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UComboBoxString* comboBox = Cast<UComboBoxString>(handle->Value.Get());
        if (comboBox == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        return CopyFStringToUtf8(comboBox->GetSelectedOption(),
                                 buffer, bufferSize, requiredSize);
    }

    uec_result UEC_CALL SetComboBoxSelectedOption(uec_object* rawComboBox,
                                                  uec_string_view option)
    {
        auto* handle = reinterpret_cast<FUECObject*>(rawComboBox);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(option) || option.size == 0u) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        UComboBoxString* comboBox = Cast<UComboBoxString>(handle->Value.Get());
        if (comboBox == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        const FString optionText = ToFString(option);
        if (comboBox->FindOptionIndex(optionText) == INDEX_NONE) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        comboBox->SetSelectedOption(optionText);
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

    uec_result UEC_CALL GetCheckBoxState(uec_object* rawCheckBox,
                                         uec_checkbox_state* outState)
    {
        if (outState != nullptr) *outState = UEC_CHECKBOX_UNCHECKED;
        if (outState == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* handle = reinterpret_cast<FUECObject*>(rawCheckBox);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UCheckBox* checkBox = Cast<UCheckBox>(handle->Value.Get());
        if (checkBox == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        switch (checkBox->GetCheckedState())
        {
        case ECheckBoxState::Unchecked: *outState = UEC_CHECKBOX_UNCHECKED; break;
        case ECheckBoxState::Checked: *outState = UEC_CHECKBOX_CHECKED; break;
        case ECheckBoxState::Undetermined: *outState = UEC_CHECKBOX_UNDETERMINED; break;
        default: return UEC_RESULT_UNSUPPORTED;
        }
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetCheckBoxState(uec_object* rawCheckBox,
                                         uec_checkbox_state state)
    {
        ECheckBoxState engineState;
        switch (state)
        {
        case UEC_CHECKBOX_UNCHECKED: engineState = ECheckBoxState::Unchecked; break;
        case UEC_CHECKBOX_CHECKED: engineState = ECheckBoxState::Checked; break;
        case UEC_CHECKBOX_UNDETERMINED: engineState = ECheckBoxState::Undetermined; break;
        default: return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* handle = reinterpret_cast<FUECObject*>(rawCheckBox);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UCheckBox* checkBox = Cast<UCheckBox>(handle->Value.Get());
        if (checkBox == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        checkBox->SetCheckedState(engineState);
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetWidgetChild(uec_object* rawUserWidget,
                                       uec_string_view childName,
                                       uec_object** outChild)
    {
        if (outChild != nullptr) *outChild = nullptr;
        if (outChild == nullptr || !IsValidStringView(childName) || childName.size == 0) {
            return UEC_RESULT_INVALID_ARGUMENT;
        }
        auto* handle = reinterpret_cast<FUECObject*>(rawUserWidget);
        if (!IsValidObject(handle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        UUserWidget* userWidget = Cast<UUserWidget>(handle->Value.Get());
        if (userWidget == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        UWidget* child = userWidget->GetWidgetFromName(FName(*ToFString(childName)));
        if (child == nullptr) return UEC_RESULT_NOT_INITIALIZED;
        FUECObject* childHandle = MakeObjectHandle(child);
        if (childHandle == nullptr) {
            return HandleCreationFailureResult();
        }
        *outChild = reinterpret_cast<uec_object*>(childHandle);
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
            return HandleCreationFailureResult();
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

    uec_result UEC_CALL SetComponentMaterialScalar(uec_scene_component* rawComponent,
                                                    uec_string_view parameterName,
                                                    double value)
    {
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(parameterName) || parameterName.size == 0 ||
            !IsRepresentableFloat(value)) return UEC_RESULT_INVALID_ARGUMENT;
        UMeshComponent* component = Cast<UMeshComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        component->SetScalarParameterValueOnMaterials(
            FName(*ToFString(parameterName)), static_cast<float>(value));
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL SetComponentMaterialVector(uec_scene_component* rawComponent,
                                                    uec_string_view parameterName,
                                                    uec_vector3 value)
    {
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(parameterName) || parameterName.size == 0 ||
            !IsRepresentableFloat(value.x) || !IsRepresentableFloat(value.y) ||
            !IsRepresentableFloat(value.z)) return UEC_RESULT_INVALID_ARGUMENT;
        UMeshComponent* component = Cast<UMeshComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        component->SetVectorParameterValueOnMaterials(
            FName(*ToFString(parameterName)), FVector(value.x, value.y, value.z));
        return UEC_RESULT_OK;
    }

    uec_result UEC_CALL GetComponentMaterialScalar(uec_scene_component* rawComponent,
                                                    uec_string_view parameterName,
                                                    double* outValue)
    {
        if (outValue != nullptr) *outValue = 0.0;
        if (outValue == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(parameterName) || parameterName.size == 0)
            return UEC_RESULT_INVALID_ARGUMENT;
        UMeshComponent* component = Cast<UMeshComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        const FHashedMaterialParameterInfo parameterInfo{
            FName(*ToFString(parameterName))};
        for (int32 index = 0; index < component->GetNumMaterials(); ++index)
        {
            UMaterialInterface* material = component->GetMaterial(index);
            if (!IsValid(material)) continue;
            float value = 0.0f;
            if (material->GetScalarParameterValue(parameterInfo, value))
            {
                if (!FMath::IsFinite(value)) return UEC_RESULT_INTERNAL_ERROR;
                *outValue = static_cast<double>(value);
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_NOT_INITIALIZED;
    }

    uec_result UEC_CALL GetComponentMaterialVector(uec_scene_component* rawComponent,
                                                    uec_string_view parameterName,
                                                    uec_vector3* outValue)
    {
        if (outValue != nullptr) *outValue = {};
        if (outValue == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        auto* componentHandle = reinterpret_cast<FUECSceneComponent*>(rawComponent);
        if (!IsValidComponent(componentHandle)) return UEC_RESULT_INVALID_HANDLE;
        if (!IsInGameThread()) return UEC_RESULT_WRONG_THREAD;
        if (!IsValidStringView(parameterName) || parameterName.size == 0)
            return UEC_RESULT_INVALID_ARGUMENT;
        UMeshComponent* component = Cast<UMeshComponent>(componentHandle->Value.Get());
        if (component == nullptr) return UEC_RESULT_INVALID_ARGUMENT;
        const FHashedMaterialParameterInfo parameterInfo{
            FName(*ToFString(parameterName))};
        for (int32 index = 0; index < component->GetNumMaterials(); ++index)
        {
            UMaterialInterface* material = component->GetMaterial(index);
            if (!IsValid(material)) continue;
            FLinearColor value{0.0f, 0.0f, 0.0f, 0.0f};
            if (material->GetVectorParameterValue(parameterInfo, value))
            {
                if (!FMath::IsFinite(value.R) || !FMath::IsFinite(value.G) ||
                    !FMath::IsFinite(value.B)) return UEC_RESULT_INTERNAL_ERROR;
                *outValue = {value.R, value.G, value.B};
                return UEC_RESULT_OK;
            }
        }
        return UEC_RESULT_NOT_INITIALIZED;
    }
