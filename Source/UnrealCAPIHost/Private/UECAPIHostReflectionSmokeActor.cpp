#include "UECAPIHostReflectionSmokeActor.h"

AUECAPIHostReflectionSmokeActor::AUECAPIHostReflectionSmokeActor()
    : Numbers{3, 5}, Position(1.0, 2.0, 3.0), Enabled(true), Count(7),
      Mode(EUECAPIHostReflectionSmokeMode::First), Ratio(1.25f),
      Label(TEXT("initial label")), Identifier(TEXT("InitialName")),
      Description(FText::FromString(TEXT("initial description"))),
      SoftMesh(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube"))),
      SoftActorClass(FSoftObjectPath(TEXT("/Script/Engine.Actor")))
{
    Counts.Add(7, 70);
    Counts.Add(11, 110);
    Values.Add(13);
    Values.Add(17);
}
