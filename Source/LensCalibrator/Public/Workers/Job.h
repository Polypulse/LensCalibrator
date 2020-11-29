/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
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
