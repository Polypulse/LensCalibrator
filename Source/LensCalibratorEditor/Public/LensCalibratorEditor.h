#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Modules/ModuleManager.h"
#include "Commands.h"
#include "Engine.h"
#include "SlateBasics.h"
#include "EditorStyleSet.h"

class FLensCalibratorEditorCommands : public TCommands<FLensCalibratorEditorCommands>
{
public:
	FLensCalibratorEditorCommands() : TCommands<FLensCalibratorEditorCommands>(
		TEXT("FLensCalibratorEditor"), 
		NSLOCTEXT("Contexts", "FLensCalibratorEditor", "FLensCalibratorEditor Plugin"), 
		NAME_None, 
		FEditorStyle::GetStyleSetName()) {}

	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> openLensCalibrator;
};

class FLensCalibratorEditorModule : public IModuleInterface
{
private:
	static constexpr auto ModuleName = TEXT("FLensCalibratorEditorModule");

	TSharedPtr<FUICommandList> pluginCommands;

public:
    virtual ~FLensCalibratorEditorModule() {}
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void MakeMenuEntry(FMenuBarBuilder &menuBuilder);
	void MakeMenuSubMenu(FMenuBuilder &menuBuilder);

	void OpenLensCalibrator();
};
