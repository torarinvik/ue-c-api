using UnrealBuildTool;
using System.IO;

public class UnrealCAPIHost : ModuleRules
{
    public UnrealCAPIHost(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.Add("Core");
        PrivateDependencyModuleNames.AddRange(new[] { "CoreUObject", "Engine", "UnrealCAPI" });
        PrivateIncludePaths.Add(Path.GetFullPath(
            Path.Combine(ModuleDirectory, "../../examples/c_gameplay")));
    }
}
