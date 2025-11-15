#include "AngelBatchRigProcessor.h"
#include "AngelRigTemplate.h"
#include "AngelLandmarkDetector.h"
#include "AngelSkeletonBuilder.h"
#include "AngelSkeletalMeshBuilder.h"
#include "AngelControlRigGenerator.h"
#include "AngelGeneratedRigData.h"
#include "Engine/StaticMesh.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "Editor.h"

DEFINE_LOG_CATEGORY_STATIC(LogAngelBatchRig, Log, All);

namespace AngelBatchInternal
{
	static FString MakeUniqueAssetPath(const FString& BaseFolder, const FString& AssetName)
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
}

void UAngelBatchRigProcessor::RunBatch(
	const TArray<FAngelBatchRigItem>& Items,
	UAngelRigTemplate* Template)
{
#if WITH_EDITOR
	if (!Template || Items.Num() == 0)
	{
		UE_LOG(LogAngelBatchRig, Warning, TEXT("RunBatch aborted: invalid template or empty items."));
		OnProgress.Broadcast(1.f);
		return;
	}

	const FString BaseFolder = TEXT("/Game/AngelStudio/Generated");
	int32 Total = Items.Num();
	int32 Index = 0;

	for (const FAngelBatchRigItem& Item : Items)
	{
		UStaticMesh* Mesh = Item.Mesh.LoadSynchronous();
		if (!Mesh)
		{
			UE_LOG(LogAngelBatchRig, Warning, TEXT("Skipping null mesh in batch."));
			++Index;
			OnProgress.Broadcast(float(Index) / float(Total));
			continue;
		}

		// Landmark detection
		UAngelLandmarkDetector* Detector = NewObject<UAngelLandmarkDetector>();
		FAngelLandmarkSolveResult Landmarks = Detector->DetectLandmarks(Mesh, Template);
		if (!Landmarks.bSuccess)
		{
			UE_LOG(LogAngelBatchRig, Warning, TEXT("Landmark detection failed for %s"), *Mesh->GetName());
			++Index;
			OnProgress.Broadcast(float(Index) / float(Total));
			continue;
		}

		// Package names
		FString SkeletonPkgName = AngelBatchInternal::MakeUniqueAssetPath(BaseFolder, Mesh->GetName() + TEXT("_Skeleton"));
		FString SkeletalMeshPkgName = AngelBatchInternal::MakeUniqueAssetPath(BaseFolder, Mesh->GetName() + TEXT("_SkelMesh"));
		FString ControlRigPkgName = AngelBatchInternal::MakeUniqueAssetPath(BaseFolder, Mesh->GetName() + TEXT("_ControlRig"));
		FString GeneratedDataPkgName = AngelBatchInternal::MakeUniqueAssetPath(BaseFolder, Mesh->GetName() + TEXT("_RigData"));

		UPackage* SkeletonPkg = CreatePackage(*SkeletonPkgName);
		UPackage* SkeletalMeshPkg = CreatePackage(*SkeletalMeshPkgName);
		UPackage* ControlRigPkg = CreatePackage(*ControlRigPkgName);
		UPackage* DataPkg = CreatePackage(*GeneratedDataPkgName);

		// Build skeleton (still stub)
		UAngelSkeletonBuilder* SkeletonBuilder = NewObject<UAngelSkeletonBuilder>();
		USkeleton* Skeleton = SkeletonBuilder->BuildSkeleton(SkeletonPkg, Template, Landmarks, FName(*FPackageName::GetShortName(SkeletonPkgName)));
		if (!Skeleton)
		{
			UE_LOG(LogAngelBatchRig, Warning, TEXT("Skeleton build failed for %s"), *Mesh->GetName());
			++Index;
			OnProgress.Broadcast(float(Index) / float(Total));
			continue;
		}

		// Build skeletal mesh (stub + pseudo weighting log)
		UAngelSkeletalMeshBuilder* SkelMeshBuilder = NewObject<UAngelSkeletalMeshBuilder>();
		USkeletalMesh* SkelMesh = SkelMeshBuilder->BuildSkeletalMesh(SkeletalMeshPkg, Mesh, Skeleton, FName(*FPackageName::GetShortName(SkeletalMeshPkgName)));
		if (!SkelMesh)
		{
			UE_LOG(LogAngelBatchRig, Warning, TEXT("Skeletal mesh build failed for %s"), *Mesh->GetName());
			++Index;
			OnProgress.Broadcast(float(Index) / float(Total));
			continue;
		}

		// Control Rig
		UAngelControlRigGenerator* CRGen = NewObject<UAngelControlRigGenerator>();
		UBlueprint* ControlRigBP = CRGen->GenerateControlRig(ControlRigPkg, Template, Skeleton, FName(*FPackageName::GetShortName(ControlRigPkgName)));
		if (!ControlRigBP)
		{
			UE_LOG(LogAngelBatchRig, Warning, TEXT("Control Rig build failed for %s"), *Mesh->GetName());
			++Index;
			OnProgress.Broadcast(float(Index) / float(Total));
			continue;
		}

		// Rig data asset
		UAngelGeneratedRigData* RigData = NewObject<UAngelGeneratedRigData>(DataPkg, UAngelGeneratedRigData::StaticClass(), FName(*FPackageName::GetShortName(GeneratedDataPkgName)), RF_Public | RF_Standalone);
		if (RigData)
		{
			RigData->Skeleton = Skeleton;
			RigData->SkeletalMesh = SkelMesh;
			RigData->ControlRig = ControlRigBP;
			RigData->Landmarks = Landmarks.Landmarks;
			RigData->SourceStaticMeshName = Mesh->GetFName();

			for (const FAngelBoneDefinition& BoneDef : Template->BoneDefinitions)
			{
				FAngelGeneratedBone GBone;
				GBone.BoneName = BoneDef.BoneName;
				if (BoneDef.LandmarksUsed.Num() > 0)
				{
					const FName LMName = BoneDef.LandmarksUsed[0];
					for (const FAngelSolvedLandmark& LM : Landmarks.Landmarks)
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

		UE_LOG(LogAngelBatchRig, Log, TEXT("Rig generated for %s"), *Mesh->GetName());

		++Index;
		OnProgress.Broadcast(float(Index) / float(Total));
	}
#endif
}
