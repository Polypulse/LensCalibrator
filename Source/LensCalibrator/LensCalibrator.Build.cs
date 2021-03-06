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

public class LensCalibrator : ModuleRules
{
	const string logLabel = "[Lens Calibrator]";
	public LensCalibrator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		bool isDebug = Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT;
		Console.WriteLine(string.Format("{0}: {1}", logLabel, isDebug ? "Compiling debug build." : "Compiling release build."));

		PublicIncludePaths.AddRange(
			new string[]
			{
				Path.Combine(EngineDirectory, "Plugins/Compositing/Composure/Source/Composure/Public/CompositingElements"),
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
				"Projects",
				"Slate",
				"SlateCore",
				"MediaAssets",
				"RenderCore",
				"ImageWrapper",
				"ImageWriteQueue",
				"Composure",
				"MediaFrameworkUtilities",
				"RHI",
				"Json",
				"JsonUtilities"
			}
		);

		string[] directories = Directory.GetDirectories(Path.Combine(ModuleDirectory, "Public/"));

		PrivateIncludePaths.AddRange(
			new string[]
			{
				Path.Combine(ModuleDirectory, "../../Source/ThirdParty/OpenCVWrapper/OpenCV/Include"),
				Path.Combine(ModuleDirectory, "../../Source/ThirdParty/OpenCVWrapper/Include")
			}
		);

		if (directories.Length > 0)
			PrivateIncludePaths.AddRange(directories);

		ConfigureOpenCV(isDebug, new string[1]
		{
			"OpenCVWrapper"
		});
	}

	private void ConfigureOpenCV (bool isDebug, string[] libraries)
	{
		string dllNamesDef = "";
		for (int li = 0; li < libraries.Length; li++)
		{
			string libFolderPath = isDebug ? 
				Path.GetFullPath(Path.Combine(ModuleDirectory, 
					string.Format("../../Source/ThirdParty/{0}/Binaries/Debug/Static/", libraries[li]))) :
				Path.GetFullPath(Path.Combine(ModuleDirectory, 
					string.Format("../../Source/ThirdParty/{0}/Binaries/Release/Static/", libraries[li])));

			string dllFolderPath = isDebug ?
				Path.GetFullPath(Path.Combine(ModuleDirectory, 
					string.Format("../../Source/ThirdParty/{0}/Binaries/Debug/Dynamic/", libraries[li]))) :
				Path.GetFullPath(Path.Combine(ModuleDirectory, 
					string.Format("../../Source/ThirdParty/{0}/Binaries/Release/Dynamic/", libraries[li])));

			string targetDLLFolderPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Source/ThirdParty/OpenCVWrapper/Win64/"));

			string[] libFiles = Directory.GetFiles(libFolderPath).Where(f => Path.GetExtension(f) == ".lib").ToArray();
			string[] dllFiles = Directory.GetFiles(dllFolderPath).Where(f => Path.GetExtension(f) == ".dll").ToArray();

			if (libFiles.Length == 0)
				throw new Exception(string.Format("{0}: Missing libraries at path: \"{1}\".", libFolderPath));

			if (dllFiles.Length == 0)
				throw new Exception(string.Format("{0}: Missing libraries at path: \"{1}\".", dllFolderPath));

			for (int i = 0; i < libFiles.Length; i++)
			{
				string libFileName = libFiles[i];
				string libFullPath = Path.Combine(libFolderPath, libFileName);

				Console.WriteLine(string.Format("{0}: Registering library for linking: \"{1}\".", logLabel, libFullPath));
				PublicAdditionalLibraries.Add(libFullPath);
			}

			for (int i = 0; i < dllFiles.Length; i++)
			{
				string dllFullPath = dllFiles[i];
				string dllFileName = Path.GetFileName(dllFullPath);
				string targetDLLFullPath = Path.Combine(targetDLLFolderPath, dllFileName);

				if (!Directory.Exists(targetDLLFolderPath))
				{
					Console.WriteLine(string.Format("{0}: Creating directory at path: \"{1}\".", logLabel, targetDLLFolderPath));
					Directory.CreateDirectory(targetDLLFolderPath);
				}

				if (!File.Exists(targetDLLFullPath) || File.GetLastWriteTime(targetDLLFullPath) < File.GetLastWriteTime(dllFullPath))
				{
					Console.WriteLine(string.Format("{0}: Copying DLL from: \"{1}\" to: \"{2}\".", logLabel, dllFullPath, targetDLLFullPath));

					if (!File.Exists(dllFullPath))
						throw new Exception(string.Format("{0}: Missing DLL at path: \"{1}\".", dllFullPath));

					File.Copy(dllFullPath, targetDLLFullPath, true);
				}

				Console.WriteLine(string.Format("{0}: Registering runtime DLL: \"{1}\".", logLabel, targetDLLFullPath));
				PublicDelayLoadDLLs.Add(dllFileName);

				if (i < dllFiles.Length - 1)
					dllNamesDef += Path.GetFileNameWithoutExtension(dllFileName) + " ";
				else if (dllFiles.Length == 1)
					dllNamesDef += " " + Path.GetFileNameWithoutExtension(dllFileName);
				else dllNamesDef += Path.GetFileNameWithoutExtension(dllFileName);

				Console.WriteLine(string.Format("{0}: Registering runtime dependency: \"{1}\".", logLabel, targetDLLFullPath));
				RuntimeDependencies.Add(targetDLLFullPath);
			}
		}

		/* (IMPORTANT) These allow us to define where the third party DLLs are. */
		System.Collections.Generic.Dictionary<string, string> definitions = new System.Collections.Generic.Dictionary<string, string>()
		{
			{ "LENS_CALIBRATOR_OPENCV_DLL_PATH", "Source/ThirdParty/OpenCVWrapper/Win64/" },
			{ "LENS_CALIBRATOR_OPENCV_DLL_NAMES", dllNamesDef },
		};

		string msg = string.Format("{0}: Setting definitions:\n{{\n", logLabel);
		foreach (KeyValuePair<string, string> definition in definitions)
		{
			PublicDefinitions.Add(string.Format("{0}={1}", definition.Key, definition.Value));
			msg += string.Format("\t{0} = {1}\n", definition.Key, definition.Value);
		}

		Console.WriteLine(string.Format("{0}\n}}", msg));
	}
}
