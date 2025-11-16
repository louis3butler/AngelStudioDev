#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AngelStudioSettings.generated.h"

UCLASS(config=EditorPerProjectUserSettings, DefaultConfig, meta=(DisplayName="Angel Studio"))
class ANGELSTUDIO_API UAngelStudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UAngelStudioSettings();

	// Base output folder for generated assets
	UPROPERTY(EditAnywhere, config, Category="Paths")
	FDirectoryPath DefaultOutputFolder;

	// Debug draw lifetime for landmarks (seconds)
	UPROPERTY(EditAnywhere, config, Category="Debug", meta=(ClampMin=0.0, ClampMax=120.0))
	float LandmarkDebugLifetime;

	// Debug point size
	UPROPERTY(EditAnywhere, config, Category="Debug", meta=(ClampMin=1.0, ClampMax=50.0))
	float LandmarkDebugPointSize;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
