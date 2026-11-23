using UnrealBuildTool;
public class StillAwake : ModuleRules
{
    public StillAwake(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","InputCore","Json","JsonUtilities"});
    }
}
