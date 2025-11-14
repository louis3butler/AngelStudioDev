// Copyright Epic Games, Inc. All Rights Reserved.

#include "AngelStudioModule.h"
#include "AngelStudioEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "AngelStudioModule"

void FAngelStudioModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FAngelStudioEditorModeCommands::Register();
}

void FAngelStudioModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FAngelStudioEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAngelStudioModule, AngelStudio)