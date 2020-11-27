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

/* This is really where the bulk of the work preparation and distribution occurs for the workers, data is feed in from ULensSolver
and this class handles queuing all the work units, manages the workers and receives the results from the calibration. This class follows
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

	/* The thread pool for spawning any kind of workers. */
	FQueuedThreadPool * threadPool;

	/* Find corner workers keyed via worker ID. */
	TMap<FString, FWorkerFindCornersInterfaceContainer> findCornersWorkers;

	/* Calibration workers keyed via worker ID. */
	TMap<FString, FWorkerCalibrateInterfaceContainer> calibrateWorkers;

	bool shutDownWorkersAfterCompletedTasks;

	/* Array of find corner worker IDs sorted each frame by work load. */
	TArray<FString> workLoadSortedFindCornerWorkers;

	/* Array of calibration worker IDs sorted each frame by work load. */
	TArray<FString> workLoadSortedCalibrateWorkers;

	TMap<FString, FJob> jobs;
	TMap<FString, const FString> workerCalibrationIDLUT;
	TMap<FString, FMediaStreamWorkUnit> mediaTextureJobLUT;

	/* After the calibration workers complete their work units, the 
	results are queued in this structure. Here we also need to
	explicitly state that we are declaring a queue with multiple
	producers and a single consumer of data. */
	TQueue<CalibrationResultQueueContainer, EQueueMode::Mpsc> queuedCalibrationResults;

	/* This method is called by the workers to queue log messages onto the main thread. */
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

	/* Lock access to the variables within this class, locks 
	may occur on the main thread or the worker's threads. */
	void Lock();

	/* Unlock access to the variables within this class. */
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

	/* Pass in delegates and configuration from ULensSolver 
	so that this distributor class can pass data back to ULensSolver. */
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