# Stage 8 fixes

- Replaced long `/g4cosmic/corsika/command ...` macro examples with `/g4cosmic/corsika/runner` plus `/g4cosmic/corsika/runnerScript`.
- Avoids Geant4 UI parsing a long shell command as only `python`, which opened an interactive Python prompt on Windows.
- Added a guard that refuses to launch bare `python`, `python3`, or `py` as a CORSIKA command.
- Kept `/g4cosmic/corsika/command` as a legacy single-token command path, but the documented macros now use runner/runnerScript.
- Energy validation now checks mono energy only in `energyMode mono`; power-law mode only checks min/max energy.


## Multithreaded CORSIKA cache generation fix

The first Python command fix prevented Windows from opening an interactive Python prompt, but the external runner was still launched once per Geant4 worker thread in MT mode.  The CORSIKA generator now serializes cache generation and cache parsing with a process-wide Geant4 mutex.  In batch mode, the first worker generates the cache and later workers reuse the cache instead of launching duplicate runner processes.

## Removed stream mode

Stream mode was removed after testing because in Geant4 MT jobs each worker can independently request stream refills, which made the number of generated CORSIKA batches exceed the requested event count. The supported CORSIKA paths are now:

- `generationMode file`: read an existing G4Cosmic/CORSIKA text `.dat` shower list.
- `generationMode batch`: run an external CORSIKA 8 wrapper once to generate a cache, then consume one complete shower per Geant4 event.

The duplicate external-demo macros were also removed. The batch demo is the external-runner demo.

## Batch cache policy

Batch mode now regenerates its cache by default. The normal demos omit `/g4cosmic/corsika/reuseCache`, and the code default is `reuseCache = 0`. In Geant4 MT mode, the first worker generates the fresh cache and the other workers read that same process-local batch without rerunning the external command. Add `/g4cosmic/corsika/reuseCache 1` only when you intentionally want to reuse a cache file from an earlier run.
