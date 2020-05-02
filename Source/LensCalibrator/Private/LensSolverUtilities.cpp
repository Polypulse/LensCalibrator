#include "..\Public\LensSolverUtilities.h"
#include "Engine.h"

FString LensSolverUtilities::GenerateIndexedFilePath(const FString& folder, const FString& fileName, const FString& extension)
{
	FString partialOutputPath = folder + fileName;

	int index = 0;
	while (FPaths::FileExists(FString::Printf(TEXT("%s-%d.%s"), *partialOutputPath, index, *extension)))
		index++;
	return FString::Printf(TEXT("%s-%d.%s"), *partialOutputPath, index, *extension);
}

bool LensSolverUtilities::ValidateFolder(FString& folder, const FString & backupFolder, const FString& logMessageHeader)
{
	if (folder.IsEmpty())
		folder = backupFolder;

	else
	{
		if (!FPaths::ValidatePath(folder))
		{
			UE_LOG(LogTemp, Error, TEXT("%sThe path: \"%s\" is not a valid."), *logMessageHeader, *folder);
			return false;
		}

		if (FPaths::FileExists(folder))
		{
			UE_LOG(LogTemp, Error, TEXT("%sThe path: \"%s\" is to a file, not a directory."), *logMessageHeader, *folder);
			return false;
		}
	}

	if (!FPaths::DirectoryExists(folder))
	{
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*folder);
		UE_LOG(LogTemp, Log, TEXT("%sCreated directory at path: \"%s\"."), *logMessageHeader, *folder);
	}

	return true;
}

FString LensSolverUtilities::GenerateGenericOutputPath(const FString & subFolder)
{
	return FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir() + subFolder);
}
