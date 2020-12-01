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

#include "JobType.h"
#include "JobInfo.generated.h"

/* Struct containing look up IDs and job type. */
USTRUCT(BlueprintType)
struct FJobInfo
{
	GENERATED_BODY()

	/* Continouous job or one time. */
	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	TEnumAsByte<UJobType> jobType;

	/* This allows work units to associate with a job via just the job ID. */
	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	FString jobID;

	/* A job can contain multiple calibrations essentially 1 per zoom level. */
	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	TArray<FString> calibrationIDs;
};
