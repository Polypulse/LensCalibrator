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
		Console.WriteLine(isDebug ? "Compiling debug build." : "Compiling release build.");
		
		PublicIncludePaths.AddRange(
			new string[] {
				"Runtime/MediaAssets/Public",
				"Runtime/ImageWriteQueue/Public"
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "../../ThirdParty/OpenCV/Include")
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

		string[] files = Directory.GetFiles(Path.Combine(ModuleDirectory, isDebug ? "../../ThirdParty/OpenCV/Binaries/Debug/Static" : "../../ThirdParty/OpenCV/Binaries/Release/Static"));
		files = files.Where(f => Path.GetExtension(f) == ".lib").ToArray();
		PublicAdditionalLibraries.AddRange(files);

		/*
		files = Directory.GetFiles(Path.Combine(ModuleDirectory, "../../ThirdParty/OpenCV/Binaries/Release/Dynamic"));
		files = files.Where(f => Path.GetExtension(f) == ".dll").ToArray();
		PublicDelayLoadDLLs.AddRange(files);
		*/
	}
}
