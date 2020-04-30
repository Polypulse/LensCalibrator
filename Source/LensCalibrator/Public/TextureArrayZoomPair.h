#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "TextureArrayZoomPair.generated.h"

USTRUCT(BlueprintType)
struct FTextureArrayZoomPair
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<UTexture2D*> textures;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	float zoomLevel;
};