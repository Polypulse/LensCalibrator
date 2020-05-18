/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Engine.h"
#include "Async/AsyncWork.h"
#include "SolvedPoints.h"

#pragma push_macro("check")
#undef check
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#pragma pop_macro("check")
#include <vector>

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
	virtual ~FLensSolverWorker() 
	{
		/*
		isClosingOutputDel->Unbind();
		getWorkOutputLoadDel->Unbind();

		isClosingOutputDel = nullptr;
		getWorkOutputLoadDel = nullptr;
		*/
		/*
		if (isClosingOutputDel != nullptr && isClosingOutputDel->IsBound())
			isClosingOutputDel->Unbind();
		if (getWorkOutputLoadDel != nullptr && getWorkOutputLoadDel->IsBound())
			getWorkOutputLoadDel->Unbind();
		*/
	};

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
};