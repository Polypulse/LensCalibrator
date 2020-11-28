/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/VolumeTexture.h"
#include "SolvedPoints.h"

#include "GenericLensSolverParameters.generated.h"

/* Generic set of calibration parameters used across both
media stream and texture folder calibration. */
USTRUCT(BlueprintType)
struct FGenericLensSolverParameters
{
	GENERATED_BODY()

	/* The number of background workers that process images and solve
	for the corners of the calibration pattern. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int cornerWorkerCount;

	/* The number of background workers that take sets of 2D & 3D points
	found by the corner workers and solves for the lens intrinsics for that 
	zoom level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int calibrateWorkerCount;

	/* Automatically shutdown background workers after calibration is complete. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool shutDownWorkersAfterCompletedTasks;

	/* Output path for the distortion removal texture map. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString distortionCorrectionMapOutputPath;

	/* Output path for the distortion adding texture map. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString distortionUncorrectionMapOutputPath;

	FGenericLensSolverParameters()
	{
		cornerWorkerCount = 3;
		calibrateWorkerCount = 1;
		shutDownWorkersAfterCompletedTasks = true;

		distortionCorrectionMapOutputPath = "";
		distortionUncorrectionMapOutputPath = "";
	}
};