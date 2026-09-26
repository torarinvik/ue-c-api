#include "UECAPIHostCleanupWidget.h"
#include "uec_api.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

static TWeakObjectPtr<UECAPIHostCleanupWidget> GLastCleanupWidget;

void UECAPIHostCleanupWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (WidgetTree == nullptr || WidgetTree->RootWidget != nullptr) return;

    UVerticalBox* root = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CleanupRoot"));
    UButton* cleanupButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("CleanupButton"));
    UTextBlock* textBlock = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("CleanupTextBlock"));
    UEditableTextBox* editableTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(
        UEditableTextBox::StaticClass(), TEXT("CleanupEditableTextBox"));
    USlider* slider = WidgetTree->ConstructWidget<USlider>(
        USlider::StaticClass(), TEXT("CleanupSlider"));
    UCheckBox* checkBox = WidgetTree->ConstructWidget<UCheckBox>(
        UCheckBox::StaticClass(), TEXT("CleanupCheckBox"));
    UProgressBar* progressBar = WidgetTree->ConstructWidget<UProgressBar>(
        UProgressBar::StaticClass(), TEXT("CleanupProgressBar"));
    UComboBoxString* comboBox = WidgetTree->ConstructWidget<UComboBoxString>(
        UComboBoxString::StaticClass(), TEXT("CleanupComboBox"));
    UTextBlock* statusText = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(), TEXT("StatusText"));
    UProgressBar* distanceProgress = WidgetTree->ConstructWidget<UProgressBar>(
        UProgressBar::StaticClass(), TEXT("DistanceProgress"));
    UButton* saveButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("SaveButton"));
    UButton* loadButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("LoadButton"));
    if (root == nullptr || cleanupButton == nullptr || textBlock == nullptr ||
        editableTextBox == nullptr || slider == nullptr || checkBox == nullptr ||
        progressBar == nullptr || comboBox == nullptr || statusText == nullptr ||
        distanceProgress == nullptr || saveButton == nullptr || loadButton == nullptr) return;
    checkBox->SetCheckedState(ECheckBoxState::Unchecked);
    comboBox->AddOption(TEXT("Low"));
    comboBox->AddOption(TEXT("High"));
    WidgetTree->RootWidget = root;
    GLastCleanupWidget = this;
    root->AddChildToVerticalBox(cleanupButton);
    root->AddChildToVerticalBox(textBlock);
    root->AddChildToVerticalBox(editableTextBox);
    root->AddChildToVerticalBox(slider);
    root->AddChildToVerticalBox(checkBox);
    root->AddChildToVerticalBox(progressBar);
    root->AddChildToVerticalBox(comboBox);
    root->AddChildToVerticalBox(statusText);
    root->AddChildToVerticalBox(distanceProgress);
    root->AddChildToVerticalBox(saveButton);
    root->AddChildToVerticalBox(loadButton);
}

extern "C" int UEC_CALL uec_host_cleanup_widget_click_last_button(void)
{
    UECAPIHostCleanupWidget* widget = GLastCleanupWidget.Get();
    if (!IsValid(widget) || widget->WidgetTree == nullptr) return 0;
    UButton* button = Cast<UButton>(
        widget->WidgetTree->FindWidget(FName(TEXT("CleanupButton"))));
    if (button == nullptr) return 0;
    button->OnClicked.Broadcast();
    return 1;
}

extern "C" int UEC_CALL uec_host_cleanup_widget_click_playable_button(int32 buttonIndex)
{
    static const TCHAR* const buttonNames[] = {TEXT("SaveButton"), TEXT("LoadButton")};
    if (buttonIndex < 0 || buttonIndex >= static_cast<int32>(UE_ARRAY_COUNT(buttonNames))) return 0;
    UECAPIHostCleanupWidget* widget = GLastCleanupWidget.Get();
    if (!IsValid(widget) || widget->WidgetTree == nullptr) return 0;
    UButton* button = Cast<UButton>(
        widget->WidgetTree->FindWidget(FName(buttonNames[buttonIndex])));
    if (button == nullptr) return 0;
    button->OnClicked.Broadcast();
    return 1;
}
