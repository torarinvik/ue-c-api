#include "Modules/ModuleManager.h"

class FUnrealCAPIHostModule final : public FDefaultGameModuleImpl
{
};

IMPLEMENT_PRIMARY_GAME_MODULE(FUnrealCAPIHostModule, UnrealCAPIHost, "UnrealCAPIHost");
