#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "JobType.h"
#include "JobInfo.generated.h"

USTRUCT(BlueprintType)
struct FJobInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	TEnumAsByte<UJobType> jobType;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	FString jobID;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	int workUnitCount;
};
