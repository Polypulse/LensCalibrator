#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "JobInfo.h"
#include "CalibrationWorkerParameters.h"

#include "LatchData.generated.h"

USTRUCT()
struct FLatchData
{
	GENERATED_BODY()

	FJobInfo jobInfo;
	FCalibrationWorkerParameters workerParameters;
	int workUnitCount;

	float zoomLevel;

	float sensorDiagonalMM;

	bool resize;
	float resizePercentage;

	FIntPoint sourceResolution;
	FIntPoint resizeResolution;

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