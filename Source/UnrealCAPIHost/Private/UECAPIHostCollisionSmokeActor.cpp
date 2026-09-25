#include "UECAPIHostCollisionSmokeActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

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

    AttachmentSocketComponent = CreateDefaultSubobject<UStaticMeshComponent>(
        TEXT("AttachmentSocket"));
    AttachmentSocketComponent->SetupAttachment(collisionBox);
    AttachmentSocketComponent->SetMobility(EComponentMobility::Movable);
    AttachmentSocketComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AttachmentSocketComponent->SetHiddenInGame(true);
    AttachmentSocketComponent->SetCanEverAffectNavigation(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> cubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (cubeMesh.Succeeded()) {
        static const FName socketName(TEXT("UECAPIHostAttachmentSocket"));
        UStaticMesh* mesh = cubeMesh.Object;
        UStaticMeshSocket* socket = mesh->FindSocket(socketName);
        if (socket == nullptr) {
            socket = NewObject<UStaticMeshSocket>(mesh, NAME_None, RF_Transient);
            socket->SocketName = socketName;
            socket->RelativeLocation = FVector(20.0, 0.0, 0.0);
            mesh->AddSocket(socket);
        }
        AttachmentSocketComponent->SetStaticMesh(mesh);
    }
}

void AUECAPIHostCollisionSmokeActor::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AUECAPIHostCollisionSmokeActor, AuthoritySmokeReplicatedValue);
    DOREPLIFETIME(AUECAPIHostCollisionSmokeActor, AuthoritySmokeReplicatedArray);
    DOREPLIFETIME(AUECAPIHostCollisionSmokeActor, AuthoritySmokeReplicatedStruct);
}
