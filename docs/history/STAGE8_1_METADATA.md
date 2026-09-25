# Stage 8.1: ROOT sidecar JSON metadata

Adds automatic run-provenance JSON sidecars for every G4Cosmic ROOT output.

For an output file named:

```text
run.root
```

G4Cosmic writes:

```text
run.json
```

The JSON records:

- schema version and G4Cosmic version
- run start/end UTC timestamps and duration
- executable command line
- working directory
- requested Geant4 thread count
- ROOT output file and JSON sidecar file
- git commit, branch, exact tag if present, and dirty state from configure time
- compiler and Geant4 versions
- macro entry file
- per-macro-file SHA-256 fingerprint
- cleaned macro command list, without raw macro text
- active source metadata
- detector metadata through an overridable append hook
- optional user metadata contributors

New macro command:

```text
/g4cosmic/metadata/printMacroCommands true
```

The JSON always records the cleaned macro commands. The macro command above only
enables an optional console printout.

New extension points:

```cpp
virtual void G4Cosmic::PrimaryGenerator::AppendMetadata(G4Cosmic::JsonWriter& json) const;
virtual void G4Cosmic::DetectorConstruction::AppendMetadata(G4Cosmic::JsonWriter& json) const;
```

Downstream detector projects should override `DetectorConstruction::AppendMetadata`
when they need detector-specific provenance. The core does not encode detector
geometry assumptions.
