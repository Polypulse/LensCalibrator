/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */


#include "LensCalibrator.h"

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine.h"

#include "LensSolver.h"
#include "MatQueueWriter.h"
#include "WorkerRegistry.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FLensCalibratorModule"

#define LC_STRINGIFY(x) #x
#define LC_TO_STRING(x) LC_STRINGIFY(x)

/* This sets up the tick and exit delegates */
void FLensCalibratorModule::Initialize()
{
	if (TickDelegate.IsBound())
		return;

	// Create tick delegate to redirect module tick into lens solver.
	TickDelegate = FTickerDelegate::CreateRaw(this, &FLensCalibratorModule::Tick);
	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(TickDelegate);
	FCoreDelegates::OnPreExit.AddRaw(this, &FLensCalibratorModule::OnPreExit);
}

/* When calibration starts, this is called every frame. */
bool FLensCalibratorModule::Tick(float deltatime)
{
	GetLensSolver()->Poll();
	GetDistortionProcessor()->Poll();

	return true;
}

void FLensCalibratorModule::OnPreExit()
{
	/* Flag to any worker threads that are still running that they should shutdown.*/
	WorkerRegistry::Get().FlagExitAllShutdown();
}

/* Get a reference to the lens solver instance. */
ULensSolver* FLensCalibratorModule::GetLensSolver()
{ 
	/* If the lens solver instance does not exist yet, create it. */
	if (lensSolver == nullptr)
	{
		Initialize();

		static ULensSolver* staticLensSolver = NewObject<ULensSolver>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);
		lensSolver = staticLensSolver;
		lensSolver->AddToRoot(); // Prevent lens solver from being garbage collected.

		lensSolverInitialized = true;

		UE_LOG(LogTemp, Log, TEXT("Initialized Lens Solver."));
	}

	return lensSolver; 
}

/* Get a reference to the distortion processor instance. */
UDistortionProcessor* FLensCalibratorModule::GetDistortionProcessor()
{ 
	/* If the distortion processor instance does not exist yet, create it. */
	if (distortionProcessor == nullptr)
	{
		Initialize();

		static UDistortionProcessor* staticDistortionProcessor = NewObject<UDistortionProcessor>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);
		distortionProcessor = staticDistortionProcessor;
		distortionProcessor->AddToRoot(); // Prevent lens solver from being garbage collected.

		distortionProcessorInitialized = true;

		UE_LOG(LogTemp, Log, TEXT("Initialized Distortion Processor."));
	}

	return distortionProcessor; 
}

// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
void FLensCalibratorModule::StartupModule()
{
	// Build the plugin directory path.
	FString pluginDir = IPluginManager::Get().FindPlugin(TEXT("LensCalibrator"))->GetBaseDir();

	// Get paths and DLL file names from preprocessor declarations, these get defined in the LensCalibrator.Build.cs file during the build process.
	const FString openCVDLLFolder = FPaths::Combine(pluginDir, TEXT(PREPROCESSOR_TO_STRING(LENS_CALIBRATOR_OPENCV_DLL_PATH)));
	const FString dllNames = TEXT(PREPROCESSOR_TO_STRING(LENS_CALIBRATOR_OPENCV_DLL_NAMES));

	// Something went wrong, check the LensCalibrator.Build.cs
	if (dllNames.IsEmpty())
	{
		UE_LOG(LogTemp, Fatal, TEXT("Preprocessor defintion \"LENS_CALIBRATOR_OPENCV_DLL_NAMES\" has 0 DLL names."));
		return;
	}

	// The DLL file names are packed into a single preprocessor variable, so we need to split them apart.
	TArray<FString> split;
	int splitCount = dllNames.ParseIntoArray(split, TEXT(" "), true);

	// To load a DLL, we need to first tell the process that we are going to load DLLs from this directory.
	FPlatformProcess::PushDllDirectory(*openCVDLLFolder);

	// Loop through all the DLL file names.
	for (int i = 0; i < split.Num(); i++)
	{
		// Get a DLL handle via the full DLL path.
		FString dllFullPath = FPaths::Combine(openCVDLLFolder, split[i]) + ".dll";
		void * dllHandle = FPlatformProcess::GetDllHandle(*dllFullPath); // Load via absolute path.

		// Something is missing.
		if (dllHandle == NULL)
		{
			UE_LOG(LogTemp, Fatal, TEXT("Missing OpenCV DLL at path: \"%s\"."), *dllFullPath);
			return;
		}

		// Just in case we've already loaded it for some reason, the preprocessor could potentially contain duplicate DLL file names.
		if (dllHandles.Contains(split[i]))
		{
			UE_LOG(LogTemp, Fatal, TEXT("Attempted to load DLL handle: \"%s\" more than once"), *dllFullPath);
			return;
		}

		// Save the handle to a map keyed with the DLL file name.
		dllHandles.Add(split[i], dllHandle);
		UE_LOG(LogTemp, Log, TEXT("Found handle to DLL: \"%s\"."), *dllFullPath);
	}

	// We don't need the process to know about this DLL folder since we now have handles to those DLLs.
	FPlatformProcess::PopDllDirectory(*openCVDLLFolder);

	lensSolver = nullptr;
	distortionProcessor = nullptr;

	// In order to refer to our shaders throughout the the plugin's logic, we need to map the shader folder directory.
    AddShaderSourceDirectoryMapping("/LensCalibratorShaders", FPaths::Combine(pluginDir, TEXT("Shaders")));

	// Debug the plugin.
	IConsoleManager::Get().RegisterConsoleVariable(TEXT("LensCalibrator.Debug"), 0, TEXT("Output more log information for debugging."));
}

// We don't currently unreference the handles to our DLLs, maybe we should?
void FLensCalibratorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLensCalibratorModule, LensCalibrator)