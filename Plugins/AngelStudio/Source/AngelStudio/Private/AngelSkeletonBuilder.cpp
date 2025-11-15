#include "AngelSkeletonBuilder.h"
#include "Animation/Skeleton.h"
#include "AngelRigTemplate.h"
#include "AngelBonePlacementSolver.h"

USkeleton* UAngelSkeletonBuilder::BuildSkeleton(
	UObject* Outer,
	const UAngelRigTemplate* Template,
	const FAngelLandmarkSolveResult& SolvedLandmarks,
	FName SkeletonName)
{
	if (!Outer || !Template || !SolvedLandmarks.bSuccess)
	{
		return nullptr;
	}

	// NOTE: Proper bone tree construction requires a USkeletalMesh with a built ReferenceSkeleton.
	// For the MVP we return an empty USkeleton asset so subsequent steps (Control Rig clone) can proceed.
	// TODO: Implement skeletal mesh + reference skeleton generation, then call MergeAllBonesToBoneTree(SkelMesh, true).

	USkeleton* NewSkeleton = NewObject<USkeleton>(Outer, USkeleton::StaticClass(), SkeletonName, RF_Public | RF_Standalone);
	if (NewSkeleton)
	{
		NewSkeleton->MarkPackageDirty();
	}
	return NewSkeleton;
}
