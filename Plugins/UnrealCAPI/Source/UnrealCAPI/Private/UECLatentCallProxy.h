#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UECLatentCallProxy.generated.h"

UCLASS()
class UNREALCAPI_API UECLatentCallProxy final : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION()
    void OnLatentActionCompleted();

    FSimpleMulticastDelegate OnCompleted;
};
