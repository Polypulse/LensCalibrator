#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Engine.h"

#include "LensSolverWorker.h"
#include "LensSolverWorkerCalibrate.h"
#include "LensSolverWorkerFindCorners.h"
#include "LatchData.h"
#include "LensSolverWorkUnit.h"

#include "LensSolverWorkerInterfaceContainer.generated.h"

USTRUCT()
struct FWorkerInterfaceContainer
{
	GENERATED_BODY()

	TUniquePtr<FAutoDeleteAsyncTask<FLensSolverWorker>> worker;
	FString workerID;

	FLensSolverWorkerParameters::GetWorkLoadOutputDel getWorkLoadDel;
	FLensSolverWorkerParameters::IsClosingOutputDel isClosingDel;
};

USTRUCT()
struct FWorkerFindCornersInterfaceContainer
{
	GENERATED_BODY()
	FWorkerInterfaceContainer baseContainer;

	FLensSolverWorkerFindCorners::QueueFindCornerResultOutputDel queueFindCornerResultOutputDel;
	FLensSolverWorkerFindCorners::QueuePixelArrayWorkUnitInputDel queuePixelArrayWorkUnitInputDel;
	FLensSolverWorkerFindCorners::QueueTextureFileWorkUnitInputDel queueTextureFileWorkUnitInputDel;
};

USTRUCT()
struct FWorkerCalibrateInterfaceContainer
{
	GENERATED_BODY()
	FWorkerInterfaceContainer baseContainer;

	FLensSolverWorkerCalibrate::QueueLatchInputDel signalLatch;
	FLensSolverWorkerCalibrate::QueueCalibrateWorkUnitInputDel queueCalibrateWorkUnitDel;
};
