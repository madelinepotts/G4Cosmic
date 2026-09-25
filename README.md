# G4Cosmic

Reusable Geant4 cosmic-ray simulation framework with pluggable detector geometry, CRY/CORSIKA/gun/sample primary sources, ROOT output, and JSON run-metadata sidecars.

G4Cosmic keeps detector-specific meaning out of the framework core:

```text
Detector code defines detector geometry and sensitive volumes.
Macros define simulation runs.
G4Cosmic provides reusable source, output, metadata, and Geant4 plumbing.
```

The core records generic Geant4 truth for generated primaries and steps in registered sensitive volumes: particle IDs, copy numbers, volume names, positions, times, and energy deposition. Detector-specific concepts such as hodoscopes, channels, thresholds, labels, and trigger logic belong in downstream detector code or analysis.

## Repository layout

```text
G4Cosmic/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── config/
│   └── cry/
│       └── cry_setup.txt
├── docs/
│   ├── corsika.md
│   ├── dependencies.md
│   ├── metadata.md
│   └── history/
├── examples/
│   ├── BasicDetector/
│   ├── WarpTrack/
│   └── corsika/
│       ├── demo_external_corsika_runner.py
│       └── example_particles.dat
├── external/
│   ├── README.md
│   └── .gitkeep
├── include/
│   └── G4Cosmic/
├── macros/
├── scripts/
│   ├── install_cry.ps1
│   ├── install_cry.sh
│   ├── install_corsika8.ps1
│   └── install_corsika8.sh
└── src/
```

Dependency installer scripts live in `scripts/`. Downloaded dependency trees live in `external/` and are ignored by Git. Framework-level run macros live in `macros/`. Detector-specific examples live in `examples/`.

## Dependencies

Required:

- CMake 3.20 or newer
- C++17 compiler
- Geant4 11.x
- CRY source and data files

Optional:

- CORSIKA 8 or another external CORSIKA runner/wrapper for `/g4cosmic/corsika/generationMode batch`
- Geant4 UI/visualization components when `G4COSMIC_ENABLE_UIVIS=ON`

See [`docs/dependencies.md`](docs/dependencies.md) for the CRY and CORSIKA installation layout.

## API documentation

G4Cosmic includes a Doxygen overview for the framework namespace and public extension points. Generate it with:

```bash
cmake -S . -B build -DG4COSMIC_BUILD_DOCS=ON
cmake --build build --target g4cosmic_docs
```

The generated HTML is written under `build/docs/doxygen/html/`.

## Install CRY

From the repository root on Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_cry.ps1
```

On Linux or macOS:

```bash
chmod +x ./scripts/install_cry.sh
./scripts/install_cry.sh
```

CRY is installed into `external/cry`. If CRY already exists elsewhere, configure with:

```bash
cmake -S . -B build -DCRY_ROOT=/path/to/cry
```

## Optional CORSIKA 8 install

G4Cosmic does not link directly against CORSIKA 8. Batch mode calls a runner/wrapper that writes G4Cosmic's text `.dat` shower format.

On Windows, the helper stages the CORSIKA 8 source tree under `external/corsika8/src`. The current CORSIKA 8 CMake tree may reject native Windows builds, so use WSL/Linux/container for real CORSIKA builds or pass `-Build` only if you intentionally want to try a native build.

From the repository root on Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8.ps1
```

On Linux or macOS:

```bash
chmod +x ./scripts/install_corsika8.sh
./scripts/install_corsika8.sh
```

The bundled `examples/corsika/demo_external_corsika_runner.py` is a toy plumbing test. Replace it with a real CORSIKA 8 wrapper/converter for physical shower production.

## Configure and build

Windows PowerShell with Visual Studio Build Tools:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DGeant4_DIR="E:\Geant4\Geant4-11.4\lib\cmake\Geant4" `
  -DROOT_DIR="E:\root_v6.40.02\cmake"

cmake --build build --config Release
```

Linux/macOS:

```bash
cmake -S . -B build \
  -DGeant4_DIR=/path/to/geant4/lib/cmake/Geant4

cmake --build build -j
```

Headless builds can disable Geant4 UI/visualization support:

```bash
cmake -S . -B build \
  -DG4COSMIC_ENABLE_UIVIS=OFF \
  -DGeant4_DIR=/path/to/geant4/lib/cmake/Geant4
```

## Run examples

WarpTrack example:

```powershell
.\build\Release\g4cosmic.exe .\macros\quick.mac
```

Minimal generic detector example:

```powershell
.\build\Release\g4cosmic_basic.exe .\macros\basic_quick.mac
```

Set the Geant4 worker thread count as the second command-line argument:

```powershell
.\build\Release\g4cosmic.exe .\macros\run.mac 16
```

or with an environment variable:

```powershell
$env:G4COSMIC_THREADS="16"
.\build\Release\g4cosmic.exe .\macros\run.mac
```

## Writing detector geometry

Detector projects inherit from `G4Cosmic::DetectorConstruction` and register sensitive logical volumes from the detector geometry code:

```cpp
class MyDetector : public G4Cosmic::DetectorConstruction {
protected:
  G4VPhysicalVolume* BuildGeometry() override;
};

G4VPhysicalVolume* MyDetector::BuildGeometry() {
  // Build world, materials, placements, and detector volumes.
  RegisterSensitiveVolume(myScintillatorLV);
  return worldPV;
}
```

The framework writes one raw hit tree per registered sensitive logical volume. Multiple physical placements of the same logical volume share that tree; use `copy_no`, `physical_volume`, and `logical_volume` columns to distinguish placements.

## Source selection

Built-in source modes:

```text
/g4cosmic/source cry
/g4cosmic/source corsika
/g4cosmic/source gun
/g4cosmic/source sample
```

The source implementations are independent `G4Cosmic::PrimaryGenerator` classes:

```text
G4Cosmic::CRYPrimaryGenerator
G4Cosmic::CORSIKAPrimaryGenerator
G4Cosmic::GunPrimaryGenerator
G4Cosmic::SamplePrimaryGenerator
```

CRY commands remain under `/g4cosmic/cry/...`, CORSIKA commands remain under `/g4cosmic/corsika/...`, and randomized sample-source commands remain under `/g4cosmic/sample/...`.

## CORSIKA source

CORSIKA supports two modes:

```text
/g4cosmic/corsika/generationMode file
/g4cosmic/corsika/generationMode batch
```

`file` mode reads an existing G4Cosmic/CORSIKA text `.dat` shower list.

`batch` mode runs an external runner, writes a fresh cache `.dat` file by default, then consumes the cached showers. Set `/g4cosmic/corsika/reuseCache 1` only when intentionally reusing a cache from an earlier run.

Example macros:

```text
macros/corsika_dat_file_demo.mac
macros/corsika_dat_file_reduced_demo.mac
macros/corsika_batch_demo.mac
macros/corsika_batch_volume_acceptance.mac
macros/corsika8_runner_template.mac
```

See [`docs/corsika.md`](docs/corsika.md) for the `.dat` format and runner contract.

## CRY source

A normal CRY macro does not need a detector logical volume:

```text
/g4cosmic/source cry
/g4cosmic/cry/subboxLength 2
/g4cosmic/cry/verbose 0
/g4cosmic/cry/apply
/run/beamOn 10000
```

CRY logical-volume acceptance is optional enrichment, not an absolute-rate calculation:

```text
/run/initialize
/g4cosmic/source cry
/g4cosmic/cry/acceptanceMode volume
/g4cosmic/cry/acceptanceVolume ScintillatorBarLV
/g4cosmic/cry/maxAcceptanceTrials 10000
/g4cosmic/cry/apply
/run/beamOn 10000
```

Detector-specific validation macros live under detector example folders, such as `examples/WarpTrack/macros/`, rather than the framework-level `macros/` folder.

## Sample source

The `sample` source samples particles from a detector-defined logical volume. It is useful for button sources, calibration fixtures, or embedded source volumes.

```text
/g4cosmic/source sample
/g4cosmic/sample/particle mu-
/g4cosmic/sample/minEnergy 30 MeV
/g4cosmic/sample/maxEnergy 10 GeV
/g4cosmic/sample/sourceVolume SourceButtonLV
/g4cosmic/sample/positionMode volume
/run/beamOn 1000
```

Use `positionMode surface` to sample positions on the selected volume boundary.

## ROOT output

The default output file is:

```text
g4cosmic.root
```

By default, G4Cosmic writes:

- `primaries`: generated primary-particle truth
- one raw hit tree per registered sensitive logical volume

The `primaries` tree stores event ID, primary index, PDG code, particle name, kinetic energy, time, position, unit momentum direction, and momentum components.

Each per-volume hit tree stores generic step-level sensitive-volume energy deposition:

```text
event_id
track_id
parent_id
pdg
particle_name
copy_no
edep_MeV
time_ns
x_mm
y_mm
z_mm
physical_volume
logical_volume
```

Optional outputs:

```text
/g4cosmic/output/trackEnd true
/g4cosmic/output/reducedHits true
/g4cosmic/output/reduceBy copyNo
```

Supported reduced-hit grouping modes are `copyNo`, `particleCopyNo`, and `physicalVolume`.

## JSON metadata sidecar

Every ROOT output gets a JSON sidecar with the same base name:

```text
corsika_batch.root
corsika_batch.json
```

The JSON records run provenance, including start/end time, duration, command line, working directory, thread count, output filename, build version, git commit/branch/tag/dirty state, macro SHA-256 fingerprints, cleaned macro command lists, and source metadata.

To print the cleaned macro command list to the console, add:

```text
/g4cosmic/metadata/printMacroCommands true
```

Downstream detectors and sources can append their own metadata by overriding `AppendMetadata`.

See [`docs/metadata.md`](docs/metadata.md) for details.

## Development notes

Stage-by-stage development notes were moved out of the repository root and into [`docs/history/`](docs/history/).


For real CORSIKA 8 execution through native Linux, WSL, a container, or a cluster wrapper, see `docs/corsika8_runner.md`. G4Cosmic itself does not require CORSIKA to build.
