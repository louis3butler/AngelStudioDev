#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AngelRigTemplate.h"
#include "AngelRigProfile.generated.h"

UCLASS(BlueprintType)
class ANGELSTUDIORUNTIME_API UAngelRigProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	TSoftObjectPtr<UAngelRigTemplate> BaseTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	int32 SpineSegmentsOverride = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	int32 TailSegmentsOverride = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	bool bHasWings = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AngelRig")
	bool bIsQuadruped = false;
};
