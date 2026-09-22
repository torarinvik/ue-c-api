#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "uec_api.h"

extern "C" uec_result UEC_CALL uec_host_smoke_bootstrap(void);

DEFINE_LOG_CATEGORY_STATIC(LogUnrealCAPIHost, Log, All);

class FUnrealCAPIHostModule final : public FDefaultGameModuleImpl
{
public:
    void StartupModule() override
    {
        const uec_result result = uec_host_smoke_bootstrap();
        if (result == UEC_RESULT_OK)
        {
            UE_LOG(LogUnrealCAPIHost, Log, TEXT("C consumer bootstrap completed"));
        }
        else
        {
            UE_LOG(LogUnrealCAPIHost, Error,
                TEXT("C consumer bootstrap failed with result %d"), static_cast<int32>(result));
        }
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FUnrealCAPIHostModule, UnrealCAPIHost, "UnrealCAPIHost");
