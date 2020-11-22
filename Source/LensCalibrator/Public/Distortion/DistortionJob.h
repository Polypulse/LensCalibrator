/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
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