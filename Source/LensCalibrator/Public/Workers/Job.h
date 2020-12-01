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
#include "Engine.h"

#include "ILensSolverEventReceiver.h"
#include "JobInfo.h"

#include "Job.generated.h"

/* This struct contains expected and current image counts for a single zoom level and is associated with a calibration ID. */
USTRUCT(BlueprintType)
struct FExpectedAndCurrentImageCount
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int expectedImageCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int currentImageCount;

	FExpectedAndCurrentImageCount() {}
	FExpectedAndCurrentImageCount(int inputExpectedImageCount, int inputCurrentImageCount) 
	{
		expectedImageCount = inputExpectedImageCount;
		currentImageCount = inputCurrentImageCount;
	}
};

/* Container to store various job information including a reference to the callback interface. */
USTRUCT(BlueprintType)
struct FJob
{
	GENERATED_BODY()

	/* A handle to the interface instance that the blueprint is implementing for callbacks. */
	TScriptInterface<ILensSolverEventReceiver> eventReceiver;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FJobInfo jobInfo;

	/* The expected number of calibration results per zoom level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int expectedResultCount;

	/* The current number of calibration results per zoom level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int currentResultCount;

	/* Mapping of calibration IDs (one per zoom level) to expected and current image counts associated with that zoom level. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TMap<FString, FExpectedAndCurrentImageCount> expectedAndCurrentImageCounts;

	int64 startTime;
};
