#include "UECAPIHostPlayerFlowPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
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
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> materialAsset(
        TEXT("/Engine/EngineMaterials/Widget3DPassThrough.Widget3DPassThrough"));
    if (materialAsset.Succeeded()) {
        CookedTestMaterial = materialAsset.Object;
        FlowMesh->SetMaterial(0, CookedTestMaterial);
    }
    static ConstructorHelpers::FObjectFinder<UStaticMesh> meshAsset(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (meshAsset.Succeeded()) CookedTestMesh = meshAsset.Object;
    FlowSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(
        TEXT("FlowSkeletalMesh"));
    FlowSkeletalMesh->SetupAttachment(root);
    FlowSkeletalMesh->SetMobility(EComponentMobility::Movable);
    if (CookedTestMaterial != nullptr)
        FlowSkeletalMesh->SetMaterial(0, CookedTestMaterial);
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> skeletalMeshAsset(
        TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube"));
    if (skeletalMeshAsset.Succeeded()) CookedTestSkeletalMesh = skeletalMeshAsset.Object;
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> animationMeshAsset(
        TEXT("/Engine/Tutorial/SubEditors/TutorialAssets/Character/TutorialTPP.TutorialTPP"));
    if (animationMeshAsset.Succeeded()) CookedAnimationTestMesh = animationMeshAsset.Object;
    static ConstructorHelpers::FObjectFinder<UAnimSequence> animationAsset(
        TEXT("/Engine/Tutorial/SubEditors/TutorialAssets/Character/Tutorial_Walk_Fwd.Tutorial_Walk_Fwd"));
    if (animationAsset.Succeeded()) CookedTestAnimation = animationAsset.Object;
}
