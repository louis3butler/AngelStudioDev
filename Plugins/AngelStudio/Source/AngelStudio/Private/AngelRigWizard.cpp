#include "AngelRigWizard.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "AngelRigTemplate.h"
#include "AngelLandmarkDetector.h"
#include "AngelSkeletonBuilder.h"
#include "AngelSkeletalMeshBuilder.h"
#include "AngelControlRigGenerator.h"
#include "AngelGeneratedRigData.h"
#include "AngelStudioSettings.h"
#include "AngelBoneVisualizer.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Selection.h"
#include "DrawDebugHelpers.h"
#include "Misc/PackageName.h"
#include "ObjectTools.h"
#include "Misc/MessageDialog.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "ScopedTransaction.h"
#include "Misc/ScopedSlowTask.h"
#include "AngelGeneratedTag.h"

#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "SkeletalMeshTypes.h"

struct FAngelTemplateItem
{
	TWeakObjectPtr<UAngelRigTemplate> Template;
};

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

				// Template selection
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Template:")))
					]
					+ SHorizontalBox::Slot().Padding(8.f,0.f).AutoWidth()
					[
						SAssignNew(TemplateCombo, SComboBox<TSharedPtr<FAngelTemplateItem>>)
						.OptionsSource(&TemplateItems)
						.OnGenerateWidget(this, &SAngelRigWizardWidget::MakeTemplateItemWidget)
						.OnSelectionChanged(this, &SAngelRigWizardWidget::OnTemplateChanged)
						.InitiallySelectedItem(SelectedTemplate)
						[
							SNew(STextBlock).Text(this, &SAngelRigWizardWidget::GetSelectedTemplateText)
						]
					]
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
					.Text(FText::FromString(TEXT("Visualize Bones")))
					.OnClicked(this, &SAngelRigWizardWidget::OnVisualizeBones)
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
				.AutoHeight().Padding(4.f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Rebuild Weights (Selected SkeletalMesh)")))
					.OnClicked(this, &SAngelRigWizardWidget::OnRebuildWeights)
				]
				+ SVerticalBox::Slot()
				.AutoHeight().Padding(4.f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Rebuild Mesh (Selected SkeletalMesh)")))
					.OnClicked(this, &SAngelRigWizardWidget::OnRebuildMesh)
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
	TArray<TSharedPtr<FAngelTemplateItem>> TemplateItems;
	TSharedPtr<FAngelTemplateItem> SelectedTemplate;
	TArray<TWeakObjectPtr<UAngelRigTemplate>> Templates;
	UAngelRigTemplate* CurrentTemplate = nullptr;
	TSharedPtr<STextBlock> TemplateText;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SVerticalBox> LandmarkList;
	TSharedPtr<SComboBox<TSharedPtr<FAngelTemplateItem>>> TemplateCombo;
	TArray<FVector> DebugPoints;
	FAngelLandmarkSolveResult CachedLandmarks;
	UStaticMesh* CachedMesh = nullptr;

	FText GetSelectedTemplateText() const
	{
		return (SelectedTemplate.IsValid() && SelectedTemplate->Template.IsValid())
			? FText::FromName(SelectedTemplate->Template->TemplateName)
			: FText::FromString(TEXT("Select Template"));
	}

	TSharedRef<SWidget> MakeTemplateItemWidget(TSharedPtr<FAngelTemplateItem> InItem)
	{
		if (InItem.IsValid() && InItem->Template.IsValid())
		{
			return SNew(STextBlock).Text(FText::FromName(InItem->Template->TemplateName));
		}
		return SNew(STextBlock).Text(FText::FromString(TEXT("<null>")));
	}

	void OnTemplateChanged(TSharedPtr<FAngelTemplateItem> InItem, ESelectInfo::Type)
	{
		SelectedTemplate = InItem;
		CurrentTemplate = (InItem.IsValid()) ? InItem->Template.Get() : nullptr;
		if (TemplateText.IsValid())
		{
			TemplateText->SetText(GetSelectedTemplateText());
		}
	}

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
		TemplateItems.Reset();

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> FoundAssets;
		AssetRegistryModule.Get().GetAssetsByClass(UAngelRigTemplate::StaticClass()->GetClassPathName(), FoundAssets);
		for (const FAssetData& Data : FoundAssets)
		{
			if (UAngelRigTemplate* TemplateObj = Cast<UAngelRigTemplate>(Data.GetAsset()))
			{
				Templates.Add(TemplateObj);
				TSharedPtr<FAngelTemplateItem> Item = MakeShared<FAngelTemplateItem>();
				Item->Template = TemplateObj;
				TemplateItems.Add(Item);
			}
		}

		if (Templates.Num() == 0)
		{
			for (TObjectIterator<UAngelRigTemplate> It; It; ++It)
			{
				Templates.Add(*It);
				TSharedPtr<FAngelTemplateItem> Item = MakeShared<FAngelTemplateItem>();
				Item->Template = *It;
				TemplateItems.Add(Item);
			}
		}

		CurrentTemplate = Templates.Num() > 0 ? Templates[0].Get() : nullptr;
		SelectedTemplate = TemplateItems.Num() > 0 ? TemplateItems[0] : nullptr;
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

	void ShowNoMeshDialog() const
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::FromString(TEXT("Select a Static Mesh in the Content Browser and try again.")),
			FText::FromString(TEXT("Angel Studio")));
	}

	void OpenAssetsInEditors(USkeleton* Skeleton, USkeletalMesh* SkelMesh, UBlueprint* ControlRigBP)
	{
		if (!GEditor) { return; }
		if (UAssetEditorSubsystem* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			if (Skeleton) { AssetEditor->OpenEditorForAsset(Skeleton); }
			if (SkelMesh) { AssetEditor->OpenEditorForAsset(SkelMesh); }
			if (ControlRigBP) { AssetEditor->OpenEditorForAsset(ControlRigBP); }
		}
	}

	FReply OnDetect()
	{
#if WITH_EDITOR
		LandmarkList->ClearChildren();
		CachedMesh = GetSelectedMesh();
		if (!CachedMesh || !CurrentTemplate)
		{
			SetStatus("No mesh or template.");
			ShowNoMeshDialog();
			return FReply::Handled();
		}

		FScopedSlowTask SlowTask(1.f, FText::FromString(TEXT("Detecting landmarks...")));
		SlowTask.MakeDialog();
		SlowTask.EnterProgressFrame(1.f);

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

		const UAngelStudioSettings* Settings = GetDefault<UAngelStudioSettings>();
		const float Life = Settings ? Settings->LandmarkDebugLifetime : 10.f;
		const float Size = Settings ? Settings->LandmarkDebugPointSize : 10.f;

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
				DrawDebugPoint(World, Pos, Size, FColor::Cyan, false, Life);
			}
		}
#endif
		return FReply::Handled();
	}

	FReply OnVisualizeBones()
	{
#if WITH_EDITOR
		if (!CachedLandmarks.bSuccess || !CurrentTemplate)
		{
			SetStatus("Detect landmarks first.");
			return FReply::Handled();
		}
		UWorld* World = (GEditor) ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (World)
		{
			UAngelBoneVisualizer::DrawTemplateBones(World, CurrentTemplate, CachedLandmarks);
		}
		SetStatus("Bone lines drawn.");
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
		CachedMesh = GetSelectedMesh();
		if (!CachedMesh || !CachedLandmarks.bSuccess || !CurrentTemplate)
		{
			SetStatus("Detect landmarks first.");
			if (!CachedMesh)
			{
				ShowNoMeshDialog();
			}
			return FReply::Handled();
		}

		FScopedTransaction Tx(NSLOCTEXT("AngelStudio", "GenerateRigAssets", "Angel: Generate Rig Assets"));
		FScopedSlowTask SlowTask(3.f, FText::FromString(TEXT("Generating rig assets...")));
		SlowTask.MakeDialog();

		const UAngelStudioSettings* Settings = GetDefault<UAngelStudioSettings>();
		const FString BaseFolder = Settings && !Settings->DefaultOutputFolder.Path.IsEmpty() ? Settings->DefaultOutputFolder.Path : TEXT("/Game/AngelStudio/Generated");

		SlowTask.EnterProgressFrame(1.f, FText::FromString(TEXT("Creating skeletal mesh + skeleton...")));
		FString SkeletalMeshPkgName = MakeUniqueAssetPath(BaseFolder, CachedMesh->GetName() + TEXT("_SkelMesh"));
		UPackage* SkeletalMeshPkg = CreatePackage(*SkeletalMeshPkgName);
		UAngelSkeletalMeshBuilder* SkelMeshBuilder = NewObject<UAngelSkeletalMeshBuilder>();
		USkeletalMesh* SkelMesh = SkelMeshBuilder->BuildSkeletalMesh(SkeletalMeshPkg, CachedMesh, CurrentTemplate, CachedLandmarks, FName(*FPackageName::GetShortName(SkeletalMeshPkgName)));
		if (!SkelMesh || !SkelMesh->GetSkeleton())
		{
			SetStatus("Skeletal mesh build failed.");
			return FReply::Handled();
		}
		USkeleton* Skeleton = SkelMesh->GetSkeleton();

		SlowTask.EnterProgressFrame(1.f, FText::FromString(TEXT("Creating control rig...")));
		FString ControlRigPkgName = MakeUniqueAssetPath(BaseFolder, CachedMesh->GetName() + TEXT("_ControlRig"));
		UPackage* ControlRigPkg = CreatePackage(*ControlRigPkgName);
		UAngelControlRigGenerator* CRGen = NewObject<UAngelControlRigGenerator>();
		UBlueprint* ControlRigBP = CRGen->GenerateControlRig(ControlRigPkg, CurrentTemplate, Skeleton, FName(*FPackageName::GetShortName(ControlRigPkgName)));
		if (!ControlRigBP)
		{
			SetStatus("ControlRig build failed.");
			return FReply::Handled();
		}

		SlowTask.EnterProgressFrame(1.f, FText::FromString(TEXT("Creating rig data asset...")));
		FString GeneratedDataPkgName = MakeUniqueAssetPath(BaseFolder, CachedMesh->GetName() + TEXT("_RigData"));
		UPackage* DataPkg = CreatePackage(*GeneratedDataPkgName);
		UAngelGeneratedRigData* RigData = NewObject<UAngelGeneratedRigData>(DataPkg, UAngelGeneratedRigData::StaticClass(), FName(*FPackageName::GetShortName(GeneratedDataPkgName)), RF_Public | RF_Standalone);
		if (RigData)
		{
			RigData->Skeleton = Skeleton;
			RigData->SkeletalMesh = SkelMesh;
			RigData->ControlRig = ControlRigBP;
			RigData->Landmarks = CachedLandmarks.Landmarks;
			RigData->SourceStaticMeshName = CachedMesh->GetFName();

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

		// Tag supported assets (Skeleton, SkeletalMesh) - exclude Blueprint/DataAsset which do not expose AssetUserData helpers here
		if (SkelMesh && !SkelMesh->GetAssetUserDataOfClass(UAngelGeneratedTag::StaticClass()))
		{
			UAngelGeneratedTag* LocalTagSM = NewObject<UAngelGeneratedTag>(SkelMesh, UAngelGeneratedTag::StaticClass());
			SkelMesh->AddAssetUserData(LocalTagSM);
		}
		if (Skeleton && !Skeleton->GetAssetUserDataOfClass(UAngelGeneratedTag::StaticClass()))
		{
			UAngelGeneratedTag* LocalTagSkel = NewObject<UAngelGeneratedTag>(Skeleton, UAngelGeneratedTag::StaticClass());
			Skeleton->AddAssetUserData(LocalTagSkel);
		}

		if (SkelMesh)
		{
			if (UAngelGeneratedTag* LocalMetaTagMesh = Cast<UAngelGeneratedTag>(SkelMesh->GetAssetUserDataOfClass(UAngelGeneratedTag::StaticClass())))
			{
				LocalMetaTagMesh->Metadata.GeneratorVersion = 1;
				LocalMetaTagMesh->Metadata.GeneratedUtcSeconds = FDateTime::UtcNow().ToUnixTimestamp();
				LocalMetaTagMesh->Metadata.TemplateName = CurrentTemplate->TemplateName;
				LocalMetaTagMesh->Metadata.SourceMeshName = CachedMesh->GetFName();
				uint64 Hash = 1469598103934665603ull;
				auto MixStr=[&](const FString& S){ for(auto C:S){ Hash = (Hash ^ (uint8)C) * 1099511628211ull; } }; 
				MixStr(CurrentTemplate->TemplateName.ToString());
				MixStr(CachedMesh->GetName());
				Hash ^= CachedLandmarks.Landmarks.Num(); Hash *= 1099511628211ull;
				LocalMetaTagMesh->Metadata.ContentHashHi = int32((Hash >> 32) & 0xFFFFFFFF);
				LocalMetaTagMesh->Metadata.ContentHashLo = int32(Hash & 0xFFFFFFFF);
				SkelMesh->MarkPackageDirty();
			}
		}
		if (Skeleton)
		{
			if (UAngelGeneratedTag* LocalMetaTagSkel = Cast<UAngelGeneratedTag>(Skeleton->GetAssetUserDataOfClass(UAngelGeneratedTag::StaticClass())))
			{
				LocalMetaTagSkel->Metadata.GeneratorVersion = 1;
				LocalMetaTagSkel->Metadata.GeneratedUtcSeconds = FDateTime::UtcNow().ToUnixTimestamp();
				LocalMetaTagSkel->Metadata.TemplateName = CurrentTemplate->TemplateName;
				LocalMetaTagSkel->Metadata.SourceMeshName = CachedMesh->GetFName();
				LocalMetaTagSkel->Metadata.ContentHashHi = 0;
				LocalMetaTagSkel->Metadata.ContentHashLo = 0;
				Skeleton->MarkPackageDirty();
			}
		}

		SetStatus("Assets generated.");

		OpenAssetsInEditors(Skeleton, SkelMesh, ControlRigBP);
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

	USkeletalMesh* FindSelectedSkeletalMesh() const
	{
#if WITH_EDITOR
		if (GEditor && GEditor->GetSelectedObjects())
		{
			USelection* Sel = GEditor->GetSelectedObjects();
			for (int32 i=0;i<Sel->Num();++i)
			{
				if (USkeletalMesh* Skel = Cast<USkeletalMesh>(Sel->GetSelectedObject(i)))
				{
					return Skel;
				}
			}
		}
#endif
		return nullptr;
	}

	UAngelGeneratedTag* GetGeneratedTag(USkeletalMesh* SkelMesh) const
	{
		if (!SkelMesh) return nullptr;
		return Cast<UAngelGeneratedTag>(SkelMesh->GetAssetUserDataOfClass(UAngelGeneratedTag::StaticClass()));
	}

	FReply OnRebuildWeights()
	{
#if WITH_EDITOR
		USkeletalMesh* SkelMesh = FindSelectedSkeletalMesh();
		if (!SkelMesh)
		{
			SetStatus("Select a SkeletalMesh to rebuild weights.");
			return FReply::Handled();
		}
		UAngelGeneratedTag* GeneratedTag = GetGeneratedTag(SkelMesh);
		if (!GeneratedTag)
		{
			SetStatus("Mesh not tagged (skip).");
			return FReply::Handled();
		}

		FSkeletalMeshModel* ImportedModel = SkelMesh->GetImportedModel();
		if (!ImportedModel || ImportedModel->LODModels.Num()==0)
		{
			SetStatus("No LOD model.");
			return FReply::Handled();
		}
		FSkeletalMeshLODModel& LODModel = ImportedModel->LODModels[0];

		// Compute bone world locations
		const FReferenceSkeleton& RefSkel = SkelMesh->GetRefSkeleton();
		TArray<FVector> BoneWorldPositions; BoneWorldPositions.SetNum(RefSkel.GetNum());
		for (int32 BoneIdx=0; BoneIdx<RefSkel.GetNum(); ++BoneIdx)
		{
			FTransform Accum = RefSkel.GetRefBonePose()[BoneIdx];
			int32 ParentIdx = RefSkel.GetParentIndex(BoneIdx);
			while (ParentIdx != INDEX_NONE)
			{
				Accum = Accum * RefSkel.GetRefBonePose()[ParentIdx];
				ParentIdx = RefSkel.GetParentIndex(ParentIdx);
			}
			BoneWorldPositions[BoneIdx] = Accum.GetLocation();
		}

		FScopedTransaction Tx(NSLOCTEXT("AngelStudio","RebuildWeights","Angel: Rebuild Weights"));
		FScopedSlowTask SlowTask(1.f, FText::FromString(TEXT("Rebuilding weights..."))); SlowTask.MakeDialog(); SlowTask.EnterProgressFrame(1.f);

		constexpr int32 NumInfluences = 4;
		for (FSkelMeshSection& Section : LODModel.Sections)
		{
			for (FSoftSkinVertex& V : Section.SoftVertices)
			{
				FVector P = FVector(V.Position);
				struct FBoneDist { int32 Id; float Dist; }; TArray<FBoneDist> Dists; Dists.Reserve(BoneWorldPositions.Num());
				for (int32 i=0;i<BoneWorldPositions.Num();++i)
				{
					float d = FVector::DistSquared(P, BoneWorldPositions[i]);
					Dists.Add({i,d});
				}
				Dists.Sort([](const FBoneDist& A, const FBoneDist& B){ return A.Dist < B.Dist; });
				for (int32 inf=0; inf<NumInfluences; ++inf){ V.InfluenceWeights[inf]=0; V.InfluenceBones[inf]=0; }
				float InvTotal=0.f; float Raw[NumInfluences]={0};
				int32 UseCount = FMath::Min(NumInfluences, Dists.Num());
				for (int32 inf=0; inf<UseCount; ++inf)
				{
					float dist = FMath::Sqrt(Dists[inf].Dist);
					float w = 1.f / FMath::Max(1.f, dist);
					Raw[inf]=w; InvTotal+=w; V.InfluenceBones[inf] = (uint8)Dists[inf].Id;
				}
				if (InvTotal>0.f)
				{
					int32 Sum=0;
					for (int32 inf=0; inf<UseCount; ++inf)
					{
						uint8 bw = (uint8)FMath::Clamp(FMath::RoundToInt((Raw[inf]/InvTotal)*255.f),0,255);
						V.InfluenceWeights[inf]=bw; Sum+=bw;
					}
					if (Sum!=255 && UseCount>0)
					{
						int32 Drift=255-Sum; int32 Last=UseCount-1; int32 adj = FMath::Clamp(int32(V.InfluenceWeights[Last])+Drift,0,255); V.InfluenceWeights[Last]=(uint8)adj;
					}
				}
			}
		}
		SkelMesh->MarkPackageDirty();
		SetStatus("Weights rebuilt.");
#endif
		return FReply::Handled();
	}

	FReply OnRebuildMesh()
	{
#if WITH_EDITOR
		USkeletalMesh* SkelMesh = FindSelectedSkeletalMesh();
		if (!SkelMesh)
		{
			SetStatus("Select a SkeletalMesh to rebuild.");
			return FReply::Handled();
		}
		UAngelGeneratedTag* GeneratedTag = GetGeneratedTag(SkelMesh);
		if (!GeneratedTag)
		{
			SetStatus("Mesh not tagged (skip).");
			return FReply::Handled();
		}

		// Locate source static mesh by name
		FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> AllMeshes; ARM.Get().GetAssetsByClass(UStaticMesh::StaticClass()->GetClassPathName(), AllMeshes);
		UStaticMesh* SourceStatic = nullptr;
		for (const FAssetData& AD : AllMeshes)
		{
			if (AD.AssetName == GeneratedTag->Metadata.SourceMeshName)
			{
				SourceStatic = Cast<UStaticMesh>(AD.GetAsset()); break;
			}
		}
		if (!SourceStatic)
		{
			SetStatus("Source static mesh not found.");
			return FReply::Handled();
		}

		// Use current template selection (or template name from tag)
		UAngelRigTemplate* UseTemplate = CurrentTemplate;
		if (!UseTemplate && GeneratedTag->Metadata.TemplateName != NAME_None)
		{
			TArray<FAssetData> TemplatesData; ARM.Get().GetAssetsByClass(UAngelRigTemplate::StaticClass()->GetClassPathName(), TemplatesData);
			for (const FAssetData& TD : TemplatesData)
			{
				if (TD.AssetName == GeneratedTag->Metadata.TemplateName)
				{
					UseTemplate = Cast<UAngelRigTemplate>(TD.GetAsset()); break;
				}
			}
		}
		if (!UseTemplate)
		{
			SetStatus("Template not found.");
			return FReply::Handled();
		}

		UAngelLandmarkDetector* Detector = NewObject<UAngelLandmarkDetector>();
		FAngelLandmarkSolveResult Landmarks = Detector->DetectLandmarks(SourceStatic, UseTemplate);
		if (!Landmarks.bSuccess)
		{
			SetStatus("Landmark detect failed.");
			return FReply::Handled();
		}

		FString OldPath = SkelMesh->GetPathName();
		FString BasePath; FString MeshAssetName;
		if (!OldPath.Split(TEXT("."), &BasePath, &MeshAssetName))
		{
			SetStatus("Path parse failed."); return FReply::Handled();
		}
		FString NewMeshPath = BasePath + TEXT("/") + MeshAssetName + TEXT("_Regen");
		if (FPackageName::DoesPackageExist(NewMeshPath))
		{
			NewMeshPath += TEXT("_1");
		}
		UPackage* NewPkg = CreatePackage(*NewMeshPath);
		FName NewName(*FPackageName::GetShortName(NewMeshPath));
		UAngelSkeletalMeshBuilder* Builder = NewObject<UAngelSkeletalMeshBuilder>();
		USkeletalMesh* NewSkelMesh = Builder->BuildSkeletalMesh(NewPkg, SourceStatic, UseTemplate, Landmarks, NewName);
		if (!NewSkelMesh)
		{
			SetStatus("Regen build failed.");
			return FReply::Handled();
		}
		SetStatus("Mesh regenerated.");
#endif
		return FReply::Handled();
	}
};

void FAngelRigWizard::OpenWindow()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(TEXT("Angel Studio Rig Wizard")))
		.ClientSize(FVector2D(660.0f, 780.0f))
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	Window->SetContent(
		SNew(SAngelRigWizardWidget)
	);

	FSlateApplication::Get().AddWindow(Window);
}
