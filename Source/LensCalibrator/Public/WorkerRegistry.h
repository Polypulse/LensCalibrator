/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine.h"

class WorkerRegistry
{
public:
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
	FCriticalSection threadLock;
	int findCornerWorkerCount = 0;
	int calibrateWorkerCount = 0;

	bool isShuttingDown;

public:
	WorkerRegistry(WorkerRegistry const&) = delete;
	void operator=(WorkerRegistry const&) = delete;

	void FlagExitAllShutdown ()
	{
		threadLock.Lock();
		isShuttingDown = true;
		threadLock.Unlock();
	}

	bool ShouldExitAll ()
	{
		bool shuttingDown;

		threadLock.Lock();
		shuttingDown = isShuttingDown;
		threadLock.Unlock();

		return shuttingDown;
	}

	void CountCalibrateWorker() 
	{
		threadLock.Lock();
		calibrateWorkerCount++; 
		threadLock.Unlock();
	}

	void UncountCalibrateWorker() 
	{
		threadLock.Lock();
		calibrateWorkerCount--; 
		threadLock.Unlock();
	}

	bool CalibrateWorkersRunning()
	{
		bool running = false;
		threadLock.Lock();
		running = calibrateWorkerCount > 0;
		threadLock.Unlock();
		return running;
	}

	void CountFindCornerWorker() 
	{
		threadLock.Lock();
		findCornerWorkerCount++; 
		threadLock.Unlock();
	}

	void UncountFindCornerWorker() 
	{
		threadLock.Lock();
		findCornerWorkerCount--; 
		threadLock.Unlock();
	}

	bool FindCornersWorkersRunning()
	{
		bool running = false;
		threadLock.Lock();
		running = findCornerWorkerCount > 0;
		threadLock.Unlock();
		return running;
	}

	bool WorkersRunning()
	{
		bool running = false;
		threadLock.Lock();
		running = findCornerWorkerCount  > 0 && calibrateWorkerCount > 0;
		threadLock.Unlock();
		return running;
	}
};