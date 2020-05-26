/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolverWorkerFindCorners.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "MatQueueWriter.h"
#include "OpenCVWrapper.h"

FLensSolverWorkerFindCorners::FLensSolverWorkerFindCorners(
	FLensSolverWorkerParameters & inputParameters,
	QueueTextureFileWorkUnitInputDel* inputQueueTextureFileWorkUnitInputDel,
	QueuePixelArrayWorkUnitInputDel* inputQueuePixelArrayWorkUnitInputDel,
	QueueFindCornerResultOutputDel* inputQueueFindCornerResultOutputDel) :
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
	if (Debug())
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

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queued TextureFileWorkUnit with path: \"%s\", total currently queued: %d."), *JobDataToString(workUnit.baseParameters), *workUnit.textureFileParameters.absoluteFilePath, workUnitCount));
}

void FLensSolverWorkerFindCorners::QueuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit workUnit)
{
	pixelArrayWorkQueue.Enqueue(workUnit);
	Lock();
	workUnitCount++;
	Unlock();

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queued PixelArrayWorkUnit of resolution: (%d, %d), total currently queued: %d."), *JobDataToString(workUnit.baseParameters), workUnit.resizeParameters.sourceResolution.X, workUnit.resizeParameters.sourceResolution.Y, workUnitCount));
}

void FLensSolverWorkerFindCorners::DequeueTextureFileWorkUnit(FLensSolverTextureFileWorkUnit& workUnit)
{
	textureFileWorkQueue.Dequeue(workUnit);
	Lock();
	workUnitCount--;
	Unlock();

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Dequeued TextureFileWorkUnit with path: \"%s\"."), *JobDataToString(workUnit.baseParameters), *workUnit.textureFileParameters.absoluteFilePath));
}

void FLensSolverWorkerFindCorners::DequeuePixelArrayWorkUnit(FLensSolverPixelArrayWorkUnit & workUnit)
{
	pixelArrayWorkQueue.Dequeue(workUnit);
	Lock();
	workUnitCount--;
	Unlock();

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Dequeued PixelArrayWorkUnit of resolution: (%d, %d)."), *JobDataToString(workUnit.baseParameters), workUnit.resizeParameters.sourceResolution.X, workUnit.resizeParameters.sourceResolution.Y));
}

void FLensSolverWorkerFindCorners::Tick()
{
	FBaseParameters baseParameters;
	FResizeParameters resizeParameters;

	cv::Mat image;

	float * data = nullptr;
	if (!textureFileWorkQueue.IsEmpty())
	{
		FLensSolverTextureFileWorkUnit textureFileWorkUnit;

		DequeueTextureFileWorkUnit(textureFileWorkUnit);
		baseParameters = textureFileWorkUnit.baseParameters;

		resizeParameters.nativeX = textureFileWorkUnit.textureSearchParameters.nativeFullResolutionX;
		resizeParameters.nativeY = textureFileWorkUnit.textureSearchParameters.nativeFullResolutionY;

		if (textureFileWorkUnit.textureSearchParameters.resize)
		{
			resizeParameters.resizeX = FMath::FloorToInt(resizeParameters.sourceX * textureFileWorkUnit.textureSearchParameters.resizePercentage);
			resizeParameters.resizeY = FMath::FloorToInt(resizeParameters.sourceY * textureFileWorkUnit.textureSearchParameters.resizePercentage);
		}

		else
		{
			resizeParameters.resizeX = resizeParameters.sourceX;
			resizeParameters.resizeY = resizeParameters.sourceY;
		}


		if (!GetOpenCVWrapper().ProcessImageFromFile(
			resizeParameters,
			textureFileWorkUnit.textureSearchParameters,
			std::string(TCHAR_TO_UTF8(*textureFileWorkUnit.textureFileParameters.absoluteFilePath)),
			data))
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
		// textureSearchParameters = texturePixelArrayUnit.textureSearchParameters;
		// resizeParameters = texturePixelArrayUnit.resizeParameters;
		// resizeParameters.nativeResolution = textureSearchParameters.nativeFullResolution;

		/*
		if (!GetImageFromArray(texturePixelArrayUnit.pixelArrayParameters.pixels, resizeParameters.resizeResolution, image))
		{
			QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
			return;
		}
		*/
	}

	else return;

	/*
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Preparing search for calibration pattern using source image of size: (%d, %d)."), 
			*JobDataToString(baseParameters), 
			resizeParameters.sourceResolution.X,
			resizeParameters.sourceResolution.Y));
	*/

	/*
	float resizePercentage = textureSearchParameters.resizePercentage;
	bool resize = textureSearchParameters.resize;

	float checkerBoardSquareSizeMM = textureSearchParameters.checkerBoardSquareSizeMM;
	FIntPoint checkerBoardCornerCount = textureSearchParameters.checkerBoardCornerCount;
	// resizeParameters.resizeResolution = resizeParameters.sourceResolution * resizePercentage;

	// QueueLog(FString::Printf(TEXT("%sPrepared image of size: (%d, %d!"), *workerMessage, image.cols, image.rows));

	cv::Size sourceImageSize(resizeParameters.sourceResolution.X, resizeParameters.sourceResolution.Y);
	cv::Size resizedImageSize(resizeParameters.resizeResolution.X, resizeParameters.resizeResolution.Y);

	float inverseResizeRatio = resizeParameters.nativeResolution.X / (float)resizeParameters.resizeResolution.X;

	if (resize && resizePercentage != 1.0f)
	{
		if (Debug())
			QueueLog(FString::Printf(TEXT("(INFO): %s: Resizing image from: (%d, %d) to: (%d, %d)."),
				*JobDataToString(baseParameters),
				resizeParameters.sourceResolution.X,
				resizeParameters.sourceResolution.Y,
				resizeParameters.resizeResolution.X,
				resizeParameters.resizeResolution.Y));

		cv::resize(image, image, resizedImageSize, 0.0f, 0.0f, cv::INTER_LINEAR);
	}

	/*
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

	/*
	cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001f);

	std::vector<cv::Point2f> imageCorners;

	cv::Size patternSize(checkerBoardCornerCount.X, checkerBoardCornerCount.Y);

	bool patternFound = false;

	int findFlags = cv::CALIB_CB_NORMALIZE_IMAGE;
	findFlags |= cv::CALIB_CB_ADAPTIVE_THRESH;

	if (textureSearchParameters.exhaustiveSearch)
		findFlags |= cv::CALIB_CB_EXHAUSTIVE;

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Beginning calibration pattern detection for image: \"%s\"."), *JobDataToString(baseParameters), *baseParameters.friendlyName));

	// patternFound = cv::findChessboardCorners(image, patternSize, imageCorners, findFlags);
	// TArray<float> data;
	// data.SetNum(checkerBoardCornerCount.X * checkerBoardCornerCount.Y * 2);
	patternFound = GetOpenCVWrapper().FindChessboardCorners(image, patternSize, imageCorners, findFlags);
	/*
	try
	{
		patternFound = cv::findChessboardCorners(image, patternSize, imageCorners, findFlags);
	}

	// catch (const cv::Exception& exception)
	catch (...)
	{
		// FString exceptionMsg = UTF8_TO_TCHAR(exception.msg.c_str());
		// QueueLog(FString::Printf(TEXT("(ERROR): OpenCV exception: \"%s\"."), *exceptionMsg));
		QueueLog("(ERROR): OpenCV exception occurred.");
		QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
		return;
	}

	if (!patternFound)
	{
		QueueLog(FString::Printf(TEXT("(INFO): %s: Found no pattern in image: \"%s\", queuing empty work unit."), *JobDataToString(baseParameters), *baseParameters.friendlyName));
		QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
		return;
	}

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Found calibration pattern in image: \"%s\"."), *JobDataToString(baseParameters), *baseParameters.friendlyName));

	cv::TermCriteria cornerSubPixCriteria(
		cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS,
		50,
		0.0001
	);

	// cv::cornerSubPix(image, imageCorners, cv::Size(5, 5), cv::Size(-1, -1), cornerSubPixCriteria);
	/*
	try
	{
		cv::cornerSubPix(image, imageCorners, cv::Size(5, 5), cv::Size(-1, -1), cornerSubPixCriteria);
	}

	// catch (const cv::Exception& exception)
	catch (...)
	{
		// FString exceptionMsg = UTF8_TO_TCHAR(exception.msg.c_str());
		// QueueLog(FString::Printf(TEXT("(ERROR): OpenCV exception: \"%s\"."), *exceptionMsg));
		QueueLog("(ERROR): OpenCV exception occurred.");
		QueueEmptyCalibrationPointsWorkUnit(baseParameters, resizeParameters);
		return;
	}

	if (textureSearchParameters.writeDebugTextureToFile)
	{
		// cv::drawChessboardCorners(image, patternSize, imageCorners, patternFound);
		WriteMatToFile(image, textureSearchParameters.debugTextureOutputPath);
	}
	*/

	TArray<FVector2D> corners;
	TArray<FVector> objectPoints;

	// corners.SetNum(data.Num() / 2);
	corners.SetNum(textureSearchParameters.checkerBoardCornerCount.X * textureSearchParameters.checkerBoardCornerCount.Y);
	objectPoints.SetNum(textureSearchParameters.checkerBoardCornerCount.X * textureSearchParameters.checkerBoardCornerCount.Y);

	int i = 0;
	for (int y = 0; y < textureSearchParameters.checkerBoardCornerCount.Y; y++)
		for (int x = 0; x < textureSearchParameters.checkerBoardCornerCount.X; x++)
			objectPoints[i++] = FVector(
				x * textureSearchParameters.checkerBoardSquareSizeMM,
				y * textureSearchParameters.checkerBoardSquareSizeMM,
				0.0f);

	for (int ci = 0; ci < textureSearchParameters.checkerBoardCornerCount.X * textureSearchParameters.checkerBoardCornerCount.Y; ci += 2)
	{
		corners[ci].X = *(data + ci).x * inverseResizeRatio;
		corners[ci].Y = *(data + ci + 1).y * inverseResizeRatio;
	}

	// bool emptied = imageCorners.empty();
	// data.Empty();

	FLensSolverCalibrationPointsWorkUnit calibrationPointsWorkUnit;

	calibrationPointsWorkUnit.baseParameters								= baseParameters;
	calibrationPointsWorkUnit.calibrationPointParameters.corners			= corners;
	calibrationPointsWorkUnit.calibrationPointParameters.objectPoints		= objectPoints;
	calibrationPointsWorkUnit.resizeParameters								= resizeParameters;

	QueueCalibrationPointsWorkUnit(calibrationPointsWorkUnit);
}

/*
bool FLensSolverWorkerFindCorners::GetImageFromFile(const FString & absoluteFilePath, cv::Mat& image, FIntPoint & sourceResolution)
{
	std::string str(TCHAR_TO_UTF8(*absoluteFilePath));
	std::replace(str.begin(), str.end(), '\\', '/');

	if (Debug())
		QueueLog(FString::Printf(TEXT("Attempting to read image from path: \"%s\""), *absoluteFilePath));
		
	image = cv::imread(str);

	if (image.data == NULL)
	{
		if (Debug())
			QueueLog(FString::Printf(TEXT("(ERROR) Unable texture from path: \"%s\""), *absoluteFilePath));
		return false;
	}

	cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
	sourceResolution = FIntPoint(image.cols, image.rows);

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): Loaded texture from path: \"%s\" at resolution: (%d, %d)."), *absoluteFilePath, sourceResolution.X, sourceResolution.Y));
	return true;
}

bool FLensSolverWorkerFindCorners::GetImageFromArray(const TArray<FColor> & pixels, const FIntPoint resolution, cv::Mat& image)
{
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): Copying pixel data of pixel count: %d to image of size: (%d, %d)."), pixels.Num(), resolution.X, resolution.Y));

	image = cv::Mat(resolution.Y, resolution.X, cv::DataType<uint8>::type);

	int pixelCount = resolution.X * resolution.Y;
	for (int pi = 0; pi < pixelCount; pi++)
		image.at<uint8>(pi / resolution.X, pi % resolution.X) = pixels[pi].R;

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): Done copying pixel data."), pixels.Num(), resolution.X, resolution.Y));

	return true;
}
*/

/*
void FLensSolverWorkerFindCorners::WriteMatToFile(cv::Mat image, FString outputPath)
{
	MatQueueWriter::Get().QueueMat(outputPath, image);
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): Queued texture: \'%s\" to be written to file."), *outputPath));
}
*/

void FLensSolverWorkerFindCorners::QueueCalibrationPointsWorkUnit(const FLensSolverCalibrationPointsWorkUnit & calibrationPointsWorkUnit)
{
	if (!queueFindCornerResultOutputDel->IsBound())
		return;
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queuing calibration points work unit."), *JobDataToString(calibrationPointsWorkUnit.baseParameters)));
	queueFindCornerResultOutputDel->Execute(calibrationPointsWorkUnit);
}

void FLensSolverWorkerFindCorners::QueueEmptyCalibrationPointsWorkUnit(const FBaseParameters & baseParameters, const FResizeParameters & resizeParameters)
{
	if (!queueFindCornerResultOutputDel->IsBound())
		return;

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queuing EMPTY calibration points work unit."), *JobDataToString(baseParameters)));

	FLensSolverCalibrationPointsWorkUnit calibrationPointsWorkUnit;
	calibrationPointsWorkUnit.baseParameters = baseParameters;
	calibrationPointsWorkUnit.resizeParameters = resizeParameters;
	queueFindCornerResultOutputDel->Execute(calibrationPointsWorkUnit);
}

OpenCVResizeParameters UE4ToWrapperResizeParameters(const FResizeParameters& resizeParameters)
{
	return OpenCVResizeParameters();
}
