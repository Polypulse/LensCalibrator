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
	const QueueLogOutputDel * queueLogOutputDel;

	TMap<FString, FWorkerFindCornersInterfaceContainer> findCornersWorkers;
	TMap<FString, FWorkerCalibrateInterfaceContainer> calibrateWorkers;

	TArray<FString> workLoadSortedFindCornerWorkers;
	TArray<FString> workLoadSortedCalibrateWorkers;

	TMap<FString, FString> workerCalibrationIDLUT;

	TMap<FString, TUniquePtr<FFindCornerWorkerParameters>> jobFindCornerWorkerParameters;
	TMap<FString, TUniquePtr<FCalibrationWorkerParameters>> jobCalibrationWorkerParameters;
	TMap<FString, FJob> jobs;

	void QueueLogAsync(const FString msg);

	bool GetFindCornersContainerInterfacePtr(
		const FString& workerID,
		FWorkerFindCornersInterfaceContainer*& outputInterfaceContainerPtr);

	bool GetCalibrateWorkerInterfaceContainerPtr(
		const FString& calibrationID,
		FWorkerCalibrateInterfaceContainer *& outputInterfaceContainerPtr);

	void QueueCalibrateWorkUnit(FLensSolverCalibrateWorkUnit calibrateWorkUnit);
	void LatchCalibrateWorker(const FLatchData& latchData);

	bool IterateImageCount(const FString & jobID, const FString& calibrationID);

	void SortFindCornersWorkersByWorkLoad();
	void SortCalibrateWorkersByWorkLoad();

protected:
public:

	LensSolverWorkDistributor(const QueueLogOutputDel * inputQueueLogOutputDel) :
		queueLogOutputDel(inputQueueLogOutputDel)
	{
	}

	void StartFindCornerWorkers(
		int findCornerWorkerCount);

	void StartCalibrateWorkers(
		int calibrateWorkerCount,
		QueueCalibrationResultOutputDel * inputOnSolvedPointsDel);

	void StopFindCornerWorkers();
	void StopCalibrationWorkers();
	void StopBackgroundWorkers();

	int GetFindCornerWorkerCount();
	int GetCalibrateCount();

	FJobInfo RegisterJob (
		const TArray<int> & expectedImageCounts,
		const int expectedResultCount,
		const UJobType jobType);

	void QueueTextureArrayWorkUnit(const FString & jobID, FLensSolverPixelArrayWorkUnit pixelArrayWorkUnit);
	void QueueTextureFileWorkUnit(const FString & jobID, FLensSolverTextureFileWorkUnit textureFileWorkUnit);
};