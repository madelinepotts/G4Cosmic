# Best-effort installer for CORSIKA 8 under G4Cosmic\external\corsika8.
#
# CORSIKA is a full external air-shower simulator with its own dependencies.
# This script keeps it out of the G4Cosmic source tree and installs it into
# external\corsika8 so G4Cosmic macros can call it through
# /g4cosmic/corsika/inputMode command.
#
# Usage from the repository root:
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8.ps1
#
# Default tag: corsika8-v1.0-beta1, the first public beta release.
#
# Optional:
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8.ps1 -Force
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8.ps1 -Tag corsika8-v1.0-beta1

param(
    [string]$Destination = (Join-Path $PSScriptRoot "..\external\corsika8"),
    [string]$RepoUrl = "https://gitlab.iap.kit.edu/AirShowerPhysics/corsika.git",
    [string]$Tag = "corsika8-v1.0-beta1",
    [string]$BuildType = "RelWithDebInfo",
    [int]$Jobs = 0,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

function Require-Command($Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "$Name is required to install CORSIKA 8."
    }
}

Require-Command git
Require-Command cmake

$destinationPath = [System.IO.Path]::GetFullPath($Destination)
$srcDir = Join-Path $destinationPath "src"
$buildDir = Join-Path $destinationPath "build"
$installDir = Join-Path $destinationPath "install"

if ((Test-Path $installDir) -and -not $Force) {
    Write-Host "CORSIKA 8 install directory already exists at $installDir"
    Write-Host "Use -Force to reinstall."
    exit 0
}

if ($Force -and (Test-Path $destinationPath)) {
    Remove-Item -Recurse -Force $destinationPath
}

New-Item -ItemType Directory -Force $destinationPath | Out-Null

if (-not (Test-Path (Join-Path $srcDir ".git"))) {
    Write-Host "Cloning CORSIKA 8 from $RepoUrl"
    if ($Tag.Length -gt 0) {
        git clone --recursive --branch $Tag $RepoUrl $srcDir
    } else {
        git clone --recursive $RepoUrl $srcDir
    }
} else {
    Write-Host "Using existing CORSIKA 8 source tree at $srcDir"
    git -C $srcDir submodule update --init --recursive
}

cmake -S $srcDir -B $buildDir `
    -DCMAKE_BUILD_TYPE=$BuildType `
    -DCMAKE_INSTALL_PREFIX=$installDir

if ($Jobs -gt 0) {
    cmake --build $buildDir --parallel $Jobs
} else {
    cmake --build $buildDir --parallel
}

cmake --install $buildDir

Write-Host "CORSIKA 8 installed under: $destinationPath"
Write-Host ""
Write-Host "Next step: set your G4Cosmic macro command to call the CORSIKA application or wrapper that writes G4Cosmic's text shower format, for example:"
Write-Host ""
Write-Host "/g4cosmic/corsika/inputMode command"
Write-Host "/g4cosmic/corsika/file corsika_generated_particles.dat"
Write-Host "/g4cosmic/corsika/command <your-corsika-wrapper> --events __events__ --output __output__"
Write-Host "/g4cosmic/corsika/apply"
