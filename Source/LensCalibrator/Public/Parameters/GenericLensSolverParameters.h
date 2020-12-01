/*
 * Copyright (C) 2020 - LensCalibrator contributors, see Contributors.txt at the root of the project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


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