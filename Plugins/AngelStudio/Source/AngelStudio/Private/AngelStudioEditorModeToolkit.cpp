// Copyright Epic Games, Inc. All Rights Reserved.

#include "AngelStudioEditorModeToolkit.h"
#include "AngelStudioEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "AngelStudioEditorModeToolkit"

FAngelStudioEditorModeToolkit::FAngelStudioEditorModeToolkit()
{
}

void FAngelStudioEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FAngelStudioEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FAngelStudioEditorModeToolkit::GetToolkitFName() const
{
	return FName("AngelStudioEditorMode");
}

FText FAngelStudioEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "AngelStudioEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
