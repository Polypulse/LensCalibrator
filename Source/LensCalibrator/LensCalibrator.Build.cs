// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;
using System.Linq;

public class LensCalibrator : ModuleRules
{
	public LensCalibrator(ReadOnlyTargetRules Target) : base(Target)
	{
		bool isDebug = Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT;

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

		string targetDLLFolderPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Binaries/ThirdParty/Win64/")); 

		string libFileName = isDebug ? "opencv_world430d.lib" : "opencv_world430.lib"; 
		string dllFileName = isDebug ? "opencv_world430d.dll" : "opencv_world430.dll";

		string libFullPath = Path.Combine(libFolderPath, libFileName);

		string dllFullPath = Path.Combine(dllFolderPath, dllFileName);
		string targetDLLFullPath = Path.Combine(targetDLLFolderPath, dllFileName);

		if (!Directory.Exists(targetDLLFolderPath))
		{
			Console.WriteLine(string.Format("{0}: Creating directory at path: \"{1}\".", logLabel, targetDLLFolderPath));
			Directory.CreateDirectory(targetDLLFolderPath);
		}

		if (!File.Exists(targetDLLFullPath))
		{
			Console.WriteLine(string.Format("{0}: Copying DLL from: \"{1}\" to: \"{2}\".", logLabel, dllFullPath, targetDLLFullPath));

			if (!File.Exists(dllFullPath))
				throw new Exception(string.Format("{0}: Missing DLL at path: \"{1}\".", dllFullPath));

			File.Copy(dllFullPath, targetDLLFullPath, true);
		}

		Console.WriteLine(string.Format("{0}: Registering library for linking: \"{1}\".", logLabel, libFullPath));
		PublicAdditionalLibraries.Add(libFullPath);

		Console.WriteLine(string.Format("{0}: Registering runtime DLL: \"{1}\".", logLabel, targetDLLFullPath));
		PublicDelayLoadDLLs.Add(dllFileName);

		Console.WriteLine(string.Format("{0}: Registering runtime dependency: \"{1}\".", logLabel, targetDLLFullPath));
		RuntimeDependencies.Add(targetDLLFullPath);

		PublicDefinitions.Add("LENS_CALIBRATOR_OPENCV_DLL_PATH=" + "Binaries/ThirdParty/Win64/");
		PublicDefinitions.Add("LENS_CALIBRATOR_OPENCV_DLL_NAME=" + dllFileName);
	}
}
