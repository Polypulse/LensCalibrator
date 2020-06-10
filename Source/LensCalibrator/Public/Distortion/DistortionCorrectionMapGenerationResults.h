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

	float k1;
	float k2;
	float p1;
	float p2;
	float k3;

	float k4;
	float k5;
	float k6;

	TArray<FFloat16Color> distortionCorrectionPixels;
	TArray<FFloat16Color> inverseDistortionCorrectionPixels;

	FDistortionCorrectionMapGenerationResults()
	{
		width = 0;
		height = 0;

		zoomLevel = 0.0f;

		k1 = 0.0f;
		k2 = 0.0f;
		p1 = 0.0f;
		p2 = 0.0f;
		k3 = 0.0f;

		k4 = 0.0f;
		k5 = 0.0f;
		k6 = 0.0f;
	}
};
