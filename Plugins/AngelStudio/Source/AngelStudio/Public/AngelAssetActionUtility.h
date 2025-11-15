#pragma once

#include "CoreMinimal.h"
#include "AssetActionUtility.h"
#include "AngelAssetActionUtility.generated.h"

UCLASS()
class ANGELSTUDIO_API UAngelAssetActionUtility : public UAssetActionUtility
{
	GENERATED_BODY()

public:
	UFUNCTION(CallInEditor)
	void AutoRigSelectedMeshes();
};
