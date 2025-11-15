#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AngelRigTemplate.generated.h"

UENUM(BlueprintType)
enum class EAngelLandmarkType : uint8
{
	Unknown     UMETA(DisplayName="Unknown"),
	Pelvis      UMETA(DisplayName="Pelvis Center"),
	SpineStart  UMETA(DisplayName="Spine Start"),
	SpineEnd    UMETA(DisplayName="Spine End"),
	NeckBase    UMETA(DisplayName="Neck Base"),
	HeadCenter  UMETA(DisplayName="Head Center"),

	ShoulderL   UMETA(DisplayName="Left Shoulder"),
	ShoulderR   UMETA(DisplayName="Right Shoulder"),
	ElbowL      UMETA(DisplayName="Left Elbow"),
	ElbowR      UMETA(DisplayName="Right Elbow"),
	WristL      UMETA(DisplayName="Left Wrist"),
	WristR      UMETA(DisplayName="Right Wrist"),

	HipL        UMETA(DisplayName="Left Hip"),
	HipR        UMETA(DisplayName="Right Hip"),
	KneeL       UMETA(DisplayName="Left Knee"),
	KneeR       UMETA(DisplayName="Right Knee"),
	AnkleL      UMETA(DisplayName="Left Ankle"),
	AnkleR      UMETA(DisplayName="Right Ankle"),

	WingRootL   UMETA(DisplayName="Left Wing Root"),
	WingRootR   UMETA(DisplayName="Right Wing Root"),
	TailBase    UMETA(DisplayName="Tail Base"),
};

UENUM(BlueprintType)
enum class EAngelLandmarkDetectionHint : uint8
{
	None            UMETA(DisplayName="None"),
	CenterMass      UMETA(DisplayName="Center Mass"),
	UpperBody       UMETA(DisplayName="Upper Body"),
	LowerBody       UMETA(DisplayName="Lower Body"),
	Extremity       UMETA(DisplayName="Extremity"),
	SymmetricPair   UMETA(DisplayName="Symmetric Pair"),
};

UENUM(BlueprintType)
enum class EAngelBonePlacementMethod : uint8
{
	AtLandmark          UMETA(DisplayName="At Landmark"),
	BetweenLandmarks    UMETA(DisplayName="Between Landmarks"),
	InterpolatedChain   UMETA(DisplayName="Interpolated Chain"),
};

USTRUCT(BlueprintType)
struct FAngelLandmarkDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EAngelLandmarkType Type = EAngelLandmarkType::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EAngelLandmarkDetectionHint DetectionHint = EAngelLandmarkDetectionHint::None;
};

USTRUCT(BlueprintType)
struct FAngelBoneDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName BoneName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ParentBoneName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> LandmarksUsed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EAngelBonePlacementMethod PlacementMethod = EAngelBonePlacementMethod::AtLandmark;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 ChainSegments = 1;
};

/**
 * A rig template describing bones, landmark types, and a Control Rig template.
 */
UCLASS(BlueprintType)
class ANGELSTUDIORUNTIME_API UAngelRigTemplate : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	FName TemplateName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	TArray<FAngelLandmarkDefinition> LandmarkDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	TArray<FAngelBoneDefinition> BoneDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	TSoftObjectPtr<class UBlueprint> ControlRigTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	int32 DefaultSpineSegments = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	int32 DefaultTailSegments = 0;
};
