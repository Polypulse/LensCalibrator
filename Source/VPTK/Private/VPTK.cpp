// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "VPTK.h"
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine.h"

#define LOCTEXT_NAMESPACE "FVPTKModule"

void FVPTKModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
    AddShaderSourceDirectoryMapping("/VPTKShaders", FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins/VPTK/Shaders")));
}

void FVPTKModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVPTKModule, VPTK)