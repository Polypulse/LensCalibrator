/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "CorrectedDistortedImageResults.generated.h"

USTRUCT(BlueprintType)
struct FCorrectedDistortedImageResults
{
	GENERATED_BODY()
	FString id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<FColor> pixels;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int width;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	int height;
};
