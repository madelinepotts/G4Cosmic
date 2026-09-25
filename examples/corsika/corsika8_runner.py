#!/usr/bin/env python3
"""Dispatch real CORSIKA 8 runs for G4Cosmic batch mode.

This script is intentionally a thin, cross-platform boundary between G4Cosmic
and a real CORSIKA installation.  G4Cosmic does not link CORSIKA directly; it
runs this script, and this script must leave a G4Cosmic text .dat shower file at
--output.

Backends:
  auto      Use external or WSL command templates from environment variables.
  external  Run G4COSMIC_CORSIKA8_COMMAND on the current OS.
  wsl       Run G4COSMIC_CORSIKA8_WSL_COMMAND inside WSL from Windows.
  toy       Use the bundled toy runner for plumbing tests only.

The command templates may use placeholders such as {events}, {output},
{primary}, {min_energy_gev}, {max_energy_gev}, {spectral_index}, and for WSL,
{output_wsl} and {cwd_wsl}.
"""

from __future__ import annotations

import argparse
import os
import platform
import shlex
import subprocess
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Optional


class SafeDict(dict):
    def __missing__(self, key: str) -> str:
        return "{" + key + "}"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run a real CORSIKA 8 wrapper and produce G4Cosmic .dat showers."
    )
    parser.add_argument("--events", type=int, required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--primary", default="proton")
    parser.add_argument("--energy-mode", choices=("powerLaw", "mono"), default="powerLaw")
    parser.add_argument("--min-energy-gev", type=float, default=1.0e3)
    parser.add_argument("--max-energy-gev", type=float, default=1.0e5)
    parser.add_argument("--energy-gev", type=float, default=1.0e4)
    parser.add_argument("--spectral-index", type=float, default=2.7)
    parser.add_argument("--min-zenith-deg", type=float, default=0.0)
    parser.add_argument("--max-zenith-deg", type=float, default=60.0)
    parser.add_argument("--min-azimuth-deg", type=float, default=0.0)
    parser.add_argument("--max-azimuth-deg", type=float, default=360.0)

    parser.add_argument(
        "--backend",
        choices=("auto", "external", "wsl", "toy"),
        default=os.environ.get("G4COSMIC_CORSIKA8_BACKEND", "auto"),
        help="Runner backend. Default: auto."
    )
    parser.add_argument(
        "--corsika-command",
        default=os.environ.get("G4COSMIC_CORSIKA8_COMMAND", ""),
        help="Native command template that must produce the G4Cosmic .dat output."
    )
    parser.add_argument(
        "--wsl-command",
        default=os.environ.get("G4COSMIC_CORSIKA8_WSL_COMMAND", ""),
        help="WSL command template that must produce the G4Cosmic .dat output."
    )
    parser.add_argument(
        "--converter-command",
        default=os.environ.get("G4COSMIC_CORSIKA8_CONVERTER", ""),
        help="Optional native converter template run after the CORSIKA command."
    )
    parser.add_argument(
        "--wsl-converter-command",
        default=os.environ.get("G4COSMIC_CORSIKA8_WSL_CONVERTER", ""),
        help="Optional WSL converter template run after the WSL CORSIKA command."
    )
    parser.add_argument(
        "--toy-fallback",
        action="store_true",
        default=os.environ.get("G4COSMIC_CORSIKA8_TOY_FALLBACK", "0") == "1",
        help="Allow fallback to the bundled toy runner when no real command is configured."
    )
    return parser.parse_args()


def is_windows() -> bool:
    return platform.system().lower().startswith("win")


def run_command(command: str, *, cwd: Optional[Path] = None) -> None:
    print(f"G4Cosmic CORSIKA runner: {command}", flush=True)
    completed = subprocess.run(command, shell=True, cwd=str(cwd) if cwd else None)
    if completed.returncode != 0:
        raise RuntimeError(f"command failed with exit code {completed.returncode}: {command}")


def run_wsl(command: str, distro: str = "") -> None:
    base: List[str] = ["wsl"]
    if distro:
        base += ["-d", distro]
    print(f"G4Cosmic CORSIKA WSL runner: {command}", flush=True)
    completed = subprocess.run(base + ["bash", "-lc", command])
    if completed.returncode != 0:
        raise RuntimeError(f"WSL command failed with exit code {completed.returncode}: {command}")


def wslpath(path: Path, distro: str = "") -> str:
    base: List[str] = ["wsl"]
    if distro:
        base += ["-d", distro]
    completed = subprocess.run(
        base + ["wslpath", "-a", str(path)],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return completed.stdout.strip()


def quote_bash(value: str) -> str:
    return shlex.quote(value)


def build_context(args: argparse.Namespace) -> Dict[str, str]:
    output_path = Path(args.output).resolve()
    cwd_path = Path.cwd().resolve()
    context: Dict[str, str] = {
        "events": str(args.events),
        "output": str(output_path),
        "primary": args.primary,
        "energy_mode": args.energy_mode,
        "min_energy_gev": f"{args.min_energy_gev:.17g}",
        "max_energy_gev": f"{args.max_energy_gev:.17g}",
        "energy_gev": f"{args.energy_gev:.17g}",
        "spectral_index": f"{args.spectral_index:.17g}",
        "min_zenith_deg": f"{args.min_zenith_deg:.17g}",
        "max_zenith_deg": f"{args.max_zenith_deg:.17g}",
        "min_azimuth_deg": f"{args.min_azimuth_deg:.17g}",
        "max_azimuth_deg": f"{args.max_azimuth_deg:.17g}",
        "cwd": str(cwd_path),
    }

    if is_windows():
        distro = os.environ.get("G4COSMIC_CORSIKA8_WSL_DISTRO", "")
        try:
            context["output_wsl"] = wslpath(output_path, distro)
            context["cwd_wsl"] = wslpath(cwd_path, distro)
        except Exception:
            # Do not fail early.  We only need WSL paths if the WSL backend is used.
            context["output_wsl"] = str(output_path)
            context["cwd_wsl"] = str(cwd_path)
    else:
        context["output_wsl"] = str(output_path)
        context["cwd_wsl"] = str(cwd_path)

    return context


def expand(template: str, context: Dict[str, str]) -> str:
    quoted = dict(context)
    for key, value in list(context.items()):
        quoted[f"{key}_q"] = shlex.quote(value)
    return template.format_map(SafeDict(quoted))


def run_toy(args: argparse.Namespace) -> None:
    script = Path(__file__).resolve().with_name("demo_external_corsika_runner.py")
    command = [
        sys.executable,
        str(script),
        "--events", str(args.events),
        "--output", args.output,
        "--primary", args.primary,
        "--energy-mode", args.energy_mode,
        "--min-energy-gev", str(args.min_energy_gev),
        "--max-energy-gev", str(args.max_energy_gev),
        "--energy-gev", str(args.energy_gev),
        "--spectral-index", str(args.spectral_index),
        "--min-zenith-deg", str(args.min_zenith_deg),
        "--max-zenith-deg", str(args.max_zenith_deg),
        "--min-azimuth-deg", str(args.min_azimuth_deg),
        "--max-azimuth-deg", str(args.max_azimuth_deg),
    ]
    print("G4Cosmic CORSIKA runner: using bundled toy fallback", flush=True)
    completed = subprocess.run(command)
    if completed.returncode != 0:
        raise RuntimeError(f"toy runner failed with exit code {completed.returncode}")


def validate_output(path: Path) -> None:
    if not path.exists():
        raise RuntimeError(f"runner did not create output file: {path}")
    non_comment_rows = 0
    with path.open("r", encoding="utf-8") as handle:
        for line_number, line in enumerate(handle, start=1):
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue
            columns = stripped.split()
            if len(columns) < 9:
                raise RuntimeError(
                    f"output file {path} has fewer than 9 columns on line {line_number}: {stripped}"
                )
            non_comment_rows += 1
            if non_comment_rows >= 1:
                break
    if non_comment_rows == 0:
        raise RuntimeError(f"output file {path} contains no particle rows")


def choose_backend(args: argparse.Namespace) -> str:
    if args.backend != "auto":
        return args.backend
    if args.corsika_command:
        return "external"
    if is_windows() and args.wsl_command:
        return "wsl"
    if args.toy_fallback:
        return "toy"
    return "auto"


def main() -> int:
    args = parse_args()
    output_path = Path(args.output).resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    context = build_context(args)
    backend = choose_backend(args)

    try:
        if backend == "external":
            if not args.corsika_command:
                raise RuntimeError("external backend requires G4COSMIC_CORSIKA8_COMMAND or --corsika-command")
            run_command(expand(args.corsika_command, context))
            if args.converter_command:
                run_command(expand(args.converter_command, context))
        elif backend == "wsl":
            if not is_windows():
                raise RuntimeError("wsl backend is only meaningful when launching from Windows")
            if not args.wsl_command:
                raise RuntimeError("wsl backend requires G4COSMIC_CORSIKA8_WSL_COMMAND or --wsl-command")
            distro = os.environ.get("G4COSMIC_CORSIKA8_WSL_DISTRO", "")
            run_wsl(expand(args.wsl_command, context), distro)
            if args.wsl_converter_command:
                run_wsl(expand(args.wsl_converter_command, context), distro)
        elif backend == "toy":
            run_toy(args)
        else:
            raise RuntimeError(
                "No real CORSIKA command is configured. Set G4COSMIC_CORSIKA8_COMMAND "
                "for native/Linux execution, set G4COSMIC_CORSIKA8_WSL_COMMAND for Windows+WSL, "
                "or set G4COSMIC_CORSIKA8_TOY_FALLBACK=1 for plumbing tests."
            )
        validate_output(output_path)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    print(f"G4Cosmic CORSIKA runner wrote {output_path}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
