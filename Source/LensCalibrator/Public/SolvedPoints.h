/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "JobInfo.h"
#include "SolvedPoints.generated.h"

USTRUCT(Blueprintable)
struct FCalibrationResult
{
	GENERATED_BODY()

	FIntPoint resolution;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	FJobInfo jobInfo;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	bool success;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	float zoomLevel;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	int textureIndex;

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

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	TArray<float> distortionCoefficients;
};
