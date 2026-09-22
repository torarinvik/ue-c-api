#include "UECAPIHostLatentSmokeActor.h"

#include "LatentActions.h"

namespace
{
    class FUECAPIHostSmokeDelayAction final : public FPendingLatentAction
    {
    public:
        FUECAPIHostSmokeDelayAction(float duration, const FLatentActionInfo& latentInfo)
            : Remaining(FMath::Max(0.0f, duration)),
              ExecutionFunction(latentInfo.ExecutionFunction),
              Linkage(latentInfo.Linkage),
              CallbackTarget(latentInfo.CallbackTarget.Get())
        {
        }

        void UpdateOperation(FLatentResponse& response) override
        {
            Remaining -= response.ElapsedTime();
            response.FinishAndTriggerIf(Remaining <= 0.0f,
                                        ExecutionFunction,
                                        Linkage,
                                        CallbackTarget);
        }

    private:
        float Remaining;
        FName ExecutionFunction;
        int32 Linkage;
        FWeakObjectPtr CallbackTarget;
    };
}

void AUECAPIHostLatentSmokeActor::WaitForSmokeDuration(
    UObject* WorldContextObject,
    float Duration,
    FLatentActionInfo LatentInfo)
{
    UWorld* world = GetWorld();
    if (world == nullptr || WorldContextObject != world ||
        LatentInfo.CallbackTarget == nullptr) return;

    FLatentActionManager& manager = world->GetLatentActionManager();
    if (manager.FindExistingAction<FPendingLatentAction>(LatentInfo.CallbackTarget.Get(),
                                                        LatentInfo.UUID) != nullptr) {
        return;
    }
    manager.AddNewAction(LatentInfo.CallbackTarget.Get(), LatentInfo.UUID,
                         new FUECAPIHostSmokeDelayAction(Duration, LatentInfo));
}

void AUECAPIHostLatentSmokeActor::NoOpSmokeCall()
{
}

void AUECAPIHostLatentSmokeActor::ScalarSmokeCall(float Value)
{
    (void)Value;
}

bool AUECAPIHostLatentSmokeActor::ValidateSmokeText(FString Value)
{
    return Value == TEXT("mixed-smoke");
}

FString AUECAPIHostLatentSmokeActor::EchoSmokeText(FString Value)
{
    return Value;
}

FVector AUECAPIHostLatentSmokeActor::VectorSmokeCall(FVector Value)
{
    return Value;
}

FQuat AUECAPIHostLatentSmokeActor::QuaternionSmokeCall(FQuat Value)
{
    return Value;
}

FTransform AUECAPIHostLatentSmokeActor::TransformSmokeCall(FTransform Value)
{
    return Value;
}

void AUECAPIHostLatentSmokeActor::BuildSmokeOutputs(
    bool& OutFlag,
    int32& OutNumber,
    FString& OutText)
{
    OutFlag = true;
    OutNumber = 42;
    OutText = TEXT("output-smoke");
}

void AUECAPIHostLatentSmokeActor::WorldContextSmokeCall(UObject* WorldContextObject)
{
    (void)WorldContextObject;
}
