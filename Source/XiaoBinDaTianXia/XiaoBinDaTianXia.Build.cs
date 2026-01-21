// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class XiaoBinDaTianXia : ModuleRules
{
	public XiaoBinDaTianXia(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"Niagara",
			
			// GAS 相关
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			
			// AI 相关
			"AIModule",
			"NavigationSystem",
			
			// 渲染相关
			"RenderCore",
			"RHI",
			
			// 项目设置
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"SlateCore"
		});

		PublicIncludePaths.AddRange(new string[] {
			"XiaoBinDaTianXia",
			"XiaoBinDaTianXia/Variant_Platforming",
			"XiaoBinDaTianXia/Variant_Platforming/Animation",
			"XiaoBinDaTianXia/Variant_Combat",
			"XiaoBinDaTianXia/Variant_Combat/AI",
			"XiaoBinDaTianXia/Variant_Combat/Animation",
			"XiaoBinDaTianXia/Variant_Combat/Gameplay",
			"XiaoBinDaTianXia/Variant_Combat/Interfaces",
			"XiaoBinDaTianXia/Variant_Combat/UI",
			"XiaoBinDaTianXia/Variant_SideScrolling",
			"XiaoBinDaTianXia/Variant_SideScrolling/AI",
			"XiaoBinDaTianXia/Variant_SideScrolling/Gameplay",
			"XiaoBinDaTianXia/Variant_SideScrolling/Interfaces",
			"XiaoBinDaTianXia/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
