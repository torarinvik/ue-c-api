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
              meta=(Latent, LatentInfo="LatentInfo",
                    WorldContext="WorldContextObject"))
    void WaitForSmokeDuration(UObject* WorldContextObject,
                              float Duration,
                              FLatentActionInfo LatentInfo);

    UFUNCTION(BlueprintCallable, Category="Unreal C API Host Smoke")
    void NoOpSmokeCall();

    UFUNCTION(BlueprintCallable, Category="Unreal C API Host Smoke")
    void ScalarSmokeCall(float Value);

    UFUNCTION(BlueprintCallable, Category="Unreal C API Host Smoke")
    bool ValidateSmokeText(FString Value);

    UFUNCTION(BlueprintCallable, Category="Unreal C API Host Smoke")
    FString EchoSmokeText(FString Value);

    UFUNCTION(BlueprintCallable, Category="Unreal C API Host Smoke")
    void BuildSmokeOutputs(bool& OutFlag, int32& OutNumber, FString& OutText);

    UFUNCTION(BlueprintCallable, Category="Unreal C API Host Smoke",
              meta=(WorldContext="WorldContextObject"))
    void WorldContextSmokeCall(UObject* WorldContextObject);
};
