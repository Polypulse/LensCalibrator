#pragma once
#include "Engine.h"
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
	FLensSolverWorkerParameters::QueueLogOutputDel * queueLogOutputDel;

	TMap<FString, TUniquePtr<FWorkerFindCornersInterfaceContainer>> findCornersWorkers;
	TMap<FString, TUniquePtr<FWorkerCalibrateInterfaceContainer>> calibrateWorkers;

	TArray<FString> workLoadSortedFindCornerWorkers;
	TArray<FString> workLoadSortedCalibrateWorkers;

	TMap<FString, FString> workerCalibrationIDLUT;

	TMap<FString, TUniquePtr<FFindCornerWorkerParameters>> jobFindCornerWorkerParameters;
	TMap<FString, TUniquePtr<FCalibrationWorkerParameters>> jobCalibrationWorkerParameters;
	TMap<FString, FJob> jobs;

	void QueueLogAsync(const FString msg);

	bool GetFindCornersContainerInterfacePtr(const FString & workerID, FWorkerFindCornersInterfaceContainer *& interfaceContainer);

	void QueueCalibrateWorkUnit(TUniquePtr<FLensSolverCalibrateWorkUnit> calibrateWorkUnit);
	void LatchCalibrateWorker(const FLatchData& latchData);

	bool GetCalibrateWorkerInterfaceContainerPtr(
		const FString& calibrationID,
		TUniquePtr<FWorkerCalibrateInterfaceContainer> *& interfaceContainerUniquePtr);
	void SortFindCornersWorkersByWorkLoad();
	void SortCalibrateWorkersByWorkLoad();

protected:
public:

	LensSolverWorkDistributor(FLensSolverWorkerParameters::QueueLogOutputDel * inputQueueLogOutputDel) : 
		queueLogOutputDel(inputQueueLogOutputDel) 
	{}

	void StartFindCornerWorkers(
		int findCornerWorkerCount);

	void StartCalibrateWorkers(
		int calibrateWorkerCount,
		FLensSolverWorkerCalibrate::QueueCalibrationResultOutputDel * inputOnSolvedPointsDel);

	void StopFindCornerWorkers();
	void StopCalibrationWorkers();
	void StopBackgroundWorkers();

	int GetFindCornerWorkerCount();
	int GetCalibrateCount();

	FJobInfo RegisterJob (int latchedWorkUnitCount, UJobType jobType);

	void QueueTextureArrayWorkUnit(const FString & jobID, TUniquePtr<FLensSolverPixelArrayWorkUnit> pixelArrayWorkUnit);
	void QueueTextureFileWorkUnit(const FString & jobID, TUniquePtr<FLensSolverTextureFileWorkUnit> textureFileWorkUnit);
};