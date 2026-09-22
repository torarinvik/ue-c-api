using UnrealBuildTool;

public class UnrealCAPI : ModuleRules
{
    public UnrealCAPI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Projects"
        });

        // Keep the first ABI slice usable by packaged runtime builds. Editor-only
        // adapters belong in a separate module added in a later phase.
        bEnableExceptions = false;
    }
}
