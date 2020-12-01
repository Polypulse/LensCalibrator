/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */


using UnrealBuildTool;
using System.IO;
using System;
using System.Linq;
using System.Collections.Generic;
using System.Security.AccessControl;

public class LensCalibratorEditor : ModuleRules
{
	public LensCalibratorEditor (ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		PublicIncludePaths.AddRange(
			new string[]
			{
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"LevelEditor",
				"SlateCore",
                "Slate",
				"Engine",
                "EditorStyle",
                "AssetTools",
				"Blutility",
				"UMG",
				"UMGEditor",
                "EditorWidgets",
				"CoreUObject",
                "UnrealEd",
				"AssetRegistry"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"LensCalibrator",
			}
		);
	}
}
