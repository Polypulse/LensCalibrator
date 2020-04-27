#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

#include "SolveParameters.generated.h"

USTRUCT(BlueprintType)
struct FSolveParameters
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="VPTK")
	bool exhaustiveSearch;
};

