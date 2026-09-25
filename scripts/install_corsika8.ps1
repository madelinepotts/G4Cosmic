# Stage CORSIKA 8 under G4Cosmic\external\corsika8.
#
# Native Windows CMake configuration for the current CORSIKA 8 tree reports
# "This is an unsupported system."  Therefore this PowerShell script stages the
# source tree by default and does not claim a complete native Windows install.
# Build CORSIKA 8 in WSL/Linux/container, or pass -Build if you intentionally
# want to try a native build with a future CORSIKA version.
#
# Usage from the repository root:
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8.ps1
#
# Optional:
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8.ps1 -Force
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8.ps1 -Build
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8.ps1 -Tag corsika8-v1.0-beta1

param(
    [string]$Destination = (Join-Path $PSScriptRoot "..\external\corsika8"),
    [string]$RepoUrl = "https://gitlab.iap.kit.edu/AirShowerPhysics/corsika.git",
    [string]$Tag = "corsika8-v1.0-beta1",
    [string]$BuildType = "RelWithDebInfo",
    [int]$Jobs = 0,
    [switch]$Force,
    [switch]$Build
)

$ErrorActionPreference = "Stop"

function Require-Command($Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "$Name is required to stage CORSIKA 8."
    }
}

function Invoke-Native {
    param(
        [string]$Description,
        [string]$Command,
        [string[]]$Arguments
    )
    Write-Host $Description
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Description failed with exit code $LASTEXITCODE."
    }
}

Require-Command git
if ($Build) { Require-Command cmake }

$destinationPath = [System.IO.Path]::GetFullPath($Destination)
$srcDir = Join-Path $destinationPath "src"
$buildDir = Join-Path $destinationPath "build"
$installDir = Join-Path $destinationPath "install"

if ($Force -and (Test-Path $destinationPath)) {
    Remove-Item -Recurse -Force $destinationPath
}

New-Item -ItemType Directory -Force $destinationPath | Out-Null

if (-not (Test-Path (Join-Path $srcDir ".git"))) {
    if ($Tag.Length -gt 0) {
        Invoke-Native -Description "Cloning CORSIKA 8 from $RepoUrl at $Tag" -Command "git" -Arguments @("clone", "--recursive", "--branch", $Tag, $RepoUrl, $srcDir)
    } else {
        Invoke-Native -Description "Cloning CORSIKA 8 from $RepoUrl" -Command "git" -Arguments @("clone", "--recursive", $RepoUrl, $srcDir)
    }
} else {
    Write-Host "Using existing CORSIKA 8 source tree at $srcDir"
    Invoke-Native -Description "Updating CORSIKA 8 submodules" -Command "git" -Arguments @("-C", $srcDir, "submodule", "update", "--init", "--recursive")
}

if ($Build) {
    Write-Host "Attempting native CORSIKA 8 build. Note: current CORSIKA 8 may reject native Windows."
    Invoke-Native -Description "Configuring CORSIKA 8" -Command "cmake" -Arguments @("-S", $srcDir, "-B", $buildDir, "-DCMAKE_BUILD_TYPE=$BuildType", "-DCMAKE_INSTALL_PREFIX=$installDir")
    if ($Jobs -gt 0) {
        Invoke-Native -Description "Building CORSIKA 8" -Command "cmake" -Arguments @("--build", $buildDir, "--parallel", "$Jobs")
    } else {
        Invoke-Native -Description "Building CORSIKA 8" -Command "cmake" -Arguments @("--build", $buildDir, "--parallel")
    }
    Invoke-Native -Description "Installing CORSIKA 8" -Command "cmake" -Arguments @("--install", $buildDir)
    Write-Host "CORSIKA 8 installed under: $installDir"
} else {
    Write-Host "CORSIKA 8 source staged under: $srcDir"
    Write-Host "No native Windows build was attempted. Use WSL/Linux/container for real CORSIKA 8 builds, or rerun with -Build to try anyway."
}

Write-Host ""
Write-Host "G4Cosmic batch mode calls an external runner/wrapper that writes G4Cosmic's text shower format:"
Write-Host ""
Write-Host "/g4cosmic/corsika/generationMode batch"
Write-Host "/g4cosmic/corsika/cacheFile corsika_generated_particles.dat"
Write-Host "/g4cosmic/corsika/runner <python-or-wrapper-executable>"
Write-Host "/g4cosmic/corsika/runnerScript <your-corsika8-wrapper-or-script>"
Write-Host "/g4cosmic/corsika/apply"
