#pragma once
#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "AngelGeneratedTag.generated.h"

USTRUCT(BlueprintType)
struct FAngelGenerationMetadata
{
	GENERATED_BODY()

	// Version of the generator logic
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Angel")
	int32 GeneratorVersion = 1;

	// UTC timestamp (Unix seconds) when asset was generated
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Angel")
	int64 GeneratedUtcSeconds = 0;

	// Template asset name used for generation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Angel")
	FName TemplateName = NAME_None;

	// Source static mesh name
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Angel")
	FName SourceMeshName = NAME_None;

	// Hash stored in two 32-bit parts for BP compatibility
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Angel")
	int32 ContentHashHi = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Angel")
	int32 ContentHashLo = 0;
};

UCLASS()
class ANGELSTUDIO_API UAngelGeneratedTag : public UAssetUserData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Angel")
	FName TagName = FName("AngelGenerated");

	UPROPERTY(EditAnywhere, Category="Angel")
	FAngelGenerationMetadata Metadata;
};
