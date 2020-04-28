#pragma once 

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/LocalPlayer.h"

#include "UCameraPlayer.generated.h"

UCLASS(BlueprintType, Category = "APAT", meta = (Keywords = ""))
class UCameraPlayer : public ULocalPlayer
{
	GENERATED_BODY()

	FSceneView* CalcSceneView(class FSceneViewFamily* ViewFamily,
		FVector& OutViewLocation,
		FRotator& OutViewRotation,
		FViewport* Viewport,
		class FViewElementDrawer* ViewDrawer = NULL,
		EStereoscopicPass StereoPass = eSSP_FULL) override;

private:
	bool queued = false;
	FMatrix projectionMatrix;

public:
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Camera Projection Matrix"), Category = "APAT", meta = (Keywords = ""))
	void QueueCameraProjectionMatrix(FMatrix projectionMatrix);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Local Player"), Category = "APAT", meta = (Keywords = ""))
	static UCameraPlayer * GetLocalPlayerInstance(UObject * worldContext, bool & valid);
};