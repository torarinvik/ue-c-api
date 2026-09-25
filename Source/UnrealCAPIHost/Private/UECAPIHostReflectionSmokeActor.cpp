#include "UECAPIHostReflectionSmokeActor.h"

AUECAPIHostReflectionSmokeActor::AUECAPIHostReflectionSmokeActor()
    : Numbers{3, 5}, Position(1.0, 2.0, 3.0)
{
    Counts.Add(7, 70);
    Counts.Add(11, 110);
    Values.Add(13);
    Values.Add(17);
}
