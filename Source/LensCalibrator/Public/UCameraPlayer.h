/*
 * Copyright (C) 2020 - LensCalibrator contributors, see Contributors.txt at the root of the project.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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