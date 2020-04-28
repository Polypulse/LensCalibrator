/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

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
	FSceneView* View = Super::CalcSceneView(ViewFamily, OutViewLocation, OutViewRotation, Viewport, ViewDrawer, StereoPass);

	if (!queued)
		return View;

	if (View)
		View->UpdateProjectionMatrix(projectionMatrix);

	return View;
}


UCameraPlayer * UCameraPlayer::GetLocalPlayerInstance(UObject * worldContextObject, bool & valid)
{
	valid = false;

	UWorld* world = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!world)
		return nullptr;

	ULocalPlayer * player = world->GetFirstLocalPlayerFromController();
	if (!player)
		return nullptr;

	UCameraPlayer* cameraPlayer = dynamic_cast<UCameraPlayer*>(player);
	if (!cameraPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("Instance of ULocalPlayer is not a UAPATPlayer!"))
		return nullptr;
	}

	valid = true;
	return cameraPlayer;
}
