using UnrealBuildTool;
using System.Collections.Generic;

public class HideServerTarget : TargetRules
{
    public HideServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V5;

        ExtraModuleNames.AddRange(new string[] { "Hide" });
    }
}
