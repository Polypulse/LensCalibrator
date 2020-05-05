#include "LensSolverWorkDistributor.h"

void LensSolverWorkDistributor::StartBackgroundWorkers(
	int findCornerWorkerCount,
	int calibrateWorkerCount,
	FLensSolverWorkerParameters::QueueLogOutputDel* inputQueueLogOutputDel,
	FLensSolverWorkerCalibrate::QueueCalibrationResultOutputDel * inputOnSolvedPointsDel)
{
	if (findCornerWorkerCount <= 0 || calibrateWorkerCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Start Background Image Processors was called with 0 requested workers."));
		return;
	}

	threadLock.Lock();

	for (int i = 0; i < findCornerWorkerCount; i++)
	{
		findCornersWorkers.Add(MakeUnique<FWorkerFindCornersInterfaceContainer>());
		FLensSolverWorkerParameters workerParameters;

		workerParameters.inputQueueLogOutputDel		= inputQueueLogOutputDel;
		workerParameters.inputIsClosingOutputDel	= &findCornersWorkers[i]->isClosingDel;
		workerParameters.inputGetWorkOutputLoadDel	= &findCornersWorkers[i]->getWorkLoadDel;
		workerParameters.inputQueueWorkUnitInputDel = &findCornersWorkers[i]->queueWorkUnitDel;

		findCornersWorkers[i]->queueFindCornerResultOutputDel.BindRaw(this, LensSolverWorkDistributor::QueueCalibrateWorkerUnit);

		UE_LOG(LogTemp, Log, TEXT("Starting lens solver worker: %d"), i);

		findCornersWorkers[i]->worker = MakeUnique<FAutoDeleteAsyncTask<FLensSolverWorkerFindCorners>>(
			workerParameters,
			&findCornersWorkers[i]->queueFindCornerResultOutputDel);

		findCornersWorkers[i]->worker->StartBackgroundTask();
	}

	for (int i = 0; i < calibrateWorkerCount; i++)
	{
		calibrateWorkers.Add(MakeUnique<FWorkerCalibrateInterfaceContainer>());
		FLensSolverWorkerParameters workerParameters;

		workerParameters.inputQueueLogOutputDel		= inputQueueLogOutputDel;
		workerParameters.inputIsClosingOutputDel	= &calibrateWorkers[i]->isClosingDel;
		workerParameters.inputGetWorkOutputLoadDel	= &calibrateWorkers[i]->getWorkLoadDel;
		workerParameters.inputQueueWorkUnitInputDel = &calibrateWorkers[i]->queueWorkUnitDel;

		UE_LOG(LogTemp, Log, TEXT("Starting lens solver worker: %d"), i);

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

void LensSolverWorkDistributor::QueueCalibrateWorkerUnit(const TUniquePtr<FLensSolverCalibrateWorkUnit> calibrateWorkUnit)
{
}

void LensSolverWorkDistributor::StopBackgroundWorkers()
{
}
