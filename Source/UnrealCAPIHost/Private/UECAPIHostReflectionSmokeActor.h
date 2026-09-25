#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "UECAPIHostReflectionSmokeActor.generated.h"

UENUM()
enum class EUECAPIHostReflectionSmokeMode : uint8
{
    First,
    Second
};

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

    UPROPERTY()
    bool Enabled;

    UPROPERTY()
    int32 Count;

    UPROPERTY()
    EUECAPIHostReflectionSmokeMode Mode;

    UPROPERTY()
    float Ratio;

    UPROPERTY()
    FString Label;

    UPROPERTY()
    FName Identifier;

    UPROPERTY()
    FText Description;

    UPROPERTY()
    TSoftObjectPtr<UStaticMesh> SoftMesh;

    UPROPERTY()
    TSoftClassPtr<AActor> SoftActorClass;
};
