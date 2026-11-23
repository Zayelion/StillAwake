using UnrealBuildTool;
public class StillAwakeTarget : TargetRules
{
    public StillAwakeTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("StillAwake");
    }
}
