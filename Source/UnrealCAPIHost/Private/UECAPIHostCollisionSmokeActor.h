#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UECAPIHostCollisionSmokeActor.generated.h"

USTRUCT()
struct FUECAPIHostAuthoritySmokeStruct
{
    GENERATED_BODY()

    UPROPERTY()
    int32 IntegerValue = 41;
};

UCLASS(NotBlueprintable)
class AUECAPIHostCollisionSmokeActor final : public AActor
{
    GENERATED_BODY()

public:
    AUECAPIHostCollisionSmokeActor();

    UPROPERTY(Replicated)
    int32 AuthoritySmokeReplicatedValue = 23;

    UPROPERTY(Replicated)
    TArray<int32> AuthoritySmokeReplicatedArray{ 31 };

    UPROPERTY(Replicated)
    FUECAPIHostAuthoritySmokeStruct AuthoritySmokeReplicatedStruct;

    UPROPERTY()
    TMap<int32, int32> AuthoritySmokeNetMap;

    UPROPERTY()
    TSet<int32> AuthoritySmokeNetSet;

    UPROPERTY()
    int32 AuthoritySmokeLocalValue = 17;

protected:
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
