#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "DistortTextureWithCoefficientsParams.generated.h"

USTRUCT(BlueprintType)
struct FDistortTextureWithCoefficientsParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UTexture2D* distortedTexture;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<float> distortionCoefficients;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float zoomLevel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString outputPath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool reverseOperation;

	FDistortTextureWithCoefficientsParams()
	{
		distortedTexture = nullptr;
		outputPath = FString("");
		zoomLevel = 0.0f;
		reverseOperation = false;
	}
};