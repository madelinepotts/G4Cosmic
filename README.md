# G4Cosmic

G4Cosmic is an early-stage reusable Geant4 framework for cosmic-ray detector
simulation. It was extracted from a working CRY/Geant4 detector simulation and
is being refactored in small, buildable stages.

Stage 2 is still conservative: the inherited detector geometry remains in place
so the project can keep running while the framework API is introduced around it.
The important new pieces are the repo-local dependency layout, cross-platform
CMake cleanup, and the first `G4Cosmic::Application` / detector-framework
classes.

## Stage 2 contents

- CMake project: `G4Cosmic`
- Executable: `g4cosmic`
- Runtime command prefix: `/g4cosmic/...`
- Default output: `g4cosmic.root`
- Default CRY path: `external/cry`
- New framework headers under `include/G4Cosmic/`
- New `G4Cosmic::Application` wrapper used by `src/main.cc`
- Optional Geant4 UI/visualization support for headless Linux/macOS/CI builds
- `.gitignore` suitable for a public C++/CMake/Geant4 repository

The inherited detector still uses the generated `WarpTrackGeometry` namespace.
That is intentional for now. Moving that detector into `examples/WarpTrack/` is
a later stage.

## Repository layout

```text
G4Cosmic/
├── CMakeLists.txt
├── README.md
├── STAGE1_CHANGES.md
├── STAGE2_CHANGES.md
├── .gitignore
├── cry/
│   └── cry_setup.txt
├── external/
│   ├── README.md
│   └── .gitkeep
├── generated/
│   └── DetectorGeometryGenerated.hh
├── include/
│   ├── *.hh
│   └── G4Cosmic/
│       ├── Application.hh
│       ├── DetectorConstruction.hh
│       └── GenericSensitiveDetector.hh
├── macros/
├── scripts/
│   ├── install_cry.ps1
│   └── install_cry.sh
└── src/
```

## Dependencies

You need:

- CMake 3.20 or newer
- A C++17 compiler
- Geant4 11.x
- CRY source and data files
- Python 3 only if regenerating geometry from the original JSON file

ROOT is accessed through Geant4's analysis manager in this stage, so the exact
ROOT setup follows your Geant4 build.

## Install CRY

G4Cosmic follows the original WarpTrack dependency layout: CRY is installed at
`external/cry`, and CMake builds the CRY source files directly into a static
library.

By default, CMake expects:

```text
external/cry/
```

Install CRY from the repository root on Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_cry.ps1
```

Install CRY on Linux or macOS:

```bash
chmod +x ./scripts/install_cry.sh
./scripts/install_cry.sh
```

If CRY is already installed elsewhere, configure with:

```bash
cmake -S . -B build -DCRY_ROOT=/path/to/cry
```

The downloaded `external/cry` directory is ignored by Git. The repository keeps
only `external/.gitkeep`, `external/README.md`, and the installer scripts.

## Configure and build: Windows

PowerShell with Visual Studio Build Tools:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DGeant4_DIR="E:\Geant4\Geant4-11.4\lib\cmake\Geant4" `
  -DROOT_DIR="E:\root_v6.40.02\cmake"

cmake --build build --config Release
```

Run:

```powershell
$env:GEANT4_DATA_DIR="E:\Geant4\Geant4-11.4\share\Geant4\data"
.\build\Release\g4cosmic.exe .\macros\quick.mac
```

Set the thread count either as the second command-line argument:

```powershell
.\build\Release\g4cosmic.exe .\macros\run.mac 16
```

or with an environment variable:

```powershell
$env:G4COSMIC_THREADS="16"
.\build\Release\g4cosmic.exe .\macros\run.mac
```

## Configure and build: Linux/macOS

The most portable approach is to point CMake at Geant4 with either
`Geant4_DIR` or `CMAKE_PREFIX_PATH`.

```bash
cmake -S . -B build \
  -DGeant4_DIR=/path/to/geant4/lib/cmake/Geant4 \
  -DCRY_ROOT=$PWD/external/cry

cmake --build build -j
```

Run:

```bash
./build/g4cosmic macros/quick.mac
```

Set the thread count either as an argument:

```bash
./build/g4cosmic macros/run.mac 16
```

or with an environment variable:

```bash
export G4COSMIC_THREADS=16
./build/g4cosmic macros/run.mac
```

## Headless builds

Some Linux/macOS systems, CI runners, and clusters have Geant4 installed without
interactive UI/visualization components. For those environments, configure with:

```bash
cmake -S . -B build \
  -DG4COSMIC_ENABLE_UIVIS=OFF \
  -DGeant4_DIR=/path/to/geant4/lib/cmake/Geant4 \
  -DCRY_ROOT=/path/to/cry

cmake --build build -j
```

When `G4COSMIC_ENABLE_UIVIS=OFF`, run with a macro file:

```bash
./build/g4cosmic macros/quick.mac
```

Interactive mode is intentionally disabled in that configuration.

## Source selection

The default source is CRY. You can switch source mode with a macro:

```text
/g4cosmic/source cry
/g4cosmic/cry/verbose 0
/g4cosmic/cry/apply
/run/beamOn 1000
```

or use the environment variable before startup:

Windows:

```powershell
$env:G4COSMIC_SOURCE="gun"
.\build\Release\g4cosmic.exe .\macros\geometry_check.mac
```

Linux/macOS:

```bash
export G4COSMIC_SOURCE=gun
./build/g4cosmic macros/geometry_check.mac
```

Supported source names in this stage are:

- `cry`
- `gun`
- `sample`

## CRY configuration

CRY settings are exposed through `/g4cosmic/cry/...` commands:

```text
/g4cosmic/source cry
/g4cosmic/cry/returnNeutrons 1
/g4cosmic/cry/returnProtons 1
/g4cosmic/cry/returnGammas 1
/g4cosmic/cry/returnElectrons 1
/g4cosmic/cry/returnMuons 1
/g4cosmic/cry/returnPions 1
/g4cosmic/cry/returnKaons 1
/g4cosmic/cry/date 9-18-2026
/g4cosmic/cry/latitude 46.3
/g4cosmic/cry/altitude 0
/g4cosmic/cry/subboxLength 2
/g4cosmic/cry/nParticlesMin 1
/g4cosmic/cry/nParticlesMax 1000000
/g4cosmic/cry/xoffset 0
/g4cosmic/cry/yoffset 0
/g4cosmic/cry/zoffset 0
/g4cosmic/cry/verbose 0
/g4cosmic/cry/apply
/run/beamOn 1000000
```

`/g4cosmic/cry/apply` rebuilds the CRY generator from the preceding settings,
so it must appear after CRY configuration changes and before `/run/beamOn`.

## Output

The default ROOT output file is:

```text
g4cosmic.root
```

You can change it in a macro:

```text
/g4cosmic/output/file validation.root
```

The current output trees are still inherited from the source simulation:

- `hits`: nonzero scintillator energy-deposition steps and detector/channel identity
- `primaries`: generated CRY/gun/sample primary truth
- `track_end`: terminal track state and stopping/server truth

Later stages will reorganize this into the intended G4Cosmic output API,
including `Primary`, `RawHits`, and `ReducedHits`.

## Current limitation

Stage 2 introduces framework structure without fully moving detector-specific
code yet. The inherited rack/hodoscope detector construction and stopping logic
are still compiled into the main executable. The next major stage should move
that detector into `examples/WarpTrack/` and make the core framework independent
of any one detector geometry.
