/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "ILensSolverEventReceiver.h"

struct DistortionJob
{
	TScriptInterface<ILensSolverEventReceiver> eventReceiver;
	FString id;
};