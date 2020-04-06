// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;
using System.Linq;

public class VPTK : ModuleRules
{
	public VPTK(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				"Runtime/MediaAssets/Public"
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"MediaAssets",
				"RenderCore",
				"RHI",
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);

		string[] files = Directory.GetFiles(Path.Combine(ModuleDirectory, "../../ThirdParty/OpenCV/Binaries/Release/Static"));
		files = files.Where(f => Path.GetExtension(f) == ".lib").ToArray();
		PublicAdditionalLibraries.AddRange(files);

		/*
		string[] files = Directory.GetFiles(Path.Combine(ModuleDirectory, "../../ThirdParty/OpenCV/Binaries/Release/Dynamic"));
		files = files.Where(f => Path.GetExtension(f) == ".dll").ToArray();
		PublicDelayLoadDLLs.AddRange(files);
		*/
	}
}
