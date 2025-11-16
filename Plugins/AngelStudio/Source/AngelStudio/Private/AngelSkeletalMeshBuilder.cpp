#include "AngelSkeletalMeshBuilder.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Animation/Skeleton.h"
#include "AngelGeneratedRigData.h"
#include "AngelRigTemplate.h"
#include "AngelBonePlacementSolver.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODImporterData.h"
#include "Components/StaticMeshComponent.h"
#include "Algo/Transform.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "SkeletalMeshTypes.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "SkeletalMeshTypes.h"
#include "PhysicsEngine/PhysicsAsset.h"
#if WITH_EDITOR
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#endif
#include "AngelGeneratedTag.h"

// Define constants at top of file (before namespace)
static constexpr uint8 GAngelInfluencePruneMinWeight = 5; // minimum byte weight to keep
static constexpr float GAngelSphereMin = 2.f;
static constexpr float GAngelSphereMax = 25.f;

// Angel TODO: Upcoming step will allocate and populate FSkeletalMeshLODModel with vertices, indices, and skin weights.
// Current implementation logs section info and provisional per-vertex weight samples only.

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

	static void FindTopNBones(const FVector& Pos, const TArray<FAngelGeneratedBone>& Bones, int32 N, TArray<int32>& OutIndices, TArray<float>& OutWeights)
	{
		struct FTemp { int32 Index; float DistSqr; }; TArray<FTemp> Temp; Temp.Reserve(Bones.Num());
		for (int32 i=0;i<Bones.Num();++i)
		{
			float DistSqr = FVector::DistSquared(Pos, Bones[i].Transform.GetLocation());
			Temp.Add({i,DistSqr});
		}
		Temp.Sort([](const FTemp& A, const FTemp& B){ return A.DistSqr < B.DistSqr; });
		OutIndices.Reset(); OutWeights.Reset();
		float InvDistAccum = 0.f;
		for (int32 i=0;i<FMath::Min(N, Temp.Num()); ++i)
		{
			float InvDist = 1.f / FMath::Max(1.f, FMath::Sqrt(Temp[i].DistSqr));
			OutIndices.Add(Temp[i].Index);
			OutWeights.Add(InvDist);
			InvDistAccum += InvDist;
		}
		if (InvDistAccum > 0.f)
		{
			for (float& W : OutWeights) { W /= InvDistAccum; }
		}
	}

	static void FindTopNBones(const FVector& Pos, const TArray<FVector>& BoneWorldPositions, int32 N, TArray<int32>& OutIndices, TArray<float>& OutWeights)
	{
		struct FTemp { int32 Index; float DistSqr; }; TArray<FTemp> Temp; Temp.Reserve(BoneWorldPositions.Num());
		for (int32 i=0;i<BoneWorldPositions.Num();++i)
		{
			float DistSqr = FVector::DistSquared(Pos, BoneWorldPositions[i]);
			Temp.Add({i,DistSqr});
		}
		Temp.Sort([](const FTemp& A, const FTemp& B){ return A.DistSqr < B.DistSqr; });
		OutIndices.Reset(); OutWeights.Reset();
		float InvDistAccum = 0.f;
		for (int32 i=0;i<FMath::Min(N, Temp.Num()); ++i)
		{
			float InvDist = 1.f / FMath::Max(1.f, FMath::Sqrt(FMath::Max(KINDA_SMALL_NUMBER, Temp[i].DistSqr)));
			OutIndices.Add(Temp[i].Index);
			OutWeights.Add(InvDist);
			InvDistAccum += InvDist;
		}
		if (InvDistAccum > 0.f)
		{
			for (float& W : OutWeights) { W /= InvDistAccum; }
		}
	}

	static bool BuildLODFromStaticMeshStub(USkeletalMesh* SkelMesh, UStaticMesh* SourceMesh)
	{
		if (!SkelMesh || !SourceMesh || !SourceMesh->GetRenderData() || SourceMesh->GetRenderData()->LODResources.Num() == 0)
		{
			return false;
		}

		// NOTE (Angel TODO): This is a stub that inspects the source static mesh LOD0 and logs intended section splits.
		// Future implementation will:
		//  - Copy vertex positions, normals, tangents, UVs
		//  - Create FSkeletalMeshLODModel with sections per material
		//  - Populate bone influences (weights) into LODModel.SkinWeightVertexBuffer or virtualized weights
		//  - Call SkelMesh->PostEditChange() / Invalidate to finalize render resources
		const FStaticMeshLODResources& SrcLOD = SourceMesh->GetRenderData()->LODResources[0];
		int32 NumSections = SrcLOD.Sections.Num();
		UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("Angel: Stub LOD build - StaticMesh LOD0 has %d sections."), NumSections);
		for (int32 SectionIdx = 0; SectionIdx < NumSections; ++SectionIdx)
		{
			const FStaticMeshSection& S = SrcLOD.Sections[SectionIdx];
			UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("Angel: Section %d -> MaterialIndex %d, NumTriangles %d"), SectionIdx, S.MaterialIndex, S.NumTriangles);
		}
		// Return true to signify we inspected geometry (even though we did not build skeletal render data)
		return true;
	}

	static bool BuildBasicLOD(USkeletalMesh* SkelMesh, UStaticMesh* SourceMesh)
	{
		if (!SkelMesh || !SourceMesh || !SourceMesh->GetRenderData() || SourceMesh->GetRenderData()->LODResources.Num() == 0)
		{
			return false;
		}

		FSkeletalMeshModel* ImportedModel = SkelMesh->GetImportedModel();
		if (!ImportedModel)
		{
			return false;
		}
		// If already has verts, assume built
		if (ImportedModel->LODModels.Num() > 0 && ImportedModel->LODModels[0].NumVertices > 0)
		{
			return true;
		}

		const FStaticMeshLODResources& SrcLOD = SourceMesh->GetRenderData()->LODResources[0];
		int32 NumVerts = SrcLOD.VertexBuffers.PositionVertexBuffer.GetNumVertices();
		if (NumVerts == 0) { return false; }

		// Create LODModel (TIndirectArray needs explicit allocation)
		FSkeletalMeshLODModel* NewLODPtr = new FSkeletalMeshLODModel();
		ImportedModel->LODModels.Add(NewLODPtr);
		FSkeletalMeshLODModel& LODModel = *NewLODPtr;

		LODModel.Sections.AddDefaulted();
		FSkelMeshSection& Section = LODModel.Sections[0];
		Section.MaterialIndex = 0;
		Section.BaseIndex = 0;
		Section.BaseVertexIndex = 0;
		Section.NumVertices = NumVerts;
		Section.bDisabled = false;
		Section.ChunkedParentSectionIndex = INDEX_NONE;
		Section.bCastShadow = true;
		Section.MaxBoneInfluences = 1;

		Section.SoftVertices.SetNum(NumVerts);

		// Bone map (root bone only for now)
		Section.BoneMap.Reset();
		Section.BoneMap.Add((FBoneIndexType)0);

		constexpr int32 NumInfluences = 4; // FSoftSkinVertex supports 4 influences
		for (int32 i = 0; i < NumVerts; ++i)
		{
			FSoftSkinVertex& V = Section.SoftVertices[i];
			FVector3f Pos = SrcLOD.VertexBuffers.PositionVertexBuffer.VertexPosition(i);
			V.Position = Pos;
			V.TangentX = SrcLOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(i);
			V.TangentZ = SrcLOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(i);
			V.TangentY = SrcLOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentY(i);
			if (SrcLOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords() > 0)
			{
				V.UVs[0] = SrcLOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i, 0);
			}
			else
			{
				V.UVs[0] = FVector2f::ZeroVector;
			}
			for (int32 uv = 1; uv < MAX_TEXCOORDS; ++uv) { V.UVs[uv] = FVector2f::ZeroVector; }
			for (int32 b = 0; b < NumInfluences; ++b)
			{
				V.InfluenceBones[b] = 0;
				V.InfluenceWeights[b] = 0;
			}
			V.InfluenceBones[0] = 0;
			V.InfluenceWeights[0] = 255; // full weight root
		}

		LODModel.NumVertices = NumVerts;

		// Indices
		TArray<uint32> Indices;
		SrcLOD.IndexBuffer.GetCopy(Indices);
		LODModel.IndexBuffer = Indices;
		Section.NumTriangles = Indices.Num() / 3;

		// Required bones
		LODModel.ActiveBoneIndices.Add(0);
		LODModel.RequiredBones.Add(0);

		// Ensure there is a LODInfo entry
		if (SkelMesh->GetLODInfoArray().Num() == 0)
		{
			SkelMesh->AddLODInfo();
		}

		SkelMesh->CalculateInvRefMatrices();
		SkelMesh->MarkPackageDirty();
		SkelMesh->InvalidateDeriveDataCacheGUID();
		SkelMesh->PostEditChange();
		UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("Angel: Basic LOD0 constructed (%d verts, %d tris)."), NumVerts, Section.NumTriangles);
		return true;
	}

	static bool BuildBasicLOD(USkeletalMesh* SkelMesh, UStaticMesh* SourceMesh, const TArray<FVector>& BoneWorldPositions)
	{
		if (!SkelMesh || !SourceMesh || !SourceMesh->GetRenderData() || SourceMesh->GetRenderData()->LODResources.Num() == 0)
		{
			return false;
		}
		FSkeletalMeshModel* ImportedModel = SkelMesh->GetImportedModel();
		if (!ImportedModel) { return false; }
		if (ImportedModel->LODModels.Num() > 0 && ImportedModel->LODModels[0].NumVertices > 0)
		{
			return true; // already built
		}
		const FStaticMeshLODResources& SrcLOD = SourceMesh->GetRenderData()->LODResources[0];
		int32 NumVerts = SrcLOD.VertexBuffers.PositionVertexBuffer.GetNumVertices();
		if (NumVerts == 0) { return false; }

		// Allocate new LOD
		FSkeletalMeshLODModel* NewLODPtr = new FSkeletalMeshLODModel();
		ImportedModel->LODModels.Add(NewLODPtr);
		FSkeletalMeshLODModel& LODModel = *NewLODPtr;

		// Copy all indices from static mesh
		TArray<uint32> AllIndices; SrcLOD.IndexBuffer.GetCopy(AllIndices);
		LODModel.IndexBuffer = AllIndices;

		// Build per-section data mirroring static mesh sections
		int32 StaticSectionCount = SrcLOD.Sections.Num();
		LODModel.Sections.SetNum(StaticSectionCount);

		// Prepare a global vertex array (one vertex per static mesh vertex)
		TArray<FSoftSkinVertex> TempSoftVerts; TempSoftVerts.SetNum(NumVerts);
		constexpr int32 NumInfluences = 4;
		TArray<TArray<float>> PrimaryWeightsPerBone; PrimaryWeightsPerBone.SetNum(BoneWorldPositions.Num());
		for (int32 v=0; v<NumVerts; ++v)
		{
			FSoftSkinVertex& V = TempSoftVerts[v];
			FVector3f Pos = SrcLOD.VertexBuffers.PositionVertexBuffer.VertexPosition(v);
			V.Position = Pos;
			V.TangentX = SrcLOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(v);
			V.TangentZ = SrcLOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(v);
			V.TangentY = SrcLOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentY(v);
			if (SrcLOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords() > 0) { V.UVs[0] = SrcLOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(v,0); }
			else { V.UVs[0] = FVector2f::ZeroVector; }
			for (int32 uv=1; uv<MAX_TEXCOORDS; ++uv){ V.UVs[uv] = FVector2f::ZeroVector; }
			for (int32 inf=0; inf<NumInfluences; ++inf){ V.InfluenceBones[inf]=0; V.InfluenceWeights[inf]=0; }
			TArray<int32> BoneIndices; TArray<float> Weights;
			AngelMeshInternal::FindTopNBones((FVector)Pos, BoneWorldPositions, NumInfluences, BoneIndices, Weights);
			for (int32 inf=0; inf<BoneIndices.Num(); ++inf)
			{
				V.InfluenceBones[inf] = (uint8)BoneIndices[inf];
				V.InfluenceWeights[inf] = (uint8)FMath::Clamp(FMath::RoundToInt(Weights[inf]*255.f),0,255);
			}
			// Prune tiny weights (except first influence)
			for (int32 inf=1; inf<NumInfluences; ++inf)
			{
				if (V.InfluenceWeights[inf] < GAngelInfluencePruneMinWeight) { V.InfluenceWeights[inf]=0; V.InfluenceBones[inf]=0; }
			}
			// Renormalize
			int32 Sum=0; for(int32 inf=0; inf<NumInfluences; ++inf){ Sum += V.InfluenceWeights[inf]; }
			if (Sum>0 && Sum!=255)
			{
				float Scale = 255.f / float(Sum);
				Sum=0; for(int32 inf=0; inf<NumInfluences; ++inf){ V.InfluenceWeights[inf] = (uint8)FMath::Clamp(FMath::RoundToInt(V.InfluenceWeights[inf]*Scale),0,255); Sum+=V.InfluenceWeights[inf]; }
				int32 Drift = 255-Sum; if (Drift!=0){ int32 Last=0; V.InfluenceWeights[Last] = (uint8)FMath::Clamp(int32(V.InfluenceWeights[Last])+Drift,0,255); }
			}
			// Track primary influence distance for physics sizing
			int32 PrimaryBone = V.InfluenceBones[0];
			if (PrimaryBone >=0 && PrimaryBone < BoneWorldPositions.Num())
			{
				PrimaryWeightsPerBone[PrimaryBone].Add((FVector(V.Position) - BoneWorldPositions[PrimaryBone]).Size());
			}
		}

		// Assign vertices to sections using static mesh section ranges with compacted vertex lists per section
		for (int32 s=0; s<StaticSectionCount; ++s)
		{
			const FStaticMeshSection& SrcSection = SrcLOD.Sections[s];
			FSkelMeshSection& DstSection = LODModel.Sections[s];
			DstSection.MaterialIndex = SrcSection.MaterialIndex;
			DstSection.BaseIndex = SrcSection.FirstIndex;
			DstSection.NumTriangles = SrcSection.NumTriangles;
			DstSection.ChunkedParentSectionIndex = INDEX_NONE;
			DstSection.bDisabled = false;
			DstSection.bCastShadow = true;
			DstSection.MaxBoneInfluences = 4;
			// Gather unique vertex indices referenced by this section
			TSet<int32> UniqueVertIdx;
			for (uint32 idx = SrcSection.FirstIndex; idx < SrcSection.FirstIndex + SrcSection.NumTriangles*3; ++idx)
			{
				UniqueVertIdx.Add((int32)AllIndices[idx]);
			}
			TArray<int32> SortedRefs = UniqueVertIdx.Array();
			SortedRefs.Sort();
			DstSection.NumVertices = SortedRefs.Num();
			DstSection.BaseVertexIndex = 0; // Using per-section local vertex array
			DstSection.SoftVertices.SetNum(DstSection.NumVertices);
			for (int32 local=0; local<SortedRefs.Num(); ++local)
			{
				DstSection.SoftVertices[local] = TempSoftVerts[SortedRefs[local]];
			}
			DstSection.BoneMap.Reset();
			for (int32 BoneIdx=0; BoneIdx < BoneWorldPositions.Num(); ++BoneIdx){ DstSection.BoneMap.Add((FBoneIndexType)BoneIdx); }
		}

		LODModel.NumVertices = NumVerts;
		for (int32 BoneIdx=0; BoneIdx<BoneWorldPositions.Num(); ++BoneIdx){ LODModel.ActiveBoneIndices.Add(BoneIdx); LODModel.RequiredBones.Add(BoneIdx); }
		if (SkelMesh->GetLODInfoArray().Num()==0){ SkelMesh->AddLODInfo(); }
		SkelMesh->CalculateInvRefMatrices();
		SkelMesh->MarkPackageDirty();
		SkelMesh->InvalidateDeriveDataCacheGUID();
		SkelMesh->PostEditChange();
		UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("Angel: Multi-section weighted LOD0 built (%d verts, %d sections)."), NumVerts, StaticSectionCount);
		return true;
	}
#if WITH_EDITOR
	static UPhysicsAsset* CreateBasicPhysicsAsset(USkeletalMesh* SkelMesh)
	{
		if (!SkelMesh) { return nullptr; }
		// Derive package path from skeletal mesh
		FString MeshPath = SkelMesh->GetPathName();
		FString BasePath; FString AssetName;
		if (!MeshPath.Split(TEXT("."), &BasePath, &AssetName))
		{
			return nullptr;
		}
		FString PhysPath = BasePath + TEXT("/") + AssetName + TEXT("_Physics");
		if (FPackageName::DoesPackageExist(PhysPath))
		{
			// Already exists, do not recreate
			UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("Angel: Reusing existing physics asset %s"), *PhysPath);
			return LoadObject<UPhysicsAsset>(nullptr, *PhysPath);
		}

		UPackage* PhysPkg = CreatePackage(*PhysPath);
		UPhysicsAsset* PhysAsset = NewObject<UPhysicsAsset>(PhysPkg, UPhysicsAsset::StaticClass(), *FPackageName::GetShortName(PhysPath), RF_Public | RF_Standalone);
		if (!PhysAsset)
		{
			return nullptr;
		}

		// Minimal bodies: create one for each bone up to a limit (avoid spam). Use a simple sphere.
		const int32 MaxBodies = 16; // MVP cap
		const FReferenceSkeleton& RefSkel = SkelMesh->GetRefSkeleton();
		TMap<FName,int32> BoneNameToBodyIndex;

		for (int32 BoneIdx = 0; BoneIdx < RefSkel.GetNum() && BoneIdx < MaxBodies; ++BoneIdx)
		{
			USkeletalBodySetup* BodySetup = NewObject<USkeletalBodySetup>(PhysAsset, USkeletalBodySetup::StaticClass(), NAME_None, RF_Transactional);
			BodySetup->BoneName = RefSkel.GetBoneName(BoneIdx);
			// Create a small sphere primitive
			FKSphereElem Sphere;
			Sphere.Radius = 5.f; // placeholder size
			Sphere.Center = FVector::ZeroVector;
			BodySetup->AggGeom.SphereElems.Add(Sphere);
			PhysAsset->SkeletalBodySetups.Add(BodySetup);
			BoneNameToBodyIndex.Add(BodySetup->BoneName, PhysAsset->SkeletalBodySetups.Num()-1);
		}

		// Basic parent-child constraints (angular limited) for added bodies
		for (USkeletalBodySetup* BodySetup : PhysAsset->SkeletalBodySetups)
		{
			if (!BodySetup) continue;
			int32 BoneIdx = RefSkel.FindBoneIndex(BodySetup->BoneName);
			int32 ParentBoneIdx = RefSkel.GetParentIndex(BoneIdx);
			if (ParentBoneIdx != INDEX_NONE)
			{
				FName ParentName = RefSkel.GetBoneName(ParentBoneIdx);
				int32* ParentBodyIndex = BoneNameToBodyIndex.Find(ParentName);
				int32* ChildBodyIndex = BoneNameToBodyIndex.Find(BodySetup->BoneName);
				if (ParentBodyIndex && ChildBodyIndex)
				{
					UPhysicsConstraintTemplate* Constraint = NewObject<UPhysicsConstraintTemplate>(PhysAsset, UPhysicsConstraintTemplate::StaticClass(), NAME_None, RF_Transactional);
					Constraint->DefaultInstance.ConstraintBone1 = ParentName;
					Constraint->DefaultInstance.ConstraintBone2 = BodySetup->BoneName;
					// Limit angular swing/twist to modest values for stability
					Constraint->DefaultInstance.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, 30.f);
					Constraint->DefaultInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, 30.f);
					Constraint->DefaultInstance.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, 30.f);
					PhysAsset->ConstraintSetup.Add(Constraint);
				}
			}
		}

		PhysAsset->UpdateBodySetupIndexMap();
		PhysAsset->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(PhysAsset);
		UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("Angel: Created physics asset %s (Bodies: %d, Constraints: %d)"), *PhysPath, PhysAsset->SkeletalBodySetups.Num(), PhysAsset->ConstraintSetup.Num());
		return PhysAsset;
	}
#endif
} // namespace AngelMeshInternal

USkeletalMesh* UAngelSkeletalMeshBuilder::BuildSkeletalMesh(
	UObject* Outer,
	UStaticMesh* SourceMesh,
	const UAngelRigTemplate* Template,
	const FAngelLandmarkSolveResult& Landmarks,
	FName SkeletalMeshName)
{
	if (!Outer || !SourceMesh || !Template || !Landmarks.bSuccess)
	{
		UE_LOG(LogAngelSkeletalMeshBuilder, Warning, TEXT("Invalid inputs to BuildSkeletalMesh."));
		return nullptr;
	}

	USkeletalMesh* NewSkeletalMesh = NewObject<USkeletalMesh>(Outer, USkeletalMesh::StaticClass(), SkeletalMeshName, RF_Public | RF_Standalone);
	if (!NewSkeletalMesh)
	{
		return nullptr;
	}

	// Build a map of landmark transforms for quick lookup
	TMap<FName, FTransform> LandmarkWorld;
	for (const FAngelSolvedLandmark& LM : Landmarks.Landmarks)
	{
		LandmarkWorld.Add(LM.Name, LM.Transform);
	}

	// Precompute children list per bone for orientation hints
	TMultiMap<FName, FName> ParentToChildren;
	for (const FAngelBoneDefinition& BoneDef : Template->BoneDefinitions)
	{
		if (BoneDef.ParentBoneName != NAME_None)
		{
			ParentToChildren.Add(BoneDef.ParentBoneName, BoneDef.BoneName);
		}
	}

	// Build reference skeleton from template + landmarks (improved local transform + simple orientation)
	FReferenceSkeleton RefSkeleton;
	FReferenceSkeletonModifier RefSkelMod(RefSkeleton, nullptr);
	TMap<FName,int32> BoneNameToIndex;

	for (const FAngelBoneDefinition& BoneDef : Template->BoneDefinitions)
	{
		const FName BoneName = BoneDef.BoneName;
		if (BoneName == NAME_None) { continue; }

		// World transform for this bone
		FTransform BoneWorld = FTransform::Identity;
		if (BoneDef.LandmarksUsed.Num() > 0)
		{
			if (const FTransform* LM = LandmarkWorld.Find(BoneDef.LandmarksUsed[0]))
			{
				BoneWorld = *LM;
			}
		}

		// Orientation from child average
		TArray<FName> ChildNames;
		ParentToChildren.MultiFind(BoneName, ChildNames);
		FVector AimDir = FVector::ForwardVector;
		if (ChildNames.Num() > 0)
		{
			FVector Accum(0,0,0); int32 Count=0;
			for (const FName& ChildBoneName : ChildNames)
			{
				const FAngelBoneDefinition* ChildDef = Template->BoneDefinitions.FindByPredicate([&](const FAngelBoneDefinition& D){ return D.BoneName == ChildBoneName; });
				if (ChildDef && ChildDef->LandmarksUsed.Num() > 0)
				{
					if (const FTransform* ChildLM = LandmarkWorld.Find(ChildDef->LandmarksUsed[0]))
					{
						Accum += ChildLM->GetLocation(); ++Count;
					}
				}
			}
			if (Count>0)
			{
				AimDir = (Accum/float(Count)) - BoneWorld.GetLocation();
				if (!AimDir.Normalize()) { AimDir = FVector::ForwardVector; }
			}
		}

		FVector XAxis = AimDir;
		FVector ZAxis = FVector::UpVector;
		if (FMath::Abs(ZAxis | XAxis) > 0.99f) { ZAxis = FVector::RightVector; }
		FVector YAxis = (ZAxis ^ XAxis).GetSafeNormal();
		ZAxis = (XAxis ^ YAxis).GetSafeNormal();
		FMatrix Basis(
			FPlane(XAxis,0),
			FPlane(YAxis,0),
			FPlane(ZAxis,0),
			FPlane(FVector::ZeroVector,0)
		);
		BoneWorld.SetRotation(FQuat(Basis));

		int32 ParentIndex = INDEX_NONE;
		if (BoneDef.ParentBoneName != NAME_None)
		{
			if (int32* FoundParent = BoneNameToIndex.Find(BoneDef.ParentBoneName)) { ParentIndex = *FoundParent; }
		}

		FTransform Local = BoneWorld;
		if (ParentIndex != INDEX_NONE)
		{
			FTransform ParentWorld = RefSkeleton.GetRefBonePose()[ParentIndex];
			Local = BoneWorld.GetRelativeTransform(ParentWorld);
		}

		RefSkelMod.Add(FMeshBoneInfo(BoneName, BoneName.ToString(), ParentIndex), Local);
		BoneNameToIndex.Add(BoneName, RefSkeleton.GetNum()-1);
	}

	NewSkeletalMesh->GetRefSkeleton() = RefSkeleton;

	// Compute world-space bone positions (approx: accumulate parent transforms; using local pose order of RefSkeleton)
	TArray<FVector> BoneWorldPositions; BoneWorldPositions.SetNum(RefSkeleton.GetNum());
	for (int32 BoneIdx=0; BoneIdx<RefSkeleton.GetNum(); ++BoneIdx)
	{
		FTransform Accum = RefSkeleton.GetRefBonePose()[BoneIdx];
		int32 ParentIdx = RefSkeleton.GetParentIndex(BoneIdx);
		while (ParentIdx != INDEX_NONE)
		{
			Accum = Accum * RefSkeleton.GetRefBonePose()[ParentIdx];
			ParentIdx = RefSkeleton.GetParentIndex(ParentIdx);
		}
		BoneWorldPositions[BoneIdx] = Accum.GetLocation();
	}

	// Create skeleton asset and assign (external package instead of internal transient name for user visibility)
	FString OuterPath = Outer->GetPathName();
	FString BasePath;
	FString AssetName;
	if (OuterPath.Split(TEXT("."), &BasePath, &AssetName))
	{
		FString SkeletonPath = BasePath + TEXT("/") + SkeletalMeshName.ToString() + TEXT("_Skeleton");
		if (!FPackageName::DoesPackageExist(SkeletonPath))
		{
			UPackage* SkeletonPkg = CreatePackage(*SkeletonPath);
			USkeleton* NewSkeleton = NewObject<USkeleton>(SkeletonPkg, USkeleton::StaticClass(), *FPackageName::GetShortName(SkeletonPath), RF_Public | RF_Standalone);
			if (NewSkeleton)
			{
				NewSkeleton->MergeAllBonesToBoneTree(NewSkeletalMesh);
				NewSkeletalMesh->SetSkeleton(NewSkeleton);
				NewSkeleton->MarkPackageDirty();
				FAssetRegistryModule::AssetCreated(NewSkeleton);
				UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("Angel: Created skeleton asset %s"), *SkeletonPath);
			}
		}
		else
		{
			UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("Angel: Reusing existing skeleton package %s (not recreated)."), *SkeletonPath);
		}
	}
	else
	{
		// Fallback to internal skeleton if we cannot parse outer path
		USkeleton* NewSkeleton = NewObject<USkeleton>(Outer, USkeleton::StaticClass(), *FString::Printf(TEXT("%s_SkeletonInternal"), *SkeletalMeshName.ToString()), RF_Public | RF_Standalone);
		if (NewSkeleton)
		{
			NewSkeleton->MergeAllBonesToBoneTree(NewSkeletalMesh);
			NewSkeletalMesh->SetSkeleton(NewSkeleton);
		}
	}

	// Tag skeletal mesh (AssetUserData supported on UObject derived asset)
	if (NewSkeletalMesh && !NewSkeletalMesh->GetAssetUserDataOfClass(UAngelGeneratedTag::StaticClass()))
	{
		UAngelGeneratedTag* LocalTagSM = NewObject<UAngelGeneratedTag>(NewSkeletalMesh, UAngelGeneratedTag::StaticClass());
		NewSkeletalMesh->AddAssetUserData(LocalTagSM);
	}
	// Tag skeleton asset
	if (USkeleton* Skel = NewSkeletalMesh->GetSkeleton())
	{
		if (!Skel->GetAssetUserDataOfClass(UAngelGeneratedTag::StaticClass()))
		{
			UAngelGeneratedTag* LocalTagSkel = NewObject<UAngelGeneratedTag>(Skel, UAngelGeneratedTag::StaticClass());
			Skel->AddAssetUserData(LocalTagSkel);
		}
	}

	// Prepare generated bone positions for weighting
	TArray<FAngelGeneratedBone> GeneratedBones;
	GeneratedBones.Reserve(Template->BoneDefinitions.Num());
	for (const FAngelBoneDefinition& BoneDef : Template->BoneDefinitions)
	{
		FAngelGeneratedBone GB; GB.BoneName = BoneDef.BoneName; GB.Transform = LandmarkWorld.FindRef(BoneDef.LandmarksUsed.Num()>0?BoneDef.LandmarksUsed[0]:NAME_None); GeneratedBones.Add(GB);
	}

	// Placeholder geometry & skinning: sample vertices and compute up to 4 bone weights (not stored yet)
	if (SourceMesh->GetRenderData() && SourceMesh->GetRenderData()->LODResources.Num() > 0)
	{
		const FStaticMeshLODResources& SrcLOD = SourceMesh->GetRenderData()->LODResources[0];
		const FPositionVertexBuffer& PosBuffer = SrcLOD.VertexBuffers.PositionVertexBuffer;
		int32 NumVerts = PosBuffer.GetNumVertices();
		UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("Angel: Vertices: %d (computing provisional weights)"), NumVerts);

		int32 Sampled = FMath::Min(NumVerts, 50);
		for (int32 i = 0; i < Sampled; ++i)
		{
			FVector3f V3f = PosBuffer.VertexPosition(i);
			TArray<int32> BoneIdx; TArray<float> BoneWeights;
			AngelMeshInternal::FindTopNBones(FVector(V3f), GeneratedBones, 4, BoneIdx, BoneWeights);
			FString PairStr;
			for (int32 k=0;k<BoneIdx.Num();++k)
			{
				PairStr += FString::Printf(TEXT("%s(%.2f) "), *GeneratedBones[BoneIdx[k]].BoneName.ToString(), BoneWeights[k]);
			}
			UE_LOG(LogAngelSkeletalMeshBuilder, Verbose, TEXT("Vert %d -> %s"), i, *PairStr);
		}
		UE_LOG(LogAngelSkeletalMeshBuilder, Log, TEXT("Angel: Provisional weight sampling complete."));
	}

	// Copy basic bounds & materials from source mesh as interim so asset previews have contextual info
	NewSkeletalMesh->SetImportedBounds(SourceMesh->GetBounds());
	for (const FStaticMaterial& SrcMat : SourceMesh->GetStaticMaterials())
	{
		FSkeletalMaterial SkelMat(SrcMat.MaterialInterface, true, false, SrcMat.MaterialSlotName, SrcMat.ImportedMaterialSlotName);
		NewSkeletalMesh->GetMaterials().Add(SkelMat);
	}
	// NOTE: Full geometry & section creation pending; this interim step enables material slot inspection and correct bounds in editors.

	NewSkeletalMesh->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewSkeletalMesh);

	// After building RefSkeleton we produce BoneWorldPositions before creating skeleton asset
	// After provisional sampling and material copy, build weighted LOD
	AngelMeshInternal::BuildBasicLOD(NewSkeletalMesh, SourceMesh, BoneWorldPositions);

#if WITH_EDITOR
	// Auto-create simple physics asset (optional MVP)
	AngelMeshInternal::CreateBasicPhysicsAsset(NewSkeletalMesh);
#endif

	// Tag skeleton asset if created
	if (USkeleton* Skel = NewSkeletalMesh->GetSkeleton())
	{
		if (!Skel->GetAssetUserDataOfClass(UAngelGeneratedTag::StaticClass()))
		{
			UAngelGeneratedTag* Tag = NewObject<UAngelGeneratedTag>(Skel, UAngelGeneratedTag::StaticClass());
			Skel->AddAssetUserData(Tag);
		}
	}

	// Populate generation metadata on skeletal mesh and skeleton tags after creation.
	if (NewSkeletalMesh)
	{
		if (UAngelGeneratedTag* Tag = Cast<UAngelGeneratedTag>(NewSkeletalMesh->GetAssetUserDataOfClass(UAngelGeneratedTag::StaticClass())))
		{
			Tag->Metadata.GeneratorVersion = 1;
			Tag->Metadata.GeneratedUtcSeconds = FDateTime::UtcNow().ToUnixTimestamp();
			Tag->Metadata.TemplateName = Template->TemplateName;
			Tag->Metadata.SourceMeshName = SourceMesh->GetFName();
			// Simple hash: combine counts + name strings length; placeholder for real hashing
			uint64 Hash = 1469598103934665603ull;
			auto MixStr=[&](const FString& S){ for(auto C:S){ Hash = (Hash ^ (uint8)C) * 1099511628211ull; } }; 
			MixStr(Template->TemplateName.ToString());
			MixStr(SourceMesh->GetName());
			Hash ^= Landmarks.Landmarks.Num(); Hash *= 1099511628211ull;
			Tag->Metadata.ContentHashHi = int32((Hash >> 32) & 0xFFFFFFFF);
			Tag->Metadata.ContentHashLo = int32(Hash & 0xFFFFFFFF);
			NewSkeletalMesh->MarkPackageDirty();
		}
	}
	if (USkeleton* Skel = NewSkeletalMesh->GetSkeleton())
	{
		if (UAngelGeneratedTag* Tag = Cast<UAngelGeneratedTag>(Skel->GetAssetUserDataOfClass(UAngelGeneratedTag::StaticClass())))
		{
			Tag->Metadata.GeneratorVersion = 1;
			Tag->Metadata.GeneratedUtcSeconds = FDateTime::UtcNow().ToUnixTimestamp();
			Tag->Metadata.TemplateName = Template->TemplateName;
			Tag->Metadata.SourceMeshName = SourceMesh->GetFName();
			Tag->Metadata.ContentHashHi = 0;
			Tag->Metadata.ContentHashLo = 0;
			Skel->MarkPackageDirty();
		}
	}

	return NewSkeletalMesh;
}
