/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "WorkerParameters.h"
#include "OneTimeProcessParameters.generated.h"

USTRUCT(BlueprintType)
struct FOneTimeProcessParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FWorkerParameters workerParameters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FIntPoint cornerCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float squareSizeMM;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool resize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float resizePercentage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool flipX;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool flipY;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float sensorDiagonalSizeMM;

	// UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	// float initialVerticalFieldOfView;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FIntPoint initialPrincipalPointPixelPosition;

	FIntPoint currentResolution;

	FOneTimeProcessParameters()
	{
		workerParameters = FWorkerParameters();
		cornerCount = FIntPoint(12, 8);
		squareSizeMM = 12.7f;
		resize = true;
		resizePercentage = 0.5f;
		flipX = false;
		flipY = false;
		sensorDiagonalSizeMM = 9.960784f;
		initialPrincipalPointPixelPosition = FIntPoint(0, 0);
		currentResolution = FIntPoint(0, 0);
	}
};
