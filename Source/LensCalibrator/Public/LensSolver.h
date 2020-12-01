/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

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

#include "TextureArrayZoomPair.h"
#include "TextureFolderZoomPair.h"
#include "TextureZoomPair.h"
#include "LensSolverWorkerParameters.h"

#include "LensSolverWorkerInterfaceContainer.h"
#include "QueueContainers.h"

#include "ILensSolverEventReceiver.h"

#include "LensSolver.generated.h"

/* This is where lens calibration starts from. */
UCLASS()
class LENSCALIBRATOR_API ULensSolver : public UObject
{
	GENERATED_BODY()

private:
	/* This delegate is binded to QueueLog and is passed to LensSolverWorkDistributor
	so it can pass the logs back here to be processed. */
	QueueLogOutputDel * queueLogOutputDel;
	QueueFinishedJobOutputDel * queueFinishedJobOutputDel;

	TQueue<FinishedJobQueueContainer, EQueueMode::Mpsc> queuedFinishedJobs;
	TQueue<FString, EQueueMode::Mpsc> logQueue;

	bool ValidateMediaTexture(const UMediaTexture* inputTexture);

	/* Workers queue their log data from their own thread, then this method will dequeue 
	those logs so that they can be printed in the console on the main thread. */
	void PollLogs();

	void PollCalibrationResults ();
	void PollFinishedJobs();

	void QueueFinishedJob(FinishedJobQueueContainer queueContainer);
	bool FinishedJobIsQueued();
	void DequeuedFinishedJob(FinishedJobQueueContainer &queueContainer);
	void QueueLog(FString msg);

	/* Determine if debug mode is enabled or not. */
	bool Debug();

	/* Build path to output debug images. */
	FString PrepareDebugOutputPath (const FString & debugOutputPath);

public:

	ULensSolver() {}
	~ULensSolver() {}

	/* Start calibration from a set of folders each containing a set of textures
	representing the calibration pattern at a particular zoom level, then pass in corner 
	search parameters, calibration parameters and media stream texture 
	parameters. Also, the workers need to be started and idling before you can start this job. */
	void OneTimeProcessArrayOfTextureFolderZoomPairs(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		TArray<FTextureFolderZoomPair> inputTextures, 
		FTextureSearchParameters textureSearchParameters,
		FCalibrationParameters calibrationParameters,
		FJobInfo & ouptutJobInfo);

	/* Start calibration from a media stream and pass in corner search parameters, calibration 
	parameters and media stream texture parameters. Also, the workers need to be started
	and idling before you can start this job. */
	void StartMediaStreamCalibration(
		TScriptInterface<ILensSolverEventReceiver> eventReceiver,
		FTextureSearchParameters textureSearchParameters,
		FCalibrationParameters calibrationParameters,
		FMediaStreamParameters mediaStreamParameters,
		FJobInfo& ouptutJobInfo);

	void StartBackgroundImageProcessors(int findCornersWorkerCount, int calibrateWorkerCount, bool shutDownWorkersAfterCompletingTasks);
	void StopBackgroundImageprocessors();

	void Poll ();

protected:
};

