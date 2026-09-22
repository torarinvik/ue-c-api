using UnrealBuildTool;

public class UnrealCAPIHost : ModuleRules
{
    public UnrealCAPIHost(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.Add("Core");
        PrivateDependencyModuleNames.Add("UnrealCAPI");
    }
}
