/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "WorkerParameters.generated.h"

UENUM()
enum FindCheckerboardSearchMethod
{
	Exhaustive,
	Fast
};

USTRUCT(BlueprintType)
struct FWorkerParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool exhaustiveSearch;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool useInitialIntrinsicValues;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool keepPrincipalPixelPositionFixed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool keepAspectRatioFixed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool lensHasTangentalDistortion;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool fixRadialDistortionCoefficientK1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool fixRadialDistortionCoefficientK2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool fixRadialDistortionCoefficientK3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool fixRadialDistortionCoefficientK4;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool fixRadialDistortionCoefficientK5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool fixRadialDistortionCoefficientK6;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool writeDebugTextureToFile;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString debugTextureFolderPath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool writeCalibrationResultsToFile;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString calibrationResultsFolderPath;

	FWorkerParameters()
	{
		exhaustiveSearch = false;
		useInitialIntrinsicValues = false;
		keepPrincipalPixelPositionFixed = false;
		keepAspectRatioFixed = true;
		lensHasTangentalDistortion = false;
		fixRadialDistortionCoefficientK1 = false;
		fixRadialDistortionCoefficientK2 = false;
		fixRadialDistortionCoefficientK3 = false;
		fixRadialDistortionCoefficientK4 = false;
		fixRadialDistortionCoefficientK5 = false;
		fixRadialDistortionCoefficientK6 = false;
		writeDebugTextureToFile = false;
		debugTextureFolderPath = "";
		writeCalibrationResultsToFile = false;
		calibrationResultsFolderPath = "";
	};
};

