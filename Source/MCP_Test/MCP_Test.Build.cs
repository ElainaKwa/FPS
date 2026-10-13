// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MCP_Test : ModuleRules
{
	public MCP_Test(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayAbilities", "GameplayTags", "GameplayTasks" });

		PublicDependencyModuleNames.Add("UMG");
		PublicDependencyModuleNames.Add("DeveloperSettings");
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "AIModule", "AudioMixer", "AssetRegistry", "NetCore", "Json" });

		if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "UMGEditor", "KismetCompiler", "PropertyEditor", "AssetTools", "ToolMenus" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
