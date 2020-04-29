/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#pragma once 

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/LocalPlayer.h"

#include "UCameraPlayer.generated.h"

UCLASS(BlueprintType, Category = "Lens Calibrator", meta = (Keywords = ""))
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
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Camera Projection Matrix"), Category = "Lens Calibrator", meta = (Keywords = ""))
	void QueueCameraProjectionMatrix(FMatrix projectionMatrix);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Local Player"), Category = "Lens Calibrator", meta = (Keywords = ""))
	static UCameraPlayer * GetLocalPlayerInstance(UObject * worldContext, bool & valid);
};