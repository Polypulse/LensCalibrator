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

#include "LensSolver.h"
#include "DistortionProcessor.h"

class FLensCalibratorModule : public IModuleInterface
{
	static constexpr auto ModuleName = TEXT("LensCalibrator");

private:

	/* Singleton references to class instances that perform the actual processing. */
	UPROPERTY(Category = "APTK")
	ULensSolver * lensSolver;
	bool lensSolverInitialized;

	UDistortionProcessor * distortionProcessor;
	bool distortionProcessorInitialized;

	// Handles to all of the loaded DLLs keyed via the DLL file name.
	TMap<FString, void*> dllHandles;

	FTickerDelegate TickDelegate;
	FDelegateHandle TickDelegateHandle;

	/* This is called when solving begins. */
	void Initialize();

	/* This Tick method is registered when the ULensSolver or UDistortionProcessor is initialized and any ticking in 
	those classes propagate from the plugin module here. */
	bool Tick(float deltatime);

	void OnPreExit();

public:
	/* Return a handle to the plugin module from anywhere. */
	static FLensCalibratorModule& Get() { return FModuleManager::GetModuleChecked<FLensCalibratorModule>(FLensCalibratorModule::ModuleName); };

	/* The plugin module maintains these singleton instances of the solver and distortion processor. */
	ULensSolver * GetLensSolver();
	UDistortionProcessor* GetDistortionProcessor();

	/* This is where the plugin is initially loaded. */
	virtual void StartupModule() override;

	/* This is where the plugin is unloaded. */
	virtual void ShutdownModule() override;
};
