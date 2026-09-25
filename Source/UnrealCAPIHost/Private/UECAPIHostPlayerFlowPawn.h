#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "UECAPIHostPlayerFlowPawn.generated.h"

class UCameraComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(NotBlueprintable)
class AUECAPIHostPlayerFlowPawn final : public APawn
{
    GENERATED_BODY()

public:
    AUECAPIHostPlayerFlowPawn();

    UPROPERTY()
    int32 CApiMarker = 42;

    UPROPERTY()
    TObjectPtr<UCameraComponent> FlowCamera;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> FlowMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CookedTestMesh;
};
