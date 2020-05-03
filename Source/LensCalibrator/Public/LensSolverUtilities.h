#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Runtime/ImageWritequeue/Public/ImagePixelData.h"
#include "Runtime/ImageWritequeue/Public/ImageWriteStream.h"
#include "Runtime/ImageWritequeue/Public/ImageWriteTask.h"
#include "Runtime/ImageWritequeue/Public/ImageWriteQueue.h"
#include "UniquePtr.h"

class LensSolverUtilities
{
public:
	static FString GenerateIndexedFilePath(const FString& folder, const FString& fileName, const FString & extension);
	static bool ValidateFolder(FString& folder, const FString & backupFolder, const FString& logMessageHeader);
	static bool GetFilesInFolder(const FString& folder, TArray<FString>& files);

	static FString GenerateGenericOutputPath(const FString & subFolder);

	static bool CreateTexture2D(
		void * rawData,
		int width,
		int height,
		bool sRGB,
		bool isLUT,
		UTexture2D *& output,
		EPixelFormat pixelFormat = EPixelFormat::PF_B8G8R8A8);

	static bool LoadTexture(
		FString absoluteTexturePath, 
		bool sRGB,
		bool isLUT, 
		UTexture2D*& texture,
		EPixelFormat pixelFormat = EPixelFormat::PF_B8G8R8A8);

	static bool WriteTexture16(
		FString absoluteTexturePath,
		int width,
		int height,
		TUniquePtr<TImagePixelData<FFloat16Color>> data);
};