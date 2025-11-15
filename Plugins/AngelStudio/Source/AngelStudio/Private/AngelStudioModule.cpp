#include "AngelStudioModule.h"
#include "ToolMenus.h"
#include "LevelEditor.h"
#include "AngelRigWizard.h"
#include "AngelRigTemplate.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"

IMPLEMENT_MODULE(FAngelStudioModule, AngelStudio)

void FAngelStudioModule::StartupModule()
{
	EnsureDefaultTemplates();

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAngelStudioModule::RegisterMenus)
	);
}

void FAngelStudioModule::ShutdownModule()
{
	UToolMenus::UnregisterOwner(this);
}

void FAngelStudioModule::RegisterMenus()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	if (!Menu) return;

	FToolMenuSection& Section = Menu->AddSection("AngelStudioSection", FText::FromString("Angel Studio"));

	Section.AddMenuEntry(
		"OpenAngelRigWizard",
		FText::FromString("Angel Rig Wizard"),
		FText::FromString("Open the AngelStudio rigging wizard."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FAngelRigWizard::OpenWindow))
	);
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
