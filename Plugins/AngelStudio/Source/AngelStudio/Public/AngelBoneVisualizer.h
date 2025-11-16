#pragma once

#include "CoreMinimal.h"
#include "AngelBoneVisualizer.generated.h"

UCLASS()
class ANGELSTUDIO_API UAngelBoneVisualizer : public UObject
{
	GENERATED_BODY()
public:
	// Draw parent-child lines using template BoneDefinitions and landmark result positions
	UFUNCTION()
	static void DrawTemplateBones(class UWorld* World,
		const class UAngelRigTemplate* Template,
		const struct FAngelLandmarkSolveResult& Landmarks,
		float LifeTime = 10.f,
		float Thickness = 1.5f);
};
