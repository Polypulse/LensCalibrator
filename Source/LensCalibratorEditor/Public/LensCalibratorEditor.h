/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Modules/ModuleManager.h"
#include "Framework/Commands/Commands.h"
#include "Engine.h"
#include "SlateBasics.h"
#include "EditorStyleSet.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "AssetRegistryModule.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EditorUtilityWidget.h"
#include "UObject/Class.h"

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
	TSharedPtr<FUICommandInfo> openExampleLevel;
};

class FLensCalibratorEditorModule : public IModuleInterface
{
private:
	static constexpr auto ModuleName = TEXT("FLensCalibratorEditorModule");

	TSharedPtr<FUICommandList> pluginCommands;
	UEditorUtilityWidget* createdUMGWidget;
	UEditorUtilityWidgetBlueprint * blueprint;

public:
    virtual ~FLensCalibratorEditorModule() {}
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void MakeMenuEntry(FMenuBarBuilder &menuBuilder);
	void MakeMenuSubMenu(FMenuBuilder &menuBuilder);

	void OnMapChanged(UWorld* InWorld, EMapChangeType MapChangeType);
	void OpenLensCalibrator();
	void OpenExampleLevel();
	void OnBeginTearDownLevel(UObject * obj);
};
