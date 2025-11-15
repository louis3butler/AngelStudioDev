#pragma once

#include "CoreMinimal.h"
#include "AngelRigTemplate.h"
#include "AngelBonePlacementSolver.generated.h"

USTRUCT(BlueprintType)
struct FAngelSolvedLandmark
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTransform Transform = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct FAngelLandmarkSolveResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bSuccess = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FAngelSolvedLandmark> Landmarks;
};

UINTERFACE()
class ANGELSTUDIORUNTIME_API UAngelBonePlacementSolver : public UInterface
{
	GENERATED_BODY()
};

class ANGELSTUDIORUNTIME_API IAngelBonePlacementSolver
{
	GENERATED_BODY()

public:
	virtual FAngelLandmarkSolveResult SolveLandmarks(
		class UStaticMesh* Mesh,
		const UAngelRigTemplate* Template
	) = 0;
};
