#include "LensSolverWorkDistributor.h"

void LensSolverWorkDisttributor::StartBackgroundWorkers(int findCornerWorkerCount, int calibrateWorkerCount, TSharedPtr<FLensSolverWorker::OnSolvePointsDel> inputOnSolvedPointsDel)
{
	if (findCornerWorkerCount <= 0 || calibrateWorkerCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Start Background Image Processors was called with 0 requested workers."));
		return;
	}

	threadLock.Lock();
	for (int i = 0; i < findCornerWorkerCount; i++)
	{
		FWorkerInterfaceContainer workerInterfaceContainer;

		UE_LOG(LogTemp, Log, TEXT("Starting lens solver worker: %d"), i);

		workerInterfaceContainer.worker = new FAutoDeleteAsyncTask<FLensSolverWorkerFindCorners>(
			&workerInterfaceContainer.isClosingDel,
			&workerInterfaceContainer.getWorkLoadDel, 
			&workerInterfaceContainer.queueWorkUnitDel,
			&workerInterfaceContainer.signalLatch,
			inputOnSolvedPointsDel,
			i);

		workerInterfaceContainer.worker->StartBackgroundTask();
		workers.Add(workerInterfaceContainer);
	}

	threadLock.Unlock();
}

FJobInfo LensSolverWorkDisttributor::RegisterJob(int latchedWorkUnitCount, UJobType jobType)
{
	return FJobInfo();
}

bool LensSolverWorkDisttributor::GetFindCornerWorkerParameters(TUniquePtr<FFindCornerWorkerParameters>& findCornerWorkerParameters)
{
	return false;
}

bool LensSolverWorkDisttributor::GetCalibrationWorkerParameters(TUniquePtr<FCalibrationWorkerParameters>& calibrationWorkerParameters)
{
	return false;
}

void LensSolverWorkDisttributor::StopBackgroundWorkers()
{
}
