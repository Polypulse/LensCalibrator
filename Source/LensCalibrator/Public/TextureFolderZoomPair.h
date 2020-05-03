#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "TextureFolderZoomPair.generated.h"

USTRUCT(BlueprintType)
struct FTextureFolderZoomPair
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString absoluteFolderPath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float zoomLevel;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	bool use;

	FTextureFolderZoomPair()
	{
		zoomLevel = 0.0f;
		use = true;
	}
};
