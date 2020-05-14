#include "MatQueueWriter.h"
#include "LensSolverUtilities.h"

void MatQueueWriter::QueueMat(FString outputPath, cv::Mat inputMat)
{
	MatQueueContainer container;
	container.outputPath = outputPath;
	container.mat = inputMat;

	matQueue.Enqueue(container);
}

void MatQueueWriter::Poll()
{
	static FString defaultMatFolder = LensSolverUtilities::GenerateGenericOutputPath("DebugImages");
	static FString defaultFileName = "DebugImage";
	static FString defaultExtension = "jpg";

	bool isQueued = matQueue.IsEmpty() == false;
	while (isQueued)
	{
		MatQueueContainer container;
		matQueue.Dequeue(container);

		FString filePath = container.outputPath;
		if (!LensSolverUtilities::ValidateFilePath(filePath, defaultMatFolder, defaultFileName, defaultExtension))
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to write image to file: \"%s\", cannot validate path."), *filePath);
			return;
		}

		if (container.mat.empty())
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to write image to file: \"%s\", image is empty."), *filePath);
			return;
		}

		if (!cv::imwrite(TCHAR_TO_UTF8(*filePath), container.mat))
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to write image to file: \"%s\"."), *filePath);
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("Wrote image to file: \"%s\"."), *filePath);

		isQueued = matQueue.IsEmpty() == false;
	}
}
