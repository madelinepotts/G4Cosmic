# Run metadata JSON sidecars

Every G4Cosmic ROOT output gets a JSON sidecar with the same base name:

```text
output.root
output.json
```

Example:

```text
corsika_batch.root
corsika_batch.json
```

The sidecar records generic run provenance:

- run start/end time and duration
- command line and working directory
- Geant4 worker thread count
- ROOT output file and JSON sidecar file
- G4Cosmic version and build metadata
- git commit, branch, exact tag when available, and dirty state
- macro SHA-256 fingerprint and cleaned command list
- source metadata
- optional detector/source metadata appended by downstream code

The macro section intentionally stores a fingerprint and command list, not raw macro text.

```json
{
  "macro": {
    "entry_file": "macros/corsika_batch_demo.mac",
    "files": [
      {
        "path": "macros/corsika_batch_demo.mac",
        "sha256": "...",
        "commands": [
          "/g4cosmic/output/file corsika_batch.root",
          "/g4cosmic/source corsika",
          "/g4cosmic/corsika/generationMode batch",
          "/run/beamOn 1000"
        ]
      }
    ]
  }
}
```

To print the same cleaned command list to the console, add this to a macro:

```text
/g4cosmic/metadata/printMacroCommands true
```

Downstream detector projects can append detector metadata by overriding:

```cpp
void AppendMetadata(G4Cosmic::JsonWriter& json) const override;
```

Source classes can append source-specific metadata by overriding:

```cpp
void AppendMetadata(G4Cosmic::JsonWriter& json) const override;
```
