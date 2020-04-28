/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "WorkerParameters.generated.h"

USTRUCT(BlueprintType)
struct FWorkerParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	bool exhaustiveSearch;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	bool writeDebugTextureToFile;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	FString debugTextureFolderPath;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	bool writeCalibrationResultsToFile;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	FString calibrationResultsFolderPath;
};

