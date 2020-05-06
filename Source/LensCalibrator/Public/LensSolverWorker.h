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
#include "LatchData.h"
#include "LensSolverUtilities.h"
#include "LensSolverWorkUnit.h"
#include "LensSolverWorker.generated.h"

USTRUCT(BlueprintType)
struct FLensSolverWorkerParameters 
{
	GENERATED_BODY()

	DECLARE_DELEGATE_OneParam(QueueLogOutputDel, FString)
	DECLARE_DELEGATE_RetVal(int, GetWorkLoadOutputDel)
	DECLARE_DELEGATE_RetVal(bool, IsClosingOutputDel)

	QueueLogOutputDel * inputQueueLogOutputDel;
	IsClosingOutputDel * inputIsClosingOutputDel;
	GetWorkLoadOutputDel * inputGetWorkOutputLoadDel;

	FString inputWorkerID;
};

class FLensSolverWorker : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FLensSolverWorker>;

public:

private:
	const FString workerID;
	mutable bool flagToExit;

	FLensSolverWorkerParameters::QueueLogOutputDel* queueLogOutputDel;
	FCriticalSection threadLock;

	bool Exit ();

public:
	FLensSolverWorker(FLensSolverWorkerParameters inputParameters);
	virtual ~FLensSolverWorker() {};

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLensSolverWorker, STATGROUP_ThreadPoolAsyncTasks);
	}

	FString const& GetWorkerID();

protected:

	const FString calibrationVisualizationOutputPath;
	const FString workerMessage;
	
	void DoWork();
	virtual void Tick() {};

	bool ShouldExit();
	void Lock();
	void Unlock();
	void QueueLog(FString log);

	virtual int GetWorkLoad() { return 0; };
	virtual bool WorkUnitInQueue() { return false; };
};