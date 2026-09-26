#include "UECAPIHostInputSmokeActor.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"

AUECAPIHostInputSmokeActor::AUECAPIHostInputSmokeActor()
{
    USceneComponent* root = CreateDefaultSubobject<USceneComponent>(TEXT("SmokeRoot"));
    SetRootComponent(root);
    SmokeCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SmokeCamera"));
    SmokeCamera->SetupAttachment(root);
}

void AUECAPIHostInputSmokeActor::BeginPlay()
{
    Super::BeginPlay();

    SmokeBooleanAction = NewObject<UInputAction>(
        this, TEXT("UECSmokeBooleanAction"), RF_Transient);
    if (SmokeBooleanAction != nullptr) {
        SmokeBooleanAction->ValueType = EInputActionValueType::Boolean;
    }
    SmokeAction = NewObject<UInputAction>(this, TEXT("UECSmokeInputAction"), RF_Transient);
    if (SmokeAction != nullptr) {
        SmokeAction->ValueType = EInputActionValueType::Axis1D;
    }
    SmokeAxis2DAction = NewObject<UInputAction>(
        this, TEXT("UECSmokeAxis2DAction"), RF_Transient);
    if (SmokeAxis2DAction != nullptr) {
        SmokeAxis2DAction->ValueType = EInputActionValueType::Axis2D;
    }
    SmokeAxis3DAction = NewObject<UInputAction>(
        this, TEXT("UECSmokeAxis3DAction"), RF_Transient);
    if (SmokeAxis3DAction != nullptr) {
        SmokeAxis3DAction->ValueType = EInputActionValueType::Axis3D;
    }
    SmokeHoldAction = NewObject<UInputAction>(
        this, TEXT("UECSmokeHoldAction"), RF_Transient);
    if (SmokeHoldAction != nullptr) {
        SmokeHoldAction->ValueType = EInputActionValueType::Axis1D;
        UInputTriggerHold* holdTrigger = NewObject<UInputTriggerHold>(SmokeHoldAction);
        if (holdTrigger != nullptr) {
            holdTrigger->HoldTimeThreshold = 60.0f;
            SmokeHoldAction->Triggers.Add(holdTrigger);
        }
    }
    SmokeMappingContext = NewObject<UInputMappingContext>(
        this, TEXT("UECSmokeInputMappingContext"), RF_Transient);
    if (SmokeMappingContext != nullptr && SmokeAction != nullptr) {
        SmokeMappingContext->MapKey(SmokeAction, EKeys::Gamepad_LeftX);
    }

    if (UWorld* world = GetWorld()) {
        if (APlayerController* controller = world->GetFirstPlayerController()) {
            EnableInput(controller);
        }
    }
}
