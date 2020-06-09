/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine.h"

#include "DistortionCorrectionMapGenerationResults.generated.h"

USTRUCT(BlueprintType)
struct FDistortionCorrectionMapGenerationResults
{
	GENERATED_BODY()
	FString id;

	int width;
	int height;

	float zoomLevel;

	TArray<FFloat16Color> distortionCorrectionPixels;
	TArray<FFloat16Color> inverseDistortionCorrectionPixels;
};
