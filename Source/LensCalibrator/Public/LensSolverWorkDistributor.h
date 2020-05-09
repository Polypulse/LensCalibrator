/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once
#include "Engine.h"

#include "LensSolverWorker.h"
#include "LensSolverWorkerFindCorners.h"
#include "LensSolverWorkerCalibrate.h"
#include "LensSolverWorkUnit.h"
#include "LensSolverWorkerInterfaceContainer.h"
#include "FindCornerWorkerParameters.h"
#include "CalibrationWorkerParameters.h"
#include "StartMediaStreamParameters.h"
#include "MediaAssets/Public/MediaTexture.h"
#include "MediaAssets/Public/MediaPlayer.h"
#include "JobInfo.h"
#include "Job.h"
#include "QueueContainers.h"
#include "ILensSolverEventReceiver.h"

class LensSolverWorkDistributor
{
private:
	LensSolverWorkDistributor() {}

	FCriticalSection threadLock;

	QueueCalibrationResultOutputDel queueCalibrationResultOutputDel;
	QueueCalibrateWorkUnitInputDel queueCalibrateWorkUnitInputDel;
	// IsFenceDownDel isFenceDownDel;

	// LockDel lockDel;
	// UnlockDel unlockDel;

	QueueFinishedJobOutputDel queueFinishedJobOutputDel;
	QueueLogOutputDel queueLogOutputDel;
	bool debug;

	// FThreadSafeBool fenceUp;

	FCalibrationParameters cachedCalibrationParameters;

	TMap<FString, FWorkerFindCornersInterfaceContainer> findCornersWorkers;
	TMap<FString, FWorkerCalibrateInterfaceContainer> calibrateWorkers;
	TArray<FString> workLoadSortedFindCornerWorkers;
	TArray<FString> workLoadSortedCalibrateWorkers;

	TMap<FString, FJob> jobs;
	TMap<FString, const FString> workerCalibrationIDLUT;
	TMap<FString, FMediaStreamWorkUnit> mediaTextureJobLUT;

	TQueue<CalibrationResultQueueContainer> queuedCalibrationResults;

	// FTexture2DRHIRef blitRenderTexture;
	// bool blitRenderTextureAllocated;

	void QueueLogAsync(FString msg);

	bool GetFindCornersContainerInterfacePtr(
		const FString workerID,
		FWorkerFindCornersInterfaceContainer*& outputInterfaceContainerPtr);

	bool GetCalibrateWorkerInterfaceContainerPtr(
		const FString calibrationID,
		FWorkerCalibrateInterfaceContainer *& outputInterfaceContainerPtr);

	void QueueCalibrateWorkUnit(FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit);
	void LatchCalibrateWorker(const FCalibrateLatch& latchData);
	void QueueCalibrationResult(const FCalibrationResult calibrationResult);

	bool IterateImageCount(const FString & jobID, const FString& calibrationID);

	void SortFindCornersWorkersByWorkLoad();
	void SortCalibrateWorkersByWorkLoad();

	bool ValidateMediaTexture(const UMediaTexture* inputTexture);

	void Lock();
	void Unlock();

	int64 GetTickNow();
	// bool IsFenceDown();
	void MediaTextureRenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FMediaStreamWorkUnit mediaStreamParameters);

protected:
public:

	static LensSolverWorkDistributor& GetInstance()
	{
		static LensSolverWorkDistributor workDistributor;
		return workDistributor;
	}

	LensSolverWorkDistributor(LensSolverWorkDistributor const&) = delete;
	void operator=(LensSolverWorkDistributor const&) = delete;

	/*
	static FString ExpectedAndCurrentImageCountToString(const TMap<FString, FExpectedAndCurrentImageCount> & map, const int tabCount);
	static FString FJobInfoToString(const FJobInfo& job, const int tabCount = 0);
	static FString FJobToString(const FJob& job, const int tabcount = 0);
	*/

	void Configure(QueueLogOutputDel*& inputQueueLogOutputDel,
		QueueFinishedJobOutputDel*& inputQueueFinishedJobOutputDel,
		bool debugEnabled = false)
	{
		inputQueueFinishedJobOutputDel = &queueFinishedJobOutputDel ;
		inputQueueLogOutputDel = &queueLogOutputDel ;
		debug = debugEnabled;
	}

	void PrepareFindCornerWorkers(
		int findCornerWorkerCount);
	void PrepareCalibrateWorkers(
		int calibrateWorkerCount);

	void StopFindCornerWorkers();
	void StopCalibrationWorkers();
	void StopBackgroundWorkers();

	int GetFindCornerWorkerCount();
	int GetCalibrateCount();

	FJobInfo RegisterJob (
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		const TArray<int> & expectedImageCounts,
		const int expectedResultCount,
		const UJobType jobType);

	void SetCalibrateWorkerParameters(FCalibrationParameters calibrationParameters);
	void QueueTextureArrayWorkUnit(const FString & jobID, FLensSolverPixelArrayWorkUnit pixelArrayWorkUnit);
	void QueueTextureFileWorkUnit(const FString & jobID, FLensSolverTextureFileWorkUnit textureFileWorkUnit);
	void QueueMediaStreamWorkUnit(const FMediaStreamWorkUnit mediaStreamWorkUnit);

	bool CalibrationResultIsQueued();
	void DequeueCalibrationResult(CalibrationResultQueueContainer & queueContainer);
	void PollMediaTextureStreams();

	// void SetFenceDown();
};