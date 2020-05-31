/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once

#include "JobInfo.h"
#include "LensSolverWorkerParameters.h"
#include "SolvedPoints.generated.h"

USTRUCT(BlueprintType)
struct FCalibrationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FBaseParameters baseParameters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool success;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float fovX;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float fovY;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float focalLengthMM;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float aspectRatio;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FVector2D sensorSizeMM;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FVector2D principalPixelPoint;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FIntPoint resolution;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FMatrix perspectiveMatrix;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float k1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float k2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float p1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float p2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float k3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int imageCount;

	FCalibrationResult()
	{
		success = false;
		fovX = 0.0f;
		fovY = 0.0f;
		focalLengthMM = 0.0f;
		aspectRatio = 0.0f;
		sensorSizeMM = FVector2D(0, 0);
		principalPixelPoint = FVector2D(0, 0);
		resolution = FIntPoint(0, 0);
		perspectiveMatrix = FMatrix::Identity;
		k1 = 0.0f;
		k2 = 0.0f;
		p1 = 0.0f;
		p2 = 0.0f;
		k3 = 0.0f;
		imageCountUsed = 0;
	}
};
