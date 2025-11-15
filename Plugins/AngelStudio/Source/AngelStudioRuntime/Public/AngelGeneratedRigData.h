#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AngelBonePlacementSolver.h"
#include "AngelGeneratedRigData.generated.h"

USTRUCT(BlueprintType)
struct FAngelGeneratedBone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName BoneName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTransform Transform = FTransform::Identity; // world/mesh space at generation time
};

UCLASS(BlueprintType)
class ANGELSTUDIORUNTIME_API UAngelGeneratedRigData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	TSoftObjectPtr<class USkeleton> Skeleton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	TSoftObjectPtr<class USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	TSoftObjectPtr<class UBlueprint> ControlRig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	TArray<FAngelSolvedLandmark> Landmarks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	TArray<FAngelGeneratedBone> GeneratedBones;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	FName SourceStaticMeshName = NAME_None;
};
