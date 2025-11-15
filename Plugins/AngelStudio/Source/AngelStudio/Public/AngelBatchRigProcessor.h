#pragma once

#include "CoreMinimal.h"
#include "AngelBatchRigProcessor.generated.h"

USTRUCT()
struct FAngelBatchRigItem
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<class UStaticMesh> Mesh;

	UPROPERTY()
	FString OutputPath;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FAngelBatchProgress, float /*Progress0to1*/);

UCLASS()
class ANGELSTUDIO_API UAngelBatchRigProcessor : public UObject
{
	GENERATED_BODY()

public:
	FAngelBatchProgress OnProgress;

	void RunBatch(
		const TArray<FAngelBatchRigItem>& Items,
		class UAngelRigTemplate* Template);
};
