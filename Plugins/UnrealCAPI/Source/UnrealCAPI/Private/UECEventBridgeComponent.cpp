#include "UECEventBridgeComponent.h"

UECEventBridgeComponent::UECEventBridgeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UECEventBridgeComponent::EmitEvent(int64 EventId,
                                        int64 IntegerValue,
                                        double RealValue,
                                        const FString& TextValue)
{
    NativeEvent.Broadcast(EventId, IntegerValue, RealValue, TextValue);
    if (!IsValid(this)) return;
    OnEvent.Broadcast(EventId, IntegerValue, RealValue, TextValue);
}

void UECEventBridgeComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    NativeDestroyedEvent.Broadcast(this);
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}
