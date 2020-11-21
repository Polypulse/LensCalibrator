/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
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
