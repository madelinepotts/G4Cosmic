# Install CRY into G4Cosmic/external/cry.
#
# This follows the original WarpTrack layout: CRY lives outside the simulation
# source under external/cry, and CMake builds CRY from its src/*.cc files.
#
# Usage from the repository root:
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_cry.ps1
#
# Optional:
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_cry.ps1 -Force
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_cry.ps1 -Destination .\external\cry

param(
    [string]$Destination = (Join-Path $PSScriptRoot "..\external\cry"),
    [string]$Url = "https://github.com/PKMuon/cry/archive/refs/heads/master.zip",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$destinationPath = [System.IO.Path]::GetFullPath($Destination)

if ((Test-Path (Join-Path $destinationPath "src\CRYGenerator.cc")) -and -not $Force) {
    Write-Host "CRY already exists at $destinationPath"
    Write-Host "Use -Force to reinstall."
    exit 0
}

$tmpRoot = Join-Path $repoRoot ".download"
$tmpDir = Join-Path $tmpRoot "cry"
$zipPath = Join-Path $tmpRoot "cry.zip"

if (Test-Path $tmpDir) {
    Remove-Item -Recurse -Force $tmpDir
}
New-Item -ItemType Directory -Force $tmpDir | Out-Null
New-Item -ItemType Directory -Force (Split-Path -Parent $zipPath) | Out-Null

Write-Host "Downloading CRY from: $Url"
Invoke-WebRequest -Uri $Url -OutFile $zipPath

Write-Host "Extracting CRY..."
Expand-Archive -Path $zipPath -DestinationPath $tmpDir -Force

$sourceRoot = Get-ChildItem -Path $tmpDir -Recurse -File -Filter "CRYGenerator.cc" |
    Where-Object { $_.FullName -match "[\\/]src[\\/]CRYGenerator\.cc$" } |
    Select-Object -First 1

if (-not $sourceRoot) {
    throw "Could not find src\CRYGenerator.cc in the downloaded CRY archive."
}

$cryRoot = Split-Path -Parent (Split-Path -Parent $sourceRoot.FullName)

if (Test-Path $destinationPath) {
    Remove-Item -Recurse -Force $destinationPath
}
New-Item -ItemType Directory -Force (Split-Path -Parent $destinationPath) | Out-Null
New-Item -ItemType Directory -Force $destinationPath | Out-Null
Copy-Item -Recurse -Force (Join-Path $cryRoot "*") -Destination $destinationPath

if (-not (Test-Path (Join-Path $destinationPath "src\CRYGenerator.cc"))) {
    throw "CRY install failed: missing src\CRYGenerator.cc at $destinationPath"
}

if (-not (Test-Path (Join-Path $destinationPath "data"))) {
    throw "CRY install failed: missing data directory at $destinationPath\data"
}

Write-Host "CRY installed at: $destinationPath"
Write-Host "You can now configure G4Cosmic with CMake."
