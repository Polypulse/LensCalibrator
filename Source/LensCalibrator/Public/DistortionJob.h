#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "ILensSolverEventReceiver.h"

struct DistortionJob
{
	TScriptInterface<ILensSolverEventReceiver> eventReceiver;
	FString id;
};