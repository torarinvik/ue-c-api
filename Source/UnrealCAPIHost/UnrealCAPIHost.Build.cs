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

        // The packaged macOS host links Unreal's TBB runtime, but installed-engine
        // receipts do not stage these dylibs with this project target.
        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string tbbLibraryDirectory = Path.Combine(
                EngineDirectory,
                "Source",
                "ThirdParty",
                "Intel",
                "TBB",
                "Deploy",
                "oneTBB-2022.3.0",
                "Mac",
                "lib");

            foreach (string libraryName in new[] { "libtbb.12.dylib", "libtbbmalloc.2.dylib" })
            {
                RuntimeDependencies.Add(
                    Path.Combine("$(TargetOutputDir)", libraryName),
                    Path.Combine(tbbLibraryDirectory, libraryName),
                    StagedFileType.NonUFS);
            }
        }
    }
}
