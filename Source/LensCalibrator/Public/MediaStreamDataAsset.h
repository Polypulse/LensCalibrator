#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/DataAsset.h"

#include "GenericLensSolverParameters.h"
#include "StartMediaStreamParameters.h"

#include "MediaStreamDataAsset.generated.h"

UCLASS(BlueprintType)
class UMediaStreamDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FGenericLensSolverParameters genericLensSolverParameters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FStartMediaStreamParameters startMediaStreamParameters;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	UMediaPlayer * mediaPlayer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Lens Calibrator")
	FString url;

	UMediaStreamDataAsset()
	{
		mediaPlayer = nullptr;
		url = FString("");
	}
};
