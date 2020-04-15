#pragma once

#include "SolvedPoints.generated.h"

USTRUCT(BlueprintType)
struct FSolvedPoints
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	bool success;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	float zoomLevel;

	// UPROPERTY(BlueprintReadWrite, Category="VPTK")
	int width;

	// UPROPERTY(BlueprintReadWrite, Category="VPTK")
	int height;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	FTransform cameraTransform;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	FMatrix perspectiveMatrix;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	TArray<FVector2D> points;

	// UPROPERTY(BlueprintReadWrite, Category="VPTK")
	TArray<FColor> visualizationData;
};
