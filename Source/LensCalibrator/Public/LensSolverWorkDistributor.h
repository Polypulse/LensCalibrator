#pragma once
#include "LensSolverWorker.h"
#include "LensSolverWorkerFindCorners.h"
#include "LensSolverWorkerCalibrate.h"
#include "LensSolverWorkUnit.h"
#include "LensSolverWorkerInterfaceContainer.h"
#include "FindCornerWorkerParameters.h"
#include "CalibrationWorkerParameters.h"
#include "JobInfo.h"
#include "Job.h"
#include "LatchData.h"

class LensSolverWorkDistributor
{
private:
	mutable FCriticalSection threadLock;
	FLensSolverWorkerParameters::QueueLogOutputDel* queueLogOutputDel;

	TArray<TUniquePtr<FWorkerFindCornersInterfaceContainer>> findCornersWorkers;
	TArray<TUniquePtr<FWorkerCalibrateInterfaceContainer>> calibrateWorkers;

	TMap<FString, TUniquePtr<FFindCornerWorkerParameters>> jobFindCornerWorkerParameters;
	TMap<FString, TUniquePtr<FCalibrationWorkerParameters>> jobCalibrationWorkerParameters;

	TMap<FString, TQueue<TUniquePtr<FLensSolverCalibrateWorkUnit>>> exclusiveQueues;
	TMap<FString, FJob> jobs;

protected:
public:

	void StartBackgroundWorkers(
		int findCornerWorkerCount,
		int calibrateWorkerCount,
		FLensSolverWorkerParameters::QueueLogOutputDel* inputQueueLogOutputDel,
		FLensSolverWorkerCalibrate::QueueCalibrationResultOutputDel * inputOnSolvedPointsDel);

	void StopBackgroundWorkers();

	FJobInfo RegisterJob (int latchedWorkUnitCount, UJobType jobType);
	void QueueCalibrateWorkerUnit(TUniquePtr<FLensSolverCalibrateWorkUnit> calibrateWorkUnit);
};