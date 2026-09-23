#pragma once

#include "CoreMinimal.h"
#include "UECButtonClickBridge.generated.h"

DECLARE_MULTICAST_DELEGATE(FUECButtonClickNativeEvent);

UCLASS()
class UNREALCAPI_API UECButtonClickBridge final : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION()
    void HandleButtonClicked()
    {
        NativeClicked.Broadcast();
    }

    FUECButtonClickNativeEvent& GetNativeClickedEvent()
    {
        return NativeClicked;
    }

private:
    FUECButtonClickNativeEvent NativeClicked;
};
