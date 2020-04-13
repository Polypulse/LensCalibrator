#include "LensSolverWorker.h"

#pragma push_macro("check")
#undef check
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#pragma pop_macro("check")

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
	OnSolvePointsDel inputOnSolvePointsDel)
: 
	onSolvePointsDel(inputOnSolvePointsDel)
{
	inputQueueWorkUnitDel->BindRaw(this, &FLensSolverWorker::QueueWorkUnit);
	inputGetWorkLoadDel->BindRaw(this, &FLensSolverWorker::GetWorkLoad);
	inputIsClosingDel->BindRaw(this, &FLensSolverWorker::IsClosing);

	workUnitCount = 0;
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

void FLensSolverWorker::DoWork()
{
	while (!exited)
	{
		if (workQueue.IsEmpty())
			continue;

		FLensSolverWorkUnit workUnit;
		workQueue.Dequeue(workUnit);

		// threadLock.Lock();
		workUnitCount--;
		// threadLock.Unlock();

		// FString outputPath("D:\\Test.bmp");
		// FFileHelper::CreateBitmap(*outputPath, ExtendXWithMSAA, texture->GetSizeY(), Bitmap.GetData());
		// UE_LOG(LogTemp, Log, TEXT("Wrote test bitmap with: %d pixels to file."), Bitmap.Num());

		cv::Mat image(workUnit.height, workUnit.width, cv::DataType<uint8>::type);
		if (image.rows != workUnit.height || image.cols != workUnit.width)
			image = cv::Mat(workUnit.height, workUnit.width, cv::DataType<uint8>::type);

		for (int i = 0; i < workUnit.width * workUnit.height; i++)
			image.at<uint8>(i / workUnit.width, i % workUnit.width) = workUnit.pixels[i].R;

		// cv::imwrite("D:\\output.jpg", image);

		/*
		cv::Mat gray(width, height, CV_8U);
		cv::cvtColor(image, gray, CV_BGR2GRAY);
		*/

		std::vector<cv::Point2f> corners;
		cv::Size patternSize(workUnit.cornerCount.X, workUnit.cornerCount.Y);
		bool patternFound = false;

		try
		{
			patternFound = cv::findChessboardCorners(image, patternSize, corners);
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

		cv::cornerSubPix(image, corners, cv::Size(5, 5), cv::Size(-1, -1), cornerSubPixCriteria);
		cv::drawChessboardCorners(image, patternSize, corners, patternFound);

		// UE_LOG(LogTemp, Log, TEXT("Chessboard detected."));
		static TArray<FVector2D> pointsCache;
		if (pointsCache.Num() != corners.size())
			pointsCache.SetNum(corners.size());

		// FString msg = FString::Printf(TEXT("Found %d points:"), pointsCache.Num());
		for (int i = 0; i < pointsCache.Num(); i++)
		{
			// msg = FString::Printf(TEXT("%s\n(%f, %f),\n"), *msg, corners[i].x, corners[i].y);
			pointsCache[i] = FVector2D(corners[i].x, corners[i].y);
		}

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

		FSolvedPoints solvedPoints;
		solvedPoints.points = pointsCache;
		solvedPoints.zoomLevel = workUnit.zoomLevel;
		solvedPoints.success = true;

		/*
		solvedPoints.width = workUnit.width;
		solvedPoints.height = workUnit.height;
		solvedPoints.visualizationData = visualizationData;
		*/

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
