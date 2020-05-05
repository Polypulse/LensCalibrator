#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "LensSolverWorker.h"

class FLensSolverWorkerFindCorners : public FLensSolverWorker
{
public:
	DECLARE_DELEGATE_OneParam(QueueFindCornerResultOutputDel, TUniquePtr<FLensSolverCalibrateWorkUnit>)
	FLensSolverWorkerFindCorners(
		FLensSolverWorkerParameters inputParameters,
		QueueFindCornerResultOutputDel * inputQueueFindCornerResultOutputDel
	);

private:
	QueueFindCornerResultOutputDel* queueFindCornerResultOutputDel;
	void WriteMatToFile(cv::Mat image, FString folder, FString fileName);

protected:
	virtual void Tick() override;
	bool GetImageFromFile(const FString & absoluteFilePath, cv::Mat& image, FIntPoint & sourceResolution);
	bool GetImageFromArray(const TArray<FColor> & pixels, const FIntPoint resolution, cv::Mat& image);
};
