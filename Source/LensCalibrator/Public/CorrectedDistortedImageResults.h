#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "CorrectedDistortedImageResults.generated.h"

USTRUCT(BlueprintType)
struct FCorrectedDistortedImageResults
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<FColor> pixels;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int width;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int height;
};
