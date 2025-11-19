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
                "EditorFramework",
                "EditorStyle",
                "Blutility",
                "EditorInteractiveToolsFramework",
                "InteractiveToolsFramework",
                "AssetRegistry",
                "DeveloperSettings",
                "PropertyEditor",
                "ToolMenus"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "InputCore",
                "Projects",
                "ApplicationCore"
            }
        );

        if (Target.bBuildEditor)
        {
            // Control Rig modules for editor-time blueprint creation & hierarchy editing
            PublicDependencyModuleNames.AddRange(new string[]{ "ControlRig", "ControlRigEditor", "ControlRigDeveloper" });
            PrivateDependencyModuleNames.AddRange(new string[]{ "KismetCompiler" });
        }
        else
        {
            PrivateDefinitions.Add("WITH_EDITOR=0");
        }
    }
}
