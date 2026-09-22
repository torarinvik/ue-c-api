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
    float Duration,
    FLatentActionInfo LatentInfo)
{
    UWorld* world = GetWorld();
    if (world == nullptr || LatentInfo.CallbackTarget == nullptr) return;

    FLatentActionManager& manager = world->GetLatentActionManager();
    if (manager.FindExistingAction<FPendingLatentAction>(LatentInfo.CallbackTarget.Get(),
                                                        LatentInfo.UUID) != nullptr) {
        return;
    }
    manager.AddNewAction(LatentInfo.CallbackTarget.Get(), LatentInfo.UUID,
                         new FUECAPIHostSmokeDelayAction(Duration, LatentInfo));
}
