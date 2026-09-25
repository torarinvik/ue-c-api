#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UECAPIHostInputSmokeActor.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS(NotBlueprintable)
class AUECAPIHostInputSmokeActor final : public AActor
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> SmokeAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> SmokeBooleanAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> SmokeAxis2DAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> SmokeAxis3DAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> SmokeHoldAction;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> SmokeMappingContext;
};
