#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/LatentActionManager.h"
#include "UECAPIHostLatentSmokeActor.generated.h"

UCLASS(NotBlueprintable)
class AUECAPIHostLatentSmokeActor final : public AActor
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Unreal C API Host Smoke",
              meta=(Latent, LatentInfo="LatentInfo"))
    void WaitForSmokeDuration(float Duration, FLatentActionInfo LatentInfo);
};
