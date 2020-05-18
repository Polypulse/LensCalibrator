/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "..\Public\LensSolverUtilities.h"
#include "Engine.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "GenericPlatform/GenericPlatformFile.h"

class FDirectoryVisitor;

FString LensSolverUtilities::GenerateIndexedFilePath(const FString& filePathWithoutExtension, const FString& extension)
{
	int index = 0;
	while (FPaths::FileExists(FString::Printf(TEXT("%s-%d.%s"), *filePathWithoutExtension, index, *extension)))
		index++;
	return FString::Printf(TEXT("%s-%d.%s"), *filePathWithoutExtension, index, *extension);
}

bool LensSolverUtilities::ValidateFolder(FString& folderPath, const FString & backupFolder, const FString& logMessageHeader)
{
	if (folderPath.IsEmpty())
	{
		folderPath = backupFolder;
		if (!FPaths::DirectoryExists(folderPath))
		{
			FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*folderPath);
			UE_LOG(LogTemp, Log, TEXT("%sCreated directory at path: \"%s\"."), *logMessageHeader, *folderPath);
		}
	}

	else
	{
		if (!FPaths::ValidatePath(folderPath))
		{
			UE_LOG(LogTemp, Error, TEXT("%sThe path: \"%s\" is not a valid."), *logMessageHeader, *folderPath);
			return false;
		}

		FString extension = FPaths::GetExtension(folderPath);
		if (!extension.IsEmpty())
		{
			FString containingFolderPath = FPaths::GetPathLeaf(folderPath);
			if (!FPaths::DirectoryExists(containingFolderPath))
			{
				FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*containingFolderPath);
				UE_LOG(LogTemp, Log, TEXT("%sCreated directory at path: \"%s\"."), *logMessageHeader, *containingFolderPath);
			}
		}
	}

	return true;
}

bool LensSolverUtilities::ValidateFilePath(FString& filePath, const FString& absoluteBackupFolderPath, const FString & backupFileName, const FString & backupExtension)
{
	if (filePath.IsEmpty() || !FPaths::ValidatePath(filePath))
	{
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*absoluteBackupFolderPath);
		filePath = FPaths::Combine(absoluteBackupFolderPath, backupFileName);
		filePath = LensSolverUtilities::GenerateIndexedFilePath(filePath, backupExtension);
		return true;
	}

	FString folder = FPaths::GetPath(filePath);
	if (folder.IsEmpty())
		folder = absoluteBackupFolderPath;

	if (FPaths::IsRelative(filePath))
		filePath = FPaths::ConvertRelativePathToFull(filePath);

	if (FPaths::IsDrive(filePath))
	{
		filePath = FPaths::Combine(filePath, backupFileName);
		filePath = LensSolverUtilities::GenerateIndexedFilePath(filePath, backupExtension);
		return true;
	}

	FString fileName = FPaths::GetBaseFilename(filePath);
	FString extension = FPaths::GetExtension(filePath);

	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*folder);

	if (extension.IsEmpty())
	{
		if (FPaths::FileExists(folder))
		{
			UE_LOG(LogTemp, Error, TEXT("Cannot create path to file: \"%s\", the folder: \"%s\" is a file."), *filePath, *folder);
			return false;
		}

		filePath = FPaths::Combine(folder, backupFileName);
		filePath = LensSolverUtilities::GenerateIndexedFilePath(filePath, backupExtension);
		return true;
	}

	filePath = FPaths::Combine(folder, fileName);
	filePath = LensSolverUtilities::GenerateIndexedFilePath(filePath, extension);
	return true;
}

bool LensSolverUtilities::GetFilesInFolder(const FString& folder, TArray<FString>& files)
{
	if (folder.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot get files in folder, the folder string is empty!"));
		return false;
	}

	if (!FPaths::DirectoryExists(folder))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot get files in folder at path: \"%s\", the folder does not exist!"), *folder);
		return false;
	}

	struct FDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
	{
		TArray<FString>& FileNames;

		FDirectoryVisitor(TArray<FString>& InFileNames)
			: FileNames(InFileNames)
		{
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			FString FileName(FilenameOrDirectory);
			if (FileName.EndsWith(TEXT(".png"))		|| 
				FileName.EndsWith(TEXT(".jpg"))		|| 
				FileName.EndsWith(TEXT(".jpeg"))	|| 
				FileName.EndsWith(TEXT(".bmp"))		|| 
				FileName.EndsWith(TEXT(".exr")))
			{
				FileNames.Add(FileName);
			}

			return true;
		}
	};

	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();
	FDirectoryVisitor vistor(files);
	platformFile.IterateDirectory(*folder, vistor);

	if (files.Num() == 0)
		return false;

	return true;
}

FString LensSolverUtilities::GenerateGenericOutputPath(const FString & subFolder)
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), subFolder));
}

bool LensSolverUtilities::CreateTexture2D(
	void * rawData, 
	int width,
	int height,
	bool sRGB,
	bool isLUT,
	UTexture2D *& output,
	EPixelFormat pixelFormat)
{
	int stride = 0;
	int size = 0;
	switch (pixelFormat)
	{
	case EPixelFormat::PF_B8G8R8A8:
	case EPixelFormat::PF_R8G8B8A8:
	case EPixelFormat::PF_A8R8G8B8:
		size = 1;
		stride = 4;
		break;
	case EPixelFormat::PF_FloatRGBA:
	case EPixelFormat::PF_A16B16G16R16:
		size = 2;
		stride = 4;
		break;
	default:
		UE_LOG(LogTemp, Error, TEXT("Non-implemented pixel format: \"%s\"."), GetPixelFormatString(pixelFormat));
		return false;
	}

	output = UTexture2D::CreateTransient(width, height, pixelFormat);
	output->SRGB = sRGB;

	UE_LOG(LogTemp, Log, TEXT("Created transient UTexture2D with the following parameters:\n{\n\tResolution: (%d, %d),\n\tPixel Format: \"%s\",\n\tsRGB: %d\n}"), width, height, GetPixelFormatString(pixelFormat), sRGB ? 1 : 0);

	if (isLUT)
	{
		output->Filter = TextureFilter::TF_Nearest;
		output->CompressionSettings = TextureCompressionSettings::TC_HDR;
		output->LODGroup = TextureGroup::TEXTUREGROUP_16BitData;
		// output->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
		// output->CompressionNone = true;
		// output->NeverStream = true;
	}

	if (output == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to create transient texture"));
		return false;
	}

	output->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	output->PlatformData->Mips[0].SizeX = width;
	output->PlatformData->Mips[0].SizeY = height;
	output->PlatformData->Mips[0].BulkData.Realloc(width * height * stride * size);
	output->PlatformData->Mips[0].BulkData.Unlock();

	uint8 * textureData = (uint8*)output->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);

	if (textureData == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("BulkData.Lock returned nullptr!"));
		return false;
	}
	
	FMemory::Memcpy(textureData, rawData, width * height * stride * size);
	output->PlatformData->Mips[0].BulkData.Unlock();

	// texture->Resource = texture->CreateResource();
	output->UpdateResource();
	output->RefreshSamplerStates();

	return true;
}

bool LensSolverUtilities::LoadTexture16(FString absoluteTexturePath, UTexture2D*& texture)
{
	if (!FPaths::FileExists(absoluteTexturePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot find texture at path: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	TArray<uint8> fileData;
	if (!FFileHelper::LoadFileToArray(fileData, *absoluteTexturePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to load data into memory from path: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	IImageWrapperModule& imageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

	EImageFormat format = imageWrapperModule.DetectImageFormat(fileData.GetData(), fileData.Num());
	if (format == EImageFormat::Invalid)
	{
		UE_LOG(LogTemp, Error, TEXT("Unrecognized image file format in texture: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Detected image format; %d in blending texture: \"%s\"."), format, *absoluteTexturePath);

	TSharedPtr<IImageWrapper> imageWrapper = imageWrapperModule.CreateImageWrapper(format);
	if (!imageWrapper.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create image wrapper for texture: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Loading in 16-bit pixel data into memory from file: \"%s\"."), *absoluteTexturePath);

	if (!imageWrapper->SetCompressed(fileData.GetData(), fileData.Num()))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to decompress texture data in file: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	TArray64<uint8> rawData;
	if (!imageWrapper->GetRaw(ERGBFormat::RGBA, 16, rawData))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get raw data in file: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	if (imageWrapper->GetWidth() <= 0 || imageWrapper->GetHeight() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Texture width or heiht is <= 0 in file: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	texture = UTexture2D::CreateTransient(imageWrapper->GetWidth(), imageWrapper->GetHeight(), PF_FloatRGBA);
	if (texture == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Texture2D file: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	texture->CompressionSettings = TextureCompressionSettings::TC_HDR;
	texture->LODGroup = TextureGroup::TEXTUREGROUP_16BitData;
	texture->SRGB = 0;

	texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	texture->PlatformData->Mips[0].SizeX = imageWrapper->GetWidth();
	texture->PlatformData->Mips[0].SizeY = imageWrapper->GetHeight();
	texture->PlatformData->Mips[0].BulkData.Realloc(rawData.Num());
	texture->PlatformData->Mips[0].BulkData.Unlock();

	uint8 * textureData = (uint8*)texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);

	if (textureData == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("BulkData.Lock returned nullptr!"));
		return false;
	}
	
	FMemory::Memcpy(textureData, rawData.GetData(), rawData.Num());
	texture->PlatformData->Mips[0].BulkData.Unlock();

	texture->UpdateResource();

	UE_LOG(LogTemp, Log, TEXT("Successfully read texture from file: \"%s\"."), *absoluteTexturePath);

	return true;
}

bool LensSolverUtilities::LoadTexture(FString absoluteTexturePath, UTexture2D*& texture)
{
	if (!FPaths::FileExists(absoluteTexturePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot find texture at path: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	TArray<uint8> fileData;
	if (!FFileHelper::LoadFileToArray(fileData, *absoluteTexturePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to load data into memory from path: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	IImageWrapperModule& imageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

	EImageFormat format = imageWrapperModule.DetectImageFormat(fileData.GetData(), fileData.Num());
	if (format == EImageFormat::Invalid)
	{
		UE_LOG(LogTemp, Error, TEXT("Unrecognized image file format in texture: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Detected image format; %d in blending texture: \"%s\"."), format, *absoluteTexturePath);

	TSharedPtr<IImageWrapper> imageWrapper = imageWrapperModule.CreateImageWrapper(format);
	if (!imageWrapper.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create image wrapper for texture: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Loading in 8-bit pixel data into memory from file: \"%s\"."), *absoluteTexturePath);

	if (!imageWrapper->SetCompressed(fileData.GetData(), fileData.Num()))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to decompress texture data in file: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	TArray64<uint8> rawData;
	if (!imageWrapper->GetRaw(ERGBFormat::RGBA, 8, rawData))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get raw data in file: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	if (imageWrapper->GetWidth() <= 0 || imageWrapper->GetHeight() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Texture width or heiht is <= 0 in file: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	texture = UTexture2D::CreateTransient(imageWrapper->GetWidth(), imageWrapper->GetHeight(), PF_R8G8B8A8);
	if (texture == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create Texture2D file: \"%s\"."), *absoluteTexturePath);
		return false;
	}

	texture->SRGB = 1;
	texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	texture->PlatformData->Mips[0].SizeX = imageWrapper->GetWidth();
	texture->PlatformData->Mips[0].SizeY = imageWrapper->GetHeight();
	texture->PlatformData->Mips[0].BulkData.Realloc(rawData.Num());
	texture->PlatformData->Mips[0].BulkData.Unlock();

	uint8 * textureData = (uint8*)texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);

	if (textureData == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("BulkData.Lock returned nullptr!"));
		return false;
	}
	
	FMemory::Memcpy(textureData, rawData.GetData(), rawData.Num());
	texture->PlatformData->Mips[0].BulkData.Unlock();

	texture->UpdateResource();

	UE_LOG(LogTemp, Log, TEXT("Successfully read texture from file: \"%s\"."), *absoluteTexturePath);

	return true;
}

bool LensSolverUtilities::WriteTexture16(
	FString absoluteTexturePath,
	int width,
	int height,
	TUniquePtr<TImagePixelData<FFloat16Color>> data)
{
	IImageWriteQueueModule* imageWriteQueueModule = FModuleManager::Get().GetModulePtr<IImageWriteQueueModule>("ImageWriteQueue");
	if (imageWriteQueueModule == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to retrieve ImageWriteQueue."));
		return false;
	}

	TUniquePtr<FImageWriteTask> imageTask = MakeUnique<FImageWriteTask>();

	imageTask->Format = EImageFormat::EXR;
	imageTask->Filename = absoluteTexturePath;
	imageTask->bOverwriteFile = true;
	imageTask->CompressionQuality = (int32)EImageCompressionQuality::Uncompressed;
	imageTask->PixelData = MoveTemp(data);

	TFuture<bool> dispatchedTask = imageWriteQueueModule->GetWriteQueue().Enqueue(MoveTemp(imageTask));

	if (dispatchedTask.IsValid())
		dispatchedTask.Wait();

	return true;

	/*
	const size_t BitsPerPixel = (sizeof(FFloat16Color) / 4) * 8;

	IImageWrapperModule& imageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	FImageWriter imageWrapper = imageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	
	if (imageWrapper.IsValid() &&
		imageWrapper->SetRaw(data, sizeof(FFloat16Color) * width * height, width, height, ERGBFormat::RGBA, BitsPerPixel))
	{
		EImageCompressionQuality LocalCompressionQuality = EImageCompressionQuality::Default;

		// Compress and write image
		// OutBitmapData = imageWrapper->GetCompressed((int32)LocalCompressionQuality);
		
		bSuccess = true;
	}

	*/
	return true;
}

