#include "UECAPIHostPlayerFlowPawn.h"

#include "Camera/CameraComponent.h"

AUECAPIHostPlayerFlowPawn::AUECAPIHostPlayerFlowPawn()
{
    USceneComponent* root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(root);
    FlowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FlowCamera"));
    FlowCamera->SetupAttachment(root);
    FlowCamera->SetFieldOfView(87.0f);
}
