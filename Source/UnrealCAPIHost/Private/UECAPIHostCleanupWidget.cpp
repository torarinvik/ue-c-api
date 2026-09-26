#include "UECAPIHostCleanupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
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
    if (root == nullptr || cleanupButton == nullptr || editableTextBox == nullptr) return;
    WidgetTree->RootWidget = root;
    root->AddChildToVerticalBox(cleanupButton);
    root->AddChildToVerticalBox(editableTextBox);
}
