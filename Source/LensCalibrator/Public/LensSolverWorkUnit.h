#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#pragma push_macro("check")
#undef check
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#pragma pop_macro("check")
#include <vector>

#include "LensSolverWorkUnit.generated.h"

/*
UENUM(BlueprintType)
enum class ELensSolverWorkUnitType : uint8
{
	PixelArray,
	TextureFile,
	Calibrate
};
*/

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
struct FCalibrateParameters
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

	FCalibrateParameters()
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

struct FLensSolverTextureWorkUnit
{
	FBaseParameters baseParameters;
	FTextureSearchParameters textureSearchParameters;

	FLensSolverTextureWorkUnit() 
	{
	}
};

struct FPixelArrayParameters
{
	FIntPoint sourceResolution;
	TArray<FColor> pixels;
	FPixelArrayParameters()
	{
		sourceResolution = FIntPoint(0, 0);
		pixels = TArray<FColor>();
	}
};

struct FLensSolverPixelArrayWorkUnit
{
	FBaseParameters baseParameters;
	FTextureSearchParameters textureSearchParameters;
	FPixelArrayParameters pixelArrayParameters;

	FLensSolverPixelArrayWorkUnit() 
	{
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

struct FLensSolverTextureFileWorkUnit
{
	FBaseParameters baseParameters;
	FTextureSearchParameters textureSearchParameters;

	FTextureFileParameters textureFileParameters;
	FLensSolverTextureFileWorkUnit() 
	{
	}
};

struct FCalibrationPointParameters
{
	std::vector<cv::Point2f> corners;
	std::vector<cv::Point3f> objectPoints;
	FCalibrationPointParameters()
	{
	}
};

struct FLensSolverCalibrationPointsWorkUnit
{
	FBaseParameters baseParameters;
	FCalibrationPointParameters calibrationPointParameters;

	FLensSolverCalibrationPointsWorkUnit() {}
};

struct FCalibrateLatch
{
	FBaseParameters baseParameters;
	FCalibrateParameters calibrateParameters;

	FCalibrateLatch()
	{
	}
};

USTRUCT(BlueprintType)
struct FCalibrationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FBaseParameters baseParameters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool success;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float zoomLevel;

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

	UPROPERTY(BlueprintReadWrite, Category="Lens Calibrator")
	TArray<float> distortionCoefficients;

	FCalibrationResult()
	{
		success = false;
		zoomLevel = 0.0f;
		fovX = 0.0f;
		fovY = 0.0f;
		focalLengthMM = 0.0f;
		aspectRatio = 0.0f;
		sensorSizeMM = FVector2D(0, 0);
		principalPixelPoint = FVector2D(0, 0);
		resolution = FIntPoint(0, 0);
		perspectiveMatrix = FMatrix::Identity;
		distortionCoefficients = TArray<float>();
	}
};
