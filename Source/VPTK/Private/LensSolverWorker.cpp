#include "LensSolverWorker.h"

#pragma push_macro("check")
#undef check
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/types_c.h"
#pragma pop_macro("check")

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

void FLensSolverWorker::QueueWorkUnit(TArray<FColor> inputPixels, int width, int height, float inputNormalizedZoomLevel)
{
	FLensSolverWorkUnit workerUnit;
	workerUnit.width = width;
	workerUnit.height = height;
	workerUnit.zoomLevel = inputNormalizedZoomLevel;
	workerUnit.pixels = inputPixels;

	workQueue.Enqueue(workerUnit);

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

		static cv::Mat image(workUnit.height, workUnit.width, cv::DataType<unsigned char>::type);
		if (image.cols != workUnit.width || image.rows != workUnit.height)
			image = cv::Mat(workUnit.height, workUnit.width, cv::DataType<unsigned char>::type);

		for (int i = 0; i < workUnit.width * workUnit.height; i++)
			image.at<unsigned char>(cv::Point((i % workUnit.width), workUnit.height - (i / workUnit.width))) = workUnit.pixels[i].R;

		cv::imwrite("D:\\output.jpg", image);

		/*
		cv::Mat gray(width, height, CV_8U);
		cv::cvtColor(image, gray, CV_BGR2GRAY);
		*/

		std::vector<cv::Point2f> corners;
		cv::Size patternSize(9, 6);
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

		FSolvedPoints solvedPoints;
		solvedPoints.points = pointsCache;
		solvedPoints.zoomLevel = workUnit.zoomLevel;
		solvedPoints.success = true;

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
