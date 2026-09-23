#include "UECAPIHostCollisionSmokeActor.h"

#include "Components/BoxComponent.h"

AUECAPIHostCollisionSmokeActor::AUECAPIHostCollisionSmokeActor()
{
    UBoxComponent* collisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    SetRootComponent(collisionBox);
    collisionBox->SetBoxExtent(FVector(50.0));
    collisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    collisionBox->SetCollisionObjectType(ECC_WorldDynamic);
    collisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    collisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}
