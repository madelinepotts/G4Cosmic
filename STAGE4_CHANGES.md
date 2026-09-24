# Stage 4 changes

Stage 4 makes the CRY acceptance filter framework-level instead of WarpTrack-specific.

## What changed

- Removed the `DetectorGeometryGenerated.hh` include from `PrimaryGeneratorAction`.
- Removed the old hard-coded CRY acceptance modes:
  - `rack`
  - `hodoscope`
- Added logical-volume acceptance controlled by macro:

```text
/g4cosmic/cry/acceptanceMode volume
/g4cosmic/cry/acceptanceVolume *ScintillatorLV_*
/g4cosmic/cry/maxAcceptanceTrials 10000
```

- `acceptanceVolume` accepts either an exact Geant4 logical-volume name or a simple wildcard pattern:
  - `ScintillatorLV`
  - `BottomScintillatorLV_0`
  - `*ScintillatorLV_*`
- At runtime, G4Cosmic traverses the constructed Geant4 world, finds physical placements whose logical-volume name matches the configured pattern, computes their world-space bounding boxes, and accepts CRY primaries whose forward ray intersects one of those boxes.
- Added retry protection so volume-conditioned CRY generation cannot loop forever:

```text
/g4cosmic/cry/maxAcceptanceTrials 10000
```

- Added new macros:
  - `macros/cry_volume_acceptance.mac`
  - `macros/cry_volume_training.mac`
- Kept the old `cry_rack_*` macro filenames as backward-compatible aliases that execute the new volume-based macros.
- Bumped project version to `0.4.0`.

## Why this matters

Detector geometry should define logical volumes. Runtime macros should define how CRY is used against those volumes. This stage removes detector-specific acceptance knowledge from the CRY generator path and moves G4Cosmic closer to being a reusable framework.

## Current limitation

The acceptance test uses conservative world-space bounding boxes for matching physical placements. That is a good first framework-level cut and works for enrichment/generation control. Exact solid-ray intersection can be added later if needed.

`SteppingAction` still contains some WarpTrack-specific track-end classification inherited from the original project. That should move into the WarpTrack example or become an optional framework hook in a later stage.
