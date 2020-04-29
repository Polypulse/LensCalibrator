/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "JobInfo.h"
#include "SolvedPoints.generated.h"

USTRUCT(BlueprintType)
struct FCalibrationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	FJobInfo jobInfo;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	bool success;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	float zoomLevel;

	int width;
	int height;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	float fovX;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	float fovY;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	float focalLength;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	float aspectRatio;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	FMatrix perspectiveMatrix;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	TArray<FVector2D> points;
};
