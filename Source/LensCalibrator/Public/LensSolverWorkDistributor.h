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

	TMap<FString, TQueue<TWeakPtr<FLensSolverWorkerParameters::QueueWorkUnitInputDel>>> exclusiveQueues;
	TMap<FString, FJob> jobs;

	void QueueLogAsync(const FString msg);

	void SetupExclusiveCalibrateWorker(const int workerID, const FString jobID, const FString calibrationID);
	void LatchCalibrateWorker(TUniquePtr<FLensSolverCalibrateWorkUnit> calibrateWorkUnit);

	void SortFindCornersWorkersByWorkLoad();

protected:
public:

	void StartFindCornerWorkers(
		int findCornerWorkerCount,
		FLensSolverWorkerParameters::QueueLogOutputDel* inputQueueLogOutputDel);

	void StartCalibrateWorkers(
		int calibrateWorkerCount,
		FLensSolverWorkerParameters::QueueLogOutputDel* inputQueueLogOutputDel,
		FLensSolverWorkerCalibrate::QueueCalibrationResultOutputDel * inputOnSolvedPointsDel);

	void StopFindCornerWorkers();
	void StopCalibrationWorkers();
	void StopBackgroundWorkers();

	int GetFindCornerWorkerCount();
	int GetCalibrateCount();

	FJobInfo RegisterJob (int latchedWorkUnitCount, UJobType jobType);

	void QueueTextureArrayWorkUnit(TUniquePtr<FLensSolverPixelArrayWorkUnit> pixelArrayWorkUnit);
	void QueueTextureFileWorkUnit(TUniquePtr<FLensSolverTextureFileWorkUnit> textureFileWorkUnit);
};