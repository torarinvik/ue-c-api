#include "UECAPIHostReflectionSmokeActor.h"

AUECAPIHostReflectionSmokeActor::AUECAPIHostReflectionSmokeActor()
    : Numbers{3, 5}, Position(1.0, 2.0, 3.0),
      Coordinates2D(11.5, -22.25), HomogeneousPoint(1.0, 2.0, 3.0, 4.0),
      Orientation(FQuat::Identity), Rotation(10.0, 20.0, 30.0),
      Tint(0.25f, 0.5f, 0.75f, 1.0f), PackedTint(32, 64, 128, 255),
      Pose(FTransform::Identity),
      Enabled(true), Count(7),
      Mode(EUECAPIHostReflectionSmokeMode::First), Ratio(1.25f),
      Label(TEXT("initial label")), Identifier(TEXT("InitialName")),
      Description(FText::FromString(TEXT("initial description"))),
      SoftMesh(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube"))),
      SoftActorClass(FSoftObjectPath(TEXT("/Script/Engine.Actor")))
{
    SelfObject = this;
    Counts.Add(7, 70);
    Counts.Add(11, 110);
    Values.Add(13);
    Values.Add(17);
}

FVector2D AUECAPIHostReflectionSmokeActor::EchoVector2D(FVector2D value)
{
    return value;
}

FVector4 AUECAPIHostReflectionSmokeActor::EchoVector4(FVector4 value)
{
    return value;
}

FRotator AUECAPIHostReflectionSmokeActor::EchoRotator(FRotator value)
{
    return value;
}

FLinearColor AUECAPIHostReflectionSmokeActor::EchoLinearColor(FLinearColor value)
{
    return value;
}

FColor AUECAPIHostReflectionSmokeActor::EchoColor(FColor value)
{
    return value;
}
