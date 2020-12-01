/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine.h"

#include "DistortionCorrectionMapGenerationResults.generated.h"

/* Post calibration, a distortion correction map is generated and the data that's used
to generate that map including the actual pixels is stored in this struct. */
USTRUCT(BlueprintType)
struct FDistortionCorrectionMapGenerationResults
{
	GENERATED_BODY()
	/* The associated job ID. */
	FString id;

	int width;
	int height;

	/* The zoom level associated with this distortion correction. */
	float zoomLevel;

	/* k_x distortion coefficients. */
	float k1;
	float k2;
	float p1;
	float p2;
	float k3;

	float k4;
	float k5;
	float k6;

	/* Array of pixels representing the distortion correction.*/
	TArray<FFloat16Color> distortionCorrectionPixels;

	/* Array of pixels to distort an image.*/
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
