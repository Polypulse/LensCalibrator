#include "LensSolverWorkDistributor.h"

void LensSolverWorkDistributor::StartFindCornerWorkers(
	int findCornerWorkerCount)
{
	if (findCornerWorkerCount <= 0)
	{
		QueueLogAsync("Start Background Image Processors was called with 0 requested workers.");
		return;
	}

	threadLock.Lock();

	for (int i = 0; i < findCornerWorkerCount; i++)
	{
		FWorkerFindCornersInterfaceContainer interfaceContianer;
		FLensSolverWorkerParameters workerParameters;

		workerParameters.inputQueueLogOutputDel		= queueLogOutputDel;
		workerParameters.inputIsClosingOutputDel	= &interfaceContianer.baseContainer.isClosingDel;
		workerParameters.inputGetWorkOutputLoadDel	= &interfaceContianer.baseContainer.getWorkLoadDel;

		interfaceContianer.queueFindCornerResultOutputDel.BindRaw(this, &LensSolverWorkDistributor::QueueCalibrateWorkUnit);

		FString guid = FGuid::NewGuid().ToString();
		interfaceContianer.baseContainer.workerID = guid;
		workerParameters.inputWorkerID = guid;

		UE_LOG(LogTemp, Log, TEXT("Starting lens solver worker: %d"), i);

		interfaceContianer.baseContainer.worker = MakeUnique<FAutoDeleteAsyncTask<FLensSolverWorkerFindCorners>>(
			workerParameters,
			interfaceContianer.queueTextureFileWorkUnitInputDel,
			&interfaceContianer.queuePixelArrayWorkUnitInputDel,
			&interfaceContianer.queueFindCornerResultOutputDel);

		interfaceContianer.baseContainer.worker->StartBackgroundTask();
		findCornersWorkers.Add(guid, MoveTemp(interfaceContianer));
	}

	threadLock.Unlock();
}

void LensSolverWorkDistributor::StartCalibrateWorkers(
	int calibrateWorkerCount,
	FLensSolverWorkerCalibrate::QueueCalibrationResultOutputDel * inputOnSolvedPointsDel)
{
	if (calibrateWorkerCount <= 0)
	{
		QueueLogAsync("(ERROR): Start Background Image Processors was called with 0 requested workers.");
		return;
	}

	threadLock.Lock();

	for (int i = 0; i < calibrateWorkerCount; i++)
	{
		FWorkerCalibrateInterfaceContainer interfaceContainer;
		FLensSolverWorkerParameters workerParameters;

		workerParameters.inputQueueLogOutputDel		= queueLogOutputDel;
		workerParameters.inputIsClosingOutputDel	= &interfaceContainer.baseContainer.isClosingDel;
		workerParameters.inputGetWorkOutputLoadDel	= &interfaceContainer.baseContainer.getWorkLoadDel;

		FString guid = FGuid::NewGuid().ToString();
		workerParameters.inputWorkerID = guid;
		interfaceContainer.baseContainer.workerID = guid;

		QueueLogAsync(FString::Printf(TEXT("Starting lens solver worker: %d"), i));

		interfaceContainer.baseContainer.worker = MakeUnique<FAutoDeleteAsyncTask<FLensSolverWorkerCalibrate>>(
			workerParameters,
			&interfaceContainer.queueCalibrateWorkUnitDel,
			&interfaceContainer.signalLatch,
			inputOnSolvedPointsDel);

		interfaceContainer.baseContainer.worker->StartBackgroundTask();
		calibrateWorkers.Add(guid, MoveTemp(interfaceContainer));
	}

	threadLock.Unlock();
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

	return jobInfo;
}

void LensSolverWorkDistributor::QueueTextureArrayWorkUnit(const FString & jobID, FLensSolverPixelArrayWorkUnit pixelArrayWorkUnit)
{
	if (workLoadSortedFindCornerWorkers.Num() == 0)
	{
		QueueLogAsync("(ERROR): The work load sorted FindCornerWorker array is empty!");
		return;
	}

	SortFindCornersWorkersByWorkLoad();

	const FString workerID = workLoadSortedFindCornerWorkers[0];
	if (workerID.IsEmpty())
	{
		QueueLogAsync("(ERROR): A worker ID in the work load sorted FindCornerWorker array is empty!");
		return;
	}

	FWorkerFindCornersInterfaceContainer* interfaceContainer;
	if (!GetFindCornersContainerInterfacePtr(workerID, interfaceContainer))
		return;

	if (!interfaceContainer->queuePixelArrayWorkUnitInputDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): FindCornerWorker: \"%s\" does not have a QueueWorkUnit delegate binded!"), *workerID));
		return;
	}

	interfaceContainer->queuePixelArrayWorkUnitInputDel.Execute(pixelArrayWorkUnit);
}

void LensSolverWorkDistributor::QueueTextureFileWorkUnit(const FString & jobID, FLensSolverTextureFileWorkUnit textureFileWorkUnit)
{
	if (workLoadSortedCalibrateWorkers.Num() == 0)
	{
		QueueLogAsync("(ERROR): The work load sorted CalibrateWorker array is empty!");
		return;
	}

	SortCalibrateWorkersByWorkLoad();

	const FString workerID = workLoadSortedCalibrateWorkers[0];
	if (workerID.IsEmpty())
	{
		QueueLogAsync("(ERROR): A worker ID in the work load sorted CalibrateWorker array is empty!");
		return;
	}

	FWorkerFindCornersInterfaceContainer* interfaceContainer;
	if (!GetFindCornersContainerInterfacePtr(workerID, interfaceContainer))
		return;

	if (!interfaceContainer->queueTextureFileWorkUnitInputDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): CalibrateWorker: \"%s\" does not have a QueueWorkUnit delegate binded!"), *workerID));
		return;
	}

	interfaceContainer->queueTextureFileWorkUnitInputDel.Execute(textureFileWorkUnit);
}

void LensSolverWorkDistributor::QueueCalibrateWorkUnit(FLensSolverCalibrateWorkUnit calibrateWorkUnit)
{
	FWorkerCalibrateInterfaceContainer * interfaceContainerPtr;
	if (!GetCalibrateWorkerInterfaceContainerPtr(calibrateWorkUnit.baseUnit.calibrationID, interfaceContainerPtr))
		return;

	if (!interfaceContainerPtr->queueCalibrateWorkUnitDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): The QueueWOrkUnit delegate is not binded for CalibrateWorker with ID: \"%s\"."), *interfaceContainerPtr->baseContainer.workerID));
		return;
	}

	interfaceContainerPtr->queueCalibrateWorkUnitDel.Execute(calibrateWorkUnit);

	if (IterateImageCount(calibrateWorkUnit.baseUnit.jobID, calibrateWorkUnit.baseUnit.jobID))
	{
		FLatchData latchData;
		latchData.jobID = calibrateWorkUnit.baseUnit.jobID;
		latchData.calibrationID = calibrateWorkUnit.baseUnit.calibrationID;

		LatchCalibrateWorker(latchData);
	}
}

void LensSolverWorkDistributor::LatchCalibrateWorker(const FLatchData& latchData)
{
	if (latchData.calibrationID.IsEmpty())
	{
		QueueLogAsync("(ERROR): The calibration ID is empty!");
		return;
	}

	FWorkerCalibrateInterfaceContainer* interfaceContainerPtr;
	if (!GetCalibrateWorkerInterfaceContainerPtr(latchData.calibrationID, interfaceContainerPtr))
		return;

	if (!interfaceContainerPtr->signalLatch.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): The CalibrateWorker: \"%s\" does not have a QueueLatchInput delegate binded!"), *interfaceContainerPtr->baseContainer.workerID));
		return;
	}

	interfaceContainerPtr->signalLatch.Execute(latchData);
}

void LensSolverWorkDistributor::QueueLogAsync(const FString msg)
{
	if (!queueLogOutputDel->IsBound())
		return;
	queueLogOutputDel->Execute(msg);
}

bool LensSolverWorkDistributor::GetFindCornersContainerInterfacePtr(
	const FString& workerID,
	FWorkerFindCornersInterfaceContainer *& outputInterfaceContainerPtr)
{
	outputInterfaceContainerPtr = nullptr;
	FWorkerFindCornersInterfaceContainer * interfaceContainerPtr = findCornersWorkers.Find(workerID);

	if (interfaceContainerPtr == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): There is no FWorkerCalibrateInterfaceContainer registered for worker ID: \"%s\"!"), *workerID));
		return false;
	}

	outputInterfaceContainerPtr = interfaceContainerPtr;

	return true;
}

bool LensSolverWorkDistributor::GetCalibrateWorkerInterfaceContainerPtr(
	const FString& calibrationID,
	FWorkerCalibrateInterfaceContainer *& outputInterfaceContainerPtr)
{
	outputInterfaceContainerPtr = nullptr;

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

	FWorkerCalibrateInterfaceContainer * interfaceContainerPtr = calibrateWorkers.Find(workerID);
	if (interfaceContainerPtr == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): We the worker ID: \"%s\". However, no CalibrateWorkerInterfaceContainer is registered with that ID!"), *workerID));
		return false;
	}

	outputInterfaceContainerPtr = interfaceContainerPtr;

	return true;
}

bool LensSolverWorkDistributor::IterateImageCount(const FString & jobID, const FString& calibrationID)
{
	threadLock.Lock();
	FJob* job = jobs.Find(jobID);
	if (job == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("Cannot iterate image count, no job with ID: \"%s\" registered."), *jobID));
		return false;
	}

	FExpectedAndCurrentImageCount* expectedAndCurrentImageCount = job->expectedAndCurrentImageCounts.Find(calibrationID);
	expectedAndCurrentImageCount->currentImageCount++;

	if (expectedAndCurrentImageCount->expectedImageCount <= expectedAndCurrentImageCount->currentImageCount)
		return true;

	threadLock.Unlock();

	return false;
}

void LensSolverWorkDistributor::SortFindCornersWorkersByWorkLoad()
{
	LensSolverWorkDistributor* workDistributor = this;
	workLoadSortedFindCornerWorkers.Sort([workDistributor](const FWorkerInterfaceContainer& workerA, const FWorkerInterfaceContainer& workerB) 
	{
		FWorkerFindCornersInterfaceContainer * interfaceContainerA = workDistributor->findCornersWorkers.Find(workerA.workerID);
		FWorkerFindCornersInterfaceContainer * interfaceContainerB = workDistributor->findCornersWorkers.Find(workerB.workerID);

		if (interfaceContainerA == nullptr || 
			interfaceContainerB == nullptr ||
			!interfaceContainerA->baseContainer.getWorkLoadDel.IsBound() ||
			!interfaceContainerB->baseContainer.getWorkLoadDel.IsBound())
			return false;

		return interfaceContainerA->baseContainer.getWorkLoadDel.Execute() > interfaceContainerB->baseContainer.getWorkLoadDel.Execute();
	});
}

void LensSolverWorkDistributor::SortCalibrateWorkersByWorkLoad()
{
	LensSolverWorkDistributor* workDistributor = this;
	workLoadSortedCalibrateWorkers.Sort([workDistributor](const FWorkerInterfaceContainer& workerA, const FWorkerInterfaceContainer& workerB) 
	{
		FWorkerCalibrateInterfaceContainer * interfaceContainerA = workDistributor->calibrateWorkers.Find(workerA.workerID);
		FWorkerCalibrateInterfaceContainer * interfaceContainerB = workDistributor->calibrateWorkers.Find(workerB.workerID);
		if (interfaceContainerA == nullptr || 
			interfaceContainerB == nullptr ||
			!interfaceContainerA->baseContainer.getWorkLoadDel.IsBound() ||
			!interfaceContainerB->baseContainer.getWorkLoadDel.IsBound())
			return false;
		return interfaceContainerA->baseContainer.getWorkLoadDel.Execute() > interfaceContainerB->baseContainer.getWorkLoadDel.Execute();
	});
}

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
