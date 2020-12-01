/*
 * Copyright (C) 2020 - LensCalibrator contributors, see Contributors.txt at the root of the project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


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

/* Queue log message to main thread so that it can be dequeued and printed to the console on the main thread. */
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

/* Used by ULensSolverWorkDistributor for identification purposes. */
FString FLensSolverWorker::GetWorkerID()
{
	FString copyOfWorkerID;
	Lock();
	copyOfWorkerID = workerID;
	Unlock();
	return copyOfWorkerID;
}

/* All background work for any derived classes stem from this call and is executed in a separate thread. */
void FLensSolverWorker::DoWork()
{
	FLensSolverWorker* baseWorker = this;

	/* Keep the thread alive in this while loop until the worker has been flagged to exit. */
	while (!ShouldExit())
	{
		/* Determine if there is any work to do. */
		if (GetWorkLoad() == 0)
		{
			/* Sleep for 1 second to reduce CPU load when this worker is idling. */
			FPlatformProcess::Sleep(1.0f);

			/* Continue to the next loop if we still don't have work to do. */
			if (GetWorkLoad() == 0)
				continue;
		}

		/* Call the overrided calculation method that implements this class. */
		baseWorker->Tick();
	}

	/* Log to main thread that this worker has exited it's loop. */
	QueueLog("Exited loop.");

	/* Notify derived class that were shutting down, so clean up. */
	NotifyShutdown();
}

/* Exit worker loop, reset anything and queue a message log to the main thread that we've exited. */
bool FLensSolverWorker::Exit()
{
	Lock();
	flagToExit = true;
	Unlock();
	if (Debug())
		QueueLog("Exiting worker.");
	return true;
}

/* Check flags from main thread whether this worker should exit it's loop. */
bool FLensSolverWorker::ShouldExit()
{
	bool shouldExit = false;

	Lock();
	shouldExit = flagToExit;
	Unlock();

	/* Check whether only this worker has been flagged to exit, or whether all workers have been flagged to exit. */
	return shouldExit || WorkerRegistry::Get().ShouldExitAll();
}

/* Is debug mode enabled? */
bool FLensSolverWorker::Debug()
{
	/* This console variable is registered in the plugin module: ULensSolver */
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

/* Build debugging log string. */
FString FLensSolverWorker::JobDataToString(const FBaseParameters & baseParameters)
{
	return FString::Printf(TEXT("Calibration ID: (%s)"), *baseParameters.calibrationID);
}

