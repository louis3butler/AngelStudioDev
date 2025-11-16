#include "AngelAssetActionUtility.h"
#include "Engine/StaticMesh.h"
#include "Editor.h"
#include "Selection.h"
#include "AngelBatchRigProcessor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AngelRigTemplate.h"

void UAngelAssetActionUtility::AutoRigSelectedMeshes()
{
#if WITH_EDITOR
	TArray<UStaticMesh*> Meshes;
	if (GEditor && GEditor->GetSelectedObjects())
	{
		USelection* Sel = GEditor->GetSelectedObjects();
		for (int32 i = 0; i < Sel->Num(); ++i)
		{
			if (UStaticMesh* Mesh = Cast<UStaticMesh>(Sel->GetSelectedObject(i)))
			{
				Meshes.Add(Mesh);
			}
		}
	}

	if (Meshes.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AngelStudio: No StaticMesh selected for batch rigging."));
		return;
	}

	// Find a rig template (first found)
	UAngelRigTemplate* Template = nullptr;
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> FoundAssets;
		AssetRegistryModule.Get().GetAssetsByClass(UAngelRigTemplate::StaticClass()->GetClassPathName(), FoundAssets);
		for (const FAssetData& Data : FoundAssets)
		{
			if (UAngelRigTemplate* T = Cast<UAngelRigTemplate>(Data.GetAsset()))
			{
				Template = T;
				break;
			}
		}
		if (!Template)
		{
			for (TObjectIterator<UAngelRigTemplate> It; It; ++It)
			{
				Template = *It;
				break;
			}
		}
	}

	if (!Template)
	{
		UE_LOG(LogTemp, Warning, TEXT("AngelStudio: No rig template available for batch rigging."));
		return;
	}

	TArray<FAngelBatchRigItem> Items;
	for (UStaticMesh* M : Meshes)
	{
		FAngelBatchRigItem Itm;
		Itm.Mesh = M;
		Items.Add(Itm);
	}

	UAngelBatchRigProcessor* Processor = NewObject<UAngelBatchRigProcessor>();
	Processor->OnProgress.AddLambda([](float Pct)
	{
		UE_LOG(LogTemp, Log, TEXT("AngelStudio Batch Progress: %.0f%%"), Pct * 100.f);
	});
	Processor->RunBatch(Items, Template);
#endif
}
