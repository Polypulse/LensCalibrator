/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolverWorker.h"

#include "Engine.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "RenderUtils.h"
#include "Engine/Texture2D.h"

FLensSolverWorker::FLensSolverWorker(const FLensSolverWorkerParameters & inputParameters) :
	workerID(inputParameters.inputWorkerID),
	calibrationVisualizationOutputPath(LensSolverUtilities::GenerateGenericOutputPath(FString::Printf(TEXT("CalibrationVisualizations/Worker-%s/"), *workerID))),
	queueLogOutputDel(inputParameters.inputQueueLogOutputDel),
	workerMessage(FString::Printf(TEXT("Worker (%s): "), *inputParameters.inputWorkerID))
{
	inputParameters.inputGetWorkOutputLoadDel->BindRaw(this, &FLensSolverWorker::GetWorkLoad);
	inputParameters.inputIsClosingOutputDel->BindRaw(this, &FLensSolverWorker::Exit);

	flagToExit = false;
}

void FLensSolverWorker::QueueLog(FString log)
{
	if (!queueLogOutputDel->IsBound())
		return;
	queueLogOutputDel->Execute(log);
}

FString FLensSolverWorker::GetWorkerID()
{
	FString copyOfWorkerID;
	Lock();
	copyOfWorkerID = workerID;
	Unlock();
	return copyOfWorkerID;
}

void FLensSolverWorker::DoWork()
{
	FLensSolverWorker* baseWorker = this;
	while (!ShouldExit())
		baseWorker->Tick();

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

