// Copyright Epic Games, Inc. All Rights Reserved.

#include "AngelStudioEditorModeToolkit.h"
#include "AngelStudioEditorMode.h"
#include "AngelRigWizard.h"
#include "AngelStudioEditorModeCommands.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "InteractiveToolManager.h" // for GetToolManager / EToolSide
#include "Tools/EdModeInteractiveToolsContext.h"

#define LOCTEXT_NAMESPACE "AngelStudioEditorModeToolkit"

FAngelStudioEditorModeToolkit::FAngelStudioEditorModeToolkit() {}

void FAngelStudioEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);

	// Create details view (optional)
	FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args;
	Args.bUpdatesFromSelection = false;
	Args.bLockable = false;
	Args.bAllowSearch = true;
	DetailsView = PropModule.CreateDetailView(Args);

	// Build command buttons manually
	TSharedRef<SVerticalBox> CommandButtons = SNew(SVerticalBox);
	TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> CmdMap = FAngelStudioEditorModeCommands::GetCommands();
	if (TArray<TSharedPtr<FUICommandInfo>>* DefaultPalette = CmdMap.Find(NAME_Default))
	{
		for (TSharedPtr<FUICommandInfo> Cmd : *DefaultPalette)
		{
			if (Cmd.IsValid())
			{
				CommandButtons->AddSlot()
				.AutoHeight()
				.Padding(2.f)
				[
					SNew(SButton)
					.Text(Cmd->GetLabel())
					.ToolTipText(Cmd->GetDescription())
					.OnClicked_Lambda([OwningMode = InOwningMode, Cmd]()
					{
						if (OwningMode.IsValid())
						{
							if (UAngelStudioEditorMode* Mode = Cast<UAngelStudioEditorMode>(OwningMode.Get()))
							{
								if (Cmd == FAngelStudioEditorModeCommands::Get().SimpleTool)
								{
									Mode->GetToolManager()->SelectActiveToolType(EToolSide::Left, UAngelStudioEditorMode::SimpleToolName);
								}
								else if (Cmd == FAngelStudioEditorModeCommands::Get().InteractiveTool)
								{
									Mode->GetToolManager()->SelectActiveToolType(EToolSide::Left, UAngelStudioEditorMode::InteractiveToolName);
								}
							}
						}
						return FReply::Handled();
					})
				];
			}
		}
	}

	ToolkitRootWidget = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(4.f)
	[
		SNew(STextBlock).Text(LOCTEXT("AngelStudioToolsHeader", "Angel Studio"))
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(4.f)
	[
		CommandButtons
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(4.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("OpenRigWizardBtn", "Open Rig Wizard"))
		.ToolTipText(LOCTEXT("OpenRigWizardTip", "Open the Angel Rig Wizard window."))
		.OnClicked_Lambda([]()
		{
			FAngelRigWizard::OpenWindow();
			return FReply::Handled();
		})
	]
	+ SVerticalBox::Slot()
	.FillHeight(1.f)
	.Padding(4.f)
	[
		SNew(SBorder)
		.Padding(4.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				DetailsView.IsValid() ? DetailsView.ToSharedRef() : SNullWidget::NullWidget
			]
		]
	];
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
	return LOCTEXT("DisplayName", "Angel Studio Mode");
}

#undef LOCTEXT_NAMESPACE
