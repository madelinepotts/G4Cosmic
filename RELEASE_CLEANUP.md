# Release cleanup for v0.7.2

This cleanup prepares the pre-CORSIKA Stage 7 code for tagging.

## Changes

- Default CRY diagnostics are now quiet (`/g4cosmic/cry/verbose 0`).
- The default `quick.mac` smoke test is quiet by default.
- Added `macros/cry_verbose_debug.mac` for explicit CRY diagnostic runs.
- Suppressed repeated worker-thread status messages for the volume/surface sample source.
- CRY acceptance-volume and initialization status messages now print from the master thread only.
- ROOT output command status messages are guarded so they only print from the master thread.
- The Geant4 `FTFP_BERT` physics list is configured with verbose level 0 in application setup.
- Basic sample-source demo macros no longer show `maxPositionTrials`; it remains an optional advanced control.

## Tag suggestion

Suggested tag after build/test/commit:

```bash
git tag -a v0.7.2 -m "G4Cosmic v0.7.2: pre-CORSIKA generator release"
git push origin v0.7.2
```
