# Stage or build CORSIKA 8 inside WSL from a Windows checkout of G4Cosmic.
#
# G4Cosmic itself remains cross-platform and does not require CORSIKA to build.
# This helper is only for users who want a real CORSIKA 8 tree available through
# a Windows+WSL runner.
#
# Examples from the repository root:
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8_wsl.ps1 -StageOnly
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8_wsl.ps1 -Build -Jobs 8
#   powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8_wsl.ps1 -Distro Ubuntu -Build

param(
    [string]$Distro = "",
    [string]$Tag = "corsika8-v1.0-beta1",
    [int]$Jobs = 0,
    [switch]$Force,
    [switch]$StageOnly,
    [switch]$Build
)

$ErrorActionPreference = "Stop"

function Require-Command($Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "$Name is required. Install WSL first or use the native staging script."
    }
}

function Escape-WslSingleQuoted($Text) {
    return $Text -replace "'", "'\"'\"'"
}

Require-Command wsl

if ($StageOnly -and $Build) {
    throw "Use either -StageOnly or -Build, not both."
}

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$distroArgs = @()
if ($Distro.Length -gt 0) {
    $distroArgs += @("-d", $Distro)
}

$wslRepoRoot = (& wsl @distroArgs wslpath -a $repoRoot).Trim()
if ($LASTEXITCODE -ne 0 -or $wslRepoRoot.Length -eq 0) {
    throw "Unable to map repository path into WSL: $repoRoot"
}

$buildFlag = "1"
if ($StageOnly -or (-not $Build)) {
    $buildFlag = "0"
}

$forceFlag = "0"
if ($Force) { $forceFlag = "1" }

$jobsValue = ""
if ($Jobs -gt 0) { $jobsValue = "$Jobs" }

$repoQuoted = Escape-WslSingleQuoted $wslRepoRoot
$tagQuoted = Escape-WslSingleQuoted $Tag
$jobsQuoted = Escape-WslSingleQuoted $jobsValue

$command = "cd '$repoQuoted' && CORSIKA8_TAG='$tagQuoted' CORSIKA8_BUILD=$buildFlag CORSIKA8_JOBS='$jobsQuoted' G4COSMIC_FORCE_CORSIKA8_INSTALL=$forceFlag bash scripts/install_corsika8.sh"

Write-Host "Running CORSIKA 8 helper inside WSL:"
Write-Host "  $command"
Write-Host ""

& wsl @distroArgs bash -lc $command
if ($LASTEXITCODE -ne 0) {
    throw "WSL CORSIKA helper failed with exit code $LASTEXITCODE. G4Cosmic itself is unchanged; only the optional CORSIKA setup failed."
}

Write-Host ""
if ($buildFlag -eq "1") {
    Write-Host "CORSIKA 8 WSL build/install step completed."
} else {
    Write-Host "CORSIKA 8 source was staged in WSL-accessible external/corsika8/src. No build was attempted."
}
Write-Host ""
Write-Host "Use examples/corsika/corsika8_runner.py with G4COSMIC_CORSIKA8_WSL_COMMAND to call your real CORSIKA wrapper."
