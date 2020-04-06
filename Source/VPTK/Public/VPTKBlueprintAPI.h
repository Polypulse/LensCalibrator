#pragma once

#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"
#include "MediaTexture.h"

#include "VPTKBlueprintAPI.generated.h"

UCLASS(BlueprintType, Category = "VPTK", meta = (Keywords = ""))
class VPTK_API UVPTKBlueprintAPI : public UObject
{
	GENERATED_BODY()
private:
	~UVPTKBlueprintAPI() {}
protected:
public:

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Calibrate Lens"), Category = "VPTK", meta = (Keywords = ""))
	static void CalibrateLens (UMediaTexture * mediaTexture);
};