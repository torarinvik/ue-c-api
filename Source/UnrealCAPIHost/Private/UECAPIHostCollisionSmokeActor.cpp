#include "UECAPIHostCollisionSmokeActor.h"

#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

AUECAPIHostCollisionSmokeActor::AUECAPIHostCollisionSmokeActor()
{
    bReplicates = true;
    AuthoritySmokeNetMap.Add(5, 11);
    AuthoritySmokeNetSet.Add(13);
    FProperty* mapProperty = FindFProperty<FProperty>(
        GetClass(), GET_MEMBER_NAME_CHECKED(AUECAPIHostCollisionSmokeActor,
                                            AuthoritySmokeNetMap));
    FProperty* setProperty = FindFProperty<FProperty>(
        GetClass(), GET_MEMBER_NAME_CHECKED(AUECAPIHostCollisionSmokeActor,
                                            AuthoritySmokeNetSet));
    check(mapProperty != nullptr && setProperty != nullptr);
    mapProperty->SetPropertyFlags(CPF_Net);
    setProperty->SetPropertyFlags(CPF_Net);
    UBoxComponent* collisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    SetRootComponent(collisionBox);
    collisionBox->SetMobility(EComponentMobility::Movable);
    collisionBox->SetBoxExtent(FVector(50.0));
    collisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    collisionBox->SetCollisionObjectType(ECC_WorldDynamic);
    collisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    collisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void AUECAPIHostCollisionSmokeActor::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AUECAPIHostCollisionSmokeActor, AuthoritySmokeReplicatedValue);
    DOREPLIFETIME(AUECAPIHostCollisionSmokeActor, AuthoritySmokeReplicatedArray);
    DOREPLIFETIME(AUECAPIHostCollisionSmokeActor, AuthoritySmokeReplicatedStruct);
}
