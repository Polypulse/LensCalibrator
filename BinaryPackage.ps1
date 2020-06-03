$editorCmd = [IO.File]::ReadAllText(".\EnginePath.txt")
echo "Editor path: $editorCmd"

$projectRoot = Get-Location
$currentLocation = $projectRoot
$outputPath = "D:\Distribution\Binary\"

echo "Packaging LensCalibrator plugin..."
& "$editorCmd\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="$currentLocation/LensCalibrator.uplugin" -TargetPlatforms=Win64 -Package="$outputPath/LensCalibrator" -Rocket

Copy-Item -Recurse -Force -Path .\License.txt -Destination "$outputPath\LensCalibrator\License.txt"

New-Item -Force -ItemType Directory "$outputPath\LensCalibrator\Source\ThirdParty\"
Copy-Item -Recurse -Force -Path .\Source\ThirdParty\OpenCV\OpenCV.txt -Destination "$outputPath\LensCalibrator\Source\ThirdParty\OpenCV.txt"

Remove-Item -Recurse -Force -Path "$outputPath\LensCalibrator\Source\LensCalibrator\Private"
Remove-Item -Recurse -Force -Path "$outputPath\LensCalibrator\Intermediate"
Remove-Item -Recurse -Force -Path "$outputPath\LensCalibrator\Binaries\Win64\UE4Editor-LensCalibrator.pdb"

pause
