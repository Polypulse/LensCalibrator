// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;
using System.Linq;

public class LensCalibrator : ModuleRules
{
	public LensCalibrator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bool isDebug = Target.Configuration == UnrealTargetConfiguration.Debug;
		Console.WriteLine(isDebug ? "Compiling Debug() build." : "Compiling release build.");
		
		PublicIncludePaths.AddRange(
			new string[] {
				"Runtime/MediaAssets/Public",
				"Runtime/ImageWriteQueue/Public"
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
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
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);

		string[] files = Directory.GetFiles(Path.Combine(ModuleDirectory, isDebug ? "../../Source/ThirdParty/OpenCV/Binaries/Debug/Static" : "../../Source/ThirdParty/OpenCV/Binaries/Release/Static"));
		files = files.Where(f => Path.GetExtension(f) == ".lib").ToArray();
		files = files.Where(s => !s.Contains("libpng.lib")).ToArray();
		for (int i = 0; i < files.Length; i++)
			Console.WriteLine(string.Format("Including lib: {0}", files[i]));
		PublicAdditionalLibraries.AddRange(files);

		string libPNGPath = Path.Combine(EngineDirectory, "Source/ThirdParty/libPNG/libPNG-1.5.2/lib/Win64/VS2015/libpng_64.lib");
		PublicAdditionalLibraries.Add(libPNGPath);

		/*
		files = Directory.GetFiles(Path.Combine(ModuleDirectory, "../../ThirdParty/OpenCV/Binaries/Release/Dynamic"));
		files = files.Where(f => Path.GetExtension(f) == ".dll").ToArray();
		PublicDelayLoadDLLs.AddRange(files);
		*/
	}
}
