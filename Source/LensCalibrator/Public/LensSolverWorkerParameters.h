#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "LensSolverWorkerParameters.generated.h"

USTRUCT(BlueprintType)
struct FBaseParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString jobID;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString calibrationID;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString friendlyName;

	FBaseParameters () 
	{
		jobID = "";
		calibrationID = "";
		friendlyName = "";
	}
};

USTRUCT(BlueprintType)
struct FCalibrationParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float zoomLevel;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float sensorDiagonalSizeMM;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FIntPoint initialPrincipalPointPixelPosition;

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
	bool writeCalibrationResultsToFile;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString calibrationResultsFolderPath;

	FIntPoint sourceResolution;
	FIntPoint resizeResolution;

	FCalibrationParameters()
	{
		zoomLevel = 0.0f;
		sensorDiagonalSizeMM = 9.960784f;
		initialPrincipalPointPixelPosition = FIntPoint(0, 0);

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

		writeCalibrationResultsToFile = false;
		calibrationResultsFolderPath = "";

		sourceResolution = FIntPoint(0, 0);
		resizeResolution = FIntPoint(0, 0);
	}
};

USTRUCT(BlueprintType)
struct FTextureSearchParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float resizePercentage;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool resize;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool exhaustiveSearch;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float checkerBoardSquareSizeMM;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FIntPoint checkerBoardCornerCount;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool writeDebugTextureToFile;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString debugTextureFolderPath;

	FTextureSearchParameters()
	{
		resizePercentage = 0.5f;
		resize = true;
		exhaustiveSearch = false;
		checkerBoardSquareSizeMM = 12.7f;
		checkerBoardCornerCount = FIntPoint(12, 8);
		writeDebugTextureToFile = false;
		debugTextureFolderPath = "";
	}
};

USTRUCT(BlueprintType)
struct FTextureFileParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString absoluteFilePath;

	FTextureFileParameters() 
	{
		absoluteFilePath = "";
	}
};

