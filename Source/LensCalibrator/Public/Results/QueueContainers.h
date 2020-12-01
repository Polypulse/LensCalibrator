/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */


#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "ILensSolverEventReceiver.h"
#include "LensSolverWorkUnit.h"
#include "JobInfo.h"

struct CalibrationResultQueueContainer
{
	TScriptInterface<ILensSolverEventReceiver> eventReceiver;
	FCalibrationResult calibrationResult;
};

struct FinishedJobQueueContainer
{
	TScriptInterface<ILensSolverEventReceiver> eventReceiver;
	FJobInfo jobInfo;
};

