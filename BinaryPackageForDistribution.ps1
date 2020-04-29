$editorCmd = [IO.File]::ReadAllText(".\EnginePath.txt")
echo "Editor path: $editorCmd"

$projectRoot = Get-Location
$currentLocation = $projectRoot
$outputPath = "D:\Distribution\"

echo "Packaging LensCalibrator plugin..."
& "$editorCmd\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="$currentLocation/LensCalibrator.uplugin" -TargetPlatforms=Win64 -Package="$outputPath/LensCalibrator" -Rocket

Copy-Item -Recurse -Force -Path .\License.txt -Destination "$outputPath\LensCalibrator\License.txt"
New-Item -Force -ItemType Directory "$outputPath\ThirdParty"
Copy-Item -Recurse -Force -Path .\ThirdParty\OpenCV\OpenCV.txt -Destination "$outputPath\LensCalibrator\ThirdParty\OpenCV.txt"

Remove-Item -Recurse -Force -Path "$outputPath\LensCalibrator\Source\LensCalibrator\Private"
