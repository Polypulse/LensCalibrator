/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine.h"

#include "JobInfo.h"
#include "Job.generated.h"

USTRUCT(BlueprintType)
struct FJob
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	FJobInfo jobInfo;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	TMap<FString, int> expectedImageCount;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	TMap<FString, int> currentImageCount;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	int expectedResultCount;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	int currentResultCount;
};
