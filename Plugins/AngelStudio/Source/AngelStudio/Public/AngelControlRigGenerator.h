#pragma once

#include "CoreMinimal.h"
#include "AngelRigTemplate.h"
#include "AngelControlRigGenerator.generated.h"

UCLASS()
class ANGELSTUDIO_API UAngelControlRigGenerator : public UObject
{
	GENERATED_BODY()

public:
	class UBlueprint* GenerateControlRig(
		UObject* Outer,
		const UAngelRigTemplate* Template,
		class USkeleton* TargetSkeleton,
		FName ControlRigName);
};
