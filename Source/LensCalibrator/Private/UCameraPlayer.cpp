/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */


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
