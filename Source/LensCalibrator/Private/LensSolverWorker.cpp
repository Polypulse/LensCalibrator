/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolverWorker.h"

#include "Engine.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "RenderUtils.h"
#include "Engine/Texture2D.h"

FLensSolverWorker::FLensSolverWorker(const FLensSolverWorkerParameters& inputParameters) :
	workerID(inputParameters.inputWorkerID),
	calibrationVisualizationOutputPath(LensSolverUtilities::GenerateGenericOutputPath(FString::Printf(TEXT("CalibrationVisualizations/Worker-%s/"), *workerID))),
	queueLogOutputDel(inputParameters.inputQueueLogOutputDel),
	// lockDel(inputParameters.lockDel),
	// unlockDel(inputParameters.unlockDel),
	// isFenceDownDel(inputParameters.inputIsFenceDownDel),
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
	/*
	if (!lockDel->IsBound() || !unlockDel->IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT("Lock/Unlock delegates are not bound!"));
		return;
	}
	*/
	/*
	while (!isFenceDownDel->Execute())
		continue;
	QueueLog("Fence down!");
	*/

	while (1)
	{
		if (ShouldExit())
		{
			if (debug)
				QueueLog("Exiting DoWork loop.");
			break;
		}

		if (GetWorkLoad() == 0)
		{
			// QueueLog("No work units in queue.");
			FPlatformProcess::Sleep(1.0f);
			if (GetWorkLoad() == 0)
			{
				continue;
			}
		}

		baseWorker->Tick();
	}

	QueueLog("Exited loop.");
	flagToExit = true;
}

bool FLensSolverWorker::Exit()
{
	Lock();
	flagToExit = true;
	Unlock();
	if (debug)
		QueueLog("Exiting worker.");
	return true;
}

bool FLensSolverWorker::ShouldExit()
{
	bool shouldExit = false;
	Lock();
	shouldExit = flagToExit;
	Unlock();
	return shouldExit;
}

void FLensSolverWorker::Lock()
{
	/*
	if (!lockDel->IsBound())
	{
		flagToExit = true;
		return;
	}

	lockDel->Execute();
	*/
	/*
	while (!threadLock.TryLock())
		continue;
	*/
	threadLock.Lock();
}

void FLensSolverWorker::Unlock()
{
	/*
	if (!unlockDel->IsBound())
	{
		flagToExit = true;
		return;
	}

	unlockDel->Execute();
	*/
	threadLock.Unlock();
}

FString FLensSolverWorker::JobDataToString(const FBaseParameters & baseParameters)
{
	return FString::Printf(TEXT("Calibration ID: (%s)"), *baseParameters.calibrationID);
	// return FString::Printf(TEXT("(Job ID: \"%s\", Calibration ID: \"%s\", Friendly Name: \"%s\", Zoom Level: %f"), *baseParameters.jobID, *baseParameters.calibrationID, *baseParameters.friendlyName, baseParameters.zoomLevel);
}

