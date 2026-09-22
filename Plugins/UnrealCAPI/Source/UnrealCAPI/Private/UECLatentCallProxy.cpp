#include "UECLatentCallProxy.h"

void UECLatentCallProxy::OnLatentActionCompleted()
{
    OnCompleted.Broadcast();
}
