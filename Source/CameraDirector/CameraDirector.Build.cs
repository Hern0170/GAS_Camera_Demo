// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CameraDirector : ModuleRules
{
	public CameraDirector(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"CameraDirector",
			"CameraDirector/Variant_Platforming",
			"CameraDirector/Variant_Platforming/Animation",
			"CameraDirector/Variant_Combat",
			"CameraDirector/Variant_Combat/AI",
			"CameraDirector/Variant_Combat/Animation",
			"CameraDirector/Variant_Combat/Gameplay",
			"CameraDirector/Variant_Combat/Interfaces",
			"CameraDirector/Variant_Combat/UI",
			"CameraDirector/Variant_SideScrolling",
			"CameraDirector/Variant_SideScrolling/AI",
			"CameraDirector/Variant_SideScrolling/Gameplay",
			"CameraDirector/Variant_SideScrolling/Interfaces",
			"CameraDirector/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
