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

DECLARE_DELEGATE_OneParam(QueueLogOutputDel, FString)
DECLARE_DELEGATE_OneParam(QueueFinishedJobOutputDel, FJobInfo)
DECLARE_DELEGATE_RetVal(int, GetWorkLoadOutputDel)
DECLARE_DELEGATE_RetVal(bool, IsClosingOutputDel)
DECLARE_DELEGATE(LockDel)
DECLARE_DELEGATE(UnlockDel)

struct FLensSolverWorkerParameters 
{
	const QueueLogOutputDel * inputQueueLogOutputDel;
	IsClosingOutputDel * inputIsClosingOutputDel;
	GetWorkLoadOutputDel * inputGetWorkOutputLoadDel;
	const LockDel* lockDel;
	const UnlockDel* unlockDel;
	bool debug;

	const FString inputWorkerID;
	FLensSolverWorkerParameters(
		const QueueLogOutputDel* inQueueLogOutputDel,
		IsClosingOutputDel* inIsClosingOutputDel,
		GetWorkLoadOutputDel* inGetWorkOutputLoadDel,
		const LockDel* inLockDel,
		const UnlockDel* inUnlockDel,
		const FString inWorkerID,
		const bool inDebug) :
		inputQueueLogOutputDel(inQueueLogOutputDel),
		inputIsClosingOutputDel(inIsClosingOutputDel),
		inputGetWorkOutputLoadDel(inGetWorkOutputLoadDel),
		lockDel(inLockDel),
		unlockDel(inUnlockDel),
		inputWorkerID(inWorkerID),
		debug(inDebug)
	{
	}
};

class FLensSolverWorker : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FLensSolverWorker>;

public:

private:
	const FString workerID;
	mutable bool flagToExit;

	const QueueLogOutputDel* queueLogOutputDel;
	const LockDel* lockDel;
	const UnlockDel* unlockDel;

	bool Exit ();

public:
	static FString JobDataToString(const FBaseParameters & baseParameters);
	FLensSolverWorker(const FLensSolverWorkerParameters & inputParameters);
	virtual ~FLensSolverWorker() {};

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FLensSolverWorker, STATGROUP_ThreadPoolAsyncTasks);
	}

	FString GetWorkerID();

	void DoWork();

protected:

	const bool debug;
	const FString calibrationVisualizationOutputPath;
	const FString workerMessage;
	
	bool ShouldExit();
	inline bool Debug() { return debug; }
	void Lock();
	void Unlock();
	void QueueLog(FString log);

	virtual void Tick() {};
	virtual int GetWorkLoad() { return 0; };
};