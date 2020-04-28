/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensCalibrator.h"
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine.h"

#define LOCTEXT_NAMESPACE "FLensCalibratorModule"

void FLensCalibratorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
    AddShaderSourceDirectoryMapping("/LensCalibratorShaders", FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins/LensCalibrator/Shaders")));
}

void FLensCalibratorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLensCalibratorModule, LensCalibrator)