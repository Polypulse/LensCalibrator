#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"

#include "LenssolverWorker.h"
#include "LatchData.h"
#include "LensSolverWorkUnit.h"

#include "LensSolverWorkerInterfaceContainer.generated.h"

USTRUCT()
struct FWorkerInterfaceContainer
{
	GENERATED_BODY()

	FAutoDeleteAsyncTask<FLensSolverWorker> * worker;

	FLensSolverWorker::GetWorkLoadDel getWorkLoadDel;
	FLensSolverWorker::QueueWorkUnitDel queueWorkUnitDel;
	FLensSolverWorker::IsClosingDel isClosingDel;
	FLensSolverWorker::SignalLatchDel signalLatch;
};

USTRUCT()
struct FWorkerFindCornersInterfaceContainer
{
	GENERATED_BODY()
};

USTRUCT()
struct FWorkerCalibrateInterfaceContainer
{
	GENERATED_BODY()
};
