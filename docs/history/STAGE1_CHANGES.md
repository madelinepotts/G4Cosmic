# Stage 1 Changes

This archive is the Stage 1 rename/extraction pass from the uploaded simulation tree.

## Done

- Renamed CMake project to `G4Cosmic`.
- Renamed executable target to `g4cosmic`.
- Renamed Geant4 UI command namespace to `/g4cosmic/...`.
- Renamed default ROOT output file to `g4cosmic.root`.
- Renamed runtime environment variables to `G4COSMIC_SOURCE` and `G4COSMIC_THREADS`.
- Renamed CMake-provided CRY compile definitions to `G4COSMIC_CRY_DATA_DIR` and `G4COSMIC_CRY_SETUP_FILE`.
- Updated all included macros to use `/g4cosmic/...` commands.
- Added a `generated/DetectorGeometryGenerated.hh` fallback because the uploaded ZIP did not include the original sibling `geometry/` directory.
- Updated README build/run instructions for the Stage 1 project.

## Deliberately not done yet

- Did not move detector-specific geometry into `examples/WarpTrack/` yet.
- Did not replace `ScintillatorSD` with a generic sensitive detector yet.
- Did not introduce `G4Cosmic::Application` yet.
- Did not introduce the final generator abstraction yet.
- Did not rename the internal generated `WarpTrackGeometry` namespace, because the original geometry generator was not included in the uploaded ZIP.

## Expected next stage

Stage 2 should split the project into framework code and example detector code:

```text
G4Cosmic/
├── include/G4Cosmic/
├── src/
├── examples/
│   ├── WarpTrack/
│   └── BasicDetector/
└── macros/
```
