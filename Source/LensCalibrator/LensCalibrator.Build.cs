// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;
using System.Linq;

public class LensCalibrator : ModuleRules
{
	public LensCalibrator(ReadOnlyTargetRules Target) : base(Target)
	{
		// PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bool isDebug = Target.Configuration == UnrealTargetConfiguration.Debug;

		string logLabel = "[Lens Calibrator]";
		Console.WriteLine(string.Format("{0}: {1}", logLabel, isDebug ? "Compiling debug build." : "Compiling release build."));

		PublicIncludePaths.AddRange(
			new string[]
			{
				"Runtime/MediaAssets/Public",
				"Runtime/ImageWriteQueue/Public"
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"MediaAssets",
				"RenderCore",
				"ImageWrapper",
				"ImageWriteQueue",
				"RHI",
				"Json",
				"JsonUtilities"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
			{
				Path.Combine(ModuleDirectory, "../../Source/ThirdParty/OpenCV/Include")
			}
		);

		string libFolderPath = isDebug ? 
			Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Source/ThirdParty/OpenCV/Binaries/Debug/Static/")) :
			Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Source/ThirdParty/OpenCV/Binaries/Release/Static/"));

		string dllFolderPath = isDebug ? 
			Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Source/ThirdParty/OpenCV/Binaries/Debug/Dynamic/")) :
			Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Source/ThirdParty/OpenCV/Binaries/Release/Dynamic/"));

		string libFileName = isDebug ? "opencv_world430d.lib" : "opencv_world430.lib"; 
		string dllFileName = isDebug ? "opencv_world430d.dll" : "opencv_world430.dll";

		string libFullPath = Path.Combine(libFolderPath, libFileName);
		string dllFullPath = Path.Combine(dllFolderPath, dllFileName);

		Console.WriteLine(string.Format("{0}: Registering library for linking: \"{1}\".", logLabel, libFullPath));
		PublicAdditionalLibraries.Add(libFullPath);

		Console.WriteLine(string.Format("{0}: Registering runtime DLL: \"{1}\".", logLabel, dllFullPath));
		PublicDelayLoadDLLs.Add(dllFullPath);

		Console.WriteLine(string.Format("{0}: Registering runtime dependency: \"{1}\".", logLabel, dllFullPath));
		RuntimeDependencies .Add(dllFullPath);
	}
}
