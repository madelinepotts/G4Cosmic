# Stage 3 changes

Stage 3 separates the inherited WarpTrack detector from the framework source and
removes the clunky JSON/code-generation build path.

## Added

- `examples/WarpTrack/` example detector directory.
- `examples/WarpTrack/include/WarpTrackDetectorConstruction.hh`.
- `examples/WarpTrack/src/WarpTrackDetectorConstruction.cc`.
- `examples/WarpTrack/include/WarpTrackSensitiveDetector.hh`.
- `examples/WarpTrack/src/WarpTrackSensitiveDetector.cc`.
- `examples/WarpTrack/generated/DetectorGeometryGenerated.hh` as a normal source artifact.

## Changed

- `src/main.cc` now configures `G4Cosmic::Application` with the WarpTrack example detector:

  ```cpp
  app.SetDetector<WarpTrackExample::DetectorConstruction>();
  ```

- `G4Cosmic::Application` now accepts a detector factory instead of hard-coding one detector.
- `G4Cosmic::DetectorConstruction` now supports a detector-specific sensitive detector by overriding `CreateSensitiveDetector()`.
- The WarpTrack detector now registers sensitive volumes from its geometry code with `RegisterSensitiveVolume(lv)`.
- CMake now uses the checked-in WarpTrack example geometry header directly.

## Removed

- The old CMake path that looked for `../geometry/detector_geometry.json`.
- The old CMake custom command that regenerated `DetectorGeometryGenerated.hh` with Python.
- The top-level `generated/` directory.
- Top-level detector-specific `DetectorConstruction.*` and `ScintillatorSD.*` files.

## Still intentionally unchanged

- The executable is still named `g4cosmic`.
- The top-level executable still runs the WarpTrack example detector.
- The generated geometry namespace is still `WarpTrackGeometry`.
- The current ROOT trees are still `hits`, `primaries`, and `track_end`.

The next stage should introduce `examples/BasicDetector/` and start moving the
remaining generator/output/action classes toward reusable framework components.
