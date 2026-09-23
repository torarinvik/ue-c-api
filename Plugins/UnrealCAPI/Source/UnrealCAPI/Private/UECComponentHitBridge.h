#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "UECComponentHitBridge.generated.h"

DECLARE_MULTICAST_DELEGATE_FiveParams(FUECComponentHitNativeEvent,
    UPrimitiveComponent*, AActor*, UPrimitiveComponent*, FVector, const FHitResult&);

UCLASS()
class UNREALCAPI_API UECComponentHitBridge final : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION()
    void HandleComponentHit(UPrimitiveComponent* hitComponent,
                            AActor* otherActor,
                            UPrimitiveComponent* otherComponent,
                            FVector normalImpulse,
                            const FHitResult& hit)
    {
        NativeHit.Broadcast(hitComponent, otherActor, otherComponent, normalImpulse, hit);
    }

    FUECComponentHitNativeEvent& GetNativeHitEvent()
    {
        return NativeHit;
    }

private:
    FUECComponentHitNativeEvent NativeHit;
};
