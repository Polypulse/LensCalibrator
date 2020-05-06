#include "LensSolverWorkDistributor.h"

void LensSolverWorkDistributor::StartFindCornerWorkers(
	int findCornerWorkerCount,
	FLensSolverWorkerParameters::QueueLogOutputDel* inputQueueLogOutputDel)
{
	if (findCornerWorkerCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Start Background Image Processors was called with 0 requested workers."));
		return;
	}

	queueLogOutputDel = inputQueueLogOutputDel;
	threadLock.Lock();

	for (int i = 0; i < findCornerWorkerCount; i++)
	{
		findCornersWorkers.Add(MakeUnique<FWorkerFindCornersInterfaceContainer>());
		FLensSolverWorkerParameters workerParameters;

		workerParameters.inputQueueLogOutputDel		= inputQueueLogOutputDel;
		workerParameters.inputIsClosingOutputDel	= &findCornersWorkers[i]->isClosingDel;
		workerParameters.inputGetWorkOutputLoadDel	= &findCornersWorkers[i]->getWorkLoadDel;
		workerParameters.inputQueueWorkUnitInputDel = &findCornersWorkers[i]->queueWorkUnitDel;

		findCornersWorkers[i]->queueFindCornerResultOutputDel.BindRaw(this, LensSolverWorkDistributor::QueueCalibrateWorkUnit);

		UE_LOG(LogTemp, Log, TEXT("Starting lens solver worker: %d"), i);

		findCornersWorkers[i]->worker = MakeUnique<FAutoDeleteAsyncTask<FLensSolverWorkerFindCorners>>(
			workerParameters,
			&findCornersWorkers[i]->queueFindCornerResultOutputDel);

		findCornersWorkers[i]->worker->StartBackgroundTask();
	}

	threadLock.Unlock();
}

void LensSolverWorkDistributor::StartCalibrateWorkers(int calibrateWorkerCount, FLensSolverWorkerParameters::QueueLogOutputDel* inputQueueLogOutputDel, FLensSolverWorkerCalibrate::QueueCalibrationResultOutputDel* inputOnSolvedPointsDel)
{
	if (calibrateWorkerCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Start Background Image Processors was called with 0 requested workers."));
		return;
	}

	queueLogOutputDel = inputQueueLogOutputDel;
	threadLock.Lock();

	for (int i = 0; i < calibrateWorkerCount; i++)
	{
		calibrateWorkers.Add(MakeUnique<FWorkerCalibrateInterfaceContainer>());
		FLensSolverWorkerParameters workerParameters;

		workerParameters.inputQueueLogOutputDel		= inputQueueLogOutputDel;
		workerParameters.inputIsClosingOutputDel	= &calibrateWorkers[i]->isClosingDel;
		workerParameters.inputGetWorkOutputLoadDel	= &calibrateWorkers[i]->getWorkLoadDel;
		workerParameters.inputQueueWorkUnitInputDel = &calibrateWorkers[i]->queueWorkUnitDel;

		QueueLogAsync(FString::Printf(TEXT("Starting lens solver worker: %d"), i));

		calibrateWorkers[i]->worker = MakeUnique<FAutoDeleteAsyncTask<FLensSolverWorkerCalibrate>>(
			workerParameters,
			&calibrateWorkers[i]->signalLatch,
			inputOnSolvedPointsDel);

		findCornersWorkers[i]->worker->StartBackgroundTask();
	}

	threadLock.Unlock();
}

FJobInfo LensSolverWorkDistributor::RegisterJob(int latchedWorkUnitCount, UJobType jobType)
{
	return FJobInfo();
}

void LensSolverWorkDistributor::QueueTextureArrayWorkUnit(TUniquePtr<FLensSolverPixelArrayWorkUnit> pixelArrayWorkUnit)
{
	SortFindCornersWorkersByWorkLoad();

	if (!findCornersWorkers[0]->queueWorkUnitDel.IsBound())
	{
		QueueLogAsync("(ERROR): Worker does not have a QueueWorkUnit delegate binded!");
		return;
	}

	findCornersWorkers[0]->queueWorkUnitDel.Execute(MoveTemp(pixelArrayWorkUnit));
}

void LensSolverWorkDistributor::QueueTextureFileWorkUnit(TUniquePtr<FLensSolverTextureFileWorkUnit> textureFileWorkUnit)
{
	SortFindCornersWorkersByWorkLoad();

	if (!findCornersWorkers[0]->queueWorkUnitDel.IsBound())
	{
		QueueLogAsync("(ERROR): Worker does not have a QueueWorkUnit delegate binded!");
		return;
	}

	findCornersWorkers[0]->queueWorkUnitDel.Execute(MoveTemp(textureFileWorkUnit));
}

void LensSolverWorkDistributor::QueueCalibrateWorkUnit(TUniquePtr<FLensSolverCalibrateWorkUnit> calibrateWorkUnit)
{
	exclusiveQueues.Find(calibrateWorkUnit->calibrationID);
	if (exclusiveQueues.Contains())
}

void LensSolverWorkDistributor::QueueLogAsync(const FString msg)
{
	if (!queueLogOutputDel->IsBound())
		return;
	queueLogOutputDel->Execute(msg);
}

void LensSolverWorkDistributor::SortFindCornersWorkersByWorkLoad()
{
	findCornersWorkers.Sort([](const FWorkerInterfaceContainer& workerA, const FWorkerInterfaceContainer& workerB) {
		return workerA.getWorkLoadDel.Execute() > workerB.getWorkLoadDel.Execute();
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
