using UnrealBuildTool;

public class UnrealCAPIHostServerTarget : TargetRules
{
    public UnrealCAPIHostServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("UnrealCAPIHost");
    }
}
