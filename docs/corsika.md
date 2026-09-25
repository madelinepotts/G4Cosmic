# CORSIKA source

G4Cosmic treats CORSIKA as a primary-source backend. The framework boundary is a simple text `.dat` shower list: one CORSIKA shower becomes one Geant4 event, and all particles in that shower are injected as Geant4 primaries.

## Modes

```text
/g4cosmic/source corsika
/g4cosmic/corsika/generationMode file
/g4cosmic/corsika/generationMode batch
```

`file` mode reads an existing G4Cosmic/CORSIKA `.dat` file and never runs an external generator.

`batch` mode runs an external runner, writes a fresh `.dat` cache, then reads that cache into Geant4. Batch mode regenerates by default. Add `reuseCache 1` only when you intentionally want to rerun exactly the same cached shower set.

```text
/g4cosmic/corsika/reuseCache 1
```

## Text `.dat` interchange format

Each non-comment row is:

```text
event_id particle_id x y z px py pz time
```

Rows with the same contiguous `event_id` are grouped into one shower. `particle_id` can be PDG codes or common CORSIKA particle IDs, depending on:

```text
/g4cosmic/corsika/idScheme pdg
/g4cosmic/corsika/idScheme corsika
```

Unit controls:

```text
/g4cosmic/corsika/positionUnit m
/g4cosmic/corsika/momentumUnit GeV
/g4cosmic/corsika/timeUnit ns
```

## Batch runner contract

A batch runner should accept enough arguments to write a G4Cosmic/CORSIKA `.dat` file. The built-in demo macros use:

```text
/g4cosmic/corsika/runner python
/g4cosmic/corsika/runnerScript examples/corsika/demo_external_corsika_runner.py
```

G4Cosmic expands its current CORSIKA settings into command-line arguments for the runner, including event count, output path, primary, energy mode, energy range, spectral index, and angular range.

The bundled runner is only a plumbing test. A real CORSIKA 8 integration should call CORSIKA 8 and convert observation-level output into the `.dat` format above.


## Real CORSIKA 8 runner

The recommended real-CORSIKA path is the cross-platform dispatcher:

```text
examples/corsika/corsika8_runner.py
```

This keeps G4Cosmic portable.  The core builds even when CORSIKA is absent or unsupported on the local OS.  Configure the dispatcher with `G4COSMIC_CORSIKA8_COMMAND` for native/Linux execution, or `G4COSMIC_CORSIKA8_WSL_COMMAND` for Windows+WSL.  See [`docs/corsika8_runner.md`](corsika8_runner.md).

## Example macros

```text
macros/corsika_dat_file_demo.mac
macros/corsika_dat_file_reduced_demo.mac
macros/corsika_batch_demo.mac
macros/corsika_batch_volume_acceptance.mac
```

## Volume acceptance

CORSIKA volume acceptance mirrors CRY volume acceptance. It is an enrichment filter, not an absolute-rate calculation. Whole showers are accepted or rejected; particles inside a shower are not sampled independently.

```text
/g4cosmic/corsika/acceptanceMode volume
/g4cosmic/corsika/acceptanceVolume ScintillatorBarLV
/g4cosmic/corsika/maxAcceptanceTrials 10000
```
