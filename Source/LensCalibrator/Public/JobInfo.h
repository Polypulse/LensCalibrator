/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "JobType.h"
#include "JobInfo.generated.h"

USTRUCT(BlueprintType)
struct FJobInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	TEnumAsByte<UJobType> jobType;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	FString jobID;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	int latchedWorkUnitCount;
};
