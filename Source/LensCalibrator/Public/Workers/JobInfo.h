/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

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
