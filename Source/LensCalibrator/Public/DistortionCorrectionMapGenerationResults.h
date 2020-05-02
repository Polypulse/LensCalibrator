#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "DistortionCorrectionMapGenerationResults.generated.h"

USTRUCT(BlueprintType)
struct FDistortionCorrectionMapGenerationResults
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<FColor> pixels;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int width;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int height;
};
