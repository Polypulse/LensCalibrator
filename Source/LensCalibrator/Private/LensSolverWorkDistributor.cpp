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

		TUniquePtr<FWorkerFindCornersInterfaceContainer> interfaceContianer = MakeUnique<FWorkerFindCornersInterfaceContainer>();
		FLensSolverWorkerParameters workerParameters;

		workerParameters.inputQueueLogOutputDel		= queueLogOutputDel;
		workerParameters.inputIsClosingOutputDel	= &interfaceContianer->isClosingDel;
		workerParameters.inputGetWorkOutputLoadDel	= &interfaceContianer->getWorkLoadDel;
		workerParameters.inputQueueWorkUnitInputDel = &interfaceContianer->queueWorkUnitDel;

		interfaceContianer->queueFindCornerResultOutputDel.BindRaw(this, LensSolverWorkDistributor::QueueCalibrateWorkUnit);

		FString guid = FGuid::NewGuid().ToString();
		interfaceContianer->workerID = guid;
		workerParameters.inputWorkerID = guid;

		UE_LOG(LogTemp, Log, TEXT("Starting lens solver worker: %d"), i);

		interfaceContianer->worker = MakeUnique<FAutoDeleteAsyncTask<FLensSolverWorkerFindCorners>>(
			workerParameters,
			&interfaceContianer->queueFindCornerResultOutputDel);

		interfaceContianer->worker->StartBackgroundTask();
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
		TUniquePtr<FWorkerCalibrateInterfaceContainer> interfaceContainer = MakeUnique<FWorkerCalibrateInterfaceContainer>();
		FLensSolverWorkerParameters workerParameters;

		workerParameters.inputQueueLogOutputDel		= queueLogOutputDel;
		workerParameters.inputIsClosingOutputDel	= &interfaceContainer->isClosingDel;
		workerParameters.inputGetWorkOutputLoadDel	= &interfaceContainer->getWorkLoadDel;
		workerParameters.inputQueueWorkUnitInputDel = &interfaceContainer->queueWorkUnitDel;

		FString guid = FGuid::NewGuid().ToString();
		workerParameters.inputWorkerID = guid;
		interfaceContainer->workerID = guid;

		QueueLogAsync(FString::Printf(TEXT("Starting lens solver worker: %d"), i));

		interfaceContainer->worker = MakeUnique<FAutoDeleteAsyncTask<FLensSolverWorkerCalibrate>>(
			workerParameters,
			&interfaceContainer->signalLatch,
			inputOnSolvedPointsDel);

		interfaceContainer->worker->StartBackgroundTask();
		calibrateWorkers.Add(guid, MoveTemp(interfaceContainer));
	}

	threadLock.Unlock();
}

FJobInfo LensSolverWorkDistributor::RegisterJob(const int expectedResultCount, const UJobType jobType)
{
	FJobInfo jobInfo;
	jobInfo.jobID = FGuid::NewGuid().ToString();
	jobInfo.jobType = jobType;

	FJob job;
	job.jobInfo = jobInfo;
	job.expectedResultCount = expectedResultCount;
	job.currentResultCount = 0;

	jobs.Add(jobInfo.jobID, job);

	return jobInfo;
}

void LensSolverWorkDistributor::QueueTextureArrayWorkUnit(const FString & jobID, TUniquePtr<FLensSolverPixelArrayWorkUnit> pixelArrayWorkUnit)
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

	if (!interfaceContainer->queueWorkUnitDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): FindCornerWorker: \"%s\" does not have a QueueWorkUnit delegate binded!"), *workerID));
		return;
	}

	interfaceContainer->queueWorkUnitDel.Execute(MoveTemp(pixelArrayWorkUnit));
}

void LensSolverWorkDistributor::QueueTextureFileWorkUnit(const FString & jobID, TUniquePtr<FLensSolverTextureFileWorkUnit> textureFileWorkUnit)
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

	if (!interfaceContainer->queueWorkUnitDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): CalibrateWorker: \"%s\" does not have a QueueWorkUnit delegate binded!"), *workerID));
		return;
	}

	interfaceContainer->queueWorkUnitDel.Execute(MoveTemp(textureFileWorkUnit));
}

void LensSolverWorkDistributor::QueueCalibrateWorkUnit(TUniquePtr<FLensSolverCalibrateWorkUnit> calibrateWorkUnit)
{
	if (!calibrateWorkUnit.IsValid())
	{
		QueueLogAsync("(ERROR): Received NULL LensSolverCalibrateWorkUnit from FindCornerWorker!");
		return;
	}

	TUniquePtr<FWorkerCalibrateInterfaceContainer>* interfaceContainerUniquePtr;
	if (!GetCalibrateWorkerInterfaceContainerPtr(calibrateWorkUnit->calibrationID, interfaceContainerUniquePtr))
		return;
	FWorkerCalibrateInterfaceContainer* interfaceContainerPtr = interfaceContainerUniquePtr->Get();

	if (!interfaceContainerPtr->queueWorkUnitDel.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): The QueueWOrkUnit delegate is not binded for CalibrateWorker with ID: \"%s\"."), *interfaceContainerPtr->workerID));
		return;
	}
	
	interfaceContainerPtr->queueWorkUnitDel.Execute(MoveTemp(calibrateWorkUnit));
}

void LensSolverWorkDistributor::LatchCalibrateWorker(const FLatchData& latchData)
{
	if (latchData.calibrationID.IsEmpty())
	{
		QueueLogAsync("(ERROR): The calibration ID is empty!");
		return;
	}

	TUniquePtr<FWorkerCalibrateInterfaceContainer>* interfaceContainerUniquePtr;
	if (!GetCalibrateWorkerInterfaceContainerPtr(latchData.calibrationID, interfaceContainerUniquePtr))
		return;
	FWorkerCalibrateInterfaceContainer* interfaceContainerPtr = interfaceContainerUniquePtr->Get();
	if (!interfaceContainerPtr->signalLatch.IsBound())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): The CalibrateWorker: \"%s\" does not have a QueueLatchInput delegate binded!"), *interfaceContainerPtr->workerID));
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

bool LensSolverWorkDistributor::GetFindCornersContainerInterfacePtr(const FString& workerID, FWorkerFindCornersInterfaceContainer *& interfaceContainer)
{
	interfaceContainer = nullptr;
	TUniquePtr<FWorkerFindCornersInterfaceContainer> * cornerWorker = findCornersWorkers.Find(workerID);

	if (cornerWorker == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): There is no FWorkerCalibrateInterfaceContainer registered for worker ID: \"%s\"!"), *workerID));
		return false;
	}

	if (cornerWorker->IsValid())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): The worker ID: \"%s\" is registered. However, the TUniquePtr reference to FWorkerCalibrateInterfaceContainer is NULL!"), *workerID));
		return false;
	}

	interfaceContainer = cornerWorker->Get();

	return true;
}

bool LensSolverWorkDistributor::GetCalibrateWorkerInterfaceContainerPtr(
	const FString& calibrationID,
	TUniquePtr<FWorkerCalibrateInterfaceContainer> *& interfaceContainerUniquePtr)
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

	interfaceContainerUniquePtr = calibrateWorkers.Find(workerID);
	if (interfaceContainerUniquePtr == nullptr)
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): We the worker ID: \"%s\". However, no CalibrateWorkerInterfaceContainer is registered with that ID!"), *workerID));
		return false;
	}

	if (!interfaceContainerUniquePtr->IsValid())
	{
		QueueLogAsync(FString::Printf(TEXT("(ERROR): The worker ID: \"%s\" is registered. However, the CalibrateWorkerInterfaceContainer is NULL!"), *workerID));
		interfaceContainerUniquePtr = nullptr;
		return false;
	}

	return true;
}

void LensSolverWorkDistributor::SortFindCornersWorkersByWorkLoad()
{
	LensSolverWorkDistributor* workDistributor = this;
	workLoadSortedFindCornerWorkers.Sort([workDistributor](const FWorkerInterfaceContainer& workerA, const FWorkerInterfaceContainer& workerB) 
	{
		TUniquePtr<FWorkerFindCornersInterfaceContainer> * interfaceContainerA = workDistributor->findCornersWorkers.Find(workerA.workerID);
		TUniquePtr<FWorkerFindCornersInterfaceContainer> * interfaceContainerB = workDistributor->findCornersWorkers.Find(workerB.workerID);
		return interfaceContainerA->Get()->getWorkLoadDel.Execute() > interfaceContainerB->Get()->getWorkLoadDel.Execute();
	});
}

void LensSolverWorkDistributor::SortCalibrateWorkersByWorkLoad()
{
	LensSolverWorkDistributor* workDistributor = this;
	workLoadSortedCalibrateWorkers.Sort([workDistributor](const FWorkerInterfaceContainer& workerA, const FWorkerInterfaceContainer& workerB) 
	{
		TUniquePtr<FWorkerCalibrateInterfaceContainer> * interfaceContainerA = workDistributor->calibrateWorkers.Find(workerA.workerID);
		TUniquePtr<FWorkerCalibrateInterfaceContainer> * interfaceContainerB = workDistributor->calibrateWorkers.Find(workerB.workerID);
		return interfaceContainerA->Get()->getWorkLoadDel.Execute() > interfaceContainerB->Get()->getWorkLoadDel.Execute();
	});
}

void LensSolverWorkDistributor::StopFindCornerWorkers()
{
}

void LensSolverWorkDistributor::StopCalibrationWorkers()
{
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
