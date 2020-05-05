#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"

#include "LensSolverWorker.h"
#include "LensSolverWorkerCalibrate.h"
#include "LensSolverWorkerFindcorners.h"
#include "LatchData.h"
#include "LensSolverWorkUnit.h"

#include "LensSolverWorkerInterfaceContainer.generated.h"

USTRUCT()
struct FWorkerInterfaceContainer
{
	GENERATED_BODY()

	FAutoDeleteAsyncTask<FLensSolverWorker> * worker;

	FLensSolverWorkerParameters::QueueLogOutputDel queueLogOutputDel;
	FLensSolverWorkerParameters::GetWorkLoadOutputDel getWorkLoadDel;
	FLensSolverWorkerParameters::QueueWorkUnitInputDel queueWorkUnitDel;
	FLensSolverWorkerParameters::IsClosingOutputDel isClosingDel;
};

USTRUCT()
struct FWorkerFindCornersInterfaceContainer
{
	GENERATED_BODY()
	FLensSolverWorkerFindCorners::QueueFindCornerResultOutputDel queueFindCornerResultOutputDel;
};

USTRUCT()
struct FWorkerCalibrateInterfaceContainer
{
	GENERATED_BODY()
	FLensSolverWorkerCalibrate::QueueLatchInputDel signalLatch;
};
