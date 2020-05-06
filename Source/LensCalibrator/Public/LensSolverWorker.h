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
	DECLARE_DELEGATE_OneParam(QueueWorkUnitInputDel, TUniquePtr<FLensSolverWorkUnit>)

	QueueLogOutputDel * inputQueueLogOutputDel;
	IsClosingOutputDel * inputIsClosingOutputDel;
	GetWorkLoadOutputDel * inputGetWorkOutputLoadDel;
	QueueWorkUnitInputDel * inputQueueWorkUnitInputDel;

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
	~FLensSolverWorker() {}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLensSolverWorker, STATGROUP_ThreadPoolAsyncTasks);
	}

	FString const& GetWorkerID();

protected:

	const FString calibrationVisualizationOutputPath;
	const FString workerMessage;
	
	void DoWork();
	virtual void Tick() = 0;

	bool ShouldExit();
	void Lock();
	void Unlock();
	void QueueLog(FString log);

	virtual int GetWorkLoad() = 0;
	virtual bool WorkUnitInQueue() = 0;
	virtual void QueueWorkUnit(TUniquePtr<FLensSolverWorkUnit> workUnit) = 0;
};