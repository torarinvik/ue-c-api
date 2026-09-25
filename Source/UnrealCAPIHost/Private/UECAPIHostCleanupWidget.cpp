#include "UECAPIHostCleanupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"

void UECAPIHostCleanupWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (WidgetTree == nullptr || WidgetTree->RootWidget != nullptr) return;

    UButton* cleanupButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(), TEXT("CleanupButton"));
    WidgetTree->RootWidget = cleanupButton;
}
