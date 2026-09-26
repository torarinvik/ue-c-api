#include "UECAPIHostCleanupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/Slider.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UECAPIHostCleanupWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (WidgetTree == nullptr || WidgetTree->RootWidget != nullptr) return;

    UVerticalBox* root = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CleanupRoot"));
    UButton* cleanupButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("CleanupButton"));
    UEditableTextBox* editableTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(
        UEditableTextBox::StaticClass(), TEXT("CleanupEditableTextBox"));
    USlider* slider = WidgetTree->ConstructWidget<USlider>(
        USlider::StaticClass(), TEXT("CleanupSlider"));
    UComboBoxString* comboBox = WidgetTree->ConstructWidget<UComboBoxString>(
        UComboBoxString::StaticClass(), TEXT("CleanupComboBox"));
    if (root == nullptr || cleanupButton == nullptr || editableTextBox == nullptr ||
        slider == nullptr || comboBox == nullptr) return;
    comboBox->AddOption(TEXT("Low"));
    comboBox->AddOption(TEXT("High"));
    WidgetTree->RootWidget = root;
    root->AddChildToVerticalBox(cleanupButton);
    root->AddChildToVerticalBox(editableTextBox);
    root->AddChildToVerticalBox(slider);
    root->AddChildToVerticalBox(comboBox);
}
