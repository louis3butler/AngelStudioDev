// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/BaseToolkit.h"
#include "AngelStudioEditorMode.h"

class IDetailsView;

/**
 * Custom toolkit panel for Angel Studio editor mode.
 */
class FAngelStudioEditorModeToolkit : public FModeToolkit
{
public:
	FAngelStudioEditorModeToolkit();

	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
	virtual void GetToolPaletteNames(TArray<FName>& PaletteNames) const override;

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;

private:
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SWidget> ToolkitRootWidget;

public:
	virtual TSharedPtr<SWidget> GetInlineContent() const override { return ToolkitRootWidget; }
};
