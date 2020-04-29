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

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	FWorkerParameters workerParameters;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	FIntPoint cornerCount;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	float squareSize;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	bool resize;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	FIntPoint resizeResolution;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	bool flipX;

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	bool flipY;

	FIntPoint currentResolution;
};
