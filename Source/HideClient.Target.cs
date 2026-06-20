using UnrealBuildTool;
using System.Collections.Generic;

public class HideClientTarget : TargetRules
{
    public HideClientTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Client;
        DefaultBuildSettings = BuildSettingsVersion.V5;

        ExtraModuleNames.AddRange(new string[] { "Hide" });
    }
}
