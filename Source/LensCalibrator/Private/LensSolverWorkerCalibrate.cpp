#include "LensSolverWorkerCalibrate.h"
#include "JsonUtilities.h"
#include "GenericPlatform/GenericPlatformProcess.h"

FLensSolverWorkerCalibrate::FLensSolverWorkerCalibrate(
	const FLensSolverWorkerParameters & inputParameters,
	QueueCalibrateWorkUnitInputDel* inputQueueCalibrateWorkUnitDel,
	QueueLatchInputDel* inputSignalLatch,
	const QueueCalibrationResultOutputDel* inputOnSolvePointsDel) :
	FLensSolverWorker(inputParameters),
	onSolvePointsDel(inputOnSolvePointsDel)
{
	inputQueueCalibrateWorkUnitDel->BindRaw(this, &FLensSolverWorkerCalibrate::QueueWorkUnit);
	inputSignalLatch->BindRaw(this, &FLensSolverWorkerCalibrate::QueueLatch);
	workUnitCount = 0;
}

FMatrix FLensSolverWorkerCalibrate::GeneratePerspectiveMatrixFromFocalLength(cv::Size& imageSize, cv::Point2d principlePoint, float focalLength)
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

void FLensSolverWorkerCalibrate::TransformVectorFromCVToUE4(FVector& v)
{
	float x, y, z;

	x = v.X;
	y = v.Y;
	z = v.Z;

	v.X = z;
	v.Y = x;
	v.Z = -y;
}

FTransform FLensSolverWorkerCalibrate::GenerateTransformFromRAndTVecs(std::vector<cv::Mat>& rvecs, std::vector<cv::Mat>& tvecs)
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

void FLensSolverWorkerCalibrate::Tick()
{
	if (!LatchInQueue())
		return;

	FCalibrateLatch latchData;
	DequeueLatch(latchData);

	std::vector<std::vector<cv::Point3f>> objectPoints;
	std::vector<std::vector<cv::Point2f>> corners;

	if (!DequeueAllWorkUnits(latchData.baseParameters.calibrationID, corners, objectPoints))
		return;

	if (corners.size() == 0 || objectPoints.size() == 0)
	{
		QueueLog("No calibration corners or object points to use in calibration process.");
		QueueCalibrationResultError(latchData.baseParameters);
		return;
	}

	if (ShouldExit())
		return;

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): Done dequeing work units, preparing calibration using %d sets of points."), corners.size()));

	std::vector<cv::Mat> rvecs, tvecs;
	cv::Mat cameraMatrix = cv::Mat::eye(3, 3, cv::DataType<float>::type);
	cv::Mat distortionCoefficients = cv::Mat::zeros(5, 1, cv::DataType<float>::type);
	cv::Size sourceImageSize(latchData.resizeParameters.sourceResolution.X, latchData.resizeParameters.sourceResolution.Y);
	cv::Point2d principalPoint = cv::Point2d(0.0, 0.0);
	cv::TermCriteria termCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001f);

	int sourcePixelWidth = latchData.resizeParameters.sourceResolution.X;
	int sourcePixelHeight = latchData.resizeParameters.sourceResolution.Y;
	
	float sensorHeight = latchData.calibrationParameters.sensorDiagonalSizeMM / FMath::Sqrt(FMath::Square(sourcePixelWidth / (float)sourcePixelHeight) + 1.0f);
	float sensorWidth = sensorHeight * (sourcePixelWidth / (float)sourcePixelHeight);
	/*
	float sensorHeight = (latchData.calibrationParameters.sensorDiagonalSizeMM * sourcePixelWidth) / FMath::Sqrt(sourcePixelWidth * sourcePixelWidth + sourcePixelHeight * sourcePixelHeight);
	float sensorWidth = sensorHeight * (sourcePixelWidth / (float)sourcePixelHeight);
	*/

	if (debug)
		QueueLog(FString::Printf(TEXT("(INFO): Sensor size: (%f, %f) mm, diagonal: (%f) mm."), sensorWidth, sensorHeight, latchData.calibrationParameters.sensorDiagonalSizeMM));

	double fovX = 0.0f, fovY = 0.0f, focalLength = 0.0f;
	double aspectRatio = 0.0f;

	int flags = 0;
	flags |= latchData.calibrationParameters.useInitialIntrinsicValues				?	cv::CALIB_USE_INTRINSIC_GUESS : 0;
	flags |= latchData.calibrationParameters.keepPrincipalPixelPositionFixed		?	cv::CALIB_FIX_PRINCIPAL_POINT : 0;
	flags |= latchData.calibrationParameters.keepAspectRatioFixed					?	cv::CALIB_FIX_ASPECT_RATIO : 0;
	flags |= latchData.calibrationParameters.lensHasTangentalDistortion				?	cv::CALIB_ZERO_TANGENT_DIST : 0;
	flags |= latchData.calibrationParameters.fixRadialDistortionCoefficientK1		?	cv::CALIB_FIX_K1 : 0;
	flags |= latchData.calibrationParameters.fixRadialDistortionCoefficientK2		?	cv::CALIB_FIX_K2 : 0;
	flags |= latchData.calibrationParameters.fixRadialDistortionCoefficientK3		?	cv::CALIB_FIX_K3 : 0;
	flags |= latchData.calibrationParameters.fixRadialDistortionCoefficientK4		?	cv::CALIB_FIX_K4 : 0;
	flags |= latchData.calibrationParameters.fixRadialDistortionCoefficientK5		?	cv::CALIB_FIX_K5 : 0;
	flags |= latchData.calibrationParameters.fixRadialDistortionCoefficientK6		?	cv::CALIB_FIX_K6 : 0;

	if (flags & cv::CALIB_USE_INTRINSIC_GUESS)
	{
		cameraMatrix.at<float>(0, 2) = latchData.calibrationParameters.initialPrincipalPointPixelPosition.X;
		cameraMatrix.at<float>(1, 2) = latchData.calibrationParameters.initialPrincipalPointPixelPosition.Y;
		if (debug)
			QueueLog(FString::Printf(TEXT("(INFO): Setting initial principal point to: (%f, %f)"), 
				latchData.calibrationParameters.initialPrincipalPointPixelPosition.X,
				latchData.calibrationParameters.initialPrincipalPointPixelPosition.Y));
	}

	else if (flags & cv::CALIB_FIX_ASPECT_RATIO)
	{
		cameraMatrix.at<float>(0, 0) = 1.0f / (latchData.resizeParameters.sourceResolution.X * 0.5f);
		cameraMatrix.at<float>(1, 1) = 1.0f / (latchData.resizeParameters.sourceResolution.Y * 0.5f);
		if (debug)
			QueueLog(FString::Printf(TEXT("(INFO): Keeping aspect ratio at: %f"), 
				(latchData.resizeParameters.sourceResolution.X / (float)latchData.resizeParameters.sourceResolution.Y)));
	}

	QueueLog("(INFO): Calibrating...");

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

	if (ShouldExit())
		return;

	QueueLog("(INFO): Done.");

	cv::calibrationMatrixValues(cameraMatrix, sourceImageSize, sensorWidth, sensorHeight, fovX, fovY, focalLength, principalPoint, aspectRatio);
	FMatrix perspectiveMatrix = GeneratePerspectiveMatrixFromFocalLength(sourceImageSize, principalPoint, focalLength);

	fovX *= 2.0f;
	fovY *= 2.0f;
	focalLength *= 2.0f;

	principalPoint.x = sourcePixelWidth * (principalPoint.x / sensorWidth);
	principalPoint.y = sourcePixelHeight * (principalPoint.y / sensorHeight);

	FString format = FString("(INFO): Completed camera calibration at zoom level: %f "
		"with solve error: %f "
		"with results: ("
		"\n\tField of View in degrees: (%f, %f)"
		"\n\tSensor width in MM: %f,"
		"\n\tSensor height in MM: %f,"
		"\n\tFocal Length in MM: %f,"
		"\n\tPrincipal Point Pixel: (%f, %f),"
		"\n\tAspect Ratio: %f\n)");

	QueueLog(FString::Printf(*format,
		latchData.baseParameters.zoomLevel,
		error,
		fovX,
		fovY,
		sensorWidth,
		sensorHeight,
		focalLength,
		principalPoint.x, principalPoint.y,
		aspectRatio));

	if (ShouldExit())
		return;

	TArray<float> outputDistortionCoefficients;
	outputDistortionCoefficients.SetNum(5);
	for (int i = 0; i < distortionCoefficients.rows; i++)
		outputDistortionCoefficients[i] = distortionCoefficients.at<float>(i, 0);

	FCalibrationResult solvedPoints;

	solvedPoints.baseParameters = latchData.baseParameters;
	solvedPoints.success = true;
	solvedPoints.fovX = fovX;
	solvedPoints.fovY = fovY;
	solvedPoints.focalLengthMM = focalLength;
	solvedPoints.aspectRatio = aspectRatio;
	solvedPoints.sensorSizeMM = FVector2D(sensorWidth, sensorHeight);
	solvedPoints.principalPixelPoint = FVector2D(principalPoint.x, principalPoint.y);
	solvedPoints.resolution = latchData.resizeParameters.sourceResolution;
	solvedPoints.perspectiveMatrix = perspectiveMatrix;
	solvedPoints.distortionCoefficients = outputDistortionCoefficients;

	if (latchData.calibrationParameters.writeCalibrationResultsToFile)
		WriteSolvedPointsToJSONFile(solvedPoints, latchData.calibrationParameters.calibrationResultsFolderPath, "result");

	if (debug)
		QueueLog(FString("(INFO): Finished with work unit."));

	QueueCalibrationResult(solvedPoints);
}

int FLensSolverWorkerCalibrate::GetWorkLoad()
{
	int count = 0;
	Lock();
	count = workQueue.Num();
	Unlock();
	return count;
}

void FLensSolverWorkerCalibrate::QueueWorkUnit(const FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit)
{
	Lock();
	TQueue<FLensSolverCalibrationPointsWorkUnit> ** queuePtr = workQueue.Find(calibrateWorkUnit.baseParameters.calibrationID);
	if (queuePtr == nullptr)
	{
		workQueue.Add(calibrateWorkUnit.baseParameters.calibrationID, new TQueue<FLensSolverCalibrationPointsWorkUnit>());
		queuePtr = workQueue.Find(calibrateWorkUnit.baseParameters.calibrationID);
		if (debug)
			QueueLog(FString::Printf(TEXT("Registered expected calibration work units with ID: \"%s\""), *calibrateWorkUnit.baseParameters.calibrationID));
	}
	Unlock();

	if (debug)
		QueueLog(FString::Printf(TEXT("Queued calibration work unit with calibration ID: \"%s\""), *calibrateWorkUnit.baseParameters.calibrationID));

	TQueue<FLensSolverCalibrationPointsWorkUnit>* queue = *queuePtr;
	queue->Enqueue(calibrateWorkUnit);
}

bool FLensSolverWorkerCalibrate::DequeueAllWorkUnits(
	const FString calibrationID, 
	std::vector<std::vector<cv::Point2f>> & corners,
	std::vector<std::vector<cv::Point3f>> & objectPoints)
{
	Lock();
	TQueue<FLensSolverCalibrationPointsWorkUnit> ** queuePtr = workQueue.Find(calibrationID);
	if (queuePtr == nullptr)
	{
		if (debug)
			QueueLog(FString::Printf(TEXT("(ERROR): No work units in calibration queue with ID: \"%s\"."), *calibrationID));
		Unlock();
		return false;
	}

	TQueue<FLensSolverCalibrationPointsWorkUnit>* queue = *queuePtr; 

	while (!queue->IsEmpty())
	{
		FLensSolverCalibrationPointsWorkUnit calibrateWorkUnit;
		queue->Dequeue(calibrateWorkUnit);
		workUnitCount--;

		if (calibrateWorkUnit.calibrationPointParameters.corners.size() == 0 || calibrateWorkUnit.calibrationPointParameters.objectPoints.size() == 0)
		{
			if (debug)
				QueueLog(FString::Printf(TEXT("(WARNING): No detected calibration pattern corners in image: \"%s\" for calibration: \"%s\", skipping and continuing to next image."),
					*calibrateWorkUnit.baseParameters.friendlyName,
					*calibrateWorkUnit.baseParameters.calibrationID));
			continue;
		}

		corners.push_back(calibrateWorkUnit.calibrationPointParameters.corners);
		objectPoints.push_back(calibrateWorkUnit.calibrationPointParameters.objectPoints);

		if (debug)
			QueueLog(FString::Printf(TEXT("(INFO): Dequeued %d corner points and %d object points for image: \"%s\" for calibration: \"%s\"."),
				calibrateWorkUnit.calibrationPointParameters.corners.size(),
				calibrateWorkUnit.calibrationPointParameters.objectPoints.size(),
				*calibrateWorkUnit.baseParameters.friendlyName,
				*calibrateWorkUnit.baseParameters.calibrationID));
	}

	delete queue;
	workQueue.Remove(calibrationID);
	Unlock();

	return true;
}

void FLensSolverWorkerCalibrate::WriteSolvedPointsToJSONFile(const FCalibrationResult& solvePoints, FString folder, const FString fileName)
{
	if (!LensSolverUtilities::ValidateFolder(folder, calibrationVisualizationOutputPath, workerMessage))
		return;

	FString outputPath = LensSolverUtilities::GenerateIndexedFilePath(folder, fileName, "json");

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
	for (int i = 0; i < solvePoints.distortionCoefficients.Num(); i++)
		distortionCoefficients.Add(MakeShareable(new FJsonValueNumber(static_cast<float>(solvePoints.distortionCoefficients[i]))));

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
