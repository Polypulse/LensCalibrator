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

	FString jobID;
	FString calibrationID;

	FCalibrationWorkerParameters workerParameters;
	float zoomLevel;

	FIntPoint sourceResolution;
	FIntPoint resizeResolution;
};