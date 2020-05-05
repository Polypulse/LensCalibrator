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

class LensSolverWorkDisttributor
{
private:
	mutable FCriticalSection threadLock;

	TArray<FWorkerFindCornersInterfaceContainer> findCornersWorkers;
	TArray<FWorkerCalibrateInterfaceContainer> calibrateWorkers;

	TMap<FString, TUniquePtr<FFindCornerWorkerParameters>> jobFindCornerWorkerParameters;
	TMap<FString, TUniquePtr<FCalibrationWorkerParameters>> jobCalibrationWorkerParameters;

	TMap<FString, FJob> jobs;
	void OnSolvedPoints(FCalibrationResult solvedPoints);

protected:
public:

	void StartBackgroundWorkers(int findCornerWorkerCount, int calibrateWorkerCount, TSharedPtr<FLensSolverWorkerCalibrate::QueueCalibrationResultOutputDel> inputOnSolvedPointsDel);
	void StopBackgroundWorkers();

	FJobInfo RegisterJob (int latchedWorkUnitCount, UJobType jobType);

	bool GetFindCornerWorkerParameters(TUniquePtr<FFindCornerWorkerParameters>& findCornerWorkerParameters);
	bool GetCalibrationWorkerParameters(TUniquePtr<FCalibrationWorkerParameters>& calibrationWorkerParameters);
};