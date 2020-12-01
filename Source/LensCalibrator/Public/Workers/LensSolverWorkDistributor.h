/*
 * Copyright (C) 2020 - LensCalibrator contributors, see Contributors.txt at the root of the project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

	/* Are we in debug mode? */
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

	/* Job ID and job mapping. */
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

	/* This returns an interface to a find corner worker via worker ID. */
	bool GetFindCornersContainerInterfacePtr(
		const FString workerID,
		FWorkerFindCornersInterfaceContainer *& outputInterfaceContainerPtr);

	/* This returns an interface to a calibration worker via worker ID. */
	bool GetCalibrateWorkerInterfaceContainerPtr(
		const FString calibrationID,
		FWorkerCalibrateInterfaceContainer *& outputInterfaceContainerPtr);

	/* After corners are found by the find corner workers, the results are polled, put 
	into a calibrate work unit and queued to calibration background workers for processing. */
	void QueueCalibrateWorkUnit(FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit);

	void LatchCalibrateWorker(const FCalibrateLatch& latchData);

	/* When calibration is complete, calibration background workers will queue the results back onto the main thread in this class. */
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