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
	mutable int workUnitCount;

	TQueue<TUniquePtr<FLensSolverWorkUnit>> workQueue;
	QueueFindCornerResultOutputDel* queueFindCornerResultOutputDel;
	void WriteMatToFile(cv::Mat image, FString folder, FString fileName);
	void DequeueWorkUnit(TUniquePtr<FLensSolverWorkUnit>& workUnit);

protected:
	virtual void Tick() override;
	bool GetImageFromFile(const FString & absoluteFilePath, cv::Mat& image, FIntPoint & sourceResolution);
	bool GetImageFromArray(const TArray<FColor> & pixels, const FIntPoint resolution, cv::Mat& image);

	virtual int GetWorkLoad() override;
	virtual void QueueWorkUnit(TUniquePtr<FLensSolverWorkUnit> workUnit) override;
};
