// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Hide : ModuleRules
{
	public Hide(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 파일을 하나로 묶어 컴파일하지 않는다(파일마다 있는 같은 이름의 지역 도우미 함수가 충돌하지 않게).
        // 모듈이 작아서 빌드 시간 차이는 거의 없다.
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "CoreOnline", // 재접속 식별용 FUniqueNetIdRepl
            "Engine",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "Niagara"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore"
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
