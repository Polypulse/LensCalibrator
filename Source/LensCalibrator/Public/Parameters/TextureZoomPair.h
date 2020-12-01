/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "TextureZoomPair.generated.h"

/* Tuple associating single image with a zoom level. */
USTRUCT(BlueprintType)
struct FTextureZoomPair
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lens Calibrator")
	UTexture2D * texture;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float zoomLevel;
};
