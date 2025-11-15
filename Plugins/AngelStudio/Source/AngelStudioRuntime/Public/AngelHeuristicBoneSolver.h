#pragma once

#include "CoreMinimal.h"
#include "AngelBonePlacementSolver.h"
#include "AngelHeuristicBoneSolver.generated.h"

UCLASS()
class ANGELSTUDIORUNTIME_API UAngelHeuristicBoneSolver : public UObject, public IAngelBonePlacementSolver
{
	GENERATED_BODY()

public:
	virtual FAngelLandmarkSolveResult SolveLandmarks(
		UStaticMesh* Mesh,
		const UAngelRigTemplate* Template
	) override;

private:
	bool ExtractVertices(UStaticMesh* Mesh, TArray<FVector>& OutVertices) const;
	void ComputeHumanoidLandmarks(
		const TArray<FVector>& Vertices,
		TMap<EAngelLandmarkType, FVector>& OutPositions
	) const;
	FVector ComputeCenterOfMass(const TArray<FVector>& Vertices) const;
};
