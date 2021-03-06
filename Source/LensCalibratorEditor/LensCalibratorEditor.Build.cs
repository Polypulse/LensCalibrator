/*
 * Copyright (C) 2020 - LensCalibrator contributors, see Contributors.txt at the root of the project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


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
