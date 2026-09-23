#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UECEventBridgeComponent.generated.h"

class UECEventBridgeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FUECEventBridgeBlueprintEvent,
                                               int64, EventId,
                                               int64, IntegerValue,
                                               double, RealValue,
                                               const FString&, TextValue);
DECLARE_MULTICAST_DELEGATE_FourParams(FUECEventBridgeNativeEvent,
                                      int64, int64, double, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FUECEventBridgeNativeDestroyed,
                                    UECEventBridgeComponent*);

UCLASS(ClassGroup=(UnrealCAPI), meta=(BlueprintSpawnableComponent))
class UNREALCAPI_API UECEventBridgeComponent final : public UActorComponent
{
    GENERATED_BODY()

public:
    UECEventBridgeComponent();

    UPROPERTY(BlueprintAssignable, Category="Unreal C API")
    FUECEventBridgeBlueprintEvent OnEvent;

    UFUNCTION(BlueprintCallable, Category="Unreal C API")
    void EmitEvent(int64 EventId, int64 IntegerValue, double RealValue,
                   const FString& TextValue);

    FUECEventBridgeNativeDestroyed& GetNativeDestroyedEvent() { return NativeDestroyedEvent; }
    void OnComponentDestroyed(bool bDestroyingHierarchy) override;

    FUECEventBridgeNativeEvent& GetNativeEvent() { return NativeEvent; }

private:
    FUECEventBridgeNativeEvent NativeEvent;
    FUECEventBridgeNativeDestroyed NativeDestroyedEvent;
};
