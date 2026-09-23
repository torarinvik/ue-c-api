using UnrealBuildTool;

public class UnrealCAPIHostEditorTarget : TargetRules
{
    public UnrealCAPIHostEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("UnrealCAPIHost");
    }
}
