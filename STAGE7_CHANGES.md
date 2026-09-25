# Stage 7 changes

Stage 7 separates primary-source selection from the primary-generator action.
The goal is to make CRY, the deterministic particle gun, randomized sample
sources, and future CORSIKA input look like interchangeable generator modules.

## New generator interface

A new framework interface was added:

```cpp
namespace G4Cosmic {

class PrimaryGenerator {
public:
    virtual ~PrimaryGenerator() = default;
    virtual const char* Name() const = 0;
    virtual void GeneratePrimaries(G4Event* event, RunAction* runAction) = 0;
};

}
```

`PrimaryGeneratorAction` now owns a registry of named generators and delegates
event generation to the active generator selected by `/g4cosmic/source`.

## Built-in generators

The existing source modes were moved behind the interface:

```text
cry
sample
gun
```

New files:

```text
include/G4Cosmic/PrimaryGenerator.hh
include/G4Cosmic/CRYPrimaryGenerator.hh
include/G4Cosmic/GunPrimaryGenerator.hh
include/G4Cosmic/SamplePrimaryGenerator.hh
src/CRYPrimaryGenerator.cc
src/GunPrimaryGenerator.cc
src/SamplePrimaryGenerator.cc
```

## Macro compatibility

The existing macro API is preserved:

```text
/g4cosmic/source cry
/g4cosmic/source gun
/g4cosmic/source sample
```

CRY controls remain under:

```text
/g4cosmic/cry/...
```

Sample-source controls remain under:

```text
/g4cosmic/sample/...
```

## Why this matters

`PrimaryGeneratorAction` is now only the Geant4 action/dispatcher. It no longer
contains all CRY, gun, and sample-source implementation details. This creates a
clean path for Stage 8, where CORSIKA can be added as another implementation of
`G4Cosmic::PrimaryGenerator` without touching detector geometry or output code.

## Stage 7.1: logical-volume sample source

- Updated `G4Cosmic::SamplePrimaryGenerator` so `sample` draws source positions from a user-selected logical volume.
- Removed the old rectangular plane-source mode.
- The sample source is volume-based by default and now requires `/g4cosmic/sample/sourceVolume ...`.
- Added macro commands:

```text
/g4cosmic/sample/positionMode volume
/g4cosmic/sample/positionMode surface
/g4cosmic/sample/sourceVolume <logical-volume-name-or-wildcard>
/g4cosmic/sample/maxPositionTrials 10000
```

- `positionMode volume` samples points inside randomly selected physical placements of the chosen logical volume.
- `positionMode surface` samples a point on the selected logical volume surface by sampling an interior point and projecting along a random local direction to the solid boundary.
- `maxPositionTrials` controls rejection-sampling attempts when finding a point inside the selected solid. `maxVolumeTrials` is kept as a deprecated alias.
- CRY remains plane-based by default (`acceptanceMode all`). Volume acceptance is only an optional event-enrichment filter, not the CRY source definition.
- Added `macros/sample_volume_demo.mac` and `macros/sample_surface_demo.mac`.
