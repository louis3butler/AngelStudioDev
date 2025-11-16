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
#include "AngelGeneratedTag.h"

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

		FString SkeletalMeshPkgName = AngelBatchInternal::MakeUniqueAssetPath(BaseFolder, Mesh->GetName() + TEXT("_SkelMesh"));
		FString ControlRigPkgName = AngelBatchInternal::MakeUniqueAssetPath(BaseFolder, Mesh->GetName() + TEXT("_ControlRig"));
		FString GeneratedDataPkgName = AngelBatchInternal::MakeUniqueAssetPath(BaseFolder, Mesh->GetName() + TEXT("_RigData"));

		UPackage* SkeletalMeshPkg = CreatePackage(*SkeletalMeshPkgName);
		UPackage* ControlRigPkg = CreatePackage(*ControlRigPkgName);
		UPackage* DataPkg = CreatePackage(*GeneratedDataPkgName);

		// Build skeletal mesh (stub + pseudo weighting log)
		UAngelSkeletalMeshBuilder* SkelMeshBuilder = NewObject<UAngelSkeletalMeshBuilder>();
		USkeletalMesh* SkelMesh = SkelMeshBuilder->BuildSkeletalMesh(SkeletalMeshPkg, Mesh, Template, Landmarks, FName(*FPackageName::GetShortName(SkeletalMeshPkgName)));
		if (!SkelMesh || !SkelMesh->GetSkeleton())
		{
			UE_LOG(LogAngelBatchRig, Warning, TEXT("Skeletal mesh build failed for %s"), *Mesh->GetName());
			++Index;
			OnProgress.Broadcast(float(Index) / float(Total));
			continue;
		}
		USkeleton* Skeleton = SkelMesh->GetSkeleton();

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

		// Tag only skeletal mesh and skeleton
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
				LocalMetaTagMesh->Metadata.TemplateName = Template->TemplateName;
				LocalMetaTagMesh->Metadata.SourceMeshName = Mesh->GetFName();
				uint64 Hash = 1469598103934665603ull;
				auto MixStr=[&](const FString& S){ for(auto C:S){ Hash = (Hash ^ (uint8)C) * 1099511628211ull; } }; 
				MixStr(Template->TemplateName.ToString());
				MixStr(Mesh->GetName());
				Hash ^= Landmarks.Landmarks.Num(); Hash *= 1099511628211ull;
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
				LocalMetaTagSkel->Metadata.TemplateName = Template->TemplateName;
				LocalMetaTagSkel->Metadata.SourceMeshName = Mesh->GetFName();
				LocalMetaTagSkel->Metadata.ContentHashHi = 0;
				LocalMetaTagSkel->Metadata.ContentHashLo = 0;
				Skeleton->MarkPackageDirty();
			}
		}

		UE_LOG(LogAngelBatchRig, Log, TEXT("Rig generated for %s"), *Mesh->GetName());

		++Index;
		OnProgress.Broadcast(float(Index) / float(Total));
	}
#endif
}
