/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "FindCornerWorkerParameters.generated.h"

USTRUCT(BlueprintType)
struct FFindCornerWorkerParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool exhaustiveSearch;

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
	bool writeCornerVisualizationTextureToFile;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString cornerVisualizationTextureOutputPath;

	FFindCornerWorkerParameters()
	{
		exhaustiveSearch = false;
		cornerCount = FIntPoint(12, 8);
		squareSizeMM = 12.7f;
		resize = true;
		resizePercentage = 0.5f;
		flipX = false;
		flipY = false;
		writeCornerVisualizationTextureToFile = false;
		cornerVisualizationTextureOutputPath = "";
	};
};

