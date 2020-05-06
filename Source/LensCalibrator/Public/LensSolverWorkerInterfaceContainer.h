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
	FLensSolverWorkerParameters::QueueWorkUnitInputDel queueWorkUnitDel;
	FLensSolverWorkerParameters::IsClosingOutputDel isClosingDel;

	virtual ~FWorkerInterfaceContainer () {}
};

USTRUCT()
struct FWorkerFindCornersInterfaceContainer : public FWorkerInterfaceContainer
{
	GENERATED_BODY()
	FLensSolverWorkerFindCorners::QueueFindCornerResultOutputDel queueFindCornerResultOutputDel;
	~FWorkerFindCornersInterfaceContainer () {}
};

USTRUCT()
struct FWorkerCalibrateInterfaceContainer : public FWorkerInterfaceContainer
{
	GENERATED_BODY()
	FLensSolverWorkerCalibrate::QueueLatchInputDel signalLatch;
	~FWorkerCalibrateInterfaceContainer () {}
};
