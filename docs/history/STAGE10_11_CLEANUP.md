# Stage 10/11 cleanup

This checkpoint keeps the portable-framework boundary intact while cleaning up example names, metadata, and local tests.

## Executables

- Renamed the WarpTrack example executable target from `g4cosmic` to `g4cosmic_warptrack`.
- Kept `g4cosmic_basic` as the minimal detector smoke-test executable.
- Kept the reusable framework as `g4cosmic_core` / `G4Cosmic::core`.

## Geant4 compatibility

- Added `G4Cosmic/Geant4Compat.hh`.
- Switched the analysis-manager include at compile time:
  - Geant4 10: `g4root.hh`
  - Geant4 11: `G4AnalysisManager.hh`
- Removed the hard dependency on `G4RunManagerFactory` in the application entry path.
- Guarded Geant4-11-only verbosity handling with version checks.

## CRY configuration

- CRY runtime settings are macro-driven and use CRY's native setup-key names under `/g4cosmic/cry/...`.
- `config/cry/cry_setup.txt` remains as a readable native-CRY reference, not the normal runtime control path.

## Metadata

- Added detector metadata for the built-in examples.
- Registered sensitive logical-volume names are now reported under detector metadata.

## Tests

- Added `scripts/test_local.ps1` and `scripts/test_local.sh`.
- The tests exercise BasicDetector, CORSIKA file mode, CORSIKA batch mode, CORSIKA volume acceptance, and the optional CORSIKA 8 runner bridge using the explicit toy fallback.
