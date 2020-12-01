/*
 * Copyright (C) 2020 - LensCalibrator contributors, see Contributors.txt at the root of the project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
