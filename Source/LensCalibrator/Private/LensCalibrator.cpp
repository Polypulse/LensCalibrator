/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensCalibrator.h"

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine.h"

#include "LensSolver.h"

#define LOCTEXT_NAMESPACE "FLensCalibratorModule"

#define LC_STRINGIFY(x) #x
#define LC_TO_STRING(x) LC_STRINGIFY(x)

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
	GetLensSolver()->Poll();
	GetDistortionProcessor()->Poll();

	return true;
}

ULensSolver* FLensCalibratorModule::GetLensSolver()
{ 
	if (lensSolver == nullptr)
	{
		Initialize();

		static ULensSolver* staticLensSolver = NewObject<ULensSolver>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);
		lensSolver = staticLensSolver;
		lensSolver->AddToRoot(); // Prevent lens solver from being garbage collected.
		UE_LOG(LogTemp, Log, TEXT("Initialized Lens Solver."));
	}

	return lensSolver; 
}

UDistortionProcessor* FLensCalibratorModule::GetDistortionProcessor()
{ 
	if (distortionProcessor == nullptr)
	{
		Initialize();
		static UDistortionProcessor* staticDistortionProcessor = NewObject<UDistortionProcessor>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);
		distortionProcessor = staticDistortionProcessor;
		distortionProcessor->AddToRoot(); // Prevent lens solver from being garbage collected.
		UE_LOG(LogTemp, Log, TEXT("Initialized Distortion Processor."));
	}

	return distortionProcessor; 
}

void FLensCalibratorModule::StartupModule()
{
	const FString pluginName("LensCalibrator/");
	const FString openCVDLLFolder = FPaths::Combine(FPaths::ProjectPluginsDir(), pluginName, TEXT(PREPROCESSOR_TO_STRING(LENS_CALIBRATOR_OPENCV_DLL_PATH)));
	const FString dllNames = TEXT(PREPROCESSOR_TO_STRING(LENS_CALIBRATOR_OPENCV_DLL_NAMES));

	if (dllNames.IsEmpty())
	{
		UE_LOG(LogTemp, Fatal, TEXT("Preprocessor defintion \"LENS_CALIBRATOR_OPENCV_DLL_NAMES\" has 0 DLL names."));
		return;
	}

	TArray<FString> split;
	int splitCount = dllNames.ParseIntoArray(split, TEXT(" "), true);

	FPlatformProcess::PushDllDirectory(*openCVDLLFolder);
	for (int i = 0; i < split.Num(); i++)
	{
		FString dllFullPath = FPaths::Combine(openCVDLLFolder, split[i]) + ".dll";
		void * dllHandle = FPlatformProcess::GetDllHandle(*dllFullPath);
		if (dllHandle == NULL)
		{
			UE_LOG(LogTemp, Fatal, TEXT("Missing OpenCV DLL at path: \"%s\"."), *dllFullPath);
			return;
		}

		if (dllHandles.Contains(split[i]))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Attempted to load DLL handle: \"%s\" more than once"), *dllFullPath);
			return;
		}

		dllHandles.Add(split[i], dllHandle);
		UE_LOG(LogTemp, Log, TEXT("Found handle to DLL: \"%s\"."), *dllFullPath);
	}

	FPlatformProcess::PopDllDirectory(*openCVDLLFolder);

	lensSolver = nullptr;
	distortionProcessor = nullptr;

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
    AddShaderSourceDirectoryMapping("/LensCalibratorShaders", FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins/LensCalibrator/Shaders")));
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("LensCalibrator.Debug"), 0, TEXT("Output more log information for debugging."));
}

void FLensCalibratorModule::ShutdownModule()
{
	// delete GetLensSolver();
	// delete GetDistortionProcessor();
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLensCalibratorModule, LensCalibrator)