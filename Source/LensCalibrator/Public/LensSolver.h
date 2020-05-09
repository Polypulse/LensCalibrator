/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraComponent.h"
#include "MediaAssets/Public/MediaTexture.h"
#include "MediaAssets/Public/MediaPlayer.h"
#include "CommonRenderResources.h"

#include "SolvedPoints.h"
#include "LensSolverWorker.h"
#include "LensSolverWorkerCalibrate.h"
#include "Job.h"
#include "LensSolverWorkDistributor.h"
#include "OneTimeProcessParameters.h"
#include "StartMediaStreamParameters.h"

#include "TextureArrayZoomPair.h"
#include "TextureFolderZoomPair.h"
#include "TextureZoomPair.h"

#include "LensSolverWorkerInterfaceContainer.h"
#include "QueueContainers.h"

#include "ILensSolverEventReceiver.h"

#include "LensSolver.generated.h"

UCLASS()
class LENSCALIBRATOR_API ULensSolver : public UObject
{
	GENERATED_BODY()

private:
	QueueLogOutputDel * queueLogOutputDel;
	QueueFinishedJobOutputDel * queueFinishedJobOutputDel;

	TQueue<FinishedJobQueueContainer> queuedFinishedJobs;
	TQueue<FString> logQueue;

	bool ValidateMediaTexture(const UMediaTexture* inputTexture);

	void PollLogs();
	void PollCalibrationResults ();
	void PollFinishedJobs();

	void QueueFinishedJob(FinishedJobQueueContainer queueContainer);
	bool FinishedJobIsQueued();
	void DequeuedFinishedJob(FinishedJobQueueContainer &queueContainer);
	void QueueLog(FString msg);
	bool Debug();

public:

	ULensSolver()  {}
	~ULensSolver() {}

	void OneTimeProcessArrayOfTextureFolderZoomPairs(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		TArray<FTextureFolderZoomPair> inputTextures, 
		FOneTimeProcessParameters oneTimeProcessParameters,
		FJobInfo & ouptutJobInfo);

	void StartMediaStreamCalibration(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FStartMediaStreamParameters mediaStreamParameters,
		FJobInfo& ouptutJobInfo);

	void StartBackgroundImageProcessors(int findCornersWorkerCount, int calibrateWorkerCount);
	void StopBackgroundImageprocessors();

	void Poll ();

protected:
};

