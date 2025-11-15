#include "AngelControlRigGenerator.h"
#include "Animation/Skeleton.h"
#include "Engine/Blueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

UBlueprint* UAngelControlRigGenerator::GenerateControlRig(
	UObject* Outer,
	const UAngelRigTemplate* Template,
	USkeleton* TargetSkeleton,
	FName ControlRigName)
{
	if (!Outer || !Template || !TargetSkeleton)
	{
		return nullptr;
	}

	UPackage* Package = Outer->GetOutermost();
	UBlueprint* TemplateCR = Template->ControlRigTemplate.LoadSynchronous();
	UBlueprint* NewCR = nullptr;

	if (TemplateCR)
	{
		NewCR = Cast<UBlueprint>(StaticDuplicateObject(TemplateCR, Package, ControlRigName));
	}

	if (!NewCR)
	{
		// Fallback: empty blueprint as placeholder
		NewCR = NewObject<UBlueprint>(Package, ControlRigName, RF_Public | RF_Standalone);
	}

	if (NewCR)
	{
		FAssetRegistryModule::AssetCreated(NewCR);
		NewCR->MarkPackageDirty();
	}
	return NewCR;
}
