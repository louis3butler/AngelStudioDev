// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AngelStudioDev : ModuleRules
{
	public AngelStudioDev(ReadOnlyTargetRules Target) : base(Target)
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
			"AngelStudioDev",
			"AngelStudioDev/Variant_Platforming",
			"AngelStudioDev/Variant_Platforming/Animation",
			"AngelStudioDev/Variant_Combat",
			"AngelStudioDev/Variant_Combat/AI",
			"AngelStudioDev/Variant_Combat/Animation",
			"AngelStudioDev/Variant_Combat/Gameplay",
			"AngelStudioDev/Variant_Combat/Interfaces",
			"AngelStudioDev/Variant_Combat/UI",
			"AngelStudioDev/Variant_SideScrolling",
			"AngelStudioDev/Variant_SideScrolling/AI",
			"AngelStudioDev/Variant_SideScrolling/Gameplay",
			"AngelStudioDev/Variant_SideScrolling/Interfaces",
			"AngelStudioDev/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
