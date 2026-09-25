#include "UECAPIHostPlayerFlowPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AUECAPIHostPlayerFlowPawn::AUECAPIHostPlayerFlowPawn()
{
    USceneComponent* root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(root);
    FlowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FlowCamera"));
    FlowCamera->SetupAttachment(root);
    FlowCamera->SetFieldOfView(87.0f);
    FlowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlowMesh"));
    FlowMesh->SetupAttachment(root);
    FlowMesh->SetMobility(EComponentMobility::Movable);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> meshAsset(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (meshAsset.Succeeded()) CookedTestMesh = meshAsset.Object;
}
