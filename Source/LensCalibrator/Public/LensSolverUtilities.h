#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"

class LensSolverUtilities
{
public:
	static FString GenerateIndexedFilePath(const FString& folder, const FString& fileName, const FString & extension);
	static bool ValidateFolder(FString& folder, const FString & backupFolder, const FString& logMessageHeader);
	static bool GetFilesInFolder(const FString& folder, TArray<FString>& files);

	static FString GenerateGenericOutputPath(const FString & subFolder);
	static UTexture2D * CreateTexture2D(TArray<FColor> * rawData, int width, int height, bool sRGB, bool isLUT);
	static bool LoadTexture(FString absoluteTexturePath, bool sRGB, bool isLUT, UTexture2D*& texture);
};