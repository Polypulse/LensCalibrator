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
	workerMessage(FString::Printf(TEXT("Worker (%s): "), *inputParameters.inputWorkerID)),
	debug(inputParameters.debug)
{
	inputParameters.inputGetWorkOutputLoadDel->BindRaw(this, &FLensSolverWorker::GetWorkLoad);
	inputParameters.inputIsClosingOutputDel->BindRaw(this, &FLensSolverWorker::Exit);

	flagToExit = false;
}

void FLensSolverWorker::QueueLog(FString log)
{
	log = FString::Printf(TEXT("Worker (%s): %s"), *workerID, *log);
	if (!queueLogOutputDel->IsBound())
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), *log);
		return;
	}
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

	while (1)
	{
		if (ShouldExit())
		{
			if (debug)
				QueueLog("Exiting DoWork loop.");
			break;
		}

		baseWorker->Tick();
	}

	flagToExit = true;
}

bool FLensSolverWorker::Exit()
{
	threadLock.Lock();
	flagToExit = true;
	threadLock.Unlock();
	if (debug)
		QueueLog("Exiting worker.");
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

FString FLensSolverWorker::JobDataToString(const FBaseParameters & baseParameters)
{
	return FString::Printf(TEXT("(Job ID: \"%s\", Calibration ID: \"%s\", Friendly Name: \"%s\", Zoom Level: %f"), *baseParameters.jobID, *baseParameters.calibrationID, *baseParameters.friendlyName, baseParameters.zoomLevel);
}

