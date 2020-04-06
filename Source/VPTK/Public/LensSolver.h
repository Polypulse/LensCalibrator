#pragma once
#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraComponent.h"
#include "MediaAssets/Public/MediaTexture.h"
#include "MediaAssets/Public/MediaPlayer.h"

#pragma push_macro("check")
#undef check
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#pragma pop_macro("check")

#include "LensSolver.generated.h"

USTRUCT(BlueprintType)
struct FSolvedPoints
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	TArray<FVector2D> points;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	float zoomLevel;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	bool success;
};

UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VPTK_API ULensSolver : public UActorComponent
{
	GENERATED_BODY()
private:
	TQueue<FSolvedPoints> queuedSolvedPoints;
	static void BeginDetectPoints(UMediaTexture* inputMediaTexture, float inputZoomLevel, TQueue<FSolvedPoints> * inputQueuedSolvedPoints);

protected:
	// virtual void BeginPlay() override {}

public:

	ULensSolver() 
	{
	}

	~ULensSolver() 
	{
		queuedSolvedPoints.Empty();
	}

	// virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override {}

	UFUNCTION(BlueprintCallable, Category="VPTK")
	bool ValidateMediaInputs (UMediaPlayer * mediaPlayer, UMediaTexture * mediaTexture, FString url);

	UFUNCTION(BlueprintCallable, Category="VPTK")
	void ProcessMediaTexture(UMediaTexture* inputMediaTexture, float normalizedZoomValue);
};