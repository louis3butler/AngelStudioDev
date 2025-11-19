#include "AngelControlRigGenerator.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogAngelControlRigGen, Log, All);

// Attempt to include Control Rig headers if available (UE may differ by version). Use __has_include guards.
#if WITH_EDITOR && (__has_include("ControlRigBlueprint.h") && __has_include("Factories/ControlRigBlueprintFactory.h"))
	#define ANGEL_HAS_CONTROLRIG 1
	#include "ControlRigBlueprint.h"
	#include "ControlRig.h"
	#include "Factories/ControlRigBlueprintFactory.h"
	#include "Kismet2/KismetEditorUtilities.h"
	#include "RigVMModel/RigVMController.h"
	#include "RigVMModel/RigVMGraph.h"
	#include "Units/Execution/RigUnit_BeginExecution.h"
#else
	#define ANGEL_HAS_CONTROLRIG 0
#endif

UBlueprint* UAngelControlRigGenerator::GenerateControlRig(
	UObject* Outer,
	const UAngelRigTemplate* Template,
	USkeleton* TargetSkeleton,
	FName ControlRigName)
{
	if (!Outer || !Template || !TargetSkeleton)
	{
		UE_LOG(LogAngelControlRigGen, Warning, TEXT("GenerateControlRig: invalid inputs."));
		return nullptr;
	}

	UPackage* Package = Outer->GetOutermost();
	UBlueprint* NewCR = nullptr;

	// Duplicate template rig if provided
	if (UBlueprint* TemplateCR = Template->ControlRigTemplate.LoadSynchronous())
	{
		NewCR = Cast<UBlueprint>(StaticDuplicateObject(TemplateCR, Package, ControlRigName));
		if (NewCR)
		{
			UE_LOG(LogAngelControlRigGen, Log, TEXT("Duplicated template Control Rig %s"), *TemplateCR->GetName());
		}
	}

#if WITH_EDITOR && ANGEL_HAS_CONTROLRIG
	if (!NewCR)
	{
		UControlRigBlueprintFactory* Factory = NewObject<UControlRigBlueprintFactory>();
		Factory->ParentClass = UControlRig::StaticClass();
		NewCR = Cast<UBlueprint>(Factory->FactoryCreateNew(UControlRigBlueprint::StaticClass(), Package, ControlRigName, RF_Public | RF_Standalone, nullptr, GWarn));
	}

	if (UControlRigBlueprint* CRBP = Cast<UControlRigBlueprint>(NewCR))
	{
#if WITH_EDITORONLY_DATA
		CRBP->SetTargetSkeleton(TargetSkeleton);
#endif
		// Add FK controls for each bone
		const FReferenceSkeleton& RefSkel = TargetSkeleton->GetReferenceSkeleton();
		for (int32 BoneIdx=0; BoneIdx<RefSkel.GetNum(); ++BoneIdx)
		{
			FName BoneName = RefSkel.GetBoneName(BoneIdx);
			if (!CRBP->GetHierarchyController()->FindBone(BoneName))
			{
				CRBP->GetHierarchyController()->AddBone(BoneName, FTransform::Identity, RefSkel.GetParentIndex(BoneIdx));
			}
			FName ControlName = FName(*FString::Printf(TEXT("CTRL_%s"), *BoneName.ToString()));
			if (!CRBP->GetHierarchyController()->FindControl(ControlName))
			{
				CRBP->GetHierarchyController()->AddControl(ControlName, ERigControlType::EulerTransform, BoneName);
			}
		}
		// Ensure begin execution node
		CRBP->GetController()->SetReportWarningsAndErrors(false);
		if (URigVMGraph* Model = CRBP->GetModel())
		{
			bool bHasBegin = false;
			for (URigVMNode* Node : Model->GetNodes())
			{
				if (Node->GetStruct() == FRigUnit_BeginExecution::StaticStruct()) { bHasBegin = true; break; }
			}
			if (!bHasBegin)
			{
				CRBP->GetController()->AddStructNode(FRigUnit_BeginExecution::StaticStruct(), TEXT("BeginExecution"));
			}
		}
		CRBP->GetController()->SetReportWarningsAndErrors(true);
		FKismetEditorUtilities::CompileBlueprint(CRBP);
	}
#else
	if (!NewCR)
	{
		// Fallback: create plain blueprint (will not be functional rig without editor module)
		NewCR = NewObject<UBlueprint>(Package, ControlRigName, RF_Public | RF_Standalone);
		UE_LOG(LogAngelControlRigGen, Warning, TEXT("Control Rig editor headers not found; created placeholder blueprint."));
	}
#endif

	if (NewCR)
	{
		FAssetRegistryModule::AssetCreated(NewCR);
		NewCR->MarkPackageDirty();
	}
	return NewCR;
}
