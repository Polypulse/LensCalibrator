/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/DataAsset.h"

#include "TextureFolderZoomPair.h"

#include "TextureFolderParametersDataAsset.generated.h"

UCLASS(BlueprintType)
class UTextureFolderParametersDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	TArray<FTextureFolderZoomPair> inputs;
};
