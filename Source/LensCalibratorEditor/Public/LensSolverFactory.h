#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Factories/Factory.h"

UCLASS(hidecategories=Object)
class ULensSolverFactory : public UFactory
{
	GENERATED_BODY()

public:
	ULensSolverFactory();
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};