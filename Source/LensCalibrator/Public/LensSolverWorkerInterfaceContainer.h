#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Engine.h"

#include "LensSolverWorker.h"
#include "LensSolverWorkerCalibrate.h"
#include "LensSolverWorkerFindCorners.h"
#include "LatchData.h"
#include "LensSolverWorkUnit.h"

// #include "LensSolverWorkerInterfaceContainer.generated.h"

struct FWorkerInterfaceContainer
{
	FString workerID;

	GetWorkLoadOutputDel getWorkLoadDel;
	IsClosingOutputDel isClosingDel;
};

struct FWorkerFindCornersInterfaceContainer
{
	FAutoDeleteAsyncTask<FLensSolverWorkerFindCorners> * worker;
	FWorkerInterfaceContainer baseContainer;

	QueuePixelArrayWorkUnitInputDel queuePixelArrayWorkUnitInputDel;
	QueueTextureFileWorkUnitInputDel queueTextureFileWorkUnitInputDel;
};

struct FWorkerCalibrateInterfaceContainer
{
	FAutoDeleteAsyncTask<FLensSolverWorkerCalibrate> * worker;
	FWorkerInterfaceContainer baseContainer;

	QueueLatchInputDel signalLatch;
	QueueCalibrateWorkUnitInputDel queueCalibrateWorkUnitDel;
};
