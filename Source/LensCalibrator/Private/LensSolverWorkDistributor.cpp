#include "LensSolverWorkDistributor.h"

void LensSolverWorkDistributor::StartFindCornerWorkers(
	int findCornerWorkerCount)
{
	if (findCornerWorkerCount <= 0)
	{
		QueueLogAsync("(ERROR): Cannot start calibration with 0 FindCorner workers.");
		return;
	}

	threadLock.Lock();

	queueCalibrateWorkUnitInputDel.BindRaw(this, &LensSolverWorkDistributor::QueueCalibrateWorkUnit);

	for (int i = 0; i < findCornerWorkerCount; i++)
	{
		FString guid = FGuid::NewGuid().ToString();
		FWorkerFindCornersInterfaceContainer & interfaceContainer = findCornersWorkers.Add(guid, FWorkerFindCornersInterfaceContainer());

		FLensSolverWorkerParameters workerParameters(
			queueLogOutputDel,
			&interfaceContainer.baseContainer.isClosingDel,
			&interfaceContainer.baseContainer.getWorkLoadDel,
			guid,
			debug
		);

		interfaceContainer.baseContainer.workerID = guid;

		if (debug)
			QueueLogAsync(FString::Printf(TEXT("(INFO): Starting FindCorner worker: %d"), i));

		interfaceContainer.worker = new FAutoDeleteAsyncTask<FLensSolverWorkerFindCorners>(
			workerParameters,
			&interfaceContainer.queueTextureFileWorkUnitInputDel,
			&interfaceContainer.queuePixelArrayWorkUnitInputDel,
			&queueCalibrateWorkUnitInputDel);

		interfaceContainer.worker->StartBackgroundTask();
		workLoadSortedFindCornerWorkers.Add(guid);
	}

	threadLock.Unlock();
	if (debug)
		QueueLogAsync(FString::Printf(TEXT("(INFO): Started %d FindCorner workers"), findCornerWorkerCount));
}

void LensSolverWorkDistributor::StartCalibrateWorkers(
	int calibrateWorkerCount)
{
	if (calibrateWorkerCount <= 0)
	{
		QueueLogAsync("(ERROR): Cannot start calibration with 0 Calibrate workers.");
		return;
	}

	threadLock.Lock();

	for (int i = 0; i < calibrateWorkerCount; i++)
	{
		FWorkerCalibrateInterfaceContainer interfaceContainer;

		FString guid = FGuid::NewGuid().ToString();
		FLensSolverWorkerParameters workerParameters(
			queueLogOutputDel,
			&interfaceContainer.baseContainer.isClosingDel,
			&interfaceContainer.baseContainer.getWorkLoadDel,
			guid,
			debug
		);

		interfaceContainer.baseContainer.workerID = guid;
		if (debug)
			QueueLogAsync(FString::Printf(TEXT("(INFO): Starting Calibrate worker: %d"), i));

		interfaceContainer.worker = new FAutoDeleteAsyncTask<FLensSolverWorkerCalibrate>(
			workerParameters,
			&interfaceContainer.queueCalibrateWorkUnitDel,
			&interfaceContainer.signalLatch,
			&queueCalibrationResultOutputDel);

		interfaceContainer.worker->StartBackgroundTask();
		calibrateWorkers.Add(guid, MoveTemp(interfaceContainer));
		workLoadSortedCalibrateWorkers.Add(guid);
	}

	threadLock.Unlock();
	if (debug)
		QueueLogAsync(FString::Printf(TEXT("(INFO): Started %d Calibrate workers"), calibrateWorkerCount));
}

FJobInfo LensSolverWorkDistributor::RegisterJob(
	const TArray<int> & expectedImageCounts,
	const int expectedResultCount,
	const UJobType jobType)
{
	TArray<FString> calibrationIDs;
	TMap<FString, FExpectedAndCurrentImageCount> mapOfExpectedAndCurrentImageCounts;

	calibrationIDs.SetNum(expectedImageCounts.Num());
	for (int i = 0; i < calibrationIDs.Num(); i++)
	{
		calibrationIDs[i] = FGuid::NewGuid().ToString();
		mapOfExpectedAndCurrentImageCounts.Add(calibrationIDs[i], FExpectedAndCurrentImageCount(expectedImageCounts[i], 0));
	}

	FJobInfo jobInfo;
	{
		jobInfo.jobID = FGuid::NewGuid().ToString();
		jobInfo.jobType = jobType;
		jobInfo.calibrationIDs = calibrationIDs;
	}

	FJob job;
	{
		job.jobInfo = jobInfo;
		job.expectedAndCurrentImageCounts = mapOfExpectedAndCurrentImageCounts;
		job.expectedResultCount = expectedResultCount;
		job.currentResultCount = 0;
	}

	threadLock.Lock();
	jobs.Add(jobInfo.jobID, job);
	threadLock.Unlock();

	if (debug)
		QueueLogAsync(FString::Printf(TEXT("(INFO): Registered job with ID: \"%s\"."), *job.jobInfo.jobID));

	return jobInfo;
}

void LensSolverWorkDistributor::QueueTextureArrayWorkUnit(const FString & jobID, FLensSolverPixelArrayWorkUnit pixelArrayWorkUnit)
{
	threadLock.Lock();
	if (workLoadSortedFindCornerWorkers.Num() == 0)
	{
		QueueLogAsync("(ERROR): The work load sorted FindCornerWorker array is empty!");
		threadLock.Unlock();
		return;
	}

	SortFindCornersWorkersByWorkLoad();

	const FString workerID = workLoadSortedFindCornerWorkers[0];
	if (workerID.IsEmpty())
	{
		QueueLogAsync("(ERROR): A worker ID in the work load sorted FindCornerWorker array is empty!");
		threadLock.Unlock();
		return;
	}

	FWorkerFindCornersInterfaceContainer* interfaceContainer =nullptr;
	if (!GetFindCornersContainerInterfacePtr(workerID, interfaceContainer))
	{
		threadLock.Unlock();
		return;
	}

	if (!interfaceContainer->queuePixelArrayWorkUnitInputDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): FindCornerWorker: \"%s\" does not have a QueueWorkUnit delegate binded!"), *workerID));
		threadLock.Unlock();
		return;
	}

	threadLock.Unlock();
	interfaceContainer->queuePixelArrayWorkUnitInputDel.Execute(pixelArrayWorkUnit);
}

void LensSolverWorkDistributor::QueueTextureFileWorkUnit(const FString & jobID, FLensSolverTextureFileWorkUnit textureFileWorkUnit)
{
	threadLock.Lock();
	if (workLoadSortedFindCornerWorkers.Num() == 0)
	{
		QueueLogAsync("(ERROR): The work load sorted CalibrateWorker array is empty!");
		threadLock.Unlock();
		return;
	}

	SortFindCornersWorkersByWorkLoad();

	const FString workerID = workLoadSortedFindCornerWorkers[0];
	if (workerID.IsEmpty())
	{
		QueueLogAsync("(ERROR): A worker ID in the work load sorted CalibrateWorker array is empty!");
		threadLock.Unlock();
		return;
	}

	FWorkerFindCornersInterfaceContainer* interfaceContainer = nullptr;
	if (!GetFindCornersContainerInterfacePtr(workerID, interfaceContainer))
	{
		threadLock.Unlock();
		return;
	}

	if (!interfaceContainer->queueTextureFileWorkUnitInputDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): CalibrateWorker: \"%s\" does not have a QueueWorkUnit delegate binded!"), *workerID));
		threadLock.Unlock();
		return;
	}

	threadLock.Unlock();
	interfaceContainer->queueTextureFileWorkUnitInputDel.Execute(textureFileWorkUnit);
}

void LensSolverWorkDistributor::SetCalibrateWorkerParameters(FCalibrationParameters calibrationParameters)
{
	cachedCalibrationParameters = calibrationParameters;
}

void LensSolverWorkDistributor::QueueCalibrateWorkUnit(FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit)
{
	threadLock.Lock();
	FWorkerCalibrateInterfaceContainer * interfaceContainerPtr;
	if (!GetCalibrateWorkerInterfaceContainerPtr(calibrateWorkUnit.baseParameters.calibrationID, interfaceContainerPtr))
	{
		threadLock.Unlock();
		return;
	}

	if (!interfaceContainerPtr->queueCalibrateWorkUnitDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): The QueueWOrkUnit delegate is not binded for CalibrateWorker with ID: \"%s\"."), *interfaceContainerPtr->baseContainer.workerID));
		threadLock.Unlock();
		return;
	}

	threadLock.Unlock();
	interfaceContainerPtr->queueCalibrateWorkUnitDel.Execute(calibrateWorkUnit);

	bool hitExpectedImageCount = IterateImageCount(calibrateWorkUnit.baseParameters.jobID, calibrateWorkUnit.baseParameters.calibrationID);

	if (hitExpectedImageCount)
	{
		FCalibrateLatch latchData;
		latchData.baseParameters		= calibrateWorkUnit.baseParameters;
		latchData.calibrationParameters	= cachedCalibrationParameters;

		LatchCalibrateWorker(latchData);
	}
}

void LensSolverWorkDistributor::LatchCalibrateWorker(const FCalibrateLatch& latchData)
{
	threadLock.Lock();
	if (latchData.baseParameters.calibrationID.IsEmpty())
	{
		QueueLogAsync("(ERROR): The calibration ID is empty!");
		threadLock.Unlock();
		return;
	}

	FWorkerCalibrateInterfaceContainer* interfaceContainerPtr;
	if (!GetCalibrateWorkerInterfaceContainerPtr(latchData.baseParameters.calibrationID, interfaceContainerPtr))
	{
		threadLock.Unlock();
		return;
	}

	if (!interfaceContainerPtr->signalLatch.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): The CalibrateWorker: \"%s\" does not have a QueueLatchInput delegate binded!"), *interfaceContainerPtr->baseContainer.workerID));
		threadLock.Unlock();
		return;
	}

	threadLock.Unlock();
	interfaceContainerPtr->signalLatch.Execute(latchData);
}

void LensSolverWorkDistributor::QueueCalibrationResult(const FCalibrationResult calibrationResult)
{
	queuedCalibrationResults.Enqueue(calibrationResult);
	threadLock.Lock();

	FJob* jobPtr = jobs.Find(calibrationResult.baseParameters.jobID);
	if (jobPtr == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): Unknown error occurred while queuing calibration result, no jobs are registered with ID: \"%s\"."), *calibrationResult.baseParameters.jobID));
		threadLock.Unlock();
		return;
	}

	FJobInfo jobInfo;
	bool done = false;

	jobPtr->currentResultCount++;

	if (jobPtr->currentResultCount >= jobPtr->expectedResultCount)
	{
		jobs.Remove(calibrationResult.baseParameters.jobID);
		jobInfo = jobPtr->jobInfo;
		done = true;
	}

	threadLock.Unlock();
	if (done && queueFinishedJobOutputDel->IsBound())
		queueFinishedJobOutputDel->Execute(jobInfo);
}

bool LensSolverWorkDistributor::CalibrationResultIsQueued()
{
	return queuedCalibrationResults.IsEmpty() == false;
}

void LensSolverWorkDistributor::DequeueCalibrationResult(FCalibrationResult & calibrationResult)
{
	queuedCalibrationResults.Dequeue(calibrationResult);
}

void LensSolverWorkDistributor::QueueLogAsync(FString msg)
{
	msg = FString::Printf(TEXT("Work Distributor: %s"), *msg);
	if (!queueLogOutputDel->IsBound())
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), *msg);
		return;
	}

	queueLogOutputDel->Execute(msg);
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

	FString* workerIDPtr = workerCalibrationIDLUT.Find(calibrationID);
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
	threadLock.Lock();
	FJob* job = jobs.Find(jobID);
	if (job == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): Cannot iterate image count, no job with ID: \"%s\" registered."), *jobID));
		threadLock.Unlock();
		return false;
	}

	FExpectedAndCurrentImageCount* expectedAndCurrentImageCount = job->expectedAndCurrentImageCounts.Find(calibrationID);
	if (expectedAndCurrentImageCount == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): Cannot iterate image count, the job: \"%s\" does not contain the calibration ID: \"%s\"."), *jobID, *calibrationID));
		threadLock.Unlock();
		return false;
	}

	expectedAndCurrentImageCount->currentImageCount++;

	if (expectedAndCurrentImageCount->currentImageCount >= expectedAndCurrentImageCount->expectedImageCount - 1)
	{
		if (debug)
			QueueLogAsync(FString::Printf(TEXT("(INFO): Completed processing all images of count %d/%d for calibration: \"%s\"."), expectedAndCurrentImageCount->currentImageCount, expectedAndCurrentImageCount->expectedImageCount, *calibrationID));

		threadLock.Unlock();
		return true;
	}

	if (debug)
		QueueLogAsync(FString::Printf(TEXT("(INFO): Iterate image count %d/%d for calibration: \"%s\"."), expectedAndCurrentImageCount->currentImageCount, expectedAndCurrentImageCount->expectedImageCount, *calibrationID));

	threadLock.Unlock();
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

/*
FString LensSolverWorkDistributor::ExpectedAndCurrentImageCountToString(const TMap<FString, FExpectedAndCurrentImageCount> & map, const int tabCount)
{;
	FString tabs(tabCount, "\t");
	TArray<FString> keys;
	map.GetKeys(keys);
	FString str = FString::Printf(TEXT("%s;
	for (FString key : keys)
	{

	}
	return FString::Printf(TEXT("%s{\n%s\tExpected: %d,\n\t%sCurrent: %d\n%s}"), 
		*tabs, 
		strct.expectedImageCount, 
		*tabs, 
		strct.currentImageCount, 
		*tabs);
}

FString LensSolverWorkDistributor::FJobInfoToString(const FJobInfo& job, const int tabCount)
{
	FString tabs(tabCount, "\t");
	FString str = FString::Printf(TEXT("%s\tJob ID: \"%s\","), *tabs, job.jobID);
	FString::Printf(TEXT("%s%s\tCalibration IDs:\n%s\t["), *str, *tabs, *tabs);
	for (int i = 0; i < job.calibrationIDs.Num() - 1; i++)
		str = FString::Printf(TEXT("%s\n%s\t\t%s,"), *str, *tabs, *job.calibrationIDs[i]);
	return FString::Printf(TEXT("%s\n%s\t\t%s\n%s\t],"), *str, *tabs, *job.calibrationIDs[job.calibrationIDs.Num() - 1], *tabs);
}

FString LensSolverWorkDistributor::FJobToString(const FJob& job, const int tabCount)
{
	FString tabs(tabCount, "\t");
	FString str(TEXT("%s\tJob:\n%s\t{\n%s\t\tExpected Result Count: %d,\n%s\t\tCurrent Result Count: %d,\n%s\t\tJobInfo: \n%s, \n\tExpected and Current Image Counts: \n%s\n%s\t}"),
		*tabs,
		*tabs,
		*tabs,
		job.currentResultCount,
		*tabs,
		job.currentResultCount,
		*FJobInfoToString(job.jobInfo, tabCount + 2),
		*ExpectedAndCurrentImageCountToString(job.expectedAndCurrentImageCounts tabCount + 2),
		*tabs);
}
*/

void LensSolverWorkDistributor::StopFindCornerWorkers()
{
	threadLock.Lock();
	if (findCornersWorkers.Num() == 0)
		return;

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

	threadLock.Unlock();
	findCornersWorkers.Empty();
	workLoadSortedFindCornerWorkers.Empty();
}

void LensSolverWorkDistributor::StopCalibrationWorkers()
{
	threadLock.Lock();
	if (calibrateWorkers.Num() == 0)
		return;

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

	threadLock.Unlock();
	calibrateWorkers.Empty();
	workLoadSortedCalibrateWorkers.Empty();
	workerCalibrationIDLUT.Empty();
}

void LensSolverWorkDistributor::StopBackgroundWorkers()
{
	StopFindCornerWorkers();
	StopCalibrationWorkers();
}

int LensSolverWorkDistributor::GetFindCornerWorkerCount()
{
	int count = 0;
	threadLock.Lock();
	count = findCornersWorkers.Num();
	threadLock.Unlock();

	return count;
}

int LensSolverWorkDistributor::GetCalibrateCount()
{
	int count = 0;
	threadLock.Lock();
	count = calibrateWorkers.Num();
	threadLock.Unlock();

	return count;
}
