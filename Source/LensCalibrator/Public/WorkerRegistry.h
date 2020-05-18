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
	WorkerRegistry() {}
	FCriticalSection threadLock;
	int workerCount = 0;

public:
	WorkerRegistry(WorkerRegistry const&) = delete;
	void operator=(WorkerRegistry const&) = delete;

	void CountWorker() 
	{
		threadLock.Lock();
		workerCount++; 
		threadLock.Unlock();
	}

	void UncountWorker() 
	{
		threadLock.Lock();
		workerCount--; 
		threadLock.Unlock();
	}

	bool WorkersRunning()
	{
		bool running = false;
		threadLock.Lock();
		running = workerCount > 0;
		threadLock.Unlock();
		return running;
	}
};