using UnrealBuildTool;

public class AngelStudio : ModuleRules
{
    public AngelStudio(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "AngelStudioRuntime",
                "Slate",
                "SlateCore",
                "EditorSubsystem",
                "UnrealEd",
                "LevelEditor",
                "AssetTools",
                "ContentBrowser",
                "Kismet",
                "ControlRig",
                "ControlRigEditor",
                "EditorFramework",
                "EditorStyle",
                "Blutility",
                "EditorInteractiveToolsFramework",
                "InteractiveToolsFramework",
                "AssetRegistry" // for locating rig template assets
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "InputCore",
                "Projects",
                "ApplicationCore",
                "ToolMenus"
            }
        );

        if (Target.bBuildEditor == false)
        {
            PrivateDefinitions.Add("WITH_EDITOR=0");
        }
    }
}
