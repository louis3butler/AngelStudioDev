#pragma once

#include "CoreMinimal.h"
#include "AngelRigTemplate.h"
#include "AngelBonePlacementSolver.h"
#include "AngelSkeletonBuilder.generated.h"

UCLASS()
class ANGELSTUDIO_API UAngelSkeletonBuilder : public UObject
{
	GENERATED_BODY()

public:
	class USkeleton* BuildSkeleton(
		UObject* Outer,
		const UAngelRigTemplate* Template,
		const FAngelLandmarkSolveResult& SolvedLandmarks,
		FName SkeletonName);
};
