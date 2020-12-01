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


#include "UCameraPlayer.h"

#include "Engine.h"
#include "Engine/World.h"
#include "Runtime/Engine/Public/SceneView.h"

#include "DrawDebugHelpers.h"

void UCameraPlayer::QueueCameraProjectionMatrix(FMatrix inputPerspectiveMatrix)
{
	projectionMatrix = inputPerspectiveMatrix;
	queued = true;
}

FSceneView* UCameraPlayer::CalcSceneView(FSceneViewFamily* ViewFamily, FVector& OutViewLocation, FRotator& OutViewRotation, FViewport* Viewport, FViewElementDrawer* ViewDrawer, EStereoscopicPass StereoPass)
{
	/* Get scene view context. */
	FSceneView* View = Super::CalcSceneView(ViewFamily, OutViewLocation, OutViewRotation, Viewport, ViewDrawer, StereoPass);

	if (!queued)
		return View;

	/* Here we actually set the projection matrix. */
	if (View)
		View->UpdateProjectionMatrix(projectionMatrix);

	return View;
}


UCameraPlayer * UCameraPlayer::GetLocalPlayerInstance(UObject * worldContextObject, bool & valid)
{
	valid = false;

	/* Get the world. */
	UWorld* world = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!world)
		return nullptr;

	/* Get the player from the world. */
	ULocalPlayer * player = world->GetFirstLocalPlayerFromController();
	if (!player)
		return nullptr;

	UCameraPlayer* cameraPlayer = dynamic_cast<UCameraPlayer*>(player);
	/* If the ULocalPlayer is not overrided in the project settings, then this may spam in the console. */
	if (!cameraPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("Instance of ULocalPlayer is not a UCameraPlayer!"))
		return nullptr;
	}

	valid = true;
	return cameraPlayer;
}
