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
    TObjectPtr<UInputMappingContext> SmokeMappingContext;
};
