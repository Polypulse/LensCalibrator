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
		Console.WriteLine(isDebug ? "Compiling Debug() build." : "Compiling release build.");
		
		PublicIncludePaths.AddRange(
			new string[] 
			{
				"Runtime/MediaAssets/Public",
				"Runtime/ImageWriteQueue/Public"
			}
			);
				
		
		PrivateIncludePaths.AddRange(
                new string[] 
				{
                    Path.Combine(ModuleDirectory, "../../Source/ThirdParty/OpenCV/Include")
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

		string dllName = isDebug ? "opencv_world430d.dll" : "opencv_world430.dll";
		string libName = isDebug ? "opencv_world430d.lib" : "opencv_world430.lib";

		PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, isDebug ?
			Path.Combine("../../Source/ThirdParty/OpenCV/Binaries/Debug/Static/", libName) :
			Path.Combine("../../Source/ThirdParty/OpenCV/Binaries/Release/Static/", libName)));

		PublicDelayLoadDLLs.Add(dllName);

		string binaryDir = Path.Combine(ModuleDirectory, "../../Binaries/Win64/");
		if (!Directory.Exists(binaryDir))
			Directory.CreateDirectory(binaryDir);

		string targetDLL = isDebug ?
			Path.Combine("../../Source/ThirdParty/OpenCV/Binaries/Debug/Dynamic/", dllName) :
			Path.Combine("../../Source/ThirdParty/OpenCV/Binaries/Release/Dynamic/", dllName);

		string copyPath = Path.Combine(binaryDir, dllName);

		if (!File.Exists(copyPath))
		{
			string from = Path.Combine(ModuleDirectory, targetDLL);
			File.Copy(from, copyPath, true);

			Console.WriteLine(string.Format("Copied from: \"{0}\" to: \"{1}\".", from, copyPath));
		}

		RuntimeDependencies.Add(Path.Combine(ModuleDirectory, targetDLL));

		/*
		string[] files = Directory.GetFiles(Path.Combine(ModuleDirectory, 
			isDebug ? 
			"../../Source/ThirdParty/OpenCV/Binaries/Debug/Static" : 
			"../../Source/ThirdParty/OpenCV/Binaries/Release/Static"));

		files = files.Where(f => {
			return Path.GetExtension(f) == ".lib" && !f.Contains("libpng.lib");
        }).ToArray();


		PublicAdditionalLibraries.AddRange(files);

		Console.WriteLine("Linking against libPNG provided by Unreal Engine instead of the one provided by OpenCV.");
		string libPNGPath = Path.Combine(EngineDirectory, "Source/ThirdParty/libPNG/libPNG-1.5.2/lib/Win64-llvm/Release/libpng15_static.lib");
		PublicAdditionalLibraries.Add(libPNGPath);
		*/
		/*
		foreach (string publicLib in PublicAdditionalLibraries)
			Console.WriteLine(string.Format("Including additional public library: \"{0}\".", publicLib));
        */
	}
}
