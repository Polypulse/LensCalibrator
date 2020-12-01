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


#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine.h"

/* The purpose of this class is to flag to workers various global states
happening in the plugin such as a shutdown. This class is a singleton. */
class WorkerRegistry
{
public:
	/* Get the singleton instance of this class. */
	static WorkerRegistry & Get()
	{
		static WorkerRegistry workerRegistry;
		return workerRegistry;
	}

private:
	WorkerRegistry() 
	{
		isShuttingDown = false;
	}

	/* The lock for workers to access the global state. */
	FCriticalSection threadLock;

	/* When workers are initialized, they will call count/uncount methods in this class. */
	int findCornerWorkerCount = 0;
	int calibrateWorkerCount = 0;

	bool isShuttingDown;

public:
	WorkerRegistry(WorkerRegistry const&) = delete;
	void operator=(WorkerRegistry const&) = delete;

	/* This method is called on the main thread to flag to workers to exit their loops. */
	void FlagExitAllShutdown ()
	{
		threadLock.Lock();
		isShuttingDown = true;
		threadLock.Unlock();
	}

	/* Workwers call this method to determine whether they should exit their loops. */
	bool ShouldExitAll ()
	{
		bool shuttingDown;

		threadLock.Lock();
		shuttingDown = isShuttingDown;
		threadLock.Unlock();

		return shuttingDown;
	}

	/* When a calibration worker is initialized, this method will be called by that worker. */
	void CountCalibrateWorker() 
	{
		threadLock.Lock();
		calibrateWorkerCount++; 
		threadLock.Unlock();
	}

	/* When a calibration worker is de-initialized, this method will be called by that worker. */
	void UncountCalibrateWorker() 
	{
		threadLock.Lock();
		calibrateWorkerCount--; 
		threadLock.Unlock();
	}

	/* This is called by the main thread to determine if we still have calibration workers running. */
	bool CalibrateWorkersRunning()
	{
		bool running = false;
		threadLock.Lock();
		running = calibrateWorkerCount > 0;
		threadLock.Unlock();
		return running;
	}

	/* When a find corner worker is initialized, this method will be called by that worker.*/
	void CountFindCornerWorker() 
	{
		threadLock.Lock();
		findCornerWorkerCount++; 
		threadLock.Unlock();
	}

	/* When a find corner worker is de-initialized, this method will be called by that worker.*/
	void UncountFindCornerWorker() 
	{
		threadLock.Lock();
		findCornerWorkerCount--; 
		threadLock.Unlock();
	}

	/* This is called by the main thread to determine if we still have find corner workers running. */
	bool FindCornersWorkersRunning()
	{
		bool running = false;
		threadLock.Lock();
		running = findCornerWorkerCount > 0;
		threadLock.Unlock();
		return running;
	}

	/* This is called by the main thread to determine if any workers are running. */
	bool WorkersRunning()
	{
		bool running = false;
		threadLock.Lock();
		running = findCornerWorkerCount  > 0 && calibrateWorkerCount > 0;
		threadLock.Unlock();
		return running;
	}
};