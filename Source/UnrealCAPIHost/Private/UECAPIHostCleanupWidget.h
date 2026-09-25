#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UECAPIHostCleanupWidget.generated.h"

UCLASS(NotBlueprintable)
class UECAPIHostCleanupWidget final : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeOnInitialized() override;
};
