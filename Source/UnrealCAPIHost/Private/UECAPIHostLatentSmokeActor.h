#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Engine/LatentActionManager.h"
#include "UECAPIHostLatentSmokeActor.generated.h"

UCLASS(Blueprintable)
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
    FVector VectorSmokeCall(FVector Value);

    UFUNCTION()
    FQuat QuaternionSmokeCall(FQuat Value);

    UFUNCTION(BlueprintCallable, Category="Unreal C API Host Smoke")
    FTransform TransformSmokeCall(FTransform Value);

    UFUNCTION(BlueprintCallable, Category="Unreal C API Host Smoke")
    void BuildSmokeOutputs(bool& OutFlag, int32& OutNumber, FString& OutText);

    UFUNCTION(BlueprintCallable, Category="Unreal C API Host Smoke",
              meta=(WorldContext="WorldContextObject"))
    void WorldContextSmokeCall(UObject* WorldContextObject);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable,
              Category="Unreal C API Host Smoke")
    int32 BlueprintGeneratedSmokeCall(int32 Value);
};

UCLASS()
class UUECAPIHostSaveGame final : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    int32 PersistedCount = 0;

    UPROPERTY(SaveGame)
    bool PersistedEnabled = false;

    UPROPERTY(SaveGame)
    float PersistedRatio = 0.0f;

    UPROPERTY(SaveGame)
    FString PersistedLabel;
};
