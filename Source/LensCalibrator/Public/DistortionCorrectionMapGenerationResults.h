#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Color.h"
#include "Engine.h"

#include "DistortionCorrectionMapGenerationResults.generated.h"


USTRUCT(BlueprintType)
struct FDistortionCorrectionMapGenerationResults
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int width;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int height;

	TArray<FFloat16Color> pixels;
};
