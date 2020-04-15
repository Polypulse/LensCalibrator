#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "JobType.generated.h"

UENUM(BlueprintType)
enum UJobType
{
	OneTime UMETA(DisplayName = "One Time"),
	Continuous UMETA(DisplayName = "Continuous")
};
