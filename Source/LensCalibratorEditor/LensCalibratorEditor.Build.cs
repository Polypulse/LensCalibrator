// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

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
		PublicIncludePaths.AddRange(
			new string[]
			{
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
				"LensCalibrator",
				"UnrealEd"
			}
		);
	}
}
