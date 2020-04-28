/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

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
