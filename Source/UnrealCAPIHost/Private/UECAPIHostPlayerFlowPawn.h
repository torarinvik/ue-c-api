#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "UECAPIHostPlayerFlowPawn.generated.h"

class UCameraComponent;
class UAudioComponent;
class UAnimSequence;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class USoundBase;

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
    TObjectPtr<UAudioComponent> FlowAudio;

    UPROPERTY()
    TObjectPtr<UAudioComponent> FlowAudioForDestroySmoke;

    UPROPERTY()
    TObjectPtr<USoundBase> CookedTestSound;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> FlowMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CookedTestMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> CookedTestMaterial;

    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> FlowSkeletalMesh;

    UPROPERTY()
    TObjectPtr<USkeletalMesh> CookedTestSkeletalMesh;

    UPROPERTY()
    TObjectPtr<USkeletalMesh> CookedAnimationTestMesh;

    UPROPERTY()
    TObjectPtr<UAnimSequence> CookedTestAnimation;
};
