/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolverWorker.h"

#include "Engine.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "RenderUtils.h"
#include "Engine/Texture2D.h"
#include "WorkerRegistry.h"

FLensSolverWorker::FLensSolverWorker(FLensSolverWorkerParameters& inputParameters) :
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

	while (!ShouldExit())
	{
		if (GetWorkLoad() == 0)
		{
			FPlatformProcess::Sleep(1.0f);
			if (GetWorkLoad() == 0)
				continue;
		}

		baseWorker->Tick();
	}

	QueueLog("Exited loop.");
	NotifyShutdown();
}

bool FLensSolverWorker::Exit()
{
	Lock();
	flagToExit = true;
	Unlock();
	if (Debug())
		QueueLog("Exiting worker.");
	return true;
}

bool FLensSolverWorker::ShouldExit()
{
	bool shouldExit = false;

	Lock();
	shouldExit = flagToExit;
	Unlock();

	return shouldExit || WorkerRegistry::Get().ShouldExitAll();
}

bool FLensSolverWorker::Debug()
{
	static IConsoleVariable * variable = IConsoleManager::Get().FindConsoleVariable(TEXT("LensCalibrator.Debug"));
	if (variable != nullptr && variable->GetInt() == 0)
		return false;
	return true;
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
	return FString::Printf(TEXT("Calibration ID: (%s)"), *baseParameters.calibrationID);
	// return FString::Printf(TEXT("(Job ID: \"%s\", Calibration ID: \"%s\", Friendly Name: \"%s\", Zoom Level: %f"), *baseParameters.jobID, *baseParameters.calibrationID, *baseParameters.friendlyName, baseParameters.zoomLevel);
}

