/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine.h"

#include "JobInfo.h"
#include "Job.generated.h"

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

USTRUCT(BlueprintType)
struct FJob
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FJobInfo jobInfo;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int expectedResultCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int currentResultCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TMap<FString, FExpectedAndCurrentImageCount> expectedAndCurrentImageCounts;

	int64 startTime;
};
