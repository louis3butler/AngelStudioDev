#include "AngelRigWizard.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "AngelRigTemplate.h"
#include "AngelLandmarkDetector.h"
#include "AngelSkeletonBuilder.h"
#include "AngelSkeletalMeshBuilder.h"
#include "AngelControlRigGenerator.h"
#include "AngelGeneratedRigData.h"
#include "Engine/StaticMesh.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Selection.h"
#include "DrawDebugHelpers.h"
#include "Misc/PackageName.h"
#include "ObjectTools.h"

class SAngelRigWizardWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAngelRigWizardWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		RefreshTemplates();

		ChildSlot
		[
			SNew(SBorder)
			.Padding(8.f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Angel Studio Rig Wizard")))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.f)
				[
					SAssignNew(StatusText, STextBlock)
					.Text(FText::FromString(TEXT("Idle")))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.f)
				[
					SAssignNew(TemplateText, STextBlock)
					.Text(FText::FromString(CurrentTemplate ? CurrentTemplate->TemplateName.ToString() : TEXT("No Template")))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Detect Landmarks")))
					.OnClicked(this, &SAngelRigWizardWidget::OnDetect)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Generate Rig Assets")))
					.OnClicked(this, &SAngelRigWizardWidget::OnGenerateAssets)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Clear Debug Landmarks")))
					.OnClicked(this, &SAngelRigWizardWidget::OnClearDebug)
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				.Padding(4.f)
				[
					SAssignNew(LandmarkList, SVerticalBox)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Close")))
					.OnClicked_Lambda([]()
					{
						if (TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow())
						{
							ActiveWindow->RequestDestroyWindow();
						}
						return FReply::Handled();
					})
				]
			]
		];
	}

private:
	TArray<TWeakObjectPtr<UAngelRigTemplate>> Templates;
	UAngelRigTemplate* CurrentTemplate = nullptr;
	TSharedPtr<STextBlock> TemplateText;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SVerticalBox> LandmarkList;
	TArray<FVector> DebugPoints;
	FAngelLandmarkSolveResult CachedLandmarks;
	UStaticMesh* CachedMesh = nullptr;

	void SetStatus(const FString& In)
	{
		if (StatusText.IsValid())
		{
			StatusText->SetText(FText::FromString(In));
		}
	}

	void RefreshTemplates()
	{
		Templates.Reset();

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> FoundAssets;
		AssetRegistryModule.Get().GetAssetsByClass(UAngelRigTemplate::StaticClass()->GetClassPathName(), FoundAssets);
		for (const FAssetData& Data : FoundAssets)
		{
			if (UAngelRigTemplate* TemplateObj = Cast<UAngelRigTemplate>(Data.GetAsset()))
			{
				Templates.Add(TemplateObj);
			}
		}

		if (Templates.Num() == 0)
		{
			for (TObjectIterator<UAngelRigTemplate> It; It; ++It)
			{
				Templates.Add(*It);
			}
		}

		CurrentTemplate = Templates.Num() > 0 ? Templates[0].Get() : nullptr;
	}

	UStaticMesh* GetSelectedMesh() const
	{
		if (GEditor && GEditor->GetSelectedObjects())
		{
			USelection* Sel = GEditor->GetSelectedObjects();
			for (int32 i = 0; i < Sel->Num(); ++i)
			{
				if (UStaticMesh* Mesh = Cast<UStaticMesh>(Sel->GetSelectedObject(i)))
				{
					return Mesh;
				}
			}
		}
		return nullptr;
	}

	FReply OnDetect()
	{
#if WITH_EDITOR
		LandmarkList->ClearChildren();
		CachedMesh = GetSelectedMesh();
		if (!CachedMesh || !CurrentTemplate)
		{
			SetStatus("No mesh or template.");
			return FReply::Handled();
		}

		UAngelLandmarkDetector* Detector = NewObject<UAngelLandmarkDetector>();
		CachedLandmarks = Detector->DetectLandmarks(CachedMesh, CurrentTemplate);
		if (!CachedLandmarks.bSuccess)
		{
			SetStatus("Detection failed.");
			return FReply::Handled();
		}

		SetStatus("Landmarks detected.");

		DebugPoints.Reset();
		UWorld* World = (GEditor) ? GEditor->GetEditorWorldContext().World() : nullptr;

		for (const FAngelSolvedLandmark& LM : CachedLandmarks.Landmarks)
		{
			const FVector Pos = LM.Transform.GetLocation();
			FString Line = FString::Printf(TEXT("%s : (%.1f, %.1f, %.1f)"), *LM.Name.ToString(), Pos.X, Pos.Y, Pos.Z);
			LandmarkList->AddSlot()
			.AutoHeight()
			.Padding(2.f)
			[
				SNew(STextBlock).Text(FText::FromString(Line))
			];

			DebugPoints.Add(Pos);
			if (World)
			{
				DrawDebugPoint(World, Pos, 10.f, FColor::Cyan, false, 10.f);
			}
		}
#endif
		return FReply::Handled();
	}

	FString MakeUniqueAssetPath(const FString& BaseFolder, const FString& AssetName) const
	{
		FString Path = BaseFolder + TEXT("/") + AssetName;
		FString PackageName = Path;
		int32 Suffix = 1;
		while (FPackageName::DoesPackageExist(PackageName))
		{
			PackageName = Path + FString::Printf(TEXT("_%d"), Suffix++);
		}
		return PackageName;
	}

	FReply OnGenerateAssets()
	{
#if WITH_EDITOR
		if (!CachedMesh || !CachedLandmarks.bSuccess || !CurrentTemplate)
		{
			SetStatus("Detect landmarks first.");
			return FReply::Handled();
		}

		const FString BaseFolder = TEXT("/Game/AngelStudio/Generated");
		FString SkeletonPkgName = MakeUniqueAssetPath(BaseFolder, CachedMesh->GetName() + TEXT("_Skeleton"));
		FString SkeletalMeshPkgName = MakeUniqueAssetPath(BaseFolder, CachedMesh->GetName() + TEXT("_SkelMesh"));
		FString ControlRigPkgName = MakeUniqueAssetPath(BaseFolder, CachedMesh->GetName() + TEXT("_ControlRig"));
		FString GeneratedDataPkgName = MakeUniqueAssetPath(BaseFolder, CachedMesh->GetName() + TEXT("_RigData"));

		UPackage* SkeletonPkg = CreatePackage(*SkeletonPkgName);
		UPackage* SkeletalMeshPkg = CreatePackage(*SkeletalMeshPkgName);
		UPackage* ControlRigPkg = CreatePackage(*ControlRigPkgName);
		UPackage* DataPkg = CreatePackage(*GeneratedDataPkgName);

		UAngelSkeletonBuilder* SkeletonBuilder = NewObject<UAngelSkeletonBuilder>();
		USkeleton* Skeleton = SkeletonBuilder->BuildSkeleton(SkeletonPkg, CurrentTemplate, CachedLandmarks, FName(*FPackageName::GetShortName(SkeletonPkgName)));
		if (!Skeleton)
		{
			SetStatus("Skeleton build failed.");
			return FReply::Handled();
		}

		UAngelSkeletalMeshBuilder* SkelMeshBuilder = NewObject<UAngelSkeletalMeshBuilder>();
		USkeletalMesh* SkelMesh = SkelMeshBuilder->BuildSkeletalMesh(SkeletalMeshPkg, CachedMesh, Skeleton, FName(*FPackageName::GetShortName(SkeletalMeshPkgName)));
		if (!SkelMesh)
		{
			SetStatus("SkelMesh build failed.");
			return FReply::Handled();
		}

		UAngelControlRigGenerator* CRGen = NewObject<UAngelControlRigGenerator>();
		UBlueprint* ControlRigBP = CRGen->GenerateControlRig(ControlRigPkg, CurrentTemplate, Skeleton, FName(*FPackageName::GetShortName(ControlRigPkgName)));
		if (!ControlRigBP)
		{
			SetStatus("ControlRig build failed.");
			return FReply::Handled();
		}

		UAngelGeneratedRigData* RigData = NewObject<UAngelGeneratedRigData>(DataPkg, UAngelGeneratedRigData::StaticClass(), FName(*FPackageName::GetShortName(GeneratedDataPkgName)), RF_Public | RF_Standalone);
		if (RigData)
		{
			RigData->Skeleton = Skeleton;
			RigData->SkeletalMesh = SkelMesh;
			RigData->ControlRig = ControlRigBP;
			RigData->Landmarks = CachedLandmarks.Landmarks;
			RigData->SourceStaticMeshName = CachedMesh->GetFName();

			// Populate GeneratedBones using first landmark per bone definition (MVP approximation)
			for (const FAngelBoneDefinition& BoneDef : CurrentTemplate->BoneDefinitions)
			{
				FAngelGeneratedBone GBone;
				GBone.BoneName = BoneDef.BoneName;
				if (BoneDef.LandmarksUsed.Num() > 0)
				{
					const FName LMName = BoneDef.LandmarksUsed[0];
					for (const FAngelSolvedLandmark& LM : CachedLandmarks.Landmarks)
					{
						if (LM.Name == LMName)
						{
							GBone.Transform = LM.Transform;
							break;
						}
					}
				}
				RigData->GeneratedBones.Add(GBone);
			}

			RigData->MarkPackageDirty();
			FAssetRegistryModule::AssetCreated(RigData);
		}

		SetStatus("Assets generated.");
#endif
		return FReply::Handled();
	}

	FReply OnClearDebug()
	{
#if WITH_EDITOR
		UWorld* World = (GEditor) ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (World)
		{
			for (const FVector& Pos : DebugPoints)
			{
				DrawDebugPoint(World, Pos, 10.f, FColor::Black, false, 0.f);
			}
		}
		DebugPoints.Reset();
		LandmarkList->ClearChildren();
		SetStatus("Cleared.");
#endif
		return FReply::Handled();
	}
};

void FAngelRigWizard::OpenWindow()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(TEXT("Angel Studio Rig Wizard")))
		.ClientSize(FVector2D(640.0f, 720.0f))
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	Window->SetContent(
		SNew(SAngelRigWizardWidget)
	);

	FSlateApplication::Get().AddWindow(Window);
}
