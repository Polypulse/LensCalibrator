/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/LocalPlayer.h"

#include "UCameraPlayer.generated.h"

/* The purpose of this class is to override the projection matrix for the player's camera. */
UCLASS(BlueprintType, Category = "Lens Calibrator", meta = (Keywords = ""))
class UCameraPlayer : public ULocalPlayer
{
	GENERATED_BODY()

	/* This method is automatically going to be called by UE4 when the local player is overrided with this class. */
	FSceneView* CalcSceneView(class FSceneViewFamily* ViewFamily,
		FVector& OutViewLocation,
		FRotator& OutViewRotation,
		FViewport* Viewport,
		class FViewElementDrawer* ViewDrawer = NULL,
		EStereoscopicPass StereoPass = eSSP_FULL) override;

private:
	bool queued = false;
	/* The queued projection matrix. */
	FMatrix projectionMatrix;

public:
	/* This queues the projection matrix for rendering in this frame. */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Camera Projection Matrix"), Category = "Lens Calibrator", meta = (Keywords = ""))
	void QueueCameraProjectionMatrix(FMatrix projectionMatrix);

	/* Get the player instance. */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Local Player"), Category = "Lens Calibrator", meta = (Keywords = ""))
	static UCameraPlayer * GetLocalPlayerInstance(UObject * worldContext, bool & valid);
};