/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Engine.h"
#include "Async/AsyncWork.h"
#include "SolvedPoints.h"

#include "JobInfo.h"
#include "LensSolverUtilities.h"
#include "LensSolverWorkUnit.h"
#include "QueueContainers.h"

DECLARE_DELEGATE_OneParam(QueueLogOutputDel, FString)
DECLARE_DELEGATE_OneParam(QueueFinishedJobOutputDel, FinishedJobQueueContainer)
DECLARE_DELEGATE_RetVal(int, GetWorkLoadOutputDel)
DECLARE_DELEGATE_RetVal(bool, IsClosingOutputDel)

struct FLensSolverWorkerParameters 
{
	QueueLogOutputDel * inputQueueLogOutputDel;
	IsClosingOutputDel * inputIsClosingOutputDel;
	GetWorkLoadOutputDel * inputGetWorkOutputLoadDel;
	FString inputWorkerID;

	FLensSolverWorkerParameters(
		QueueLogOutputDel* inQueueLogOutputDel,
		IsClosingOutputDel* inIsClosingOutputDel,
		GetWorkLoadOutputDel* inGetWorkOutputLoadDel,
		FString inWorkerID) :
		inputQueueLogOutputDel(inQueueLogOutputDel),
		inputIsClosingOutputDel(inIsClosingOutputDel),
		inputGetWorkOutputLoadDel(inGetWorkOutputLoadDel),
		inputWorkerID(inWorkerID)
	{
	}
};

/* Base class for workers that executes code on a separate thread to provide
asynchronous calibration throughout this system. */
class FLensSolverWorker : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FLensSolverWorker>;

public:

private:
	FCriticalSection threadLock;
	FString workerID;
	bool flagToExit;

	QueueLogOutputDel* queueLogOutputDel;
	IsClosingOutputDel * isClosingOutputDel;
	GetWorkLoadOutputDel * getWorkOutputLoadDel;

	bool Exit ();

public:
	static FString JobDataToString(const FBaseParameters & baseParameters);
	FLensSolverWorker(FLensSolverWorkerParameters & inputParameters);
	virtual ~FLensSolverWorker() {};

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLensSolverWorker, STATGROUP_ThreadPoolAsyncTasks);
	}

	FString GetWorkerID();

	void DoWork();

protected:

	const FString calibrationVisualizationOutputPath;
	const FString workerMessage;
	
	bool ShouldExit();
	bool Debug();

	void Lock();
	void Unlock();

	void QueueLog(FString log);

	virtual void Tick() {};
	virtual int GetWorkLoad() { return 0; };
	virtual void NotifyShutdown () {};
};