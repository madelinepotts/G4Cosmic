# External dependencies

This directory is reserved for third-party source dependencies.

## CRY

G4Cosmic follows the original WarpTrack layout and expects CRY at:

```text
external/cry
```

Install CRY with one of the helper scripts from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_cry.ps1
```

```bash
./scripts/install_cry.sh
```

The downloaded `external/cry` directory is ignored by Git. If you want to
point at an existing CRY checkout instead, configure CMake with:

```bash
cmake -S . -B build -DCRY_ROOT=/path/to/cry
```
