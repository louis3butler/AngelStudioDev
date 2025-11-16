#include "AngelStudioModule.h"
#include "ToolMenus.h"
#include "LevelEditor.h"
#include "AngelRigWizard.h"
#include "AngelRigTemplate.h"
#include "AngelStudioEditorModeCommands.h"
#include "AngelAssetActionUtility.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"

IMPLEMENT_MODULE(FAngelStudioModule, AngelStudio)

void FAngelStudioModule::StartupModule()
{
	// Register editor mode commands used by the mode/tool palette before any mode/tool code runs
	FAngelStudioEditorModeCommands::Register();

	EnsureDefaultTemplates();

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAngelStudioModule::RegisterMenus)
	);
}

void FAngelStudioModule::ShutdownModule()
{
	UToolMenus::UnregisterOwner(this);
	FAngelStudioEditorModeCommands::Unregister();
}

void FAngelStudioModule::RegisterMenus()
{
	// Main menu Window > Angel Studio
	if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window"))
	{
		FToolMenuSection& Section = Menu->AddSection("AngelStudioSection", FText::FromString("Angel Studio"));
		Section.AddMenuEntry(
			"OpenAngelRigWizard",
			FText::FromString("Angel Rig Wizard"),
			FText::FromString("Open the AngelStudio rigging wizard."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&FAngelRigWizard::OpenWindow))
		);
	}

	// Main menu Tools > Angel Studio
	if (UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools"))
	{
		FToolMenuSection& Section = ToolsMenu->AddSection("AngelStudioTools", FText::FromString("Angel Studio"));
		Section.AddMenuEntry(
			"Angel_OpenRigWizard",
			FText::FromString("Open Rig Wizard"),
			FText::FromString("Open the AngelStudio rigging wizard."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&FAngelRigWizard::OpenWindow))
		);
		Section.AddMenuEntry(
			"Angel_AutoRigSelectedMeshes",
			FText::FromString("Auto Rig Selected Meshes"),
			FText::FromString("Run the batch auto-rigging on selected StaticMeshes."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
#if WITH_EDITOR
				if (UAngelAssetActionUtility* Util = NewObject<UAngelAssetActionUtility>())
				{
					Util->AutoRigSelectedMeshes();
				}
#endif
			}))
		);
	}

	// Toolbar button
	if (UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar"))
	{
		FToolMenuSection& Section = Toolbar->AddSection("AngelStudioToolbar", FText::FromString("Angel Studio"));
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(
			"AngelRigWizard_Toolbar",
			FUIAction(FExecuteAction::CreateStatic(&FAngelRigWizard::OpenWindow)),
			FText::FromString("Angel Studio"),
			FText::FromString("Open Angel Rig Wizard"),
			FSlateIcon()
		));
	}
}

void FAngelStudioModule::EnsureDefaultTemplates()
{
	static const FString HumanoidName = TEXT("HumanoidDefaultTemplate");

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> FoundAssets;
	AssetRegistryModule.Get().GetAssetsByClass(UAngelRigTemplate::StaticClass()->GetClassPathName(), FoundAssets);
	for (const FAssetData& Data : FoundAssets)
	{
		if (Data.AssetName.ToString() == HumanoidName)
		{
			return; // already exists
		}
	}

	UAngelRigTemplate* TempTemplate = NewObject<UAngelRigTemplate>(GetTransientPackage(), UAngelRigTemplate::StaticClass(), *HumanoidName);
	if (TempTemplate)
	{
		TempTemplate->TemplateName = FName(HumanoidName);
		TempTemplate->LandmarkDefinitions = {
			{ FName("PelvisCenter"), EAngelLandmarkType::Pelvis, EAngelLandmarkDetectionHint::CenterMass },
			{ FName("SpineStart"), EAngelLandmarkType::SpineStart, EAngelLandmarkDetectionHint::UpperBody },
			{ FName("SpineEnd"), EAngelLandmarkType::SpineEnd, EAngelLandmarkDetectionHint::UpperBody },
			{ FName("NeckBase"), EAngelLandmarkType::NeckBase, EAngelLandmarkDetectionHint::UpperBody },
			{ FName("HeadCenter"), EAngelLandmarkType::HeadCenter, EAngelLandmarkDetectionHint::UpperBody },
			{ FName("ShoulderL"), EAngelLandmarkType::ShoulderL, EAngelLandmarkDetectionHint::SymmetricPair },
			{ FName("ShoulderR"), EAngelLandmarkType::ShoulderR, EAngelLandmarkDetectionHint::SymmetricPair },
			{ FName("HipL"), EAngelLandmarkType::HipL, EAngelLandmarkDetectionHint::SymmetricPair },
			{ FName("HipR"), EAngelLandmarkType::HipR, EAngelLandmarkDetectionHint::SymmetricPair },
			{ FName("KneeL"), EAngelLandmarkType::KneeL, EAngelLandmarkDetectionHint::LowerBody },
			{ FName("KneeR"), EAngelLandmarkType::KneeR, EAngelLandmarkDetectionHint::LowerBody },
			{ FName("AnkleL"), EAngelLandmarkType::AnkleL, EAngelLandmarkDetectionHint::LowerBody },
			{ FName("AnkleR"), EAngelLandmarkType::AnkleR, EAngelLandmarkDetectionHint::LowerBody }
		};
		TempTemplate->BoneDefinitions = {
			{ FName("root"), NAME_None, { FName("PelvisCenter") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("pelvis"), FName("root"), { FName("PelvisCenter") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("spine_01"), FName("pelvis"), { FName("SpineStart") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("spine_02"), FName("spine_01"), { FName("SpineEnd") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("neck"), FName("spine_02"), { FName("NeckBase") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("head"), FName("neck"), { FName("HeadCenter") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("clavicle_l"), FName("spine_02"), { FName("ShoulderL") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("clavicle_r"), FName("spine_02"), { FName("ShoulderR") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("thigh_l"), FName("pelvis"), { FName("HipL") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("thigh_r"), FName("pelvis"), { FName("HipR") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("calf_l"), FName("thigh_l"), { FName("KneeL") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("calf_r"), FName("thigh_r"), { FName("KneeR") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("foot_l"), FName("calf_l"), { FName("AnkleL") }, EAngelBonePlacementMethod::AtLandmark, 1 },
			{ FName("foot_r"), FName("calf_r"), { FName("AnkleR") }, EAngelBonePlacementMethod::AtLandmark, 1 }
		};
	}
}
