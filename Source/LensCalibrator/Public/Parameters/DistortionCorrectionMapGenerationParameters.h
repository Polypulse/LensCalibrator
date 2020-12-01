/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "SolvedPoints.h"

#include "DistortionCorrectionMapGenerationParameters.generated.h"

USTRUCT(BlueprintType)
struct FDistortionCorrectionMapGenerationParameters
{
	GENERATED_BODY()
	FString id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float zoomLevel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FIntPoint sourceResolution;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FVector2D sourcePrincipalPixelPoint;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FIntPoint outputMapResolution;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString correctionOutputPath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString inverseCorrectionOutputPath;

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
};