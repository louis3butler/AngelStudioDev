#include "AngelSkeletalMeshBuilder.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AngelGeneratedRigData.h"
#include "AngelRigTemplate.h"
#include "AngelBonePlacementSolver.h"

DEFINE_LOG_CATEGORY_STATIC(LogAngelSkeletalMeshBuilder, Log, All);

namespace AngelMeshInternal
{
	static int32 FindNearestBoneIndex(const FVector& Pos, const TArray<FAngelGeneratedBone>& Bones)
	{
		int32 BestIndex = INDEX_NONE;
		float BestDistSqr = FLT_MAX;
		for (int32 i = 0; i < Bones.Num(); ++i)
		{
			float DistSqr = FVector::DistSquared(Pos, Bones[i].Transform.GetLocation());
			if (DistSqr < BestDistSqr)
			{
				BestDistSqr = DistSqr;
				BestIndex = i;
			}
		}
		return BestIndex;
	}
}

USkeletalMesh* UAngelSkeletalMeshBuilder::BuildSkeletalMesh(
	UObject* Outer,
	UStaticMesh* SourceMesh,
	USkeleton* TargetSkeleton,
	FName SkeletalMeshName)
{
	if (!Outer || !SourceMesh || !TargetSkeleton)
	{
		UE_LOG(LogAngelSkeletalMeshBuilder, Warning, TEXT("Invalid inputs to BuildSkeletalMesh."));
		return nullptr;
	}

	USkeletalMesh* NewSkeletalMesh = NewObject<USkeletalMesh>(Outer, USkeletalMesh::StaticClass(), SkeletalMeshName, RF_Public | RF_Standalone);
	if (!NewSkeletalMesh)
	{
		return nullptr;
	}

	// Assign skeleton (still empty bone tree MVP)
	NewSkeletalMesh->SetSkeleton(TargetSkeleton);

	// TODO: Proper geometry + reference skeleton construction.
	// MVP: Log a pseudo-weighting pass over static mesh vertices to nearest generated bone (debug only).
	if (SourceMesh->GetRenderData() && SourceMesh->GetRenderData()->LODResources.Num() > 0)
	{
		const FStaticMeshLODResources& LOD = SourceMesh->GetRenderData()->LODResources[0];
		const FPositionVertexBuffer& PosBuffer = LOD.VertexBuffers.PositionVertexBuffer;
		int32 NumVerts = PosBuffer.GetNumVertices();

		UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("Pseudo weighting %d vertices."), NumVerts);

		// Attempt to find generated rig data (optional)
		TArray<FAngelGeneratedBone> Bones;
		for (TObjectIterator<UAngelGeneratedRigData> It; It; ++It)
		{
			if (It->SkeletalMesh.Get() == NewSkeletalMesh || It->SkeletalMesh.Get() == nullptr)
			{
				Bones = It->GeneratedBones;
				break;
			}
		}

		if (Bones.Num() == 0)
		{
			UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("No generated bones found for weighting (RigData not yet created)."));
		}
		else
		{
			int32 Sampled = FMath::Min(NumVerts, 25); // only log a sample to avoid spam
			for (int32 i = 0; i < Sampled; ++i)
			{
				FVector3f V3f = PosBuffer.VertexPosition(i);
				FVector Pos = FVector(V3f);
				int32 BoneIndex = AngelMeshInternal::FindNearestBoneIndex(Pos, Bones);
				if (BoneIndex != INDEX_NONE)
				{
					UE_LOG(LogAngelSkeletalMeshBuilder, Verbose, TEXT("Vert %d -> Bone %s"), i, *Bones[BoneIndex].BoneName.ToString());
				}
			}
			UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("Pseudo weighting complete (sample logged)."));
		}
	}

	NewSkeletalMesh->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewSkeletalMesh);
	return NewSkeletalMesh;
}
