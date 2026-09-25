# Real CORSIKA 8 runner

G4Cosmic should work on every supported operating system whether or not CORSIKA 8 is installed.  The framework therefore does **not** link CORSIKA 8 directly.  Instead, batch mode calls an external runner, and the runner writes the G4Cosmic text `.dat` shower format.

```text
G4Cosmic batch mode
  -> external runner
      -> optional real CORSIKA 8 executable, WSL command, container, or cluster job
      -> converter/wrapper writes G4Cosmic .dat
  -> G4Cosmic reads .dat
  -> one shower becomes one Geant4 event
```

This keeps the core portable:

- G4Cosmic builds without CORSIKA.
- CORSIKA can live in Linux, WSL, a container, or a cluster environment.
- The stable interface is the `.dat` interchange file, not CORSIKA internals.

## Runner script

The cross-platform dispatcher is:

```text
examples/corsika/corsika8_runner.py
```

Use it in a macro with:

```text
/g4cosmic/source corsika
/g4cosmic/corsika/generationMode batch
/g4cosmic/corsika/cacheFile corsika8_generated_particles.dat
/g4cosmic/corsika/runner python
/g4cosmic/corsika/runnerScript examples/corsika/corsika8_runner.py
/g4cosmic/corsika/apply
```

By default, the dispatcher does not pretend to have a real CORSIKA installation.  It exits with a clear error unless a backend command is configured.

For plumbing tests only, allow the bundled toy fallback:

```powershell
$env:G4COSMIC_CORSIKA8_TOY_FALLBACK="1"
```

or on Linux/macOS:

```bash
export G4COSMIC_CORSIKA8_TOY_FALLBACK=1
```

## Native/Linux command backend

Set `G4COSMIC_CORSIKA8_COMMAND` to a command template that writes the final G4Cosmic `.dat` file at `{output}`.

Example template:

```bash
export G4COSMIC_CORSIKA8_COMMAND='python /path/to/my_real_corsika_wrapper.py --events {events} --output {output_q} --primary {primary} --min-energy-gev {min_energy_gev} --max-energy-gev {max_energy_gev}'
```

The command template can use these placeholders:

```text
{events}
{output}
{output_q}
{primary}
{energy_mode}
{min_energy_gev}
{max_energy_gev}
{energy_gev}
{spectral_index}
{min_zenith_deg}
{max_zenith_deg}
{min_azimuth_deg}
{max_azimuth_deg}
{cwd}
{cwd_q}
```

`*_q` placeholders are shell-quoted versions of path/value placeholders.

The command is responsible for producing the G4Cosmic `.dat` rows:

```text
event_id particle_id x y z px py pz time
```

## Windows + WSL backend

Native Windows CMake configuration for the current CORSIKA 8 tree may reject the platform.  The recommended Windows path is to keep G4Cosmic native on Windows, but run real CORSIKA through WSL.

Stage or build CORSIKA in WSL:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8_wsl.ps1 -StageOnly
```

Attempt a WSL build:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install_corsika8_wsl.ps1 -Build -Jobs 8
```

Then configure a WSL command template:

```powershell
$env:G4COSMIC_CORSIKA8_WSL_COMMAND = "python /home/maddie/corsika_tools/my_real_corsika_wrapper.py --events {events} --output {output_wsl_q} --primary {primary} --min-energy-gev {min_energy_gev} --max-energy-gev {max_energy_gev}"
```

When launched from Windows, the dispatcher converts the Windows output path to a WSL path and provides it as:

```text
{output_wsl}
{output_wsl_q}
{cwd_wsl}
{cwd_wsl_q}
```

The WSL command should write to `{output_wsl}`.  That file will appear at the Windows path G4Cosmic expects.

## Optional converter command

If your real CORSIKA command writes a native CORSIKA output file first, wrap both steps in your own script or use a converter command:

```bash
export G4COSMIC_CORSIKA8_COMMAND='my_corsika_launcher --events {events} --native-output native.out'
export G4COSMIC_CORSIKA8_CONVERTER='python convert_to_g4cosmic_dat.py --input native.out --output {output_q}'
```

On Windows+WSL, use:

```powershell
$env:G4COSMIC_CORSIKA8_WSL_COMMAND = "my_corsika_launcher --events {events} --native-output native.out"
$env:G4COSMIC_CORSIKA8_WSL_CONVERTER = "python convert_to_g4cosmic_dat.py --input native.out --output {output_wsl_q}"
```

## Failure behavior

If real CORSIKA is not configured or fails, only the requested CORSIKA batch run fails.  G4Cosmic itself still builds and the other source modes still work.
