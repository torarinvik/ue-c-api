#include "UECAPIHostInputSmokeActor.h"

#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"

void AUECAPIHostInputSmokeActor::BeginPlay()
{
    Super::BeginPlay();

    SmokeAction = NewObject<UInputAction>(this, TEXT("UECSmokeInputAction"), RF_Transient);
    if (SmokeAction != nullptr) {
        SmokeAction->ValueType = EInputActionValueType::Axis1D;
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
