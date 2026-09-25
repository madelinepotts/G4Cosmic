# Stage 5 changes

Stage 5 makes the G4Cosmic base framework detector-agnostic and adds a minimal framework test example.

## Major changes

- Added a `g4cosmic_core` static library target with alias `G4Cosmic::core`.
- Kept the WarpTrack example executable as `g4cosmic`.
- Added a minimal `examples/BasicDetector` executable named `g4cosmic_basic`.
- Added BasicDetector macros:
  - `macros/basic_quick.mac`
  - `macros/basic_cry_volume_acceptance.mac`
- Removed WarpTrack geometry knowledge from core stepping/output records.
- The base framework does not know about hodoscopes, server models, detector thresholds, triggers, channel labels, or ML labels.
- The global primary-particle truth tree is named `primaries`.
- Hit output is now one ROOT tree per registered sensitive logical volume.
  - Example: registering a logical volume named `det_bar` produces a ROOT tree named `det_bar`.
  - Multiple physical placements of the same logical volume share the same tree. Use `copy_no`, `physical_volume`, and `logical_volume` columns to distinguish placements.
- Per-volume hit trees contain only generic Geant4 step information:
  - event/track/parent/PDG/particle name
  - leaf copy number
  - deposited energy
  - time
  - position
  - physical volume name
  - logical volume name
- `track_end` is kept as an optional generic output feature. It is disabled by default and is created only when a macro enables:

```text
/g4cosmic/output/trackEnd true
```

`track_end` intentionally contains only generic Geant4 terminal-track facts. It does not include WarpTrack-specific hodoscope/server/channel labels, trigger thresholds, or analysis classes.

## Design note

G4Cosmic records generic primary and sensitive-volume hit truth. Detector projects can map that generic truth to detector-specific observables outside the framework.


## Primary tree naming

The primary truth tree stores both `pdg` and the convenience string `particle_name`. The primary truth tree also avoids ambiguous `dir_x/dir_y/dir_z` branch names. It records the unit momentum direction as:

```text
momentum_unit_x
momentum_unit_y
momentum_unit_z
```

and also records derived momentum components:

```text
px_MeV_c
py_MeV_c
pz_MeV_c
```

This makes the distinction between direction cosines and physical momentum components explicit for analysis.


## Particle names

Every tree that stores a PDG code now also stores a convenience `particle_name` branch. `pdg` remains the authoritative analysis field; `particle_name` is included for readability in ROOT scans, quick checks, and debugging. This applies to:

```text
primaries
all per-sensitive-logical-volume hit trees
track_end, when enabled
```
