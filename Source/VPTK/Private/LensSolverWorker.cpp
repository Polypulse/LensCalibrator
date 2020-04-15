#include "LensSolverWorker.h"

#include "Engine.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "RenderUtils.h"
#include "Engine/Texture2D.h"

#include "Queue.h"

FLensSolverWorker::FLensSolverWorker(
	IsClosingDel * inputIsClosingDel,
	GetWorkLoadDel * inputGetWorkLoadDel,
	QueueWorkUnitDel * inputQueueWorkUnitDel,
	OnSolvePointsDel inputOnSolvePointsDel,
	int inputWorkerID)
: 
	onSolvePointsDel(inputOnSolvePointsDel)
{
	inputQueueWorkUnitDel->BindRaw(this, &FLensSolverWorker::QueueWorkUnit);
	inputGetWorkLoadDel->BindRaw(this, &FLensSolverWorker::GetWorkLoad);
	inputIsClosingDel->BindRaw(this, &FLensSolverWorker::IsClosing);

	workUnitCount = 0;
	workerID = inputWorkerID;
	exited = false;
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

	static cv::Mat image;

	cv::Point2d principalPoint(0, 0);
	double fovX = 0, fovY = 0, focalLength = 0, aspectRatio = 0;
	FMatrix perspectiveMatrix = FMatrix::Identity;


	while (!exited)
	{
		if (workQueue.IsEmpty())
			continue;

		FLensSolverWorkUnit workUnit;
		workQueue.Dequeue(workUnit);

		UE_LOG(LogTemp, Log, TEXT("Worker (%d) with current workload: %d dequeued work unit."), workerID, workUnitCount);

		// threadLock.Lock();
		workUnitCount--;
		// threadLock.Unlock();

		// FString outputPath("D:\\Test.bmp");
		// FFileHelper::CreateBitmap(*outputPath, ExtendXWithMSAA, texture->GetSizeY(), Bitmap.GetData());
		// UE_LOG(LogTemp, Log, TEXT("Wrote test bitmap with: %d pixels to file."), Bitmap.Num());

		cv::Size imageSize(workUnit.width, workUnit.height);

		std::vector<std::vector<cv::Point2f>> corners(1);
		std::vector<std::vector<cv::Point3f>> objectPoints(1);

		cv::Size patternSize(workUnit.cornerCount.X, workUnit.cornerCount.Y);

		std::vector<cv::Mat> rvecs, tvecs;

		cv::Mat cameraMatrix = cv::Mat::eye(3, 3, cv::DataType<float>::type);
		cv::Mat distortionCoefficients = cv::Mat::zeros(8, 1, cv::DataType<float>::type);

		if (image.rows != workUnit.height || image.cols != workUnit.width)
		{
			UE_LOG(LogTemp, Log, TEXT("Reallocating OpenCV mat from size: (%d, %d) to: (%d, %d)."), image.cols, image.rows, workUnit.width, workUnit.height);
			image = cv::Mat(workUnit.height, workUnit.width, cv::DataType<uint8>::type);
		}

		for (int i = 0; i < workUnit.width * workUnit.height; i++)
			image.at<uint8>(i / workUnit.width, i % workUnit.width) = workUnit.pixels[i].R;

		// cv::imwrite("D:\\output.jpg", image);

		/*
		cv::Mat gray(width, height, CV_8U);
		cv::cvtColor(image, gray, CV_BGR2GRAY);
		*/

		bool patternFound = false;

		try
		{
			patternFound = cv::findChessboardCorners(image, patternSize, corners[0]);
		}

		catch (std::exception e)
		{
			UE_LOG(LogTemp, Log, TEXT("OpenCV exception occurred: %s"), e.what());
			QueueSolvedPointsError(workUnit.zoomLevel);
			continue;
		}

		if (!patternFound)
		{
			UE_LOG(LogTemp, Warning, TEXT("No pattern in view."));
			QueueSolvedPointsError(workUnit.zoomLevel);
			continue;
		}

		cv::TermCriteria cornerSubPixCriteria(
			cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS,
			50, 
			0.0001
		);

		cv::cornerSubPix(image, corners[0], cv::Size(5, 5), cv::Size(-1, -1), cornerSubPixCriteria);
		// cv::drawChessboardCorners(image, patternSize, corners, patternFound);

		// UE_LOG(LogTemp, Log, TEXT("Chessboard detected."));
		// objectPoints.resize(corners.size(), objectPoints[0]);

		for (int y = 0; y < workUnit.cornerCount.Y; y++)
			for (int x = 0; x < workUnit.cornerCount.X; x++)
				objectPoints[0].push_back(cv::Point3f(x * workUnit.squareSize, y * workUnit.squareSize, 0.0f));

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

		UE_LOG(LogTemp, Log, TEXT("Completed camera calibration with solve error: %f for zoom value: %f with results: (\n\tFov X: %f,\n\tFov Y: %f,\n\tFocal Length: %f,\n\tAspect Ratio: %f\n)"),
			error,
			workUnit.zoomLevel,
			fovX,
			fovY,
			focalLength,
			aspectRatio);

		/*
		TArray<uint8> visualizationData;
		int count = workUnit.width * workUnit.height * 4;
		visualizationData.SetNum(count);

		for (int i = 0; i < workUnit.width * workUnit.height * 4; i += 4)
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

		FSolvedPoints solvedPoints;

		solvedPoints.zoomLevel = workUnit.zoomLevel;
		solvedPoints.success = true;
		solvedPoints.width = workUnit.width;
		solvedPoints.height = workUnit.height;
		solvedPoints.fovX = fovX;
		solvedPoints.fovY = fovY;
		solvedPoints.focalLength = focalLength;
		solvedPoints.aspectRatio = aspectRatio;
		solvedPoints.perspectiveMatrix = perspectiveMatrix;

		solvedPoints.points = pointsCache;
		solvedPoints.visualizationData = workUnit.pixels;

		UE_LOG(LogTemp, Log, TEXT("Worker (%d) finished with work unit."), workerID);
		QueueSolvedPoints(solvedPoints);
	}

	exited = true;
}

void FLensSolverWorker::QueueSolvedPointsError(float zoomLevel)
{
	static TArray<FVector2D> emptyPoints;

	FSolvedPoints solvedPoints;
	solvedPoints.points = emptyPoints;
	solvedPoints.zoomLevel = zoomLevel;
	solvedPoints.success = false;

	if (!onSolvePointsDel.IsBound())
		return;

	onSolvePointsDel.Execute(solvedPoints);
}

void FLensSolverWorker::QueueSolvedPoints(FSolvedPoints solvedPoints)
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
