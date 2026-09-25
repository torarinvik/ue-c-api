#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UECAPIHostCollisionSmokeActor.generated.h"

UCLASS(NotBlueprintable)
class AUECAPIHostCollisionSmokeActor final : public AActor
{
    GENERATED_BODY()

public:
    AUECAPIHostCollisionSmokeActor();

    UPROPERTY(Replicated)
    int32 AuthoritySmokeReplicatedValue = 23;

    UPROPERTY()
    int32 AuthoritySmokeLocalValue = 17;

protected:
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
