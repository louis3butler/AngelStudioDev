#pragma once

#include "CoreMinimal.h"
#include "AngelBonePlacementSolver.h"
#include "AngelSkeletalMeshBuilder.generated.h"

UCLASS()
class ANGELSTUDIO_API UAngelSkeletalMeshBuilder : public UObject
{
	GENERATED_BODY()

public:
	class USkeletalMesh* BuildSkeletalMesh(
		UObject* Outer,
		class UStaticMesh* SourceMesh,
		const class UAngelRigTemplate* Template,
		const struct FAngelLandmarkSolveResult& Landmarks,
		FName SkeletalMeshName);
};
