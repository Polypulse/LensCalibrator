#pragma once

#include "SolvedPoints.generated.h"

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

	/*
	TArray<uint8> visualizationData;
	int width;
	int height;
	*/
	// UTexture2D * visualizationTexture;
};
