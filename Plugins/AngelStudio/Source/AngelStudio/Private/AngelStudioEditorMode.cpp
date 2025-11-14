// Copyright Epic Games, Inc. All Rights Reserved.

#include "AngelStudioEditorMode.h"
#include "AngelStudioEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "AngelStudioEditorModeCommands.h"
#include "Modules/ModuleManager.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
#include "Tools/AngelStudioSimpleTool.h"
#include "Tools/AngelStudioInteractiveTool.h"

// step 2: register a ToolBuilder in FAngelStudioEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "AngelStudioEditorMode"

const FEditorModeID UAngelStudioEditorMode::EM_AngelStudioEditorModeId = TEXT("EM_AngelStudioEditorMode");

FString UAngelStudioEditorMode::SimpleToolName = TEXT("AngelStudio_ActorInfoTool");
FString UAngelStudioEditorMode::InteractiveToolName = TEXT("AngelStudio_MeasureDistanceTool");


UAngelStudioEditorMode::UAngelStudioEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(UAngelStudioEditorMode::EM_AngelStudioEditorModeId,
		LOCTEXT("ModeName", "AngelStudio"),
		FSlateIcon(),
		true);
}


UAngelStudioEditorMode::~UAngelStudioEditorMode()
{
}


void UAngelStudioEditorMode::ActorSelectionChangeNotify()
{
}

void UAngelStudioEditorMode::Enter()
{
	UEdMode::Enter();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// AddYourTool Step 2 - register the ToolBuilders for your Tools here.
	// The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
	//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////// 
	const FAngelStudioEditorModeCommands& SampleToolCommands = FAngelStudioEditorModeCommands::Get();

	RegisterTool(SampleToolCommands.SimpleTool, SimpleToolName, NewObject<UAngelStudioSimpleToolBuilder>(this));
	RegisterTool(SampleToolCommands.InteractiveTool, InteractiveToolName, NewObject<UAngelStudioInteractiveToolBuilder>(this));

	// active tool type is not relevant here, we just set to default
	GetToolManager()->SelectActiveToolType(EToolSide::Left, SimpleToolName);
}

void UAngelStudioEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FAngelStudioEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UAngelStudioEditorMode::GetModeCommands() const
{
	return FAngelStudioEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
