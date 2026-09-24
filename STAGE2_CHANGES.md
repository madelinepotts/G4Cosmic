# Stage 2 changes

Stage 2 keeps the inherited detector runnable while beginning the real G4Cosmic
framework extraction.

## Added

- `.gitignore` for CMake, Visual Studio, ROOT outputs, logs, and editor noise.
- `external/` directory with README and `.gitkeep`.
- CMake default CRY path set to `external/cry`.
- `G4COSMIC_ENABLE_UIVIS` CMake option.
  - `ON` by default for interactive Geant4 sessions.
  - `OFF` for headless Linux/macOS/CI/cluster builds.
- `include/G4Cosmic/Application.hh` and `src/Application.cc`.
- `include/G4Cosmic/DetectorConstruction.hh` and `src/FrameworkDetectorConstruction.cc`.
- `include/G4Cosmic/GenericSensitiveDetector.hh` and `src/GenericSensitiveDetector.cc`.

## Changed

- `src/main.cc` now delegates startup to `G4Cosmic::Application`.
- README now includes Windows, Linux, macOS, and headless build instructions.
- The project layout now anticipates repo-local external dependencies.

## Deliberately not changed yet

- The current detector-specific geometry remains in the main executable.
- The generated geometry namespace is still `WarpTrackGeometry`.
- `ScintillatorSD` is still used by the inherited detector path.
- ROOT trees are still named `hits`, `primaries`, and `track_end`.

Those are Stage 3+ tasks once the inherited detector is moved into
`examples/WarpTrack/` and a minimal `examples/BasicDetector/` is introduced.

## CRY installer update

- Matched the original WarpTrack dependency layout: CMake expects CRY at `external/cry`.
- Added `scripts/install_cry.ps1` for Windows.
- Added `scripts/install_cry.sh` for Linux/macOS.
- Updated the CMake missing-CRY error to point at those scripts.
- Kept downloaded CRY source out of Git by ignoring `external/cry/`; the tracked repo keeps `external/.gitkeep` and `external/README.md`.
