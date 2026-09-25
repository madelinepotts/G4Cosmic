# External dependencies

This directory is reserved for third-party source dependencies downloaded by helper scripts. These dependency directories are ignored by Git.

```text
external/
├── README.md
├── .gitkeep
├── cry/          # created by scripts/install_cry.*
└── corsika8/     # created by scripts/install_corsika8.*
```

## CRY

G4Cosmic expects CRY at:

```text
external/cry
```

Install from the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_cry.ps1
```

```bash
./scripts/install_cry.sh
```

If you want to point at an existing CRY checkout instead, configure CMake with:

```bash
cmake -S . -B build -DCRY_ROOT=/path/to/cry
```

## CORSIKA 8

The optional CORSIKA 8 helper installer uses this layout:

```text
external/corsika8/src
external/corsika8/build
external/corsika8/install     # only after a successful build/install
```

Stage the source tree from the repository root on Windows:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8.ps1
```

Use WSL/Linux/container for real CORSIKA 8 builds. The current CORSIKA 8 CMake tree may reject native Windows builds; the PowerShell helper therefore does not build by default.

```bash
./scripts/install_corsika8.sh
```

G4Cosmic does not link directly against CORSIKA 8. CORSIKA batch mode calls a runner or wrapper from a macro, and that runner writes G4Cosmic's text `.dat` shower format.


## CORSIKA 8

CORSIKA 8 is optional.  G4Cosmic does not link it directly and should still build when CORSIKA is absent or unsupported by the local operating system.

Use `scripts/install_corsika8.sh` on Linux/WSL to stage or build a CORSIKA tree under `external/corsika8/`.  On Windows, use `scripts/install_corsika8.ps1` to stage source only, or `scripts/install_corsika8_wsl.ps1` to stage/build through WSL.

Real CORSIKA runs should be invoked through `examples/corsika/corsika8_runner.py`, which writes G4Cosmic's text `.dat` shower format.
