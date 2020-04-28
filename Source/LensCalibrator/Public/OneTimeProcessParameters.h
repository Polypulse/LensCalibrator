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

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	FWorkerParameters workerParameters;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	FIntPoint cornerCount;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	float squareSize;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	bool resize;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	FIntPoint resizeResolution;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	bool flipX;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	bool flipY;

	FIntPoint currentResolution;
};
