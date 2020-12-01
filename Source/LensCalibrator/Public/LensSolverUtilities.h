/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */

#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Runtime/ImageWritequeue/Public/ImagePixelData.h"
#include "Runtime/ImageWritequeue/Public/ImageWriteStream.h"
#include "Runtime/ImageWritequeue/Public/ImageWriteTask.h"
#include "Runtime/ImageWritequeue/Public/ImageWriteQueue.h"

/* These are utility macros to convert an FString to and from a char array. This is 
primarily used for interoperability between standard library structures and UE4 
structures. Furthermore, it's the primary method of communicating strings across
DLL boundaries. */
#define DeclareCharArrayFromFString(arrayName, filePath) \
    char arrayName[260]; \
	memcpy(&arrayName, TCHAR_TO_ANSI(*filePath), sizeof(char) * filePath.Len()); \
	arrayName[filePath.Len()] = '\0';

#define FillCharArrayFromFString(arrayName, filePath) \
	memcpy(&arrayName, TCHAR_TO_ANSI(*filePath), sizeof(char) * filePath.Len()); \
	arrayName[filePath.Len()] = '\0';

class LensSolverUtilities
{
public:
	static FString GenerateIndexedFilePath(const FString& fileName, const FString & extension);
	static bool ValidateFolder(FString& folder, const FString & backupFolder, const FString& logMessageHeader);
	static bool ValidateFilePath(FString& path, const FString& backupFolder, const FString & backupName, const FString & backupExtension);
	static bool GetImageFilesInFolder(const FString& folder, TArray<FString>& files);

	static FString GenerateGenericOutputPath(const FString & subFolder);
	static FString GenerateGenericDistortionCorrectionMapOutputPath(const FString & subFolder);

	static bool CreateTexture2D(
		void * rawData,
		int width,
		int height,
		bool sRGB,
		bool isLUT,
		UTexture2D *& output,
		EPixelFormat pixelFormat = EPixelFormat::PF_B8G8R8A8);

	static bool LoadTexture16(
		FString absoluteTexturePath, 
		UTexture2D*& texture
	);

	static bool LoadTexture(
		FString absoluteTexturePath,
		UTexture2D*& texture);

	static bool WriteTexture16(
		FString absoluteTexturePath,
		int width,
		int height,
		TUniquePtr<TImagePixelData<FFloat16Color>> data);
};