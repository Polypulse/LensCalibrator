/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolverWorkDistributor.h"
#include "TextureResource.h"
#include "CoreTypes.h"
#include "GlobalShader.h"
#include "RHIStaticStates.h"
#include "Engine/RendererSettings.h"
#include "PixelShaderUtils.h"

#include "Engine.h"
#include "BlitShader.h"
#include "LensSolverUtilities.h"

/* This spawns a thread pool and prepares a set of find corner and calibration workers. */
void LensSolverWorkDistributor::PrepareWorkers(
	int findCornerWorkerCount,
	int calibrateWorkerCount)
{
	UE_LOG(LogTemp, Log, TEXT("Preparing workers."));

	if (threadPool == nullptr)
	{
		threadPool = FQueuedThreadPool::Allocate();

		uint threadPoolSize = 64;
		if (!threadPool->Create(threadPoolSize))
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to create thread pool of size: %i"), threadPoolSize);
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("Allocated thread pool of size: %i"), threadPoolSize);
	}

	PrepareFindCornerWorkers(findCornerWorkerCount);
	PrepareCalibrateWorkers(calibrateWorkerCount);
}

void LensSolverWorkDistributor::PrepareFindCornerWorkers(
	int findCornerWorkerCount)
{
	if (findCornerWorkerCount <= 0)
	{
		QueueLogAsync("(ERROR): Cannot start calibration with 0 FindCorner workers.");
		return;
	}

	queueCalibrateWorkUnitInputDel.BindRaw(this, &LensSolverWorkDistributor::QueueCalibrateWorkUnit);

	Lock();

	for (int i = 0; i < findCornerWorkerCount; i++)
	{
		FString guid = FGuid::NewGuid().ToString();
		workLoadSortedFindCornerWorkers.Add(guid);
		FWorkerFindCornersInterfaceContainer & interfaceContainer = findCornersWorkers.Add(guid, FWorkerFindCornersInterfaceContainer());

		FLensSolverWorkerParameters workerParameters(
			&queueLogOutputDel,
			&interfaceContainer.baseContainer.isClosingDel,
			&interfaceContainer.baseContainer.getWorkLoadDel,
			guid
		);

		interfaceContainer.baseContainer.workerID = guid;

		QueueLogAsync(FString::Printf(TEXT("(INFO): Starting FindCorner worker: %d"), i));

		interfaceContainer.worker = new FAutoDeleteAsyncTask<FLensSolverWorkerFindCorners>(
			workerParameters,
			&interfaceContainer.queueTextureFileWorkUnitInputDel,
			&interfaceContainer.queuePixelArrayWorkUnitInputDel,
			&queueCalibrateWorkUnitInputDel);
	}

	int count = 0;
	for (auto entry : findCornersWorkers)
	{
		entry.Value.worker->StartBackgroundTask(threadPool);
		count++;
	}

	Unlock();

	UE_LOG(LogTemp, Log, TEXT("(INFO): Started %d FindCorner workers"), count);
}

void LensSolverWorkDistributor::PrepareCalibrateWorkers(
	int calibrateWorkerCount)
{
	if (calibrateWorkerCount <= 0)
	{
		QueueLogAsync("(ERROR): Cannot start calibration with 0 Calibrate workers.");
		return;
	}

	queueCalibrationResultOutputDel.BindRaw(this, &LensSolverWorkDistributor::QueueCalibrationResult);
	Lock();

	for (int i = 0; i < calibrateWorkerCount; i++)
	{
		FString guid = FGuid::NewGuid().ToString();
		workLoadSortedCalibrateWorkers.Add(guid);
		FWorkerCalibrateInterfaceContainer & interfaceContainer = calibrateWorkers.Add(guid, FWorkerCalibrateInterfaceContainer());

		FLensSolverWorkerParameters workerParameters(
			&queueLogOutputDel,
			&interfaceContainer.baseContainer.isClosingDel,
			&interfaceContainer.baseContainer.getWorkLoadDel,
			guid
		);

		interfaceContainer.baseContainer.workerID = guid;
		if (Debug())
			QueueLogAsync(FString::Printf(TEXT("(INFO): Starting Calibrate worker: %d"), i));

		interfaceContainer.worker = new FAutoDeleteAsyncTask<FLensSolverWorkerCalibrate>(
			workerParameters,
			&interfaceContainer.queueCalibrateWorkUnitDel,
			&interfaceContainer.signalLatch,
			&queueCalibrationResultOutputDel);
	}

	int count = 0;
	for (auto entry : calibrateWorkers)
	{
		entry.Value.worker->StartBackgroundTask(threadPool);
		count++;
	}

	Unlock();

	UE_LOG(LogTemp, Log, TEXT("(INFO): Started %d Calibrate workers"), count);
}

/* Create job a one time or continuous job and return the job info. */
FJobInfo LensSolverWorkDistributor::RegisterJob(
	TScriptInterface<ILensSolverEventReceiver> eventReceiver, /* The interface that a blueprint class implements for callbacks. */

	/* The expected number of images for this job, if this is a media stream its the number of snapshots of 
	that stream, if its texture folders its the number of images in those folders. */
	const TArray<int> & expectedImageCounts, 

	/* The expected number of results for this job, for a media stream job this should only be 1, if its a set of texture folders
	it should be the number of texture folders associated with each zoom level. */
	const int expectedResultCount, 
	const UJobType jobType /* Job type either continuous or one shot. */ )
{
	TArray<FString> calibrationIDs;
	TMap<FString, FExpectedAndCurrentImageCount> mapOfExpectedAndCurrentImageCounts;

	/* If there are 3 folders of images where each folder represents a zoom level, this means
	there will be 3 calibration IDs and 3 expected results. */
	calibrationIDs.SetNum(expectedImageCounts.Num());

	/* Loop through calibrations. */
	for (int i = 0; i < calibrationIDs.Num(); i++)
	{
		calibrationIDs[i] = FGuid::NewGuid().ToString();
		/* Map expected images to calibration IDs. */
		mapOfExpectedAndCurrentImageCounts.Add(calibrationIDs[i], FExpectedAndCurrentImageCount(expectedImageCounts[i], 0));
	}

	/* Handle to new job. */
	FJobInfo jobInfo;
	{
		jobInfo.jobID = FGuid::NewGuid().ToString();
		jobInfo.jobType = jobType;
		jobInfo.calibrationIDs = calibrationIDs;
	}

	FJob job;
	{
		job.eventReceiver = eventReceiver;
		job.jobInfo = jobInfo;
		job.expectedAndCurrentImageCounts = mapOfExpectedAndCurrentImageCounts;
		job.expectedResultCount = expectedResultCount;
		job.currentResultCount = 0;
		job.startTime = GetTickNow();
	}

	Lock();

	/* Map job to job ID. */
	jobs.Add(jobInfo.jobID, job);

	Unlock();

	QueueLogAsync(FString::Printf(TEXT("(INFO): Registered job with ID: \"%s\"."), *job.jobInfo.jobID));

	return jobInfo;
}

void LensSolverWorkDistributor::QueueTextureArrayWorkUnit(const FString & jobID, FLensSolverPixelArrayWorkUnit pixelArrayWorkUnit)
{
	Lock();
	if (workLoadSortedFindCornerWorkers.Num() == 0)
	{
		QueueLogAsync("(ERROR): The work load sorted FindCornerWorker array is empty!");
		Unlock();
		return;
	}

	/* Sort our corner finding background workers by least busy to most busy so we can load balance correctly. */
	SortFindCornersWorkersByWorkLoad();

	/* Get first corner finding background worker. */
	const FString workerID = workLoadSortedFindCornerWorkers[0];
	if (workerID.IsEmpty())
	{
		QueueLogAsync("(ERROR): A worker ID in the work load sorted FindCornerWorker array is empty!");
		Unlock();
		return;
	}

	/* Get interface to worker. */
	FWorkerFindCornersInterfaceContainer* interfaceContainer =nullptr;
	if (!GetFindCornersContainerInterfacePtr(workerID, interfaceContainer))
	{
		Unlock();
		return;
	}
	Unlock();

	/* Determine if we have a valid delegate to execute methods in our background worker. */
	if (!interfaceContainer->queuePixelArrayWorkUnitInputDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): FindCornerWorker: \"%s\" does not have a QueueWorkUnit delegate binded!"), *workerID));
		return;
	}

	/* Queue pixel data work unit to worker to find calibration pattern corners in the image. */
	interfaceContainer->queuePixelArrayWorkUnitInputDel.Execute(pixelArrayWorkUnit);
}

void LensSolverWorkDistributor::QueueTextureFileWorkUnit(const FString & jobID, FLensSolverTextureFileWorkUnit textureFileWorkUnit)
{
	// static int count = 0;
	Lock();
	if (workLoadSortedFindCornerWorkers.Num() == 0)
	{
		QueueLogAsync("(ERROR): The work load sorted CalibrateWorker array is empty!");
		Unlock();
		return;
	}

	SortFindCornersWorkersByWorkLoad();

	const FString workerID = workLoadSortedFindCornerWorkers[0];
	if (workerID.IsEmpty())
	{
		QueueLogAsync("(ERROR): A worker ID in the work load sorted CalibrateWorker array is empty!");
		Unlock();
		return;
	}

	FWorkerFindCornersInterfaceContainer* interfaceContainer = nullptr;
	if (!GetFindCornersContainerInterfacePtr(workerID, interfaceContainer))
	{
		Unlock();
		return;
	}
	Unlock();

	if (!interfaceContainer->queueTextureFileWorkUnitInputDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): CalibrateWorker: \"%s\" does not have a QueueWorkUnit delegate binded!"), *workerID));
		return;
	}

	interfaceContainer->queueTextureFileWorkUnitInputDel.Execute(textureFileWorkUnit);
}

/* Queuing work unit for taking snapshots of media stream. */
void LensSolverWorkDistributor::QueueMediaStreamWorkUnit(const FMediaStreamWorkUnit mediaStreamWorkUnit)
{
	if (mediaTextureJobLUT.Contains(mediaStreamWorkUnit.baseParameters.jobID))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Attempted to re-register already registered job ID: \"%s\" in MediaTexture Job LUT."), *mediaStreamWorkUnit.baseParameters.jobID);
		return;
	}

	if (Debug())
		QueueLogAsync(FString::Printf(TEXT("Queued MediaStreamWorkUnit for calibration"), *mediaStreamWorkUnit.baseParameters.calibrationID));

	/* Media stream calibration job mapping between work unit and job ID. */
	mediaTextureJobLUT.Add(mediaStreamWorkUnit.baseParameters.jobID, mediaStreamWorkUnit);
}

void LensSolverWorkDistributor::SetCalibrateWorkerParameters(FCalibrationParameters calibrationParameters)
{
	cachedCalibrationParameters = calibrationParameters;
}

void LensSolverWorkDistributor::QueueCalibrateWorkUnit(FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit)
{
	Lock();
	if (calibrateWorkers.Num() == 0)
	{
		Unlock();
		return;
	}

	FWorkerCalibrateInterfaceContainer * interfaceContainerPtr;
	if (!GetCalibrateWorkerInterfaceContainerPtr(calibrateWorkUnit.baseParameters.calibrationID, interfaceContainerPtr))
	{
		Unlock();
		return;
	}
	Unlock();

	if (!interfaceContainerPtr->queueCalibrateWorkUnitDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): The QueueWOrkUnit delegate is not binded for CalibrateWorker with ID: \"%s\"."), *interfaceContainerPtr->baseContainer.workerID));
		Unlock();
		return;
	}

	interfaceContainerPtr->queueCalibrateWorkUnitDel.Execute(calibrateWorkUnit);

	bool hitExpectedImageCount = IterateImageCount(calibrateWorkUnit.baseParameters.jobID, calibrateWorkUnit.baseParameters.calibrationID);

	if (hitExpectedImageCount)
	{
		FCalibrateLatch latchData;
		latchData.baseParameters		= calibrateWorkUnit.baseParameters;
		latchData.calibrationParameters	= cachedCalibrationParameters;
		latchData.resizeParameters		= calibrateWorkUnit.resizeParameters;

		LatchCalibrateWorker(latchData);
		PollShutdownFindCornerWorkersIfNecessary();
	}
}

void LensSolverWorkDistributor::LatchCalibrateWorker(const FCalibrateLatch& latchData)
{
	if (latchData.baseParameters.calibrationID.IsEmpty())
	{
		QueueLogAsync("(ERROR): The calibration ID is empty!");
		return;
	}

	Lock();
	FWorkerCalibrateInterfaceContainer* interfaceContainerPtr;
	if (!GetCalibrateWorkerInterfaceContainerPtr(latchData.baseParameters.calibrationID, interfaceContainerPtr))
	{
		Unlock();
		return;
	}
	Unlock();

	if (!interfaceContainerPtr->signalLatch.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): The CalibrateWorker: \"%s\" does not have a QueueLatchInput delegate binded!"), *interfaceContainerPtr->baseContainer.workerID));
		return;
	}

	interfaceContainerPtr->signalLatch.Execute(latchData);
}

void LensSolverWorkDistributor::QueueCalibrationResult(const FCalibrationResult calibrationResult)
{
	Lock();

	FJob* jobPtr = jobs.Find(calibrationResult.baseParameters.jobID);
	if (jobPtr == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): Unknown error occurred while queuing calibration result, no jobs are registered with ID: \"%s\"."), *calibrationResult.baseParameters.jobID));
		Unlock();
		return;
	}

	CalibrationResultQueueContainer queueContainer;
	queueContainer.eventReceiver = jobPtr->eventReceiver;
	queueContainer.calibrationResult = calibrationResult;

	queuedCalibrationResults.Enqueue(queueContainer);

	FJobInfo jobInfo;
	bool done = false;

	jobPtr->currentResultCount++;

	if (jobPtr->currentResultCount >= jobPtr->expectedResultCount)
	{
		jobInfo = jobPtr->jobInfo;

		float duration = (GetTickNow() - jobPtr->startTime) / 1000.0f;
		QueueLogAsync(FString::Printf(TEXT("Job: \"%s\" took %f seconds."), *jobInfo.jobID, duration));

		FinishedJobQueueContainer finishedJobQueueContainer;
		finishedJobQueueContainer.jobInfo = jobInfo;
		finishedJobQueueContainer.eventReceiver = jobPtr->eventReceiver;
		queueFinishedJobOutputDel.Execute(finishedJobQueueContainer);

		jobs.Remove(calibrationResult.baseParameters.jobID);
		done = true;
	}

	Unlock();

	PollShutdownAllWorkersIfNecessary();
 }

bool LensSolverWorkDistributor::CalibrationResultIsQueued()
{
	return queuedCalibrationResults.IsEmpty() == false;
}

void LensSolverWorkDistributor::DequeueCalibrationResult(CalibrationResultQueueContainer & queueContainer)
{
	queuedCalibrationResults.Dequeue(queueContainer);
}

/* This is called from blueprints every frame in order to continue 
feeding new frames into the system for calibration. */
void LensSolverWorkDistributor::PollMediaTextureStreams()
{
	Lock();
	if (mediaTextureJobLUT.Num() == 0)
	{
		Unlock();
		return;
	}

	LensSolverWorkDistributor* workDistributor = this;

	TArray<FString> jobIDs;
	mediaTextureJobLUT.GetKeys(jobIDs);

	/* Loop through job IDs related to media stream calibration work units. */
	for (int i = 0; i < jobIDs.Num(); i++)
	{
		FMediaStreamWorkUnit * mediaStreamWorkUnit = mediaTextureJobLUT.Find(jobIDs[i]);

		int64 tickNow = GetTickNow();

		/* Snapshots are taken from the media stream at a frequency that the user defines, so if our tick has not met the snapshot to frame, then skip this frame.  */
		if ((tickNow - mediaStreamWorkUnit->mediaStreamParameters.previousSnapshotTime) / 1000.0f < mediaStreamWorkUnit->mediaStreamParameters.streamSnapshotIntervalFrequencyInSeconds)
			continue;

		mediaStreamWorkUnit->mediaStreamParameters.previousSnapshotTime = tickNow;

		/* Determine whether the media stream texture is valid. */
		if (!ValidateMediaTexture(mediaStreamWorkUnit->mediaStreamParameters.mediaTexture))
		{
			QueueLogAsync(FString::Printf(TEXT("Media texture in media stream work unit is invalid for calibration: \"%s\""), *mediaStreamWorkUnit->baseParameters.calibrationID));
			continue;
		}

		FMediaStreamWorkUnit copyMediaSreamWorkUnit = *mediaStreamWorkUnit;

		/* Queue command to take a snapshot of the render stream to the rendering thread. */
		ENQUEUE_RENDER_COMMAND(MediaStreamSnapshotRenderCommand)
		(
			/* This works like so:
			[{variables local to scope that you want to use on the render thread}]({UE4 variables}) 
			{
				{code executed on render thread. }
			}
			*/
			[workDistributor, copyMediaSreamWorkUnit](FRHICommandListImmediate& RHICmdList)
			{
				workDistributor->MediaTextureRenderThread(
					RHICmdList,
					copyMediaSreamWorkUnit);
			}
		);

		/* Render command has been queued to take a snapshot of the media stream, so perform a count. */
		mediaStreamWorkUnit->mediaStreamParameters.currentStreamSnapshotCount++;

		/* If we reached the expected snapshot count, finish the job. */
		if (mediaStreamWorkUnit->mediaStreamParameters.currentStreamSnapshotCount > mediaStreamWorkUnit->mediaStreamParameters.expectedStreamSnapshotCount - 1)
		{
			QueueLogAsync(FString::Printf(TEXT("(INFO): Completed queuing media stream snapshots %d/%d to render thread for calibration: \"%s\"."), 
				mediaStreamWorkUnit->mediaStreamParameters.currentStreamSnapshotCount, 
				mediaStreamWorkUnit->mediaStreamParameters.expectedStreamSnapshotCount, 
				*mediaStreamWorkUnit->baseParameters.calibrationID));

			mediaTextureJobLUT.Remove(jobIDs[i]);
			continue;
		}

		QueueLogAsync(FString::Printf(TEXT("(INFO): Queued media stream snapshot %d/%d to render thread for calibration: \"%s\""), 
				mediaStreamWorkUnit->mediaStreamParameters.currentStreamSnapshotCount, 
				mediaStreamWorkUnit->mediaStreamParameters.expectedStreamSnapshotCount, 
				*mediaStreamWorkUnit->baseParameters.calibrationID));
	}

	Unlock();
}

bool LensSolverWorkDistributor::Debug()
{
	static IConsoleVariable * variable = IConsoleManager::Get().FindConsoleVariable(TEXT("LensCalibrator.Debug"));
	if (variable != nullptr && variable->GetInt() == 0)
		return false;
	return true;
}

void LensSolverWorkDistributor::QueueLogAsync(FString msg)
{
	msg = FString::Printf(TEXT("Work Distributor: %s"), *msg);
	if (!queueLogOutputDel.IsBound())
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), *msg);
		return;
	}

	queueLogOutputDel.Execute(msg);
}

bool LensSolverWorkDistributor::GetFindCornersContainerInterfacePtr(
	const FString workerID,
	FWorkerFindCornersInterfaceContainer *& outputInterfaceContainerPtr)
{
	outputInterfaceContainerPtr = findCornersWorkers.Find(workerID);

	if (outputInterfaceContainerPtr == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): There is no FWorkerCalibrateInterfaceContainer registered for worker ID: \"%s\"!"), *workerID));
		return false;
	}

	return true;
}

bool LensSolverWorkDistributor::GetCalibrateWorkerInterfaceContainerPtr(
	const FString calibrationID,
	FWorkerCalibrateInterfaceContainer *& outputInterfaceContainerPtr)
{
	if (calibrationID.IsEmpty())
	{
		QueueLogAsync("(ERROR): Received NULL LensSolverCalibrateWorkUnit with empty calibrationID FString member!");
		return false;
	}

	const FString* workerIDPtr = workerCalibrationIDLUT.Find(calibrationID);
	FString workerID;

	if (workerIDPtr == nullptr)
	{
		SortCalibrateWorkersByWorkLoad();
		workerID = workLoadSortedCalibrateWorkers[0];
		workerCalibrationIDLUT.Add(calibrationID, workerID);
	}

	else workerID = *workerIDPtr;

	outputInterfaceContainerPtr = calibrateWorkers.Find(workerID);
	if (outputInterfaceContainerPtr == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): We the worker ID: \"%s\". However, no CalibrateWorkerInterfaceContainer is registered with that ID!"), *workerID));
		return false;
	}

	return true;
}

bool LensSolverWorkDistributor::IterateImageCount(const FString & jobID, const FString& calibrationID)
{
	Lock();
	FJob* job = jobs.Find(jobID);
	if (job == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): Cannot iterate image count, no job with ID: \"%s\" registered."), *jobID));
		Unlock();
		return false;
	}

	FExpectedAndCurrentImageCount* expectedAndCurrentImageCount = job->expectedAndCurrentImageCounts.Find(calibrationID);
	if (expectedAndCurrentImageCount == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): Cannot iterate image count, the job: \"%s\" does not contain the calibration ID: \"%s\"."), *jobID, *calibrationID));
		Unlock();
		return false;
	}

	int currentImageCount = expectedAndCurrentImageCount->currentImageCount;
	int expectedImageCount = expectedAndCurrentImageCount->expectedImageCount;

	currentImageCount++;
	expectedAndCurrentImageCount->currentImageCount = currentImageCount;

	if (currentImageCount > expectedImageCount - 1)
	{
		QueueLogAsync(FString::Printf(TEXT("(INFO): Completed processing all images of count %d/%d for calibration: \"%s\"."), currentImageCount, expectedImageCount, *calibrationID));

		Unlock();
		return true;
	}

	Unlock();
	QueueLogAsync(FString::Printf(TEXT("(INFO): Iterate image count %d/%d for calibration: \"%s\"."), currentImageCount, expectedImageCount, *calibrationID));

	return false;
}

void LensSolverWorkDistributor::SortFindCornersWorkersByWorkLoad()
{
	LensSolverWorkDistributor* workDistributor = this;
	workLoadSortedFindCornerWorkers.Sort([workDistributor](const FString& workerAID, const FString& workerBID) 
	{
		FWorkerFindCornersInterfaceContainer * interfaceContainerA = workDistributor->findCornersWorkers.Find(workerAID);
		FWorkerFindCornersInterfaceContainer * interfaceContainerB = workDistributor->findCornersWorkers.Find(workerBID);

		if (interfaceContainerA == nullptr || 
			interfaceContainerB == nullptr ||
			!interfaceContainerA->baseContainer.getWorkLoadDel.IsBound() ||
			!interfaceContainerB->baseContainer.getWorkLoadDel.IsBound())
			return false;

		return interfaceContainerA->baseContainer.getWorkLoadDel.Execute() < interfaceContainerB->baseContainer.getWorkLoadDel.Execute();
	});
}

void LensSolverWorkDistributor::SortCalibrateWorkersByWorkLoad()
{
	LensSolverWorkDistributor* workDistributor = this;
	workLoadSortedCalibrateWorkers.Sort([workDistributor](const FString & workerAID, const FString & workerBID) 
	{
		FWorkerCalibrateInterfaceContainer * interfaceContainerA = workDistributor->calibrateWorkers.Find(workerAID);
		FWorkerCalibrateInterfaceContainer * interfaceContainerB = workDistributor->calibrateWorkers.Find(workerBID);
		if (interfaceContainerA == nullptr || 
			interfaceContainerB == nullptr ||
			!interfaceContainerA->baseContainer.getWorkLoadDel.IsBound() ||
			!interfaceContainerB->baseContainer.getWorkLoadDel.IsBound())
			return false;

		return interfaceContainerA->baseContainer.getWorkLoadDel.Execute() < interfaceContainerB->baseContainer.getWorkLoadDel.Execute();
	});
}

void LensSolverWorkDistributor::PollShutdownFindCornerWorkersIfNecessary()
{
	if (!shutDownWorkersAfterCompletedTasks)
		return;

	TQueue<IsClosingOutputDel> isClosingDelQueue;
	bool allImagesProcessed = true;

	Lock();

	for (auto job : jobs)
	{
		for (auto expectedAndCurrentImageCount : job.Value.expectedAndCurrentImageCounts)
		{
			if (expectedAndCurrentImageCount.Value.currentImageCount < expectedAndCurrentImageCount.Value.expectedImageCount)
			{
				allImagesProcessed = false;
				break;
			}
		}
	}

	if (allImagesProcessed)
	{
		for (auto workerContainer : findCornersWorkers)
			isClosingDelQueue.Enqueue(workerContainer.Value.baseContainer.isClosingDel);
	}

	Unlock();

	if (allImagesProcessed)
	{
		while (!isClosingDelQueue.IsEmpty())
		{
			IsClosingOutputDel isClosingDel;
			isClosingDelQueue.Dequeue(isClosingDel);
			if (isClosingDel.IsBound())
				isClosingDel.Execute();
		}

		findCornersWorkers.Empty();
		workLoadSortedFindCornerWorkers.Empty();
	}
}

void LensSolverWorkDistributor::PollShutdownAllWorkersIfNecessary()
{
	if (!shutDownWorkersAfterCompletedTasks || jobs.Num() != 0)
		return;

	TQueue<IsClosingOutputDel*> isClosingDelQueue;
	Lock();

	for (auto workerContainer : findCornersWorkers)
		isClosingDelQueue.Enqueue(&workerContainer.Value.baseContainer.isClosingDel);

	for (auto workerContainer : calibrateWorkers)
		isClosingDelQueue.Enqueue(&workerContainer.Value.baseContainer.isClosingDel);

	Unlock();

	while (!isClosingDelQueue.IsEmpty())
	{
		IsClosingOutputDel * isClosingDel;
		isClosingDelQueue.Dequeue(isClosingDel);
		if (isClosingDel->IsBound())
			isClosingDel->Execute();
	}

	findCornersWorkers.Empty();
	workLoadSortedFindCornerWorkers.Empty();

	calibrateWorkers.Empty();
	workLoadSortedCalibrateWorkers.Empty();
	workerCalibrationIDLUT.Empty();

	jobs.Empty();
	workerCalibrationIDLUT.Empty();
	mediaTextureJobLUT.Empty();
}

bool LensSolverWorkDistributor::ValidateMediaTexture(const UMediaTexture* inputTexture)
{
	if (inputTexture == nullptr)
	{
		QueueLogAsync("Cannot process null texture.");
		return false;
	}

	if (inputTexture->GetWidth() <= 3 ||
		inputTexture->GetHeight() <= 3)
	{
		QueueLogAsync("Cannot process texture, it's to small.");
		return false;
	}

	return true;
}

void LensSolverWorkDistributor::Lock()
{
	threadLock.Lock();
}

void LensSolverWorkDistributor::Unlock()
{
	threadLock.Unlock();
}

int64 LensSolverWorkDistributor::GetTickNow()
{
	FDateTime timeUtc = FDateTime::UtcNow();
	return timeUtc.ToUnixTimestamp() * 1000 + timeUtc.GetMillisecond();
}

void LensSolverWorkDistributor::StopFindCornerWorkers()
{
	UE_LOG(LogTemp, Log, TEXT("Stopping find corner workers."));
	Lock();
	if (findCornersWorkers.Num() == 0)
	{
		Unlock();
		return;
	}

	TArray<FString> keys;
	findCornersWorkers.GetKeys(keys);
	for (int i = 0; i < keys.Num(); i++)
	{
		FWorkerFindCornersInterfaceContainer* interfaceContainerPtr = findCornersWorkers.Find(keys[i]);
		if (interfaceContainerPtr == nullptr)
			continue;

		if (interfaceContainerPtr->baseContainer.isClosingDel.IsBound())
			interfaceContainerPtr->baseContainer.isClosingDel.Execute();
	}

	Unlock();
	findCornersWorkers.Empty();
	workLoadSortedFindCornerWorkers.Empty();
}

void LensSolverWorkDistributor::StopCalibrationWorkers()
{
	UE_LOG(LogTemp, Log, TEXT("Stopping calibration workers."));
	Lock();
	if (calibrateWorkers.Num() == 0)
	{
		Unlock();
		return;
	}

	TArray<FString> keys;
	calibrateWorkers.GetKeys(keys);
	for (int i = 0; i < keys.Num(); i++)
	{
		FWorkerCalibrateInterfaceContainer* interfaceContainerPtr = calibrateWorkers.Find(keys[i]);
		if (interfaceContainerPtr == nullptr)
			continue;

		if (interfaceContainerPtr->baseContainer.isClosingDel.IsBound())
			interfaceContainerPtr->baseContainer.isClosingDel.Execute();
	}

	calibrateWorkers.Empty();
	workLoadSortedCalibrateWorkers.Empty();
	workerCalibrationIDLUT.Empty();

	jobs.Empty();
	workerCalibrationIDLUT.Empty();
	mediaTextureJobLUT.Empty();
	Unlock();
}

void LensSolverWorkDistributor::StopBackgroundWorkers()
{
	UE_LOG(LogTemp, Log, TEXT("Stopping background workers."));
	StopFindCornerWorkers();
	StopCalibrationWorkers();
}

int LensSolverWorkDistributor::GetFindCornerWorkerCount()
{
	int count = 0;
	Lock();
	count = findCornersWorkers.Num();
	Unlock();
	return count;
}

int LensSolverWorkDistributor::GetCalibrateCount()
{
	int count = 0;
	Lock();
	count = calibrateWorkers.Num();
	Unlock();
	return count;
}

void LensSolverWorkDistributor::MediaTextureRenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FMediaStreamWorkUnit mediaStreamWorkUnit)
{
	/* We should check again whether the media stream texture is still valid since we are in the render thread. */
	if (!ValidateMediaTexture(mediaStreamWorkUnit.mediaStreamParameters.mediaTexture))
		return;

	/* If this boolean is toggled in the calibration parameters, then we will save a snapshot of the stream before we do any processing on it. */
	if (mediaStreamWorkUnit.mediaStreamParameters.writePreBlitRenderTextureToFile)
	{
		FString outputPath = mediaStreamWorkUnit.mediaStreamParameters.preBlitRenderTextureOutputPath;
		FString backupOutputDir = FPaths::ProjectSavedDir();
		FString folder("MediaStreamPreBlitSnapshot");

		backupOutputDir = FPaths::Combine(backupOutputDir, folder);

		if (LensSolverUtilities::ValidateFilePath(outputPath, backupOutputDir, "MediaStreamSnapshot", "bmp"))
		{
			FReadSurfaceDataFlags ReadDataFlags;
			ReadDataFlags.SetLinearToGamma(false);
			ReadDataFlags.SetOutputStencil(false);
			ReadDataFlags.SetMip(0);

			int mediaTextureWidth = mediaStreamWorkUnit.mediaStreamParameters.mediaTexture->GetWidth();
			int mediaTextureHeight = mediaStreamWorkUnit.mediaStreamParameters.mediaTexture->GetHeight();

			TArray<FColor> pixels;
			/* Read the pixel data from the texture into the pixels array. */
			RHICmdList.ReadSurfaceData(mediaStreamWorkUnit.mediaStreamParameters.mediaTexture->TextureReference.TextureReferenceRHI->GetReferencedTexture(), FIntRect(0, 0, mediaTextureWidth, mediaTextureHeight), pixels, ReadDataFlags);
			uint32 ExtendXWithMSAA = pixels.Num() / mediaTextureHeight;

			/* Write the pixels to the file. */
			FFileHelper::CreateBitmap(*outputPath, ExtendXWithMSAA, mediaTextureHeight, pixels.GetData());
			UE_LOG(LogTemp, Log, TEXT("Wrote pre blit input texture to file: \"%s\"."), *outputPath);
		}
	}

	/* Determine what the size of our snapshot buffer should be. */
	int width = mediaStreamWorkUnit.textureSearchParameters.resize ? mediaStreamWorkUnit.mediaStreamParameters.mediaTexture->GetWidth() * mediaStreamWorkUnit.textureSearchParameters.resizePercentage : mediaStreamWorkUnit.mediaStreamParameters.mediaTexture->GetWidth();
	int height = mediaStreamWorkUnit.textureSearchParameters.resize ? mediaStreamWorkUnit.mediaStreamParameters.mediaTexture->GetHeight() * mediaStreamWorkUnit.textureSearchParameters.resizePercentage : mediaStreamWorkUnit.mediaStreamParameters.mediaTexture->GetHeight();

	FTexture2DRHIRef blitRenderTexture;
	FRHIResourceCreateInfo createInfo;
	FTexture2DRHIRef dummyTexRef;
	/* Create render texture for media stream snapshot. */
	RHICreateTargetableShaderResource2D(
		width,
		height,
		/* BGRA with all channels 8 bits. */
		EPixelFormat::PF_B8G8R8A8,
		1,
		TexCreate_Transient,
		TexCreate_RenderTargetable,
		false,
		createInfo,
		blitRenderTexture,
		dummyTexRef);

	/* Clear the color, but not the depth (Since there is none). */
	FRHIRenderPassInfo RPInfo(blitRenderTexture, ERenderTargetActions::Clear_DontStore);
	RHICmdList.BeginRenderPass(RPInfo, TEXT("PresentAndCopyMediaTexture"));
	{
		const ERHIFeatureLevel::Type RenderFeatureLevel = GMaxRHIFeatureLevel;
		const auto GlobalShaderMap = GetGlobalShaderMap(RenderFeatureLevel);

		/* Initialize vertex/pixel shader. */
		TShaderMapRef<FBlitShaderVS> VertexShader(GlobalShaderMap);
		TShaderMapRef<FBlitShaderPS> PixelShader(GlobalShaderMap);

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		/* Set render size. */
		RHICmdList.SetViewport(0, 0, 0.0f, width, height, 1.0f);

		GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGB, BO_Add, BF_One, BF_SourceAlpha>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

		/* Submit data to shader. */
		PixelShader->SetParameters(RHICmdList, mediaStreamWorkUnit.mediaStreamParameters.mediaTexture->TextureReference.TextureReferenceRHI.GetReference(), FVector2D(mediaStreamWorkUnit.textureSearchParameters.flipX ? -1.0f : 1.0f, mediaStreamWorkUnit.textureSearchParameters.flipY ? 1.0f : -1.0f));

		/* Perform the render. */
		FPixelShaderUtils::DrawFullscreenQuad(RHICmdList, 1);
	}

	RHICmdList.EndRenderPass();

	FRHITexture2D * texture2D = blitRenderTexture->GetTexture2D();
	TArray<FColor> surfaceData;

	FReadSurfaceDataFlags ReadDataFlags;
	ReadDataFlags.SetLinearToGamma(false);
	ReadDataFlags.SetOutputStencil(false);
	ReadDataFlags.SetMip(0);

	/* Read pixels from texture into surfaceData pixel array. */
	RHICmdList.ReadSurfaceData(texture2D, FIntRect(0, 0, width, height), surfaceData, ReadDataFlags);

	uint32 ExtendXWithMSAA = surfaceData.Num() / texture2D->GetSizeY();

	/* After media stream snapshot occurs, we can also write that snapshot to to a file for debugging. */
	if (mediaStreamWorkUnit.mediaStreamParameters.writePostBlitRenderTextureToFile)
	{
		FString outputPath = mediaStreamWorkUnit.mediaStreamParameters.postBlitRenderTextureOutputPath;
		FString backupOutputDir = FPaths::ProjectSavedDir();
		FString folder("MediaStreamPostBlitSnapshot");
		backupOutputDir = FPaths::Combine(backupOutputDir, folder);
		if (LensSolverUtilities::ValidateFilePath(outputPath, backupOutputDir, "MediaStreamSnapshot", "bmp"))
		{
			FFileHelper::CreateBitmap(*outputPath, ExtendXWithMSAA, texture2D->GetSizeY(), surfaceData.GetData());
			UE_LOG(LogTemp, Log, TEXT("Wrote post blit input texture to file: \"%s\"."), *outputPath);
		}
	}

	QueueLogAsync(FString::Printf(TEXT("(INFO): Completed snapshot blit of media stream on render thread of count %d/%d for calibration: \"%s\", queuing resulting pixel array."), 
			mediaStreamWorkUnit.mediaStreamParameters.currentStreamSnapshotCount, 
			mediaStreamWorkUnit.mediaStreamParameters.expectedStreamSnapshotCount, 
			*mediaStreamWorkUnit.baseParameters.calibrationID));

	/* Prepare work unit with pixel data for calibration corner search and calibration.  */
	FLensSolverPixelArrayWorkUnit pixelArrayWorkUnit;
	pixelArrayWorkUnit.baseParameters = mediaStreamWorkUnit.baseParameters;
	pixelArrayWorkUnit.textureSearchParameters = mediaStreamWorkUnit.textureSearchParameters;
	pixelArrayWorkUnit.textureSearchParameters.resize = false;
	pixelArrayWorkUnit.pixelArrayParameters.pixels = surfaceData;

	pixelArrayWorkUnit.resizeParameters.sourceX = mediaStreamWorkUnit.mediaStreamParameters.mediaTexture->GetWidth();
	pixelArrayWorkUnit.resizeParameters.sourceX = mediaStreamWorkUnit.mediaStreamParameters.mediaTexture->GetHeight();
	pixelArrayWorkUnit.resizeParameters.resizeX = width;
	pixelArrayWorkUnit.resizeParameters.resizeY = height;

	QueueTextureArrayWorkUnit(pixelArrayWorkUnit.baseParameters.jobID, pixelArrayWorkUnit);
}
