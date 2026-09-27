#include "UECAPIHostReflectionSmokeActor.h"

AUECAPIHostReflectionSmokeActor::AUECAPIHostReflectionSmokeActor()
    : Numbers{3, 5}, Position(1.0, 2.0, 3.0),
      Coordinates2D(11.5, -22.25), HomogeneousPoint(1.0, 2.0, 3.0, 4.0),
      Orientation(FQuat::Identity), Rotation(10.0, 20.0, 30.0),
      Tint(0.25f, 0.5f, 0.75f, 1.0f), PackedTint(32, 64, 128, 255),
      GridCell(3, -7), VoxelCell(10, -20, 30),
      StableId(0x01234567u, 0x89ABCDEFu, 0xA0B0C0D0u, 0xFFFFFFFFu),
      RecordedAt(1234567890123456789ll),
      Elapsed(-1234567890123456789ll),
      Pose(FTransform::Identity),
      Enabled(true), Count(7),
      Mode(EUECAPIHostReflectionSmokeMode::First), Ratio(1.25f),
      Label(TEXT("initial label")), Identifier(TEXT("InitialName")),
      Description(FText::FromString(TEXT("initial description"))),
      SoftMesh(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube"))),
      SoftActorClass(FSoftObjectPath(TEXT("/Script/Engine.Actor")))
{
    SelfObject = this;
    VectorPositions.Add(FVector(4.0, -5.5, 6.25));
    VectorPositions.Add(FVector(-7.0, 8.5, 9.0));
    Counts.Add(7, 70);
    Counts.Add(11, 110);
    TypedVectors.Add(42, FVector(12.5, -3.0, 8.25));
    Values.Add(13);
    Values.Add(17);
    TypedIds.Add(FGuid(0x01020304u, 0x11121314u, 0x21222324u, 0x31323334u));
    TypedIds.Add(FGuid(0x41424344u, 0x51525354u, 0x61626364u, 0x71727374u));
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

FIntPoint AUECAPIHostReflectionSmokeActor::EchoIntPoint(FIntPoint value)
{
    return value;
}

FIntVector AUECAPIHostReflectionSmokeActor::EchoIntVector(FIntVector value)
{
    return value;
}

FGuid AUECAPIHostReflectionSmokeActor::EchoGuid(FGuid value)
{
    return value;
}

FDateTime AUECAPIHostReflectionSmokeActor::EchoDateTime(FDateTime value)
{
    return value;
}

FTimespan AUECAPIHostReflectionSmokeActor::EchoTimespan(FTimespan value)
{
    return value;
}

FVector4 AUECAPIHostReflectionSmokeActor::EchoVector2DWithColor(
    FVector2D value,
    FColor& colorOut)
{
    colorOut = FColor(101, 102, 103, 104);
    return FVector4(value.X, value.Y, colorOut.R, colorOut.A);
}
