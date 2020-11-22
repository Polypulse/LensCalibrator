/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once
#include "Engine.h"

#include "LensSolverWorker.h"
#include "LensSolverWorkerFindCorners.h"
#include "LensSolverWorkerCalibrate.h"
#include "LensSolverWorkUnit.h"
#include "LensSolverWorkerInterfaceContainer.h"
#include "FindCornerWorkerParameters.h"
#include "CalibrationWorkerParameters.h"
#include "MediaAssets/Public/MediaTexture.h"
#include "MediaAssets/Public/MediaPlayer.h"
#include "JobInfo.h"
#include "Job.h"
#include "QueueContainers.h"
#include "ILensSolverEventReceiver.h"

/* This is really where the bulk of the work preparation and distribution occurs for the workers, from blueprints data is feed in
and this class prepares all the work units, manages the workers and receives the results from the calibration. This class follows
the singleton pattern, so there should only be one throughout the lifetime of the UE4 instance. */
class LensSolverWorkDistributor
{
private:
	LensSolverWorkDistributor() {}

	FCriticalSection threadLock;

	QueueCalibrationResultOutputDel queueCalibrationResultOutputDel;
	QueueCalibrateWorkUnitInputDel queueCalibrateWorkUnitInputDel;

	QueueFinishedJobOutputDel queueFinishedJobOutputDel;
	QueueLogOutputDel queueLogOutputDel;
	bool Debug();

	FCalibrationParameters cachedCalibrationParameters;

	FQueuedThreadPool * threadPool;
	TMap<FString, FWorkerFindCornersInterfaceContainer> findCornersWorkers;
	TMap<FString, FWorkerCalibrateInterfaceContainer> calibrateWorkers;
	bool shutDownWorkersAfterCompletedTasks;

	TArray<FString> workLoadSortedFindCornerWorkers;
	TArray<FString> workLoadSortedCalibrateWorkers;

	TMap<FString, FJob> jobs;
	TMap<FString, const FString> workerCalibrationIDLUT;
	TMap<FString, FMediaStreamWorkUnit> mediaTextureJobLUT;

	TQueue<CalibrationResultQueueContainer, EQueueMode::Mpsc> queuedCalibrationResults;

	void QueueLogAsync(FString msg);

	bool GetFindCornersContainerInterfacePtr(
		const FString workerID,
		FWorkerFindCornersInterfaceContainer*& outputInterfaceContainerPtr);

	bool GetCalibrateWorkerInterfaceContainerPtr(
		const FString calibrationID,
		FWorkerCalibrateInterfaceContainer *& outputInterfaceContainerPtr);

	void QueueCalibrateWorkUnit(FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit);
	void LatchCalibrateWorker(const FCalibrateLatch& latchData);
	void QueueCalibrationResult(const FCalibrationResult calibrationResult);

	bool IterateImageCount(const FString & jobID, const FString& calibrationID);

	void SortFindCornersWorkersByWorkLoad();
	void SortCalibrateWorkersByWorkLoad();
	void PollShutdownFindCornerWorkersIfNecessary();
	void PollShutdownAllWorkersIfNecessary();

	bool ValidateMediaTexture(const UMediaTexture* inputTexture);

	void PrepareFindCornerWorkers(
		int findCornerWorkerCount);
	void PrepareCalibrateWorkers(
		int calibrateWorkerCount);

	void StopFindCornerWorkers();
	void StopCalibrationWorkers();

	void Lock();
	void Unlock();

	int64 GetTickNow();
	void MediaTextureRenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FMediaStreamWorkUnit mediaStreamParameters);

protected:
public:

	/* Get instance of class (THERE CAN ONLY BE ONE) */
	static LensSolverWorkDistributor& GetInstance()
	{
		static LensSolverWorkDistributor workDistributor;
		return workDistributor;
	}

	LensSolverWorkDistributor(LensSolverWorkDistributor const&) = delete;
	void operator=(LensSolverWorkDistributor const&) = delete;

	void Configure(
		QueueLogOutputDel*& inputQueueLogOutputDel,
		QueueFinishedJobOutputDel*& inputQueueFinishedJobOutputDel,
		bool inputShutDownWorkersAfterCompletedTasks)
	{
		inputQueueFinishedJobOutputDel = &queueFinishedJobOutputDel;
		inputQueueLogOutputDel = &queueLogOutputDel;
		shutDownWorkersAfterCompletedTasks = inputShutDownWorkersAfterCompletedTasks;
	}

	void PrepareWorkers(
		int findCornerWorkerCount,
		int calibrateWorkerCount
	);

	void StopBackgroundWorkers();

	int GetFindCornerWorkerCount();
	int GetCalibrateCount();

	FJobInfo RegisterJob (
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		const TArray<int> & expectedImageCounts,
		const int expectedResultCount,
		const UJobType jobType);

	void SetCalibrateWorkerParameters(FCalibrationParameters calibrationParameters);
	void QueueTextureArrayWorkUnit(const FString & jobID, FLensSolverPixelArrayWorkUnit pixelArrayWorkUnit);
	void QueueTextureFileWorkUnit(const FString & jobID, FLensSolverTextureFileWorkUnit textureFileWorkUnit);
	void QueueMediaStreamWorkUnit(const FMediaStreamWorkUnit mediaStreamWorkUnit);

	bool CalibrationResultIsQueued();
	void DequeueCalibrationResult(CalibrationResultQueueContainer & queueContainer);
	void PollMediaTextureStreams();
};