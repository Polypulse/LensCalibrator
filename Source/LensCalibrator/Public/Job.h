#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "JobInfo.h"
#include "Job.generated.h"

USTRUCT(BlueprintType)
struct FJob
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	FJobInfo jobInfo;

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	int completedWorkUnits;
};
