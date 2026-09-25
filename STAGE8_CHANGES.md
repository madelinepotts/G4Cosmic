# Stage 8 - CORSIKA 8 batch/cache shower source

Stage 8 adds a CORSIKA primary-generator path without changing detector
geometry, sensitive-volume registration, or ROOT output.

The important architecture decision is that CORSIKA behaves like CRY at the
G4Cosmic framework level: it is a shower source that produces one Geant4 event
containing one or more primary particles, can optionally use logical-volume
acceptance enrichment, and writes the same generic output trees.

## Source mode

```text
/g4cosmic/source corsika
```

## Shower granularity

G4Cosmic treats the shower as the natural unit:

```text
one CORSIKA shower -> one Geant4 event -> one or more Geant4 primaries
```

It does not generate one CORSIKA particle at a time.

## Generation modes

```text
/g4cosmic/corsika/generationMode file
/g4cosmic/corsika/generationMode batch
```

- `file` parses an existing G4Cosmic/CORSIKA text `.dat` shower list.
- `batch` runs a CORSIKA 8 command/wrapper, writes a cache `.dat` file, then consumes the cached showers.

Compatibility aliases remain available:

```text
/g4cosmic/corsika/inputMode file
/g4cosmic/corsika/inputMode command   # maps to generationMode batch
/g4cosmic/corsika/file <path>         # alias for cacheFile
/g4cosmic/corsika/eventsPerRun 1000   # alias for eventsPerBatch
```

## Direct `.dat` parsing

The parsed interchange format is:

```text
event_id particle_id x y z px py pz time
```

Lines with the same contiguous `event_id` are grouped into one shower and
injected into one Geant4 event. The `particle_id` column may use PDG IDs or
common CORSIKA particle IDs selected by:

```text
/g4cosmic/corsika/idScheme pdg
/g4cosmic/corsika/idScheme corsika
```

The parser supports configurable units:

```text
/g4cosmic/corsika/positionUnit m|cm|mm
/g4cosmic/corsika/momentumUnit GeV|MeV
/g4cosmic/corsika/timeUnit ns|us|ms|s
```

This is a text `.dat` interchange format. Native binary CORSIKA output should be
converted to this format by the CORSIKA 8 runner/converter.

## Batch/cache CORSIKA 8 command interface

Batch mode generates a fresh set of showers by default, caches them, then consumes them:

```text
/g4cosmic/source corsika
/g4cosmic/corsika/generationMode batch
/g4cosmic/corsika/cacheFile corsika_cache.dat
/g4cosmic/corsika/eventsPerBatch 1000
/g4cosmic/corsika/command <corsika8-wrapper> --events __events__ --output __output__
/g4cosmic/corsika/apply
/run/beamOn 1000
```

Supported command tokens:

```text
__output__ __cache__ __events__ __batch__
__primary__ __energy_mode__
__min_energy_gev__ __max_energy_gev__ __energy_gev__ __spectral_index__
__min_zenith_deg__ __max_zenith_deg__ __min_azimuth_deg__ __max_azimuth_deg__
```

## Primary-generation controls

These settings are forwarded to the external CORSIKA 8 wrapper through the
command-token system:

```text
/g4cosmic/corsika/primary proton
/g4cosmic/corsika/energyMode powerLaw
/g4cosmic/corsika/minEnergy 1 TeV
/g4cosmic/corsika/maxEnergy 100 TeV
/g4cosmic/corsika/spectralIndex 2.7
/g4cosmic/corsika/minZenith 0 deg
/g4cosmic/corsika/maxZenith 60 deg
/g4cosmic/corsika/minAzimuth 0 deg
/g4cosmic/corsika/maxAzimuth 360 deg
```

`energyMode mono` uses:

```text
/g4cosmic/corsika/energy 10 TeV
```

The spectral index shapes the distribution inside the requested energy range; it
is not a replacement for the min/max energy settings.

## CRY-like acceptance enrichment

By default, CORSIKA uses `acceptanceMode all`. If `acceptanceMode volume` is
enabled, G4Cosmic tests loaded/generated showers and selects one whose primary
ray intersects the requested logical-volume placement. The whole accepted shower
is injected into Geant4.

```text
/run/initialize
/g4cosmic/source corsika
/g4cosmic/corsika/generationMode batch
/g4cosmic/corsika/cacheFile corsika_cache_acceptance.dat
/g4cosmic/corsika/eventsPerBatch 10000
/g4cosmic/corsika/command <corsika8-wrapper> --events __events__ --output __output__
/g4cosmic/corsika/acceptanceMode volume
/g4cosmic/corsika/acceptanceVolume ScintillatorBarLV
/g4cosmic/corsika/maxAcceptanceTrials 10000
/g4cosmic/corsika/apply
/run/beamOn 1000
```

This is for enrichment/training studies, not absolute-rate calculations.

## CORSIKA 8 installer scripts

Best-effort CORSIKA 8 bootstrap scripts are included:

```text
scripts/install_corsika8.ps1
scripts/install_corsika8.sh
```

They clone/build/install CORSIKA 8 under:

```text
external/corsika8/src
external/corsika8/build
external/corsika8/install
```

The installed upstream tree is ignored by Git. G4Cosmic does not hard-link to
CORSIKA in this stage. It calls a user-configured external runner command so
CORSIKA 8 can remain application-oriented while G4Cosmic keeps a stable shower
source interface.

## Added/updated files

- `include/G4Cosmic/CORSIKAPrimaryGenerator.hh`
- `src/CORSIKAPrimaryGenerator.cc`
- `examples/corsika/example_particles.dat`
- `examples/corsika/demo_external_corsika_runner.py`
- `macros/corsika_dat_file_demo.mac`
- `macros/corsika_batch_demo.mac`
- `macros/corsika_batch_volume_acceptance.mac`
- `scripts/install_corsika8.ps1`
- `scripts/install_corsika8.sh`

## 0.8.1 macro-token fix

Geant4 treats `{name}` inside macro commands as UI aliases before command handlers receive the string. Stage 8 now uses double-underscore command tokens instead:

```text
__output__ __cache__ __events__ __batch__
__primary__ __energy_mode__
__min_energy_gev__ __max_energy_gev__ __energy_gev__ __spectral_index__
__min_zenith_deg__ __max_zenith_deg__ __min_azimuth_deg__ __max_azimuth_deg__
```

The WarpTrack and BasicDetector examples also default overlap checks to off to keep normal runs quiet. Re-enable overlap checks in detector code when debugging geometry.


Optional reproducibility mode: add `/g4cosmic/corsika/reuseCache 1` to reuse an existing cache file from an earlier run instead of regenerating it. Normal batch macros omit this command.
