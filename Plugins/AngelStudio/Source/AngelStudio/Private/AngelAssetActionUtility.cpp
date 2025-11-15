#include "AngelAssetActionUtility.h"
#include "Engine/StaticMesh.h"
#include "Editor.h"
#include "Selection.h"

void UAngelAssetActionUtility::AutoRigSelectedMeshes()
{
#if WITH_EDITOR
	TArray<UObject*> SelectedAssets;
	if (GEditor && GEditor->GetSelectedObjects())
	{
		GEditor->GetSelectedObjects()->GetSelectedObjects(SelectedAssets);
	}

	for (UObject* Object : SelectedAssets)
	{
		if (UStaticMesh* Mesh = Cast<UStaticMesh>(Object))
		{
			// TODO: call rigging pipeline here.
		}
	}
#endif
}
