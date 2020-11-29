/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
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
