/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "DistortTextureWithTextureParams.generated.h"

USTRUCT(BlueprintType)
struct FDistortTextureWithTextureParams
{
	GENERATED_BODY()
	FString id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UTexture2D* distortedTexture;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UTexture2D* distortionCorrectionTexture;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float zoomLevel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString outputPath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool reverseOperation;

	FDistortTextureWithTextureParams()
	{
		distortedTexture = nullptr;
		distortionCorrectionTexture = nullptr;
		outputPath = FString("");
		zoomLevel = 0.0f;
		reverseOperation = false;
	}
};