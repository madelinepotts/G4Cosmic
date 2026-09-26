param(
    [string]$BuildDir = "build",
    [string]$Configuration = "Release",
    [int]$Threads = 1
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $RepoRoot

function Get-ExecutablePath {
    param([string]$Name)

    $multiConfig = Join-Path $RepoRoot (Join-Path $BuildDir (Join-Path $Configuration "$Name.exe"))
    if (Test-Path $multiConfig) { return $multiConfig }

    $singleConfig = Join-Path $RepoRoot (Join-Path $BuildDir "$Name.exe")
    if (Test-Path $singleConfig) { return $singleConfig }

    $singleConfigNoExt = Join-Path $RepoRoot (Join-Path $BuildDir $Name)
    if (Test-Path $singleConfigNoExt) { return $singleConfigNoExt }

    throw "Could not find executable for $Name under $BuildDir. Build first."
}

function Invoke-G4Cosmic {
    param(
        [string]$Exe,
        [string]$Macro,
        [int]$ThreadCount = 1
    )

    Write-Host "Running $Exe $Macro $ThreadCount"
    & $Exe $Macro $ThreadCount
    if ($LASTEXITCODE -ne 0) {
        throw "Run failed with exit code $($LASTEXITCODE): $Exe $Macro"
    }
}

function Assert-Exists {
    param([string]$Path)
    if (-not (Test-Path $Path)) {
        throw "Expected output was not created: $Path"
    }
}

$WarpTrackExe = Get-ExecutablePath "g4cosmic_warptrack"
$BasicExe = Get-ExecutablePath "g4cosmic_basic"

$filesToRemove = @(
    "basic_quick.root", "basic_quick.json",
    "corsika_dat_file.root", "corsika_dat_file.json",
    "corsika_batch.root", "corsika_batch.json", "corsika_cache.dat",
    "corsika_batch_volume_acceptance.root", "corsika_batch_volume_acceptance.json", "corsika_cache_acceptance.dat",
    "corsika8_runner.root", "corsika8_runner.json", "corsika8_generated_particles.dat"
)
foreach ($file in $filesToRemove) {
    Remove-Item $file -ErrorAction SilentlyContinue
}

Invoke-G4Cosmic $BasicExe ".\macros\basic_quick.mac" $Threads
Assert-Exists ".\basic_quick.root"
Assert-Exists ".\basic_quick.json"

Invoke-G4Cosmic $WarpTrackExe ".\macros\corsika_dat_file_demo.mac" $Threads
Assert-Exists ".\corsika_dat_file.root"
Assert-Exists ".\corsika_dat_file.json"

Invoke-G4Cosmic $WarpTrackExe ".\macros\corsika_batch_demo.mac" $Threads
Assert-Exists ".\corsika_batch.root"
Assert-Exists ".\corsika_batch.json"
Assert-Exists ".\corsika_cache.dat"

Invoke-G4Cosmic $WarpTrackExe ".\macros\corsika_batch_volume_acceptance.mac" $Threads
Assert-Exists ".\corsika_batch_volume_acceptance.root"
Assert-Exists ".\corsika_batch_volume_acceptance.json"
Assert-Exists ".\corsika_cache_acceptance.dat"

$oldToyFallback = $env:G4COSMIC_CORSIKA8_TOY_FALLBACK
try {
    $env:G4COSMIC_CORSIKA8_TOY_FALLBACK = "1"
    Invoke-G4Cosmic $WarpTrackExe ".\macros\corsika8_runner_template.mac" $Threads
    Assert-Exists ".\corsika8_runner.root"
    Assert-Exists ".\corsika8_runner.json"
    Assert-Exists ".\corsika8_generated_particles.dat"
}
finally {
    if ($null -eq $oldToyFallback) {
        Remove-Item Env:\G4COSMIC_CORSIKA8_TOY_FALLBACK -ErrorAction SilentlyContinue
    } else {
        $env:G4COSMIC_CORSIKA8_TOY_FALLBACK = $oldToyFallback
    }
}

Write-Host "G4Cosmic local regression tests passed."
