/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolverWorker.h"

#include "Engine.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "RenderUtils.h"
#include "Engine/Texture2D.h"

FLensSolverWorker::FLensSolverWorker(FLensSolverWorkerParameters inputParameters) :
	workerMessage(FString::Printf("Worker (%d): "), inputParameters.inputWorkerID),
	calibrationVisualizationOutputPath(LensSolverUtilities::GenerateGenericOutputPath(FString::Printf(TEXT("CalibrationVisualizations/Worker-%d/"), workerID)))
{
	inputParameters.inputQueueWorkUnitInputDel->BindRaw(this, &FLensSolverWorker::QueueWorkUnit);
	inputParameters.inputGetWorkOutputLoadDel->BindRaw(this, &FLensSolverWorker::GetWorkLoad);
	inputParameters.inputIsClosingOutputDel->BindRaw(this, &FLensSolverWorker::Exit);
	queueLogOutputDel = inputParameters.inputQueueLogOutputDel;

	workerID = inputParameters.inputWorkerID;
	workUnitCount = 0;
	flagToExit = false;
}

int FLensSolverWorker::GetWorkLoad () 
{ 
	int count = 0;
	threadLock.Lock();
	count = workUnitCount;
	threadLock.Unlock();

	return count; 
}

void FLensSolverWorker::QueueWorkUnit(TUniquePtr<FLensSolverWorkUnit> workUnit)
{
	workQueue.Enqueue(workUnit);
	threadLock.Lock();
	workUnitCount++;
	threadLock.Unlock();
}

void FLensSolverWorker::DequeueWorkUnit(TUniquePtr<FLensSolverWorkUnit>& workUnit)
{
	workQueue.Dequeue(workUnit);
}

void FLensSolverWorker::QueueLog(FString log)
{
}

int FLensSolverWorker::GetWorkerID()
{
	return workerID;
}

void FLensSolverWorker::DoWork()
{
	while (!ShouldExit())
	{
		while (!WorkUnitInQueue() && !ShouldExit())
			continue;

		if (ShouldExit())
			break;

		Tick();
	}

	workQueue.Empty();
	flagToExit = true;
}

bool FLensSolverWorker::Exit()
{
	threadLock.Lock();
	flagToExit = true;
	threadLock.Unlock();
	return true;
}

bool FLensSolverWorker::ShouldExit()
{
	bool shouldExit = false;
	threadLock.Lock();
	shouldExit = flagToExit;
	threadLock.Unlock();
	return shouldExit;
}

void FLensSolverWorker::Lock()
{
	threadLock.Lock();
}

void FLensSolverWorker::Unlock()
{
	threadLock.Unlock();
}

