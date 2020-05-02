#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

class LensSolverUtilities
{
public:
	static FString GenerateIndexedFilePath(const FString& folder, const FString& fileName, const FString & extension);
	static bool ValidateFolder(FString& folder, const FString & backupFolder, const FString& logMessageHeader);
	static FString GenerateGenericOutputPath(const FString & subFolder);
};