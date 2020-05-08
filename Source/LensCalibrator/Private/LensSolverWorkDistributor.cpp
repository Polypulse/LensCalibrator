#include "LensSolverWorkDistributor.h"

void LensSolverWorkDistributor::PrepareFindCornerWorkers(
	int findCornerWorkerCount)
{
	if (findCornerWorkerCount <= 0)
	{
		QueueLogAsync("(ERROR): Cannot start calibration with 0 FindCorner workers.");
		return;
	}

	queueCalibrateWorkUnitInputDel.BindRaw(this, &LensSolverWorkDistributor::QueueCalibrateWorkUnit);

	/*
	isFenceDownDel.BindRaw(this, &LensSolverWorkDistributor::IsFenceDown);
	fenceUp = true;
	*/
	/*
	lockDel.BindRaw(this, &LensSolverWorkDistributor::Lock);
	unlockDel.BindRaw(this, &LensSolverWorkDistributor::Unlock);
	*/

	Lock();

	for (int i = 0; i < findCornerWorkerCount; i++)
	{
		FString guid = FGuid::NewGuid().ToString();
		workLoadSortedFindCornerWorkers.Add(guid);
		FWorkerFindCornersInterfaceContainer & interfaceContainer = findCornersWorkers.Add(guid, FWorkerFindCornersInterfaceContainer());

		FLensSolverWorkerParameters workerParameters(
			queueLogOutputDel,
			&interfaceContainer.baseContainer.isClosingDel,
			&interfaceContainer.baseContainer.getWorkLoadDel,
			// &isFenceDownDel,
			// &lockDel,
			// &unlockDel,
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
	}

	Unlock();
	if (debug)
		QueueLogAsync(FString::Printf(TEXT("(INFO): Started %d FindCorner workers"), findCornerWorkerCount));
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
			queueLogOutputDel,
			&interfaceContainer.baseContainer.isClosingDel,
			&interfaceContainer.baseContainer.getWorkLoadDel,
			// &isFenceDownDel,
			// &lockDel,
			// &unlockDel,
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
	}

	Unlock();
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

	Lock();
	jobs.Add(jobInfo.jobID, job);
	Unlock();

	if (debug)
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

	SortFindCornersWorkersByWorkLoad();

	const FString workerID = workLoadSortedFindCornerWorkers[0];
	if (workerID.IsEmpty())
	{
		QueueLogAsync("(ERROR): A worker ID in the work load sorted FindCornerWorker array is empty!");
		Unlock();
		return;
	}

	FWorkerFindCornersInterfaceContainer* interfaceContainer =nullptr;
	if (!GetFindCornersContainerInterfacePtr(workerID, interfaceContainer))
	{
		Unlock();
		return;
	}
	Unlock();

	if (!interfaceContainer->queuePixelArrayWorkUnitInputDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): FindCornerWorker: \"%s\" does not have a QueueWorkUnit delegate binded!"), *workerID));
		return;
	}

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

	// count++;
	// QueueLogAsync(FString::Printf(TEXT("Work Distributor received TextureFileWorkUnit of index: %d"), count));
	interfaceContainer->queueTextureFileWorkUnitInputDel.Execute(textureFileWorkUnit);
}

void LensSolverWorkDistributor::SetCalibrateWorkerParameters(FCalibrationParameters calibrationParameters)
{
	cachedCalibrationParameters = calibrationParameters;
}

void LensSolverWorkDistributor::QueueCalibrateWorkUnit(FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit)
{
	// static int count = 0;
	Lock();
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

	// count++;
	// QueueLogAsync(FString::Printf(TEXT("Work Distributor received TextureFileWorkUnit of index: %d"), count));
	interfaceContainerPtr->queueCalibrateWorkUnitDel.Execute(calibrateWorkUnit);

	bool hitExpectedImageCount = IterateImageCount(calibrateWorkUnit.baseParameters.jobID, calibrateWorkUnit.baseParameters.calibrationID);

	if (hitExpectedImageCount)
	{
		FCalibrateLatch latchData;
		latchData.baseParameters		= calibrateWorkUnit.baseParameters;
		latchData.calibrationParameters	= cachedCalibrationParameters;
		latchData.resizeParameters		= calibrateWorkUnit.resizeParameters;

		LatchCalibrateWorker(latchData);
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
	queuedCalibrationResults.Enqueue(calibrationResult);
	Lock();

	FJob* jobPtr = jobs.Find(calibrationResult.baseParameters.jobID);
	if (jobPtr == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): Unknown error occurred while queuing calibration result, no jobs are registered with ID: \"%s\"."), *calibrationResult.baseParameters.jobID));
		Unlock();
		return;
	}

	FJobInfo jobInfo;
	bool done = false;

	jobPtr->currentResultCount++;

	if (jobPtr->currentResultCount >= jobPtr->expectedResultCount)
	{
		jobInfo = jobPtr->jobInfo;
		jobs.Remove(calibrationResult.baseParameters.jobID);
		done = true;
	}

	Unlock();
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

	if (currentImageCount >= expectedImageCount)
	{
		if (debug)
			QueueLogAsync(FString::Printf(TEXT("(INFO): Completed processing all images of count %d/%d for calibration: \"%s\"."), currentImageCount, expectedImageCount, *calibrationID));

		Unlock();
		return true;
	}

	Unlock();
	if (debug)
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

void LensSolverWorkDistributor::Lock()
{
	/*
	while (!threadLock.TryLock())
		continue;
	*/
	threadLock.Lock();
}

void LensSolverWorkDistributor::Unlock()
{
	threadLock.Unlock();
}

/*
void LensSolverWorkDistributor::SetFenceDown()
{
	QueueLogAsync("Setting fence down.");
	fenceUp = false;
}

bool LensSolverWorkDistributor::IsFenceDown()
{
	return !fenceUp;
}
*/

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

	Unlock();
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
