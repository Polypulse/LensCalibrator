#include "LensSolverWorkerFindCorners.h"

void FLensSolverWorkerFindCorners::Tick()
{
	FString workerMessage = FString::Printf(TEXT("Worker: (ID: %d): "), GetWorkerID());

	TUniquePtr<FLensSolverWorkUnit> workUnitPtr;
	DequeueWorkUnit(workUnitPtr);

	if (!workUnitPtr.IsValid())
	{
		QueueLog(FString("(ERROR): NULL work unit in latch queue!"));
		return;
	}

	cv::Mat image;

	const FLensSolverTextureWorkUnit* textureWorkUnit = static_cast<FLensSolverTextureWorkUnit*>(workUnitPtr.Get());
	float resizePercentage = textureWorkUnit->resizePercentage;
	bool resize = textureWorkUnit->resize;

	float checkerBoardSquareSizeMM = textureWorkUnit->checkerBoardSquareSizeMM;
	FIntPoint checkerBoardCornerCount = textureWorkUnit->checkerBoardCornerCount;

	FIntPoint sourceResolution;
	FIntPoint resizeResolution;

	switch (workUnitPtr->workUnitType)
	{
	case ELensSolverWorkUnitType::PixelArray:

		{
			const FLensSolverPixelArrayWorkUnit* pixelArrayWorkUnit = static_cast<FLensSolverPixelArrayWorkUnit*>(workUnitPtr.Get());
			if (!GetImageFromArray(pixelArrayWorkUnit->pixels, pixelArrayWorkUnit->sourceResolution, image))
				return;

			sourceResolution = pixelArrayWorkUnit->sourceResolution;
		}

		break;

	case ELensSolverWorkUnitType::TextureFile:

		{
			const FLensSolverTextureFileWorkUnit* textureFileWorkUnit = static_cast<FLensSolverTextureFileWorkUnit*>(workUnitPtr.Get());
			if (!GetImageFromFile(textureFileWorkUnit->absoluteFilePath, image, sourceResolution))
				return;
		}

		break;

	case ELensSolverWorkUnitType::Calibrate:
		QueueLog(FString("(ERROR): FLensSolverWorkerFindCorners received calibrate work unit!"));
		return;

	default:
		QueueLog(FString("(ERROR): Un-handeled work unit type!"));
		return;
	}

	resizeResolution = sourceResolution * resizePercentage;

	QueueLog(FString::Printf(TEXT("%sPrepared image of size: (%d, %d!"), *workerMessage, image.cols, image.rows));

	int resizedPixelWidth = FMath::FloorToInt(sourceResolution.X * (resize ? resizePercentage : 1.0f));
	int resizedPixelHeight = FMath::FloorToInt(sourceResolution.Y * (resize ? resizePercentage : 1.0f));

	cv::Size sourceImageSize(sourceResolution.X, sourceResolution.Y);
	cv::Size resizedImageSize(resizedPixelWidth, resizedPixelHeight);

	float inverseResizeRatio = resize ? 1.0f / resizePercentage : 1.0f;

	if (image.rows != resizedPixelWidth || image.cols != resizedPixelHeight)
	{
		UE_LOG(LogTemp, Log, TEXT("%sAllocating image from size: (%d, %d) to: (%d, %d)."), *workerMessage, image.cols, image.rows, resizedPixelWidth, resizedPixelHeight);
		image = cv::Mat(resizedPixelHeight, resizedPixelWidth, cv::DataType<uint8>::type);
	}

	UE_LOG(LogTemp, Log, TEXT("%sResized pixel size: (%d, %d), source size: (%d, %d), resize ratio: %f."),
		*workerMessage,
		resizedPixelWidth,
		resizedPixelHeight,
		sourceResolution.X,
		sourceResolution.Y,
		1.0f / inverseResizeRatio);

	cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001f);

	std::vector<cv::Point2f> imageCorners;
	std::vector<cv::Point3f> imageObjectPoints;

	cv::Size patternSize(checkerBoardCornerCount.X, checkerBoardCornerCount.Y);

	bool patternFound = false;

	int findFlags = cv::CALIB_CB_NORMALIZE_IMAGE;
	findFlags |= cv::CALIB_CB_ADAPTIVE_THRESH;

	if (textureWorkUnit->exhaustiveSearch)
		findFlags |= cv::CALIB_CB_EXHAUSTIVE;

	patternFound = cv::findChessboardCorners(image, patternSize, imageCorners, findFlags);

	if (!patternFound)
	{
		QueueLog(FString::Printf(("No pattern found in image, moving onto the next image.")));
		return;
	}

	QueueLog(FString::Printf(TEXT("Found pattern in image.")));

	cv::TermCriteria cornerSubPixCriteria(
		cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS,
		50,
		0.0001
	);

	cv::cornerSubPix(image, imageCorners, cv::Size(5, 5), cv::Size(-1, -1), cornerSubPixCriteria);

	if (textureWorkUnit->writeDebugTextureToFile)
	{
		cv::drawChessboardCorners(image, patternSize, imageCorners, patternFound);
		WriteMatToFile(image, textureWorkUnit->debugTextureFolderPath);
	}

	for (int y = 0; y < checkerBoardCornerCount.Y; y++)
		for (int x = 0; x < checkerBoardCornerCount.X; x++)
			imageObjectPoints.push_back(cv::Point3f(x * checkerBoardSquareSizeMM, y * checkerBoardSquareSizeMM, 0.0f));

	for (int ci = 0; ci < imageCorners.size(); ci++)
	{
		imageCorners[ci].x = imageCorners[ci].x * inverseResizeRatio;
		imageCorners[ci].y = imageCorners[ci].y * inverseResizeRatio;
	}
}

bool FLensSolverWorkerFindCorners::GetImageFromFile(const FString & absoluteFilePath, cv::Mat& image, FIntPoint & sourceResolution)
{
	Lock();
	Unlock();
	return false;
}

bool FLensSolverWorkerFindCorners::GetImageFromArray(const TArray<FColor> & pixels, const FIntPoint resolution, cv::Mat& image)
{
	QueueLog(FString::Printf(TEXT("%sCopying pixel data of pixel count: %d to OpenCV Mat of size: (%d, %d)."), *workerMessage, pixels.Num(), resolution.X, resolution.Y));

	int pixelCount = resolution.X * resolution.Y;
	for (int pi = 0; pi < pixelCount; pi++)
		image.at<uint8>(pi / resolution.X, pi % resolution.X) = pixels[pi].R;

	QueueLog(FString::Printf(TEXT("%Done copying pixel data, beginning calibration."), *workerMessage, pixels.Num(), resolution.X, resolution.Y));
	return false;
}

FLensSolverWorkerFindCorners::FLensSolverWorkerFindCorners(
	FLensSolverWorkerParameters inputParameters,
	QueueFindCornerResultOutputDel* inputQueueFindCornerResultOutputDel) : FLensSolverWorker(inputParameters)
{
}

void FLensSolverWorkerFindCorners::WriteMatToFile(cv::Mat image, FString folder, FString fileName)
{
	if (!LensSolverUtilities::ValidateFolder(folder, calibrationVisualizationOutputPath, workerMessage))
		return;

	FString outputPath = LensSolverUtilities::GenerateIndexedFilePath(folder, fileName, "jpg");
	
	if (!cv::imwrite(TCHAR_TO_UTF8(*outputPath), image))
	{
		UE_LOG(LogTemp, Error, TEXT("%sUnable to write debug texture result to path: \"%s\", check your permissions."), *workerMessage, *outputPath);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("%sDebug texture written to file at path: \"%s\"."), *workerMessage, *outputPath);
}
