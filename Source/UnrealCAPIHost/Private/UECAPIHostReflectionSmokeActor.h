#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UECAPIHostReflectionSmokeActor.generated.h"

UCLASS(NotBlueprintable)
class AUECAPIHostReflectionSmokeActor final : public AActor
{
    GENERATED_BODY()

public:
    AUECAPIHostReflectionSmokeActor();

    UPROPERTY()
    TArray<int32> Numbers;

    UPROPERTY()
    TMap<int32, int32> Counts;

    UPROPERTY()
    TSet<int32> Values;

    UPROPERTY()
    FVector Position;
};
