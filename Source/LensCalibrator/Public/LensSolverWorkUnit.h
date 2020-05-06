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

UENUM(BlueprintType)
enum class ELensSolverWorkUnitType : uint8
{
	PixelArray,
	TextureFile,
	Calibrate
};

USTRUCT(BlueprintType)
struct FLensSolverWorkUnit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString jobID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString calibrationID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString friendlyName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	ELensSolverWorkUnitType workUnitType;
};

USTRUCT(BlueprintType)
struct FLensSolverTextureWorkUnit
{
	GENERATED_BODY()
	FLensSolverWorkUnit baseUnit;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float resizePercentage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool resize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool exhaustiveSearch;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool writeDebugTextureToFile;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float checkerBoardSquareSizeMM;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FIntPoint checkerBoardCornerCount;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString debugTextureFolderPath;
};

USTRUCT(BlueprintType)
struct FLensSolverPixelArrayWorkUnit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FLensSolverTextureWorkUnit textureUnit;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FIntPoint sourceResolution;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<FColor> pixels;
};

USTRUCT(BlueprintType)
struct FLensSolverTextureFileWorkUnit
{
	GENERATED_BODY()
	FLensSolverTextureWorkUnit textureUnit;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString absoluteFilePath;
};

USTRUCT(BlueprintType)
struct FLensSolverCalibrateWorkUnit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FLensSolverWorkUnit baseUnit;

	std::vector<cv::Point2f> corners;
	std::vector<cv::Point3f> objectPoints;
};
