/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "ILensSolverEventReceiver.h"

/* Distortion map generation job data. */
struct DistortionJob
{
	/* The blueprint class that implements ILensSolverEventReceiver, the reference is stored to 
	this class in order to call methods throughout distortion map generation. */
	TScriptInterface<ILensSolverEventReceiver> eventReceiver;

	/* Job ID. */
	FString id;
};