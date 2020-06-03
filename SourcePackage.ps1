$editorCmd = [IO.File]::ReadAllText(".\EnginePath.txt")
echo "Editor path: $editorCmd"

$projectRoot = Get-Location
$currentLocation = $projectRoot
$outputPath = "D:\Distribution\Source\"

echo "Packaging LensCalibrator plugin..."
& "$editorCmd\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="$currentLocation/LensCalibrator.uplugin" -TargetPlatforms=Win64 -Package="$outputPath/LensCalibrator" -Rocket

Copy-Item -Recurse -Force -Path .\License.txt -Destination "$outputPath\LensCalibrator\License.txt"

New-Item -Force -ItemType Directory "$outputPath\LensCalibrator\Source\ThirdParty\"
Copy-Item -Recurse -Force -Path .\Source\ThirdParty\OpenCV\OpenCV.txt -Destination "$outputPath\LensCalibrator\Source\ThirdParty\OpenCV.txt"

pause
