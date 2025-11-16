#include "AngelBoneVisualizer.h"
#include "AngelRigTemplate.h"
#include "AngelBonePlacementSolver.h"
#include "DrawDebugHelpers.h"

void UAngelBoneVisualizer::DrawTemplateBones(UWorld* World,
	const UAngelRigTemplate* Template,
	const FAngelLandmarkSolveResult& Landmarks,
	float LifeTime,
	float Thickness)
{
	if (!World || !Template || !Landmarks.bSuccess)
	{
		return;
	}

	TMap<FName, FVector> LMPos;
	for (const FAngelSolvedLandmark& LM : Landmarks.Landmarks)
	{
		LMPos.Add(LM.Name, LM.Transform.GetLocation());
	}

	for (const FAngelBoneDefinition& Def : Template->BoneDefinitions)
	{
		if (Def.ParentBoneName == NAME_None || Def.LandmarksUsed.Num() == 0)
		{
			continue;
		}

		const FName ThisLM = Def.LandmarksUsed[0];
		FVector* A = LMPos.Find(ThisLM);

		// find parent's landmark by scanning up chain; fallback to same landmark if none found
		const FAngelBoneDefinition* ParentDef = Template->BoneDefinitions.FindByPredicate([&](const FAngelBoneDefinition& B){ return B.BoneName == Def.ParentBoneName; });
		FVector* BPos = nullptr;
		if (ParentDef && ParentDef->LandmarksUsed.Num() > 0)
		{
			BPos = LMPos.Find(ParentDef->LandmarksUsed[0]);
		}
		if (!BPos) { BPos = A; }

		if (A && BPos)
		{
			DrawDebugLine(World, *A, *BPos, FColor::Yellow, false, LifeTime, 0, Thickness);
		}
	}
}
