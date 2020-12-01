/* Licensed under Apache 2.0 license, see License.txt at the root of this project. */


#include "LensSolverWorkerCalibrate.h"
#include "JsonUtilities.h"
#include "GenericPlatform/GenericPlatformProcess.h"

#include "WorkerRegistry.h"

FLensSolverWorkerCalibrate::FLensSolverWorkerCalibrate(
	FLensSolverWorkerParameters & inputParameters,
	QueueCalibrateWorkUnitInputDel* inputQueueCalibrateWorkUnitDel,
	QueueLatchInputDel* inputSignalLatch,
	QueueCalibrationResultOutputDel* inputOnSolvePointsDel) : 
	FLensSolverWorker(inputParameters), /* Call base class constructor and pass generic input parameters. */
	onSolvePointsDel(inputOnSolvePointsDel) /* Initialize class member delegate with input. */
{
	/* Bind internal methods to delegates so they can be called from ULensSolverWorkDistributor. */
	inputQueueCalibrateWorkUnitDel->BindRaw(this, &FLensSolverWorkerCalibrate::QueueWorkUnit);
	inputSignalLatch->BindRaw(this, &FLensSolverWorkerCalibrate::QueueLatch);

	workUnitCount = 0;

	/* Register that this worker has been initialized. */
	WorkerRegistry::Get().CountCalibrateWorker();
}

FMatrix FLensSolverWorkerCalibrate::GeneratePerspectiveMatrixFromFocalLength(const FIntPoint& imageSize, const FVector2D& principlePoint, const float focalLength)
{
	FMatrix perspectiveMatrix;

	float min = 0.1f;
	float max = 10000.0f;

	float left = min * (-principlePoint.X) / focalLength;
	float right = min * (imageSize.X - principlePoint.X) / focalLength;
	float bottom = min * (principlePoint.Y - imageSize.Y) / focalLength;
	float top = min * (principlePoint.Y) / focalLength;

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

/* Overridden method from ULensSolverWorker, only gets called within the worker thread when there is work queued. */
void FLensSolverWorkerCalibrate::Tick()
{
	/* Work is queued in batches since parameters can change between those batches, therefore if this latch is set
	it means that all the necessary work has been queued and we should start. */
	if (!LatchInQueue())
		return;

	/* Don't do anything if the worker thread is supposed to exit, see the base class ULensSolverWorker. */
	if (ShouldExit())
		return;

	/* Dequeue the latch which also contains parameters for this specific task. */
	FCalibrateLatch latchData;
	DequeueLatch(latchData);

	/* Found calibration patterns is put in this float array, this array has x & y coordinates packed into it via: x,y,x,y,x,y,x,y. */
	TArray<float> corners; 

	/* The number of corners along the width and height that are expected to be found in the calibration pattern.*/
	int cornerCountX, cornerCountY;

	/* The size of a single square in the calibration pattern. */
	float chessboardSquareSizeMM;

	/* The number images in the work units. */
	int imageCount;

	if (!DequeueAllWorkUnits(
		latchData.baseParameters.calibrationID, 
		corners, cornerCountX, 
		cornerCountY, 
		chessboardSquareSizeMM,
		imageCount))
		return;

	if (corners.Num() == 0)
	{
		QueueLog("No calibration corners or object points to use in calibration process.");
		QueueCalibrationResultError(latchData.baseParameters);
		return;
	}

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): Done dequeing work units, preparing calibration using %d sets of points."), corners.Num()));

	FCalibrateLensParameters parameters; 
	parameters.sensorDiagonalSizeMM							= latchData.calibrationParameters.sensorDiagonalSizeMM;
	parameters.initialPrincipalPointNativePixelPositionX	= latchData.calibrationParameters.initialPrincipalPointNativePixelPosition.X;
	parameters.initialPrincipalPointNativePixelPositionY	= latchData.calibrationParameters.initialPrincipalPointNativePixelPosition.Y;
	parameters.useInitialIntrinsicValues					= latchData.calibrationParameters.useInitialIntrinsicValues;
	parameters.keepPrincipalPixelPositionFixed				= latchData.calibrationParameters.keepPrincipalPixelPositionFixed;
	parameters.keepAspectRatioFixed							= latchData.calibrationParameters.keepAspectRatioFixed;
	parameters.lensHasTangentalDistortion					= latchData.calibrationParameters.lensHasTangentalDistortion;
	parameters.fixRadialDistortionCoefficientK1				= latchData.calibrationParameters.fixRadialDistortionCoefficientK1;
	parameters.fixRadialDistortionCoefficientK2				= latchData.calibrationParameters.fixRadialDistortionCoefficientK2;
	parameters.fixRadialDistortionCoefficientK3				= latchData.calibrationParameters.fixRadialDistortionCoefficientK3;
	parameters.fixRadialDistortionCoefficientK4				= latchData.calibrationParameters.fixRadialDistortionCoefficientK4;
	parameters.fixRadialDistortionCoefficientK5				= latchData.calibrationParameters.fixRadialDistortionCoefficientK5;
	parameters.fixRadialDistortionCoefficientK6				= latchData.calibrationParameters.fixRadialDistortionCoefficientK6;
	parameters.useRationalModel								= latchData.calibrationParameters.useRationalModel;

	FCalibrateLensOutput output;

	/* Send data across DLL boundary to be prepared and processed in OpenCV, currently this method will always return true */
	GetOpenCVWrapper().CalibrateLens(
		latchData.resizeParameters,
		parameters,
		corners.GetData(), /* Do not pass any UE4 class types across the DLL boundary, passing the pointer is fine. */
		chessboardSquareSizeMM,
		cornerCountX,
		cornerCountY,
		imageCount,
		output,
		Debug()); /* Pass the debug mode across the DLL boundary. */

	/* This isn't really necessary unless the lens has extreme lens shift, then the projection matrix will 
	need to be overridden, so we calculate it anyways for now. */
	FMatrix perspectiveMatrix = GeneratePerspectiveMatrixFromFocalLength(
		FIntPoint(latchData.resizeParameters.nativeX, latchData.resizeParameters.nativeY), 
		FVector2D(output.principalPixelPointX, output.principalPixelPointY), 
		output.focalLengthMM);

	/* Queue result message log to the main thread to be printed to the console. */
	QueueLog(FString::Printf(TEXT("(INFO): Completed camera calibration at zoom level: %f "
		"with solve error: %f "
		"with results: ("
		"\n\tField of View in degrees: (%f, %f)"
		"\n\tSensor width in MM: %f,"
		"\n\tSensor height in MM: %f,"
		"\n\tFocal Length in MM: %f,"
		"\n\tPrincipal Point Pixel: (%f, %f),"
		"\n\tAspect Ratio: %f\n)"),
		latchData.baseParameters.zoomLevel,
		output.error,
		output.fovX,
		output.fovY,
		output.sensorSizeMMX,
		output.sensorSizeMMY,
		output.focalLengthMM,
		output.principalPixelPointX, output.principalPixelPointY,
		output.aspectRatio));

	/* Store all the relevant data the user may need in this result struct. */
	FCalibrationResult result;
	result.baseParameters			= latchData.baseParameters;
	result.success					= true;
	result.fovX						= output.fovX;
	result.fovY						= output.fovY;
	result.focalLengthMM			= output.focalLengthMM;
	result.aspectRatio				= output.aspectRatio;
	result.sensorSizeMM				= FVector2D(output.sensorSizeMMX, output.sensorSizeMMY);
	result.principalPixelPoint		= FVector2D(output.principalPixelPointX, output.principalPixelPointY);
	result.resolution.X				= latchData.resizeParameters.nativeX;
	result.resolution.Y				= latchData.resizeParameters.nativeY;
	result.perspectiveMatrix		= perspectiveMatrix;
	result.k1						= output.k1;
	result.k2						= output.k2;
	result.p1						= output.p1;
	result.p2						= output.p2;
	result.k3						= output.k3;
	result.k4						= output.k4;
	result.k5						= output.k5;
	result.k6						= output.k6;
	result.imageCount				= imageCount;

	/* Write the calibration results to a JSON file if the parameter is toggled. */
	if (latchData.calibrationParameters.writeCalibrationResultsToFile)
		WriteSolvedPointsToJSONFile(result, latchData.calibrationParameters.calibrationResultsOutputPath);

	if (Debug())
		QueueLog(FString("(INFO): Finished with work unit."));

	if (ShouldExit())
		return;

	/* Queue the result back to the main thread and send to blueprint interface class. */
	QueueCalibrationResult(result);
}

int FLensSolverWorkerCalibrate::GetWorkLoad()
{
	int count = 0;
	Lock();
	count = workQueue.Num();
	Unlock();
	return count;
}

/* Called by the main thread via a delegate registered in this class's constructor. */
void FLensSolverWorkerCalibrate::QueueWorkUnit(const FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit)
{
	Lock();
	TQueue<FLensSolverCalibrationPointsWorkUnit> ** queuePtr = workQueue.Find(calibrateWorkUnit.baseParameters.calibrationID);
	if (queuePtr == nullptr)
	{
		workQueue.Add(calibrateWorkUnit.baseParameters.calibrationID, new TQueue<FLensSolverCalibrationPointsWorkUnit>());
		queuePtr = workQueue.Find(calibrateWorkUnit.baseParameters.calibrationID);
		if (Debug())
			QueueLog(FString::Printf(TEXT("Registered expected calibration work units with ID: \"%s\""), *calibrateWorkUnit.baseParameters.calibrationID));
	}
	Unlock();

	if (Debug())
		QueueLog(FString::Printf(TEXT("Queued calibration work unit with calibration ID: \"%s\""), *calibrateWorkUnit.baseParameters.calibrationID));

	TQueue<FLensSolverCalibrationPointsWorkUnit>* queue = *queuePtr;
	queue->Enqueue(calibrateWorkUnit);
}

bool FLensSolverWorkerCalibrate::DequeueAllWorkUnits(
	const FString calibrationID, 
	TArray<float> & corners,
	int & cornerCountX, int & cornerCountY,
	float & chessboardSquareSizeMM,
	int & imageCount) 
{
	Lock();
	TQueue<FLensSolverCalibrationPointsWorkUnit> ** queuePtr = workQueue.Find(calibrationID);
	if (queuePtr == nullptr)
	{
		if (Debug())
			QueueLog(FString::Printf(TEXT("(ERROR): No work units in calibration queue with ID: \"%s\"."), *calibrationID));
		Unlock();
		return false;
	}

	TQueue<FLensSolverCalibrationPointsWorkUnit>* queue = *queuePtr; 

	bool isQueued = queue->IsEmpty() == false;
	cornerCountX = -1, cornerCountY = -1;
	chessboardSquareSizeMM = -1;

	while (isQueued)
	{
		FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit;
		queue->Dequeue(calibrateWorkUnit);
		isQueued = queue->IsEmpty() == false;
		workUnitCount--;

		if (calibrateWorkUnit.calibrationPointParameters.corners.Num() == 0)
		{
			if (Debug())
				QueueLog(FString::Printf(TEXT("(WARNING): No detected calibration pattern corners in image: \"%s\" for calibration: \"%s\", skipping and continuing to next image."),
					*calibrateWorkUnit.baseParameters.friendlyName,
					*calibrateWorkUnit.baseParameters.calibrationID));
			continue;
		}

		if (cornerCountX == -1 || cornerCountY == -1)
		{
			cornerCountX = calibrateWorkUnit.calibrationPointParameters.cornerCountX;
			cornerCountY = calibrateWorkUnit.calibrationPointParameters.cornerCountY;
		}

		else if (cornerCountX != calibrateWorkUnit.calibrationPointParameters.cornerCountX || cornerCountY != calibrateWorkUnit.calibrationPointParameters.cornerCountY)
		{
			if (Debug())
				QueueLog(FString::Printf(TEXT("(ERROR): Detected different chessboard corner count of: (%i, %i) instead of (%i, %i) in calibration queue: \"%s\". Something is broken."),
					calibrateWorkUnit.calibrationPointParameters.cornerCountX,
					calibrateWorkUnit.calibrationPointParameters.cornerCountY,
					cornerCountX,
					cornerCountY,
					*calibrateWorkUnit.baseParameters.calibrationID));
			return false;
		}

		if (chessboardSquareSizeMM == -1)
			chessboardSquareSizeMM = calibrateWorkUnit.calibrationPointParameters.chessboardSquareSizeMM;
		else if (chessboardSquareSizeMM != calibrateWorkUnit.calibrationPointParameters.chessboardSquareSizeMM)
		{
			if (Debug())
				QueueLog(FString::Printf(TEXT("(ERROR): Detected different chessboard square size of: %d instead of %d in calibration queue: \"%s\". Something is broken."),
					calibrateWorkUnit.calibrationPointParameters.chessboardSquareSizeMM,
					chessboardSquareSizeMM,
					*calibrateWorkUnit.baseParameters.calibrationID));
			return false;
		}

		corners.Append(calibrateWorkUnit.calibrationPointParameters.corners);
		imageCount++;

		if (Debug())
			QueueLog(FString::Printf(TEXT("(INFO): Dequeued %d corner points image: \"%s\" for calibration: \"%s\"."),
				calibrateWorkUnit.calibrationPointParameters.corners.Num(),
				*calibrateWorkUnit.baseParameters.friendlyName,
				*calibrateWorkUnit.baseParameters.calibrationID));
	}

	delete queue;
	workQueue.Remove(calibrationID);
	Unlock();

	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): Dequeued %d images corner sets each of size: (%i, %i) for calibration: \"%s\"."),
			imageCount,
			cornerCountX,
			cornerCountY,
			*calibrationID));

	return true;
}

void FLensSolverWorkerCalibrate::WriteSolvedPointsToJSONFile(const FCalibrationResult& solvePoints, FString outputPath)
{
	if (!LensSolverUtilities::ValidateFilePath(outputPath, calibrationVisualizationOutputPath, "CalibrationResults", "json"))
		return;

	TSharedPtr<FJsonObject> obj = MakeShareable(new FJsonObject);
	TSharedPtr<FJsonObject> result = MakeShareable(new FJsonObject);
	result->SetStringField("jobid", solvePoints.baseParameters.jobID);
	result->SetStringField("calibrationid", solvePoints.baseParameters.calibrationID);
	result->SetStringField("friendlyname", solvePoints.baseParameters.friendlyName);
	result->SetNumberField("zoomlevel", solvePoints.baseParameters.zoomLevel);
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
	distortionCoefficients.Add(MakeShareable(new FJsonValueNumber(static_cast<float>(solvePoints.k1))));
	distortionCoefficients.Add(MakeShareable(new FJsonValueNumber(static_cast<float>(solvePoints.k2))));
	distortionCoefficients.Add(MakeShareable(new FJsonValueNumber(static_cast<float>(solvePoints.p1))));
	distortionCoefficients.Add(MakeShareable(new FJsonValueNumber(static_cast<float>(solvePoints.p2))));
	distortionCoefficients.Add(MakeShareable(new FJsonValueNumber(static_cast<float>(solvePoints.k3))));

	result->SetArrayField("distortioncoefficients", distortionCoefficients);
	obj->SetObjectField("result", result);

	FString outputJson(TEXT("{}"));
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&outputJson);
	FJsonSerializer::Serialize(obj.ToSharedRef(), writer);

	if (!FFileHelper::SaveStringToFile(outputJson, *outputPath))
	{
		QueueLog(FString::Printf(TEXT("(ERROR): Unable to write calibration result to path: \"%s\", check your permissions."), *outputPath));
		return;
	}

	QueueLog(FString::Printf(TEXT("(INFO): Calibration result written to file at path: \"%s\"."), *outputPath));
}

void FLensSolverWorkerCalibrate::QueueCalibrationResultError(const FBaseParameters & baseParameters)
{
	TArray<FVector2D> emptyPoints;

	FCalibrationResult solvedPoints;
	solvedPoints.baseParameters = baseParameters;
	solvedPoints.success = false;

	if (!onSolvePointsDel->IsBound())
		return;
	onSolvePointsDel->Execute(solvedPoints);
}

void FLensSolverWorkerCalibrate::QueueCalibrationResult(FCalibrationResult solvedPoints)
{
	if (!onSolvePointsDel->IsBound())
		return;
	onSolvePointsDel->Execute(solvedPoints);
}

void FLensSolverWorkerCalibrate::QueueLatch(const FCalibrateLatch latchData)
{
	latchQueue.Enqueue(latchData);
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s: Queued calibrate latch."), *JobDataToString(latchData.baseParameters)));
}

void FLensSolverWorkerCalibrate::DequeueLatch(FCalibrateLatch & latchData)
{
	latchQueue.Dequeue(latchData);
	if (Debug())
		QueueLog(FString::Printf(TEXT("(INFO): %s sDequeued calibrate latch."), *JobDataToString(latchData.baseParameters)));
}

bool FLensSolverWorkerCalibrate::LatchInQueue()
{
	return latchQueue.IsEmpty() == false;
}

void FLensSolverWorkerCalibrate::NotifyShutdown()
{
	WorkerRegistry::Get().UncountCalibrateWorker();
}
