#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UECVersionedDataSaveGame.generated.h"

UCLASS()
class UECVersionedDataSaveGame final : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    uint32 SchemaVersion = 0;

    UPROPERTY(SaveGame)
    TArray<uint8> Payload;
};
