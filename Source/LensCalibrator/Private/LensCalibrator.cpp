/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensCalibrator.h"
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine.h"

#include "LensSolver.h"

#define LOCTEXT_NAMESPACE "FLensCalibratorModule"

void FLensCalibratorModule::Initialize()
{
	if (TickDelegate.IsBound())
		return;

	// Create tick delegate to redirect module tick into lens solver.
	TickDelegate = FTickerDelegate::CreateRaw(this, &FLensCalibratorModule::Tick);
	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
}

bool FLensCalibratorModule::Tick(float deltatime)
{
	if (GetLensSolver().IsValid())
		GetLensSolver()->Poll();

	if (GetDistortionProcessor().IsValid())
		GetDistortionProcessor()->Poll();
	return true;
}

TSharedPtr<ULensSolver> FLensCalibratorModule::GetLensSolver()
{ 
	if (lensSolver == nullptr)
	{
		Initialize();
		static ULensSolver* staticLensSolver = NewObject<ULensSolver>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);
		lensSolver = TSharedPtr<ULensSolver>(staticLensSolver);
		lensSolver->AddToRoot(); // Prevent lens solver from being garbage collected.
	}

	return lensSolver; 
}

TSharedPtr<UDistortionProcessor> FLensCalibratorModule::GetDistortionProcessor()
{ 
	if (distortionProcessor == nullptr)
	{
		Initialize();
		static UDistortionProcessor* staticDistortionProcessor = NewObject<UDistortionProcessor>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);
		distortionProcessor = TSharedPtr<UDistortionProcessor>(staticDistortionProcessor);
		distortionProcessor->AddToRoot(); // Prevent lens solver from being garbage collected.
	}

	return distortionProcessor; 
}

void FLensCalibratorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
    AddShaderSourceDirectoryMapping("/LensCalibratorShaders", FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins/LensCalibrator/Shaders")));
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("LensCalibrator.Debug"), 0, TEXT("Output more log information for Debug()ging."));
}

void FLensCalibratorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLensCalibratorModule, LensCalibrator)