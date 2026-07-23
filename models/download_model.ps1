param(
    [string]$OutputPath = (Join-Path $PSScriptRoot 'hand_landmarker.task')
)

$uri = 'https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/latest/hand_landmarker.task'
$outputPath = [System.IO.Path]::GetFullPath($OutputPath)
$temporaryPath = "$outputPath.download"

if (Test-Path -LiteralPath $outputPath) {
    Write-Host "Model already exists: $outputPath"
    exit 0
}

try {
    Invoke-WebRequest -Uri $uri -OutFile $temporaryPath
    if ((Get-Item -LiteralPath $temporaryPath).Length -lt 1MB) {
        throw 'Downloaded model is unexpectedly small.'
    }
    Move-Item -LiteralPath $temporaryPath -Destination $outputPath
    Write-Host "Downloaded MediaPipe Hand Landmarker model: $outputPath"
} finally {
    if (Test-Path -LiteralPath $temporaryPath) {
        Remove-Item -LiteralPath $temporaryPath
    }
}
