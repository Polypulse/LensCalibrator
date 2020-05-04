#include "LensSolverWorkerFindCorners.h"

void FLensSolverWorkerFindCorners::Tick()
{
	FString workerMessage = FString::Printf(TEXT("Worker: (ID: %d): "), GetWorkerID());

	while (!ShouldExit())
	{
		while (!WorkUnitInQueue() && !ShouldExit())
			continue;

		if (ShouldExit())
			break;

		cv::Mat image;
		if (!GetImage(image))
		{
			QueueLog(FString::Printf(TEXT("%sPrepared image of size: (%d, %d!"), *workerMessage, image.cols, image.rows));
			continue;
		}

		// QueueLog(FString::Printf(TEXT("%sLatched!"), *workerMessage));

		/*
		FLensSolverWoUni workUnits;
		FLatchData latchData;

		{
			Lock();
			latchQueue.Dequeue(latchData);

			if (latchData.imageCount == 0)
			{
				Unlock();
				UE_LOG(LogTemp, Error, TEXT("%sNo work units in latched queue, idling..."), *workerMessage)
				continue;
			}

			workUnits.SetNum(latchData.imageCount);
			for (int i = 0; i < latchData.imageCount; i++)
				workQueue.Dequeue(workUnits[i]);

			workUnitCount -= latchData.imageCount;
			Unlock();
		}
		*/

		// UE_LOG(LogTemp, Log, TEXT("%sDequeued %d work units in latched queue."), *workerMessage, latchData.imageCount)

		int sourcePixelWidth = latchData.sourceResolution.X;
		int sourcePixelHeight = latchData.sourceResolution.Y;

		int resizedPixelWidth = FMath::FloorToInt(latchData.sourceResolution.X * (latchData.resize ? latchData.resizePercentage : 1.0f));
		int resizedPixelHeight = FMath::FloorToInt(latchData.sourceResolution.Y * (latchData.resize ? latchData.resizePercentage : 1.0f));

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

		cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001f);

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

		if (ShouldExit())
			break;
	}
}

void FLensSolverWorkerFindCorners::QueueWorkUnit(TUniquePtr<FLensSolverWorkUnit> workUnit)
{
}
