/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolverWorker.h"

#include "Engine.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "RenderUtils.h"
#include "Engine/Texture2D.h"
#include "JsonUtilities.h"
#include "LensSolverUtilities.h"

FLensSolverWorker::FLensSolverWorker(
	IsClosingDel * inputIsClosingDel,
	GetWorkLoadDel * inputGetWorkLoadDel,
	QueueWorkUnitDel * inputQueueWorkUnitDel,
	SignalLatchDel * inputSignalLatch,
	OnSolvePointsDel inputOnSolvePointsDel,
	int inputWorkerID)
: 
	onSolvePointsDel(inputOnSolvePointsDel)
{
	inputQueueWorkUnitDel->BindRaw(this, &FLensSolverWorker::QueueWorkUnit);
	inputGetWorkLoadDel->BindRaw(this, &FLensSolverWorker::GetWorkLoad);
	inputIsClosingDel->BindRaw(this, &FLensSolverWorker::Exit);
	inputSignalLatch->BindRaw(this, &FLensSolverWorker::Latch);

	workerID = inputWorkerID;

	workUnitCount = 0;
	flagToExit = false;

	calibrationVisualizationOutputPath = LensSolverUtilities::GenerateGenericOutputPath(FString::Printf(TEXT("CalibrationVisualizations/Worker-%d/"), workerID));
}

int FLensSolverWorker::GetWorkLoad () 
{ 
	int count = 0;
	threadLock.Lock();
	count = workUnitCount;
	threadLock.Unlock();

	return count; 
}

void FLensSolverWorker::QueueWorkUnit(FLensSolverWorkUnit workUnit)
{
	workQueue.Enqueue(workUnit);
	threadLock.Lock();
	workUnitCount++;
	threadLock.Unlock();
}

void FLensSolverWorker::Latch(const FLatchData inputLatchData)
{
	threadLock.Lock();
	latchQueue.Enqueue(inputLatchData);
	threadLock.Unlock();
}

void FLensSolverWorker::TransformVectorFromCVToUE4(FVector& v)
{
	float x, y, z;

	x = v.X;
	y = v.Y;
	z = v.Z;

	v.X = z;
	v.Y = x;
	v.Z = -y;
}

FMatrix FLensSolverWorker::GeneratePerspectiveMatrixFromFocalLength(cv::Size& imageSize, cv::Point2d principlePoint, float focalLength)
{
	FMatrix perspectiveMatrix;

	float min = 0.1f;
	float max = 10000.0f;

	float left = min * (-principlePoint.x) / focalLength;
	float right = min * (imageSize.width - principlePoint.x) / focalLength;
	float bottom = min * (principlePoint.y - imageSize.height) / focalLength;
	float top = min * (principlePoint.y) / focalLength;

	float a = -(right + left) / (right - left);
	float b = -(top + bottom) / (top - bottom);

	float c = min / (min - max);
	float d = -max * min / (min - max);

	float nrl = (2 * min) / (right - left);
	float ntb = (2 * min) / (top - bottom);

	perspectiveMatrix = FMatrix(
		FPlane(nrl,		0.0f,	0.0f,	0.0f),
		FPlane(0.0f,	ntb,	0.0f,	0.0f),
		FPlane(a,		b,		c,		1.0f),
		FPlane(0.0f,	0.0f,	d,		0.0f)
	);

	return perspectiveMatrix;
}

FTransform FLensSolverWorker::GenerateTransformFromRAndTVecs(std::vector<cv::Mat>& rvecs, std::vector<cv::Mat>& tvecs)
{
	FTransform outputTransform;
	FMatrix transformMatrix;

	cv::Mat rot;
	FVector forward, right, up, origin;
	FVector zero(0, 0, 0);

	cv::Rodrigues(rvecs[0], rot);

	forward		= FVector(rot.at<double>(0, 2), rot.at<double>(1, 2), rot.at<double>(2, 2));
	right		= FVector(rot.at<double>(0, 0), rot.at<double>(1, 0), rot.at<double>(2, 0));
	up			= FVector(rot.at<double>(0, 1), rot.at<double>(1, 1), rot.at<double>(2, 1));
	origin		= FVector(tvecs[0].at<double>(0, 0), tvecs[0].at<double>(1, 0), tvecs[0].at<double>(2, 0));

	TransformVectorFromCVToUE4(forward);
	TransformVectorFromCVToUE4(right);
	TransformVectorFromCVToUE4(up);
	TransformVectorFromCVToUE4(origin);

	up = -up;

	transformMatrix.SetAxes(
		&forward,
		&right,
		&up,
		&zero
	);

	transformMatrix = transformMatrix.GetTransposed();
	origin = transformMatrix.TransformPosition(origin);
	outputTransform.SetFromMatrix(transformMatrix);
	outputTransform.SetLocation(-origin);

	return outputTransform;
}

void FLensSolverWorker::DoWork()
{
	while (true)
	{
		while (workQueue.IsEmpty() && !ShouldExit())
			continue;

		while (latchQueue.IsEmpty() && !ShouldExit())
			continue;

		if (ShouldExit())
			break;

		FString workerMessage = FString::Printf(TEXT("Worker: (ID: %d): "), workerID);
		UE_LOG(LogTemp, Log, TEXT("%sLatched!"), *workerMessage);

		TArray<FLensSolverWorkUnit> workUnits;
		FLatchData latchData;

		{
			threadLock.Lock();
			latchQueue.Dequeue(latchData);

			if (latchData.imageCount == 0)
			{
				threadLock.Unlock();
				UE_LOG(LogTemp, Error, TEXT("%sNo work units in latched queue, idling..."), *workerMessage)
				continue;
			}

			workUnits.SetNum(latchData.imageCount);
			for (int i = 0; i < latchData.imageCount; i++)
				workQueue.Dequeue(workUnits[i]);

			workUnitCount -= latchData.imageCount;
			threadLock.Unlock();
		}


		UE_LOG(LogTemp, Log, TEXT("%sDequeued %d work units in latched queue."), *workerMessage, latchData.imageCount)

		std::vector<std::vector<cv::Point2f>> corners;
		std::vector<std::vector<cv::Point3f>> objectPoints;

		std::vector<cv::Mat> images;
		cv::Mat image;

		double fovX = 0, fovY = 0, focalLength = 0, aspectRatio = 0;
		FMatrix perspectiveMatrix = FMatrix::Identity;

		std::vector<cv::Mat> rvecs, tvecs;
		cv::Mat cameraMatrix = cv::Mat::eye(3, 3, cv::DataType<float>::type);
		cv::Mat distortionCoefficients = cv::Mat::zeros(5, 1, cv::DataType<float>::type);

		int sourcePixelWidth = latchData.sourceResolution.X;
		int sourcePixelHeight = latchData.sourceResolution.Y;
		int resizedPixelWidth = FMath::FloorToInt(latchData.sourceResolution.X * (latchData.resize ? latchData.resizePercentage : 1.0f));
		int resizedPixelHeight = FMath::FloorToInt(latchData.sourceResolution.Y * (latchData.resize ? latchData.resizePercentage : 1.0f));

		cv::Point2d principalPoint(sourcePixelWidth, sourcePixelHeight);
		cv::Size sourceImageSize(sourcePixelWidth, sourcePixelHeight);
		cv::Size resizedImageSize(resizedPixelWidth, resizedPixelHeight);

		float inverseResizeRatio = latchData.resize ? 1.0f / latchData.resizePercentage : 1.0f;

		if (image.rows != resizedPixelWidth || image.cols != resizedPixelHeight)
		{
			UE_LOG(LogTemp, Log, TEXT("%sAllocating image from size: (%d, %d) to: (%d, %d)."), *workerMessage, image.cols, image.rows, resizedPixelWidth, resizedPixelHeight);
			image = cv::Mat(resizedPixelHeight, resizedPixelWidth, cv::DataType<uint8>::type);
		}

		UE_LOG(LogTemp, Log, TEXT("%sResized pixel size: (%d, %d), source size: (%d, %d), resize ratio: %f."),
			*workerMessage,
			resizedPixelWidth,
			resizedPixelHeight,
			sourcePixelWidth,
			sourcePixelHeight,
			1.0f / inverseResizeRatio);

		float sensorHeight = (latchData.sensorDiagonalMM * sourcePixelHeight) / FMath::Sqrt(sourcePixelWidth * sourcePixelWidth + sourcePixelHeight * sourcePixelHeight);
		float sensorWidth = sensorHeight * (sourcePixelWidth / (float)sourcePixelHeight);

		UE_LOG(LogTemp, Log, TEXT("%sSensor size: (%f, %f) mm, diagonal: (%f) mm."), *workerMessage, sensorWidth, sensorHeight, latchData.sensorDiagonalMM);

		int flags = 0;
		flags |= latchData.workerParameters.useInitialIntrinsicValues		 ?	cv::CALIB_USE_INTRINSIC_GUESS : 0;
		flags |= latchData.workerParameters.keepPrincipalPixelPositionFixed	 ?	cv::CALIB_FIX_PRINCIPAL_POINT : 0;
		flags |= latchData.workerParameters.keepAspectRatioFixed			 ?	cv::CALIB_FIX_ASPECT_RATIO : 0;
		flags |= latchData.workerParameters.lensHasTangentalDistortion		 ?	cv::CALIB_ZERO_TANGENT_DIST : 0;
		flags |= latchData.workerParameters.fixRadialDistortionCoefficientK1 ?	cv::CALIB_FIX_K1 : 0;
		flags |= latchData.workerParameters.fixRadialDistortionCoefficientK2 ?	cv::CALIB_FIX_K2 : 0;
		flags |= latchData.workerParameters.fixRadialDistortionCoefficientK3 ?	cv::CALIB_FIX_K3 : 0;
		flags |= latchData.workerParameters.fixRadialDistortionCoefficientK4 ?	cv::CALIB_FIX_K4 : 0;
		flags |= latchData.workerParameters.fixRadialDistortionCoefficientK5 ?	cv::CALIB_FIX_K5 : 0;
		flags |= latchData.workerParameters.fixRadialDistortionCoefficientK6 ?	cv::CALIB_FIX_K6 : 0;

		if (flags & cv::CALIB_USE_INTRINSIC_GUESS)
		{
			cameraMatrix.at<float>(0, 2) = latchData.initialPrincipalPointPixelPosition.X;
			cameraMatrix.at<float>(1, 2) = latchData.initialPrincipalPointPixelPosition.Y;
			UE_LOG(LogTemp, Log, TEXT("%sSetting initial principal point to: (%f, %f)"), 
				*workerMessage, 
				latchData.initialPrincipalPointPixelPosition.X,
				latchData.initialPrincipalPointPixelPosition.Y);
		}

		else if (flags & cv::CALIB_FIX_ASPECT_RATIO)
		{
			cameraMatrix.at<float>(0, 0) = 1.0f / (sourcePixelWidth * 0.5f);
			cameraMatrix.at<float>(1, 1) = 1.0f / (sourcePixelHeight * 0.5f);
			UE_LOG(LogTemp, Log, TEXT("%sKeeping aspect ratio at: %f"), 
				*workerMessage, 
				(sourcePixelWidth / (float)sourcePixelHeight));
		}

		/*
		if (latchData.initialVerticalFieldOfView == 0.0f)
		{
			cameraMatrix.at<float>(0, 0) = (latchData.initalImageCenterPixel.X / pixelWidth) * sensorWidth;
			cameraMatrix.at<float>(1, 1) = (latchData.initalImageCenterPixel.Y / pixelHeight) * sensorHeight;

			if (!(flags & cv::CALIB_USE_INTRINSIC_GUESS))
				flags |= cv::CALIB_USE_INTRINSIC_GUESS;
		}
		*/

		cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001f);

		for (int i = 0; i < latchData.imageCount; i++)
		{
			if (ShouldExit())
				break;

			std::vector<cv::Point2f> imageCorners;
			std::vector<cv::Point3f> imageObjectPoints;

			cv::Size patternSize(latchData.cornerCount.X, latchData.cornerCount.Y);

			UE_LOG(LogTemp, Log, TEXT("%sCopying pixel data of pixel count: %d to OpenCV Mat of size: (%d, %d)."), *workerMessage, workUnits[i].pixels.Num(), resizedPixelWidth, resizedPixelHeight);
			int pixelCount = resizedPixelWidth * resizedPixelHeight;
			for (int pi = 0; pi < pixelCount; pi++)
				image.at<uint8>(pi / resizedPixelWidth, pi % resizedPixelWidth) = workUnits[i].pixels[pi].R;

			UE_LOG(LogTemp, Log, TEXT("%Done copying pixel data, beginning calibration."), *workerMessage, workUnits[i].pixels.Num(), resizedPixelWidth, resizedPixelHeight);

			bool patternFound = false;

			int findFlags = cv::CALIB_CB_NORMALIZE_IMAGE;
			findFlags |= cv::CALIB_CB_ADAPTIVE_THRESH;

			if (latchData.workerParameters.exhaustiveSearch)
				findFlags |= cv::CALIB_CB_EXHAUSTIVE;

			patternFound = cv::findChessboardCorners(image, patternSize, imageCorners, findFlags);

			if (!patternFound)
			{
				UE_LOG(LogTemp, Warning, TEXT("%sNo pattern found in image: %d, moving onto the next image."), *workerMessage, i);
				continue;
			}

			UE_LOG(LogTemp, Log, TEXT("%sFound pattern in image: %d"), *workerMessage, i);

			cv::TermCriteria cornerSubPixCriteria(
				cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS,
				50,
				0.0001
			);

			cv::cornerSubPix(image, imageCorners, cv::Size(5, 5), cv::Size(-1, -1), cornerSubPixCriteria);

			if (latchData.workerParameters.writeDebugTextureToFile)
			{
				cv::drawChessboardCorners(image, patternSize, imageCorners, patternFound);
				WriteMatToFile(image, latchData.workerParameters.debugTextureFolderPath, workUnits[i].unitName + "-debug", workerMessage);
			}

			for (int y = 0; y < latchData.cornerCount.Y; y++)
				for (int x = 0; x < latchData.cornerCount.X; x++)
					imageObjectPoints.push_back(cv::Point3f(x * latchData.squareSizeMM, y * latchData.squareSizeMM, 0.0f));

			for (int ci = 0; ci < imageCorners.size(); ci++)
			{
				imageCorners[ci].x = imageCorners[ci].x * inverseResizeRatio;
				imageCorners[ci].y = imageCorners[ci].y * inverseResizeRatio;
			}

			corners.push_back(imageCorners);
			objectPoints.push_back(imageObjectPoints);
		}

		if (ShouldExit())
			break;

		// cameraMatrix.at<float>(0, 0) *= resizeRatio.x;
		// cameraMatrix.at<float>(1, 1) *= resizeRatio.y;

		// cameraMatrix.at<float>(0, 2) *= resizeRatio.x;
		// cameraMatrix.at<float>(1, 2) *= resizeRatio.y;

		double error = cv::calibrateCamera(
			objectPoints,
			corners,
			sourceImageSize,
			cameraMatrix,
			distortionCoefficients,
			rvecs,
			tvecs,
			flags,
			termCriteria);

		/*
		UE_LOG(LogTemp, Log, TEXT("Camera matrix:\n(\n\t%f, %f, %f\n\t%f, %f, %f\n\t%f, %f, %f\n)"),
			cameraMatrix.at<float>(0, 0),
			cameraMatrix.at<float>(0, 1),
			cameraMatrix.at<float>(0, 2),
			cameraMatrix.at<float>(1, 0),
			cameraMatrix.at<float>(1, 1),
			cameraMatrix.at<float>(1, 2),
			cameraMatrix.at<float>(2, 0),
			cameraMatrix.at<float>(2, 1),
			cameraMatrix.at<float>(2, 2));

		cameraMatrix.at<float>(0, 0) *= resizeRatio.x;
		cameraMatrix.at<float>(1, 1) *= resizeRatio.y;

		cameraMatrix.at<float>(0, 2) *= resizeRatio.x;
		cameraMatrix.at<float>(1, 2) *= resizeRatio.y;

		UE_LOG(LogTemp, Log, TEXT("Camera matrix:\n(\n\t%f, %f, %f\n\t%f, %f, %f\n\t%f, %f, %f\n)"),
			cameraMatrix.at<float>(0, 0),
			cameraMatrix.at<float>(0, 1),
			cameraMatrix.at<float>(0, 2),
			cameraMatrix.at<float>(1, 0),
			cameraMatrix.at<float>(1, 1),
			cameraMatrix.at<float>(1, 2),
			cameraMatrix.at<float>(2, 0),
			cameraMatrix.at<float>(2, 1),
			cameraMatrix.at<float>(2, 2));
		*/

		cv::calibrationMatrixValues(cameraMatrix, sourceImageSize, sensorWidth, sensorHeight, fovX, fovY, focalLength, principalPoint, aspectRatio);
		perspectiveMatrix = GeneratePerspectiveMatrixFromFocalLength(sourceImageSize, principalPoint, focalLength);

		fovX *= 2.0f;
		fovY *= 2.0f;
		principalPoint.x *= 2.0f;
		principalPoint.y *= 2.0f;
		focalLength *= 2.0f;

		principalPoint.x = sourcePixelWidth * (principalPoint.x / sensorWidth);
		principalPoint.y = sourcePixelHeight * (principalPoint.y / sensorHeight);

		FString format = FString("%sCompleted camera calibration at zoom level: %f "
			"with solve error: %f "
			"with results: ("
			"\n\tField of View in degrees: (%f, %f)"
			"\n\tSensor width in MM: %f,"
			"\n\tSensor height in MM: %f,"
			"\n\tFocal Length in MM: %f,"
			"\n\tPrincipal Point Pixel: (%f, %f),"
			"\n\tAspect Ratio: %f\n)");

		FString msg = FString::Printf(*format,
			*workerMessage,
			latchData.zoomLevel,
			error,
			fovX,
			fovY,
			sensorWidth,
			sensorHeight,
			focalLength,
			principalPoint.x, principalPoint.y,
			aspectRatio);

		UE_LOG(LogTemp, Log, TEXT("%s"), *msg);

		if (ShouldExit())
			break;

		/*
		TArray<uint8> visualizationData;
		int count = width * workUnits[i].height * 4;
		visualizationData.SetNum(count);

		for (int i = 0; i < width * workUnit.height * 4; i += 4)
		{
			uint8 value = image.at<uint8>((i / 4) / workUnit.width, (i / 4) % workUnit.width);

			visualizationData[i] = value;
			visualizationData[i + 1] = value;
			visualizationData[i + 2] = value;
			visualizationData[i + 3] = value;
		}
		*/

		TArray<FVector2D> pointsCache;
		if (pointsCache.Num() != corners[0].size())
			pointsCache.SetNum(corners[0].size());

		for (int i = 0; i < pointsCache.Num(); i++)
			pointsCache[i] = FVector2D(corners[0][i].x, corners[0][i].y);

		FCalibrationResult solvedPoints;

		solvedPoints.jobInfo = latchData.jobInfo;
		solvedPoints.zoomLevel = latchData.zoomLevel;
		solvedPoints.success = true;
		solvedPoints.fovX = fovX;
		solvedPoints.fovY = fovY;
		solvedPoints.focalLengthMM = focalLength;
		solvedPoints.aspectRatio = aspectRatio;
		solvedPoints.sensorSizeMM = FVector2D(sensorWidth, sensorHeight);
		solvedPoints.principalPixelPoint = FVector2D(principalPoint.x, principalPoint.y);
		solvedPoints.resolution = latchData.sourceResolution;
		solvedPoints.perspectiveMatrix = perspectiveMatrix;

		if (latchData.workerParameters.writeCalibrationResultsToFile)
			WriteSolvedPointsToJSONFile(solvedPoints, latchData.workerParameters.calibrationResultsFolderPath, "result", workerMessage);

		UE_LOG(LogTemp, Log, TEXT("%sFinished with work unit."), *workerMessage);
		QueueSolvedPoints(solvedPoints);

		for (int i = 0; i < images.size(); i++)
		{
			images[i].release();
			workUnits[i].pixels.Empty();
		}
		cameraMatrix.release();
		distortionCoefficients.release();
		bool emptied = corners.empty();
		emptied = objectPoints.empty();
		emptied = rvecs.empty();
		emptied = tvecs.empty();
	}

	onSolvePointsDel.Unbind();
	workQueue.Empty();
	flagToExit = true;
}

void FLensSolverWorker::QueueSolvedPointsError(FJobInfo jobInfo, float zoomLevel)
{
	TArray<FVector2D> emptyPoints;

	FCalibrationResult solvedPoints;
	solvedPoints.jobInfo = jobInfo;
	// solvedPoints.points = emptyPoints;
	solvedPoints.zoomLevel = zoomLevel;
	solvedPoints.success = false;

	if (!onSolvePointsDel.IsBound())
		return;

	onSolvePointsDel.Execute(solvedPoints);
}

void FLensSolverWorker::QueueSolvedPoints(FCalibrationResult solvedPoints)
{
	if (!onSolvePointsDel.IsBound())
		return;

	onSolvePointsDel.Execute(solvedPoints);
}

bool FLensSolverWorker::Exit()
{
	threadLock.Lock();
	flagToExit = true;
	threadLock.Unlock();
	return true;
}

bool FLensSolverWorker::ShouldExit()
{
	bool shouldExit = false;
	threadLock.Lock();
	shouldExit = flagToExit;
	threadLock.Unlock();
	return shouldExit;
}

void FLensSolverWorker::WriteMatToFile(cv::Mat image, FString folder, FString fileName, const FString & workerMessage)
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

void FLensSolverWorker::WriteSolvedPointsToJSONFile(const FCalibrationResult& solvePoints, FString folder, FString fileName, const FString workerMessage)
{
	if (!LensSolverUtilities::ValidateFolder(folder, calibrationVisualizationOutputPath, workerMessage))
		return;

	FString outputPath = LensSolverUtilities::GenerateIndexedFilePath(folder, fileName, "json");

	TSharedPtr<FJsonObject> obj = MakeShareable(new FJsonObject);
	TSharedPtr<FJsonObject> result = MakeShareable(new FJsonObject);
	result->SetStringField("jobid", solvePoints.jobInfo.jobID);
	result->SetNumberField("zoomlevel", solvePoints.zoomLevel);
	result->SetNumberField("width", solvePoints.resolution.X);
	result->SetNumberField("height", solvePoints.resolution.Y);
	result->SetNumberField("fovx", solvePoints.fovX);
	result->SetNumberField("fovy", solvePoints.fovY);
	result->SetNumberField("focallength", solvePoints.focalLengthMM);
	result->SetNumberField("aspectratio", solvePoints.aspectRatio);

	TSharedPtr<FJsonObject> sensorSizeObj = MakeShareable(new FJsonObject);
	sensorSizeObj->SetNumberField("X", solvePoints.sensorSizeMM.X);
	sensorSizeObj->SetNumberField("y", solvePoints.sensorSizeMM.Y);
	result->SetObjectField("sensorsizemm", sensorSizeObj);

	TSharedPtr<FJsonObject> principalObj = MakeShareable(new FJsonObject);
	principalObj->SetNumberField("X", solvePoints.principalPixelPoint.X);
	principalObj->SetNumberField("y", solvePoints.principalPixelPoint.Y);
	result->SetObjectField("principalpixelpoint", principalObj);

	TSharedPtr<FJsonObject> resolutionObj = MakeShareable(new FJsonObject);
	resolutionObj->SetNumberField("X", solvePoints.resolution.X);
	resolutionObj->SetNumberField("y", solvePoints.resolution.Y);
	result->SetObjectField("resolution", resolutionObj);

	TArray<TSharedPtr<FJsonValue>> matVals;

	for (int i = 0; i < 16; i++)
			matVals.Add(MakeShareable(new FJsonValueNumber(solvePoints.perspectiveMatrix.M[i / 4][i % 4])));

	result->SetArrayField("perspectivematrix", matVals);

	TArray<TSharedPtr<FJsonValue>> distortionCoefficients;
	for (int i = 0; i < solvePoints.distortionCoefficients.Num(); i++)
		distortionCoefficients.Add(MakeShareable(new FJsonValueNumber(solvePoints.distortionCoefficients[i])));

	result->SetArrayField("distortioncoefficients", distortionCoefficients);
	obj->SetObjectField("result", result);

	FString outputJson(TEXT("{}"));
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&outputJson);
	FJsonSerializer::Serialize(obj.ToSharedRef(), writer);

	if (!FFileHelper::SaveStringToFile(outputJson, *outputPath))
	{
		UE_LOG(LogTemp, Error, TEXT("%sUnable to write calibration result to path: \"%s\", check your permissions."), *workerMessage, *outputPath);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("%sCalibration result written to file at path: \"%s\"."), *workerMessage, *outputPath);
}
