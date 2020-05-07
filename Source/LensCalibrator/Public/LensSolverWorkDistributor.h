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
	QueueCalibrationResultOutputDel queueCalibrationResultOutputDel;
	const QueueFinishedJobOutputDel* queueFinishedJobOutputDel;
	const QueueLogOutputDel * queueLogOutputDel;

	FCalibrationParameters cachedCalibrationParameters;

	TMap<FString, FWorkerFindCornersInterfaceContainer> findCornersWorkers;
	TMap<FString, FWorkerCalibrateInterfaceContainer> calibrateWorkers;
	TArray<FString> workLoadSortedFindCornerWorkers;
	TArray<FString> workLoadSortedCalibrateWorkers;
	TMap<FString, FString> workerCalibrationIDLUT;
	TMap<FString, FJob> jobs;

	TQueue<FCalibrationResult> queuedCalibrationResults;

	void QueueLogAsync(const FString msg);

	bool GetFindCornersContainerInterfacePtr(
		const FString workerID,
		FWorkerFindCornersInterfaceContainer*& outputInterfaceContainerPtr);

	bool GetCalibrateWorkerInterfaceContainerPtr(
		const FString calibrationID,
		FWorkerCalibrateInterfaceContainer *& outputInterfaceContainerPtr);

	void SetCalibrateWorkerParameters(FCalibrationParameters calibrationParameters);
	void QueueCalibrateWorkUnit(FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit);
	void LatchCalibrateWorker(const FCalibrateLatch& latchData);
	void QueueCalibrationResult(const FCalibrationResult calibrationResult);

	bool IterateImageCount(const FString & jobID, const FString& calibrationID);

	void SortFindCornersWorkersByWorkLoad();
	void SortCalibrateWorkersByWorkLoad();

protected:
public:

	LensSolverWorkDistributor(const QueueLogOutputDel * inputQueueLogOutputDel, const QueueFinishedJobOutputDel * inputQueueFinishedJobOutputDel) :
		queueFinishedJobOutputDel(inputQueueFinishedJobOutputDel),
		queueLogOutputDel(inputQueueLogOutputDel)
	{
		queueCalibrationResultOutputDel.BindRaw(this, &LensSolverWorkDistributor::QueueCalibrationResult);
	}

	void StartFindCornerWorkers(
		int findCornerWorkerCount);

	void StartCalibrateWorkers(
		int calibrateWorkerCount);

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

	bool CalibrationResultIsQueued();
	void DequeueCalibrationResult(FCalibrationResult & calibrationResult);
};