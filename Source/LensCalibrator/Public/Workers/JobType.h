/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "JobType.generated.h"

UENUM(BlueprintType)
enum class UJobType : uint8
{
	/* One time job performed for calibration of texture folders. */
	OneTime UMETA(DisplayName = "One Time"),
	/* Continuous job for calibration via media stream. */
	Continuous UMETA(DisplayName = "Continuous")
};
