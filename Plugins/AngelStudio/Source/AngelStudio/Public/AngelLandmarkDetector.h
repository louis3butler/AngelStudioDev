#pragma once

#include "CoreMinimal.h"
#include "AngelBonePlacementSolver.h"
#include "AngelLandmarkDetector.generated.h"

UCLASS()
class ANGELSTUDIO_API UAngelLandmarkDetector : public UObject
{
	GENERATED_BODY()

public:
	FAngelLandmarkSolveResult DetectLandmarks(
		class UStaticMesh* Mesh,
		class UAngelRigTemplate* Template);
};
