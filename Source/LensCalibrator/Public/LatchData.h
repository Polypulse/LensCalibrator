#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "JobInfo.h"
#include "WorkerParameters.h"

#include "LatchData.generated.h"

USTRUCT()
struct FLatchData
{
	GENERATED_BODY()

	FJobInfo jobInfo;
	FWorkerParameters workerParameters;
	int imageCount;
	float zoomLevel;
	FIntPoint currentResolution;
	FIntPoint sourceResolution;
	FIntPoint cornerCount;
	float squareSizeMM;
	float sensorDiagonalMM;
	FIntPoint initialPrincipalPointPixelPosition;
	// float initialVerticalFieldOfView;

	/*
	FLatchData(){}

	FLatchData(
		FJobInfo inputJobInfo, 
		FWorkerParameters inputWorkerParameters, 
		float inputZoomLevel, 
		FIntPoint inputResolution) :
		jobInfo(inputJobInfo),
		workerParameters(inputWorkerParameters),
		zoomLevel(inputZoomLevel),
		resolution(inputResolution)
	{}
	*/
};