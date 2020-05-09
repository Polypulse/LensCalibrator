#include "LensSolverWorkerFindCorners.h"
#include "GenericPlatform/GenericPlatformProcess.h"

FLensSolverWorkerFindCorners::FLensSolverWorkerFindCorners(
	const FLensSolverWorkerParameters & inputParameters,
	QueueTextureFileWorkUnitInputDel* inputQueueTextureFileWorkUnitInputDel,
	QueuePixelArrayWorkUnitInputDel* inputQueuePixelArrayWorkUnitInputDel,
	const QueueFindCornerResultOutputDel* inputQueueFindCornerResultOutputDel) :
	FLensSolverWorker(inputParameters),
	queueFindCornerResultOutputDel(inputQueueFindCornerResultOutputDel)
{
	inputQueueTextureFileWorkUnitInputDel->BindRaw(this, &FLensSolverWorkerFindCorners::QueueTextureFileWorkUnit);
	inputQueuePixelArrayWorkUnitInputDel->BindRaw(this, &FLensSolverWorkerFindCorners::QueuePixelArrayWorkUnit);

	/*
	queueTextureFileWorkUnitInputDel = inputQueueTextureFileWorkUnitInputDel;
	queuePixelArrayWorkUnitInputDel = inputQueuePixelArrayWorkUnitInputDel;
	*/

	workUnitCount = 0;
}

int FLensSolverWorkerFindCorners::GetWorkLoad()
{
	int count = 0;
	Lock();
	count = workUnitCount;
	Unlock();

	/*
	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): Retrieving FindCorners worker work load: %d"), count));
	*/

	return count;
}

void FLensSolverWorkerFindCorners::QueueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit workUnit)
{
	/*
	static int count = 0;
	count++;
	QueueLog(FString::Printf(TEXT("Worker received TextureFileWorkUnit of index: %d"), count));
	*/

	textureFileWorkQueue.Enqueue(workUnit);
	Lock();
	workUnitCount++;
	Unlock();

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queued TextureFileWorkUnit with path: \"%s\", total currently queued: %d."), *JobDataToString(workUnit.baseParameters), *workUnit.textureFileParameters.absoluteFilePath, workUnitCount));
}

void FLensSolverWorkerFindCorners::QueuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit workUnit)
{
	pixelArrayWorkQueue.Enqueue(workUnit);
	Lock();
	workUnitCount++;
	Unlock();

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queued PixelArrayWorkUnit of resolution: (%d, %d), total currently queued: %d."), *JobDataToString(workUnit.baseParameters), workUnit.resizeParameters.sourceResolution.X, workUnit.resizeParameters.sourceResolution.Y, workUnitCount));
}

void FLensSolverWorkerFindCorners::DequeueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit& workUnit)
{
	textureFileWorkQueue.Dequeue(workUnit);
	Lock();
	workUnitCount--;
	Unlock();

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): %s: Dequeued TextureFileWorkUnit with path: \"%s\"."), *JobDataToString(workUnit.baseParameters), *workUnit.textureFileParameters.absoluteFilePath));
}

void FLensSolverWorkerFindCorners::DequeuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit & workUnit)
{
	pixelArrayWorkQueue.Dequeue(workUnit);
	Lock();
	workUnitCount--;
	Unlock();

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): %s: Dequeued PixelArrayWorkUnit of resolution: (%d, %d)."), *JobDataToString(workUnit.baseParameters), workUnit.resizeParameters.sourceResolution.X, workUnit.resizeParameters.sourceResolution.Y));
}

void FLensSolverWorkerFindCorners::Tick()
{
	FBaseParameters baseParameters;
	FTextureSearchParameters textureSearchParameters;
	FResizeParameters resizeParameters;

	cv::Mat image;

	if (!textureFileWorkQueue.IsEmpty())
	{
		FLensSolverTextureFileWorkUnit textureFileWorkUnit;
		DequeueTextureFileWorkUnit(textureFileWorkUnit);
		baseParameters = textureFileWorkUnit.baseParameters;
		textureSearchParameters = textureFileWorkUnit.textureSearchParameters;

		if (!GetImageFromFile(textureFileWorkUnit.textureFileParameters.absoluteFilePath, image, resizeParameters.sourceResolution))
		{
			QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
			return;
		}
	}

	else if (!pixelArrayWorkQueue.IsEmpty())
	{
		FLensSolverPixelArrayWorkUnit texturePixelArrayUnit;
		DequeuePixelArrayWorkUnit(texturePixelArrayUnit);
		baseParameters = texturePixelArrayUnit.baseParameters;
		textureSearchParameters = texturePixelArrayUnit.textureSearchParameters;
		resizeParameters = texturePixelArrayUnit.resizeParameters;

		if (!GetImageFromArray(texturePixelArrayUnit.pixelArrayParameters.pixels, resizeParameters.resizeResolution, image))
		{
			QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
			return;
		}
	}

	else return;

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): %s: Preparing search for calibration pattern using source image of size: (%d, %d)."), 
			*JobDataToString(baseParameters), 
			resizeParameters.sourceResolution.X,
			resizeParameters.sourceResolution.Y));

	float resizePercentage = textureSearchParameters.resizePercentage;
	bool resize = textureSearchParameters.resize;

	float checkerBoardSquareSizeMM = textureSearchParameters.checkerBoardSquareSizeMM;
	FIntPoint checkerBoardCornerCount = textureSearchParameters.checkerBoardCornerCount;
	// resizeParameters.resizeResolution = resizeParameters.sourceResolution * resizePercentage;

	// QueueLog(FString::Printf(TEXT("%sPrepared image of size: (%d, %d!"), *workerMessage, image.cols, image.rows));

	if (resize)
	{
		resizeParameters.resizeResolution.X = FMath::FloorToInt(resizeParameters.sourceResolution.X * resizePercentage);
		resizeParameters.resizeResolution.Y = FMath::FloorToInt(resizeParameters.sourceResolution.Y * resizePercentage);
	}

	cv::Size sourceImageSize(resizeParameters.sourceResolution.X, resizeParameters.sourceResolution.Y);
	cv::Size resizedImageSize(resizeParameters.resizeResolution.X, resizeParameters.resizeResolution.Y);

	float inverseResizeRatio = 1.0f / resizePercentage;

	if (resize && resizePercentage != 1.0f)
	{
		if (debug)
			QueueLog(FString::Printf(TEXT("(INFO): %s: Resizing image from: (%d, %d) to: (%d, %d)."),
				*JobDataToString(baseParameters),
				resizeParameters.sourceResolution.X,
				resizeParameters.sourceResolution.Y,
				resizeParameters.resizeResolution.X,
				resizeParameters.resizeResolution.Y));

		cv::resize(image, image, resizedImageSize, 0.0f, 0.0f, cv::INTER_LINEAR);
	}

	/*/
	if (image.rows != resizedPixelWidth || image.cols != resizedPixelHeight)
	{
		UE_LOG(LogTemp, Log, TEXT("%sAllocating image from size: (%d, %d) to: (%d, %d)."), *workerMessage, image.cols, image.rows, resizedPixelWidth, resizedPixelHeight);
		image = cv::Mat(resizedPixelHeight, resizedPixelWidth, cv::DataType<uint8>::type);
	}
	*/

	/*
	UE_LOG(LogTemp, Log, TEXT("%sResized pixel size: (%d, %d), source size: (%d, %d), resize ratio: %f."),
		*workerMessage,
		resizedPixelWidth,
		resizedPixelHeight,
		resizeParameters.sourceResolution.X,
		resizeParameters.sourceResolution.Y,
		1.0f / inverseResizeRatio);
	*/

	cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001f);

	std::vector<cv::Point2f> imageCorners;
	std::vector<cv::Point3f> imageObjectPoints;

	cv::Size patternSize(checkerBoardCornerCount.X, checkerBoardCornerCount.Y);

	bool patternFound = false;

	int findFlags = cv::CALIB_CB_NORMALIZE_IMAGE;
	findFlags |= cv::CALIB_CB_ADAPTIVE_THRESH;

	if (textureSearchParameters.exhaustiveSearch)
		findFlags |= cv::CALIB_CB_EXHAUSTIVE;

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): %s: Beginning calibration pattern detection for image: \"%s\"."), *JobDataToString(baseParameters), *baseParameters.friendlyName));
	patternFound = cv::findChessboardCorners(image, patternSize, imageCorners, findFlags);

	if (!patternFound)
	{
		QueueLog(FString::Printf(TEXT("(INFO): %s: Found no pattern in image: \"%s\", queuing empty work unit."), *JobDataToString(baseParameters), *baseParameters.friendlyName));
		QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
		return;
	}

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): %s: Found calibration pattern in image: \"%s\"."), *JobDataToString(baseParameters), *baseParameters.friendlyName));

	cv::TermCriteria cornerSubPixCriteria(
		cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS,
		50,
		0.0001
	);

	cv::cornerSubPix(image, imageCorners, cv::Size(5, 5), cv::Size(-1, -1), cornerSubPixCriteria);

	if (textureSearchParameters.writeDebugTextureToFile)
	{
		cv::drawChessboardCorners(image, patternSize, imageCorners, patternFound);
		WriteMatToFile(image, textureSearchParameters.debugTextureFolderPath, "CheckerboardVisualization");
	}

	for (int y = 0; y < checkerBoardCornerCount.Y; y++)
		for (int x = 0; x < checkerBoardCornerCount.X; x++)
			imageObjectPoints.push_back(cv::Point3f(x * checkerBoardSquareSizeMM, y * checkerBoardSquareSizeMM, 0.0f));

	for (int ci = 0; ci < imageCorners.size(); ci++)
	{
		imageCorners[ci].x = imageCorners[ci].x * inverseResizeRatio;
		imageCorners[ci].y = imageCorners[ci].y * inverseResizeRatio;
	}

	FLensSolverCalibrationPointsWorkUnit calibrationPointsWorkUnit;

	calibrationPointsWorkUnit.baseParameters								= baseParameters;
	calibrationPointsWorkUnit.calibrationPointParameters.corners			= imageCorners;
	calibrationPointsWorkUnit.calibrationPointParameters.objectPoints	= imageObjectPoints;
	calibrationPointsWorkUnit.resizeParameters							= resizeParameters;

	QueueCalibrationPointsWorkUnit(calibrationPointsWorkUnit);
}

bool FLensSolverWorkerFindCorners::GetImageFromFile(const FString & absoluteFilePath, cv::Mat& image, FIntPoint & sourceResolution)
{
	cv::String cvPath(TCHAR_TO_UTF8(*absoluteFilePath));
	image = cv::imread(cvPath);

	if (image.data == NULL)
	{
		if (debug)
			QueueLog(FString::Printf(TEXT("(ERROR) Unable texture from path: \"%s\""), *absoluteFilePath));
		return false;
	}

	cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
	sourceResolution = FIntPoint(image.cols, image.rows);

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): Loaded texture from path: \"%s\" at resolution: (%d, %d)."), *absoluteFilePath, sourceResolution.X, sourceResolution.Y));
	return true;
}

bool FLensSolverWorkerFindCorners::GetImageFromArray(const TArray<FColor> & pixels, const FIntPoint resolution, cv::Mat& image)
{
	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): Copying pixel data of pixel count: %d to image of size: (%d, %d)."), pixels.Num(), resolution.X, resolution.Y));

	image = cv::Mat(resolution.Y, resolution.X, cv::DataType<uint8>::type);

	int pixelCount = resolution.X * resolution.Y;
	for (int pi = 0; pi < pixelCount; pi++)
		image.at<uint8>(pi / resolution.X, pi % resolution.X) = pixels[pi].R;

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): Done copying pixel data."), pixels.Num(), resolution.X, resolution.Y));

	return true;
}

void FLensSolverWorkerFindCorners::WriteMatToFile(cv::Mat image, FString folder, FString fileName)
{
	if (!LensSolverUtilities::ValidateFolder(folder, calibrationVisualizationOutputPath, workerMessage))
		return;

	FString outputPath = LensSolverUtilities::GenerateIndexedFilePath(folder, fileName, "jpg");
	
	if (!cv::imwrite(TCHAR_TO_UTF8(*outputPath), image))
	{
		QueueLog(FString::Printf(TEXT("(INFO): Unable to write debug texture result to path: \"%s\", check your permissions."), *outputPath));
		return;
	}

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): Debug texture written to file at path: \"%s\"."), *outputPath));
}

void FLensSolverWorkerFindCorners::QueueCalibrationPointsWorkUnit(const FLensSolverCalibrationPointsWorkUnit & calibrationPointsWorkUnit)
{
	if (!queueFindCornerResultOutputDel->IsBound())
		return;
	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queuing calibration points work unit."), *JobDataToString(calibrationPointsWorkUnit.baseParameters)));
	queueFindCornerResultOutputDel->Execute(calibrationPointsWorkUnit);
}

void FLensSolverWorkerFindCorners::QueueEmptyCalibrationPointsWorkUnit(const FBaseParameters & baseParameters, const FResizeParameters & resizeParameters)
{
	if (!queueFindCornerResultOutputDel->IsBound())
		return;

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queuing EMPTY calibration points work unit."), *JobDataToString(baseParameters)));

	FLensSolverCalibrationPointsWorkUnit calibrationPointsWorkUnit;
	calibrationPointsWorkUnit.baseParameters = baseParameters;
	calibrationPointsWorkUnit.resizeParameters = resizeParameters;
	queueFindCornerResultOutputDel->Execute(calibrationPointsWorkUnit);
}
