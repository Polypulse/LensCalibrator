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

	UPROPERTY(Category = "APTK")
	ULensSolver * lensSolver;
	UDistortionProcessor * distortionProcessor;

	void * openCVDLLHandle;

	FTickerDelegate TickDelegate;
	FDelegateHandle TickDelegateHandle;

	void Initialize();
	bool Tick(float deltatime);

public:
	static FLensCalibratorModule& Get() { return FModuleManager::GetModuleChecked<FLensCalibratorModule>(FLensCalibratorModule::ModuleName); };
	ULensSolver * GetLensSolver();
	UDistortionProcessor* GetDistortionProcessor();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
