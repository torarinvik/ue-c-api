using UnrealBuildTool;

public class UnrealCAPIHostTarget : TargetRules
{
    public UnrealCAPIHostTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("UnrealCAPIHost");
    }
}
