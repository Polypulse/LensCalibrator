#pragma once

#include "JobInfo.h"
#include "SolvedPoints.generated.h"

USTRUCT(BlueprintType)
struct FSolvedPoints
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	FJobInfo jobInfo;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	bool success;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	float zoomLevel;

	// UPROPERTY(BlueprintReadWrite, Category="VPTK")
	int width;

	// UPROPERTY(BlueprintReadWrite, Category="VPTK")
	int height;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	float fovX;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	float fovY;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	float focalLength;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	float aspectRatio;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	FMatrix perspectiveMatrix;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	TArray<FVector2D> points;
};
