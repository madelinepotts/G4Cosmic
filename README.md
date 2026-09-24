# G4Cosmic Stage 1

G4Cosmic is the first-stage extraction of the CRY/Geant4 cosmic-ray simulation code from the existing detector-specific simulation. This stage is intentionally conservative: it renames the project, executable, macro command namespace, output defaults, and user-facing strings while preserving the existing detector geometry and physics behavior.

This is **not yet** the final reusable framework API. The next stages will move the detector geometry into an example application, introduce a reusable detector base class, and replace detector-specific sensitive-detector code with a generic G4Cosmic hit recorder.

## What changed in Stage 1

- CMake project renamed from `WarpTrackSimulation` to `G4Cosmic`.
- Executable renamed from `warptrack_sim` to `g4cosmic`.
- Macro command prefix renamed from `/warptrack/...` to `/g4cosmic/...`.
- Default ROOT file renamed from `warptrack.root` to `g4cosmic.root`.
- Environment variables renamed:
  - `WARPTRACK_SOURCE` -> `G4COSMIC_SOURCE`
  - `WARPTRACK_THREADS` -> `G4COSMIC_THREADS`
- Compile definitions renamed:
  - `WARPTRACK_CRY_DATA_DIR` -> `G4COSMIC_CRY_DATA_DIR`
  - `WARPTRACK_CRY_SETUP_FILE` -> `G4COSMIC_CRY_SETUP_FILE`
- Build products and copied macro paths now use the `g4cosmic` target name.

## Important Stage 1 note

The uploaded archive contained the simulation source tree, but it did **not** contain the sibling `geometry/` directory or the full `external/cry/` source tree. To keep this Stage 1 archive usable, the CMake file supports two geometry modes:

1. If `../geometry/detector_geometry.json` and `../geometry/generate_cpp_geometry.py` exist, CMake regenerates `DetectorGeometryGenerated.hh` just like the original project.
2. Otherwise, CMake falls back to the already-generated header included in `generated/DetectorGeometryGenerated.hh`.

The internal generated namespace is still `WarpTrackGeometry` for now. That is deliberate for Stage 1 because the original geometry generator was not included in the uploaded ZIP. This will go away when the detector is moved into `examples/WarpTrack/` in a later stage.

## Dependencies

You need:

- Geant4 with UI and visualization support.
- CRY source and data files.
- A C++17 compiler.
- Python 3 only if you are regenerating geometry from the original JSON file.

Set `CRY_ROOT` if CRY is not located at `external/cry` or `../external/cry`.

## Build

From a shell where Geant4 is configured:

```powershell
cd G4Cosmic
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DGeant4_DIR="E:\Geant4\Geant4-11.4\lib\cmake\Geant4" -DCRY_ROOT="C:\Users\Maddie\Documents\GitHub\WarpTrack\external\cry"
cmake --build build --config Release
```

If CRY is in `../external/cry`, the `-DCRY_ROOT=...` argument can be omitted.

## Run

Run a validation macro:

```powershell
.\build\Release\g4cosmic.exe .\macros\validation.mac
```

Run interactively:

```powershell
.\build\Release\g4cosmic.exe
```

Run with a chosen worker count:

```powershell
.\build\Release\g4cosmic.exe .\macros\run.mac 16
```

Or use the environment variable:

```powershell
$env:G4COSMIC_THREADS="16"
.\build\Release\g4cosmic.exe .\macros\run.mac
```

## Source selection

The default source is CRY. You can switch source mode with a macro:

```text
/g4cosmic/source cry
/g4cosmic/cry/verbose 0
/g4cosmic/cry/apply
/run/beamOn 1000
```

Or use the environment variable before startup:

```powershell
$env:G4COSMIC_SOURCE="gun"
.\build\Release\g4cosmic.exe .\macros\geometry_check.mac
```

Supported source names in this stage are:

- `cry`
- `gun`
- `sample`

## CRY configuration

CRY settings are available through `/g4cosmic/cry/...` commands:

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

`/g4cosmic/cry/apply` rebuilds the CRY generator from the preceding settings, so it must appear after CRY configuration changes and before `/run/beamOn`.

## Output

The default ROOT output file is:

```text
g4cosmic.root
```

You can change it in a macro:

```text
/g4cosmic/output/file validation.root
```

The current Stage 1 output trees are unchanged from the source project:

- `hits`: nonzero scintillator energy-deposition steps and detector/channel identity.
- `primaries`: generated CRY/gun/sample primary truth.
- `track_end`: terminal track state and stopping/server truth.

Later stages will rename and reorganize these into the intended G4Cosmic output API, likely `Primary`, `RawHits`, and `ReducedHits`.

## Current limitation

Stage 1 is a project rename and stabilization pass. It still contains the inherited rack/hodoscope detector construction and detector-specific channel/stopping logic. The next stage should split this into:

```text
G4Cosmic framework code
examples/WarpTrack detector code
examples/BasicDetector minimal detector code
```

That is where G4Cosmic becomes a proper reusable framework instead of a renamed application.
