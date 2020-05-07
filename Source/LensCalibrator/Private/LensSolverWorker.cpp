/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolverWorker.h"

#include "Engine.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "RenderUtils.h"
#include "Engine/Texture2D.h"

FLensSolverWorker::FLensSolverWorker(FLensSolverWorkerParameters inputParameters) :
	workerID(inputParameters.inputWorkerID),
	calibrationVisualizationOutputPath(LensSolverUtilities::GenerateGenericOutputPath(FString::Printf(TEXT("CalibrationVisualizations/Worker-%d/"), workerID))),
	workerMessage(FString::Printf(TEXT("Worker (%s): "), inputParameters.inputWorkerID))
{
	inputParameters.inputGetWorkOutputLoadDel->BindRaw(this, &FLensSolverWorker::GetWorkLoad);
	inputParameters.inputIsClosingOutputDel->BindRaw(this, &FLensSolverWorker::Exit);
	queueLogOutputDel = inputParameters.inputQueueLogOutputDel;

	flagToExit = false;
}

void FLensSolverWorker::QueueLog(FString log)
{
	if (!queueLogOutputDel->IsBound())
		return;
	queueLogOutputDel->Execute(log);
}

FString const& FLensSolverWorker::GetWorkerID()
{
	FString copyOfWorkerID;
	Lock();
	copyOfWorkerID = workerID;
	Unlock();
	return copyOfWorkerID;
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

