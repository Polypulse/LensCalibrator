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
	FCriticalSection threadLock;

	QueueCalibrationResultOutputDel queueCalibrationResultOutputDel;
	QueueCalibrateWorkUnitInputDel queueCalibrateWorkUnitInputDel;
	// IsFenceDownDel isFenceDownDel;

	// LockDel lockDel;
	// UnlockDel unlockDel;

	const QueueFinishedJobOutputDel* queueFinishedJobOutputDel;
	const QueueLogOutputDel * queueLogOutputDel;
	const bool debug;

	// FThreadSafeBool fenceUp;

	FCalibrationParameters cachedCalibrationParameters;

	TMap<FString, FWorkerFindCornersInterfaceContainer> findCornersWorkers;
	TMap<FString, FWorkerCalibrateInterfaceContainer> calibrateWorkers;
	TArray<FString> workLoadSortedFindCornerWorkers;
	TArray<FString> workLoadSortedCalibrateWorkers;
	TMap<FString, FString> workerCalibrationIDLUT;
	TMap<FString, FJob> jobs;

	TQueue<FCalibrationResult> queuedCalibrationResults;

	void QueueLogAsync(FString msg);

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

	void Lock();
	void Unlock();

	// bool IsFenceDown();

protected:
public:

	/*
	static FString ExpectedAndCurrentImageCountToString(const TMap<FString, FExpectedAndCurrentImageCount> & map, const int tabCount);
	static FString FJobInfoToString(const FJobInfo& job, const int tabCount = 0);
	static FString FJobToString(const FJob& job, const int tabcount = 0);
	*/

	LensSolverWorkDistributor(const QueueLogOutputDel* inputQueueLogOutputDel, const QueueFinishedJobOutputDel* inputQueueFinishedJobOutputDel, bool debugEnabled = false) :
		queueFinishedJobOutputDel(inputQueueFinishedJobOutputDel),
		queueLogOutputDel(inputQueueLogOutputDel),
		debug(debugEnabled)
	{
	}

	void PrepareFindCornerWorkers(
		int findCornerWorkerCount);
	void PrepareCalibrateWorkers(
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

	// void SetFenceDown();
};