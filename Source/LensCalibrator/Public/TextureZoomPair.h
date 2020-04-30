#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "TextureZoomPair.generated.h"

USTRUCT(BlueprintType)
struct FTextureZoomPair
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lens Calibrator")
	UTexture2D * texture;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float zoomLevel;
};
