/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */

#include "LensSolverWorker.h"

#include "Engine.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "RenderUtils.h"
#include "Engine/Texture2D.h"
#include "JsonUtilities.h"

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
	inputIsClosingDel->BindRaw(this, &FLensSolverWorker::IsClosing);
	inputSignalLatch->BindRaw(this, &FLensSolverWorker::Latch);

	latchedWorkUnitCount = 0;
	workUnitCount = 0;

	workerID = inputWorkerID;

	latched = false;
	exited = false;

	calibrationVisualizationOutputPath = FPaths::ConvertRelativePathToFull(FPaths::GameDevelopersDir() + FString::Printf(TEXT("CalibrationVisualizations/Worker-%d/"), workerID));
}

int FLensSolverWorker::GetWorkLoad () 
{ 
	int count = 0;
	// threadLock.Lock();
	count = workUnitCount;
	// threadLock.Unlock();

	return count; 
}

void FLensSolverWorker::QueueWorkUnit(FLensSolverWorkUnit workUnit)
{
	workQueue.Enqueue(workUnit);
	// threadLock.Lock();
	workUnitCount++;
	// threadLock.Unlock();
}

void FLensSolverWorker::Latch(const FLatchData inputLatchData)
{
	latchedWorkUnitCount = workUnitCount;
	latchData = inputLatchData;
	latched = true;
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
	while (!exited)
	{
		while (!latched) 
			continue;

		latched = false;

		if (latchedWorkUnitCount == 0)
			continue;

		TArray<FLensSolverWorkUnit> workUnits;
		workUnits.SetNum(latchedWorkUnitCount);
		for (int i = 0; i < latchedWorkUnitCount; i++)
		{
			FLensSolverWorkUnit workUnit;
			workQueue.Dequeue(workUnit);
			workUnits[i] = workUnit;
		}

		workUnitCount -= latchedWorkUnitCount;
		FString workerMessage = FString::Printf(TEXT("Worker: (ID: %d): "), workerID);

		std::vector<std::vector<cv::Point2f>> corners(latchedWorkUnitCount);
		std::vector<std::vector<cv::Point3f>> objectPoints(latchedWorkUnitCount);

		std::vector<cv::Mat> images;
		cv::Size imageSize(latchData.resolution.X, latchData.resolution.Y);

		cv::Point2d principalPoint(0, 0);
		double fovX = 0, fovY = 0, focalLength = 0, aspectRatio = 0;
		FMatrix perspectiveMatrix = FMatrix::Identity;

		std::vector<cv::Mat> rvecs, tvecs;
		cv::Mat cameraMatrix = cv::Mat::eye(3, 3, cv::DataType<float>::type);
		cv::Mat distortionCoefficients = cv::Mat::zeros(8, 1, cv::DataType<float>::type);

		int width = latchData.resolution.X;
		int height = latchData.resolution.Y;

		int
			flags = cv::CALIB_FIX_ASPECT_RATIO;
		// flags |= cv::CALIB_USE_INTRINSIC_GUESS;
		flags |= cv::CALIB_FIX_PRINCIPAL_POINT;
		flags |= cv::CALIB_ZERO_TANGENT_DIST;
		// flags |= cv::CALIB_FIX_K1;
		// flags |= cv::CALIB_FIX_K2;
		// flags |= cv::CALIB_FIX_K3;
		flags |= cv::CALIB_FIX_K4;
		flags |= cv::CALIB_FIX_K5;

		cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001f);

		for (int i = 0; i < latchedWorkUnitCount; i++)
		{
			FLensSolverWorkUnit workUnit = workUnits[i];

			UE_LOG(LogTemp, Log, TEXT("%sDequeued work unit with queued workload: %d"), *workerMessage, latchedWorkUnitCount);

			cv::Size patternSize(workUnit.cornerCount.X, workUnit.cornerCount.Y);
			cv::Mat image;

			if (image.rows != width || image.cols != height)
			{
				UE_LOG(LogTemp, Log, TEXT("%Allocating image from size: (%d, %d) to: (%d, %d)."), *workerMessage, image.cols, image.rows, width, height);
				image = cv::Mat(width, width, cv::DataType<uint8>::type);
			}

			UE_LOG(LogTemp, Log, TEXT("%sCopying pixel data of pixel count: %d to OpenCV Mat of size: (%d, %d)."), *workerMessage, workUnit.pixels.Num(), width, height);
			int pixelCount = width * height;
			for (int pi = 0; pi < pixelCount; pi++)
				image.at<uint8>(pi / width, pi % width) = workUnit.pixels[(pixelCount - 1) - pi].R;
			UE_LOG(LogTemp, Log, TEXT("%Done copying pixel data, beginning calibration."), *workerMessage, workUnit.pixels.Num(), width, height);

			bool patternFound = false;

			int findFlags = cv::CALIB_CB_NORMALIZE_IMAGE;
			findFlags |= cv::CALIB_CB_ADAPTIVE_THRESH;

			if (latchData.workerParameters.exhaustiveSearch)
				findFlags |= cv::CALIB_CB_EXHAUSTIVE;

			patternFound = cv::findChessboardCorners(image, patternSize, corners[i], findFlags);

			if (!patternFound)
			{
				UE_LOG(LogTemp, Warning, TEXT("%sNo pattern in view."), *workerMessage);
				QueueSolvedPointsError(latchData.jobInfo, workUnit.zoomLevel);
				continue;
			}

			cv::TermCriteria cornerSubPixCriteria(
				cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS,
				50,
				0.0001
			);

			cv::cornerSubPix(image, corners[i], cv::Size(5, 5), cv::Size(-1, -1), cornerSubPixCriteria);

			// cv::drawChessboardCorners(image, patternSize, corners, patternFound);

			// UE_LOG(LogTemp, Log, TEXT("Chessboard detected."));
			// objectPoints.resize(corners.size(), objectPoints[0]);

			for (int y = 0; y < workUnit.cornerCount.Y; y++)
				for (int x = 0; x < workUnit.cornerCount.X; x++)
					objectPoints[i].push_back(cv::Point3f(x * workUnit.squareSize, y * workUnit.squareSize, 0.0f));

			cv::drawChessboardCorners(image, patternSize, corners[0], patternFound);
			if (latchData.workerParameters.writeDebugTextureToFile)
				WriteMatToFile(image, latchData.workerParameters.debugTextureFolderPath, workUnit.unitName + "-debug", workerMessage);
		}

		double error = cv::calibrateCamera(
			objectPoints,
			corners,
			imageSize,
			cameraMatrix,
			distortionCoefficients,
			rvecs,
			tvecs,
			flags,
			termCriteria);

		cv::calibrationMatrixValues(cameraMatrix, imageSize, (double)imageSize.width, (double)imageSize.height, fovX, fovY, focalLength, principalPoint, aspectRatio);
		perspectiveMatrix = GeneratePerspectiveMatrixFromFocalLength(imageSize, principalPoint, focalLength);

		UE_LOG(LogTemp, Log, TEXT("%sCompleted camera calibration with solve error: %f with results: (\n\tFov X: %f,\n\tFov Y: %f,\n\tFocal Length: %f,\n\tAspect Ratio: %f\n)"),
			*workerMessage,
			error,
			fovX,
			fovY,
			focalLength,
			aspectRatio);

		/*
		TArray<uint8> visualizationData;
		int count = width * workUnit.height * 4;
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
		solvedPoints.resolution = latchData.resolution;
		solvedPoints.fovX = fovX;
		solvedPoints.fovY = fovY;
		solvedPoints.focalLength = focalLength;
		solvedPoints.aspectRatio = latchData.resolution.X / (float)latchData.resolution.Y;
		solvedPoints.perspectiveMatrix = perspectiveMatrix;
		solvedPoints.points = pointsCache;

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
	exited = true;
}

void FLensSolverWorker::QueueSolvedPointsError(FJobInfo jobInfo, float zoomLevel)
{
	TArray<FVector2D> emptyPoints;

	FCalibrationResult solvedPoints;
	solvedPoints.jobInfo = jobInfo;
	solvedPoints.points = emptyPoints;
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

bool FLensSolverWorker::IsClosing()
{
	exited = true;
	return true;
}

FString FLensSolverWorker::GenerateIndexedFilePath(const FString& folder, const FString& fileName, const FString & extension)
{
	FString partialOutputPath = folder + fileName;

	int index = 0;
	while (FPaths::FileExists(FString::Printf(TEXT("%s-%d.%s"), *partialOutputPath, index, *extension)))
		index++;
	return FString::Printf(TEXT("%s-%d.%s"), *partialOutputPath, index, *extension);
}

bool FLensSolverWorker::ValidateFolder(FString& folder, const FString & workerMessage)
{
	if (folder.IsEmpty())
		folder = calibrationVisualizationOutputPath;

	else
	{
		if (!FPaths::ValidatePath(folder))
		{
			UE_LOG(LogTemp, Error, TEXT("%sThe path: \"%s\" is not a valid."), *workerMessage, *folder);
			return false;
		}

		if (FPaths::FileExists(folder))
		{
			UE_LOG(LogTemp, Error, TEXT("%sThe path: \"%s\" is to a file, not a directory."), *workerMessage, *folder);
			return false;
		}
	}

	if (!FPaths::DirectoryExists(folder))
	{
		FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*folder);
		UE_LOG(LogTemp, Log, TEXT("%sCreated directory at path: \"%s\"."), *workerMessage, *folder);
	}

	return true;
}

void FLensSolverWorker::WriteMatToFile(cv::Mat image, FString folder, FString fileName, const FString & workerMessage)
{
	if (!ValidateFolder(folder, workerMessage))
		return;

	FString outputPath = GenerateIndexedFilePath(folder, fileName, "jpg");
	
	if (!cv::imwrite(TCHAR_TO_UTF8(*outputPath), image))
	{
		UE_LOG(LogTemp, Error, TEXT("%sUnable to write debug texture result to path: \"%s\", check your permissions."), *workerMessage, *outputPath);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("%sDebug texture written to file at path: \"%s\"."), *workerMessage, *outputPath);
}

void FLensSolverWorker::WriteSolvedPointsToJSONFile(const FCalibrationResult& solvePoints, FString folder, FString fileName, const FString workerMessage)
{
	if (!ValidateFolder(folder, workerMessage))
		return;

	FString outputPath = GenerateIndexedFilePath(folder, fileName, "json");

	TSharedPtr<FJsonObject> obj = MakeShareable(new FJsonObject);
	TSharedPtr<FJsonObject> result = MakeShareable(new FJsonObject);
	result->SetStringField("jobid", solvePoints.jobInfo.jobID);
	result->SetNumberField("zoomlevel", solvePoints.zoomLevel);
	result->SetNumberField("width", solvePoints.resolution.X);
	result->SetNumberField("height", solvePoints.resolution.Y);
	result->SetNumberField("fovx", solvePoints.fovX);
	result->SetNumberField("fovy", solvePoints.fovY);
	result->SetNumberField("focallength", solvePoints.focalLength);
	result->SetNumberField("aspectratio", solvePoints.aspectRatio);

	TArray<TSharedPtr<FJsonValue>> matVals;

	for (int i = 0; i < 16; i++)
			matVals.Add(MakeShareable(new FJsonValueNumber(solvePoints.perspectiveMatrix.M[i / 4][i % 4])));

	result->SetArrayField("perspectivematrix", matVals);

	TArray<TSharedPtr<FJsonValue>> points;
	for (int i = 0; i < solvePoints.points.Num(); i++)
	{
		points.Add(MakeShareable(new FJsonValueNumber(solvePoints.points[i].X)));
		points.Add(MakeShareable(new FJsonValueNumber(solvePoints.points[i].Y)));
	}

	result->SetArrayField("points", points);
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
