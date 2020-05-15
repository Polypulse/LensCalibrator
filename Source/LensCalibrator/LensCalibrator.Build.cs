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
		
		
		string[] files = Directory.GetFiles(Path.Combine(ModuleDirectory, isDebug ? "../../Source/ThirdParty/OpenCV/Binaries/Debug/Static" : "../../Source/ThirdParty/OpenCV/Binaries/Release/Static"));

		files = files.Where(f => {
			return Path.GetExtension(f) == ".lib" && !f.Contains("libpng.lib");
        }).ToArray();

		PublicAdditionalLibraries.AddRange(files);

		string libPNGPath = Path.Combine(EngineDirectory, "Source/ThirdParty/libPNG/libPNG-1.5.2/lib/Win64-llvm/Release/libpng15_static.lib");
		PublicAdditionalLibraries.Add(libPNGPath);
	}
}
