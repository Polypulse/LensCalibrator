
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "DistortTextureWithTextureFileParams.generated.h"

USTRUCT(BlueprintType)
struct FDistortTextureWithTextureFileParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UTexture2D* distortedTexture;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString absoluteFilePath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float zoomLevel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString outputPath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool reverseOperation;

	FDistortTextureWithTextureFileParams()
	{
		distortedTexture = nullptr;
		absoluteFilePath = FString("");
		outputPath = FString("");
		zoomLevel = 0.0f;
		reverseOperation = false;
	}
};
