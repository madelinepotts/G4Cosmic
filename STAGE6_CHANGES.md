# Stage 6 changes

Stage 6 adds optional reduced sensitive-volume hit trees while preserving the
raw per-step hit trees from Stage 5.

## Output behavior

Raw hit trees are still created one per registered sensitive logical volume.
For example, registering a logical volume named `det_bar` creates a raw tree:

```text
det_bar
```

When reduced output is enabled, G4Cosmic also creates:

```text
det_bar_reduced
```

Reduced output is disabled by default.

## New macro commands

```text
/g4cosmic/output/reducedHits true
/g4cosmic/output/reduceBy copyNo
```

Supported reduction modes:

```text
copyNo
particleCopyNo
physicalVolume
```

Accepted aliases include `particleTypeCopyNo`, `pdgCopyNo`, and `volume`.

## Reduced tree schema

Each reduced tree uses a common schema so analysis code does not have to switch
on the mode-specific layout:

```text
event_id
reduction_mode
copy_no
pdg
particle_name
physical_volume
logical_volume
n_steps
n_tracks
total_edep_MeV
first_time_ns
last_time_ns
edep_weighted_x_mm
edep_weighted_y_mm
edep_weighted_z_mm
```

`copyNo` groups hits by event and leaf copy number.

`particleCopyNo` groups hits by event, PDG particle type, and leaf copy number.
This is useful for truth studies, but detector-response analysis should usually
prefer `copyNo` or `physicalVolume`.

`physicalVolume` groups hits by event, physical-volume name, and copy number.
The copy number is included because repeated Geant4 placements often share the
same physical-volume name.

## Implementation notes

A small `EventAction` was added so reduced rows are flushed at the end of each
Geant4 event. `EndOfRunAction` also flushes once as a safety net.

The raw trees are not changed or disabled by this feature. Reduced trees are an
additional convenience output, not a replacement for raw truth.


## Follow-up cleanup: WarpTrack scintillator tree grouping

The WarpTrack example now gives every sensitive scintillator-bar logical
volume the shared logical-volume name `ScintillatorBarLV`. Each bar is still a
separate Geant4 logical-volume object when its generated solid differs, but the
shared name causes G4Cosmic to write one raw hit tree, `ScintillatorBarLV`, and
one optional reduced tree, `ScintillatorBarLV_reduced`. Bar/channel identity is
preserved through the placement `copy_no`.
