#!/usr/bin/env python3
"""Toy CORSIKA-8-runner stand-in for testing G4Cosmic batch mode.

This is not a physics shower simulation. It mimics the *interface* expected from
an installed CORSIKA 8 application or wrapper: generate a batch of showers and
write a G4Cosmic text .dat shower list:

    event_id particle_id x y z px py pz time

Replace this script with a real CORSIKA 8 application wrapper/converter that
honors the same command-line options and writes the same interchange file.
"""

from __future__ import annotations

import argparse
import math
import random
from pathlib import Path

CORSIKA_IDS = {
    "gamma": 1,
    "e+": 2,
    "positron": 2,
    "e-": 3,
    "electron": 3,
    "mu+": 5,
    "mu-": 6,
    "neutron": 13,
    "proton": 14,
}


def sample_power_law(rng: random.Random, emin: float, emax: float, gamma: float) -> float:
    """Sample dN/dE proportional to E^-gamma over [emin, emax]."""
    if emax <= emin:
        return emin
    u = rng.random()
    if abs(gamma - 1.0) < 1.0e-12:
        return emin * (emax / emin) ** u
    a = 1.0 - gamma
    return (emin ** a + u * (emax ** a - emin ** a)) ** (1.0 / a)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--events", type=int, default=100)
    parser.add_argument("--output", required=True)
    parser.add_argument("--seed", type=int, default=12345)
    parser.add_argument("--primary", default="proton")
    parser.add_argument("--energy-mode", choices=["powerLaw", "mono"], default="powerLaw")
    parser.add_argument("--min-energy-gev", type=float, default=1.0e3)
    parser.add_argument("--max-energy-gev", type=float, default=1.0e5)
    parser.add_argument("--energy-gev", type=float, default=1.0e4)
    parser.add_argument("--spectral-index", type=float, default=2.7)
    parser.add_argument("--min-zenith-deg", type=float, default=0.0)
    parser.add_argument("--max-zenith-deg", type=float, default=60.0)
    parser.add_argument("--min-azimuth-deg", type=float, default=0.0)
    parser.add_argument("--max-azimuth-deg", type=float, default=360.0)
    args = parser.parse_args()

    rng = random.Random(args.seed)
    output = Path(args.output)
    if output.parent != Path(""):
        output.parent.mkdir(parents=True, exist_ok=True)

    primary_id = CORSIKA_IDS.get(args.primary, 14)
    theta_min = math.radians(args.min_zenith_deg)
    theta_max = math.radians(args.max_zenith_deg)
    phi_min = math.radians(args.min_azimuth_deg)
    phi_max = math.radians(args.max_azimuth_deg)

    with output.open("w", encoding="utf-8") as f:
        f.write("# G4Cosmic CORSIKA text .dat shower list\n")
        f.write("# event_id corsika_id x_m y_m z_m px_GeV py_GeV pz_GeV time_ns\n")
        f.write(f"# primary={args.primary} energyMode={args.energy_mode} spectralIndex={args.spectral_index}\n")
        for event_id in range(args.events):
            if args.energy_mode == "mono":
                primary_energy_gev = args.energy_gev
            else:
                primary_energy_gev = sample_power_law(
                    rng,
                    max(args.min_energy_gev, 1.0e-12),
                    max(args.max_energy_gev, args.min_energy_gev),
                    args.spectral_index,
                )

            # Toy observation-level multiplicity. A real CORSIKA 8 wrapper should
            # use the generated shower particles instead.
            multiplicity = 2 + rng.randrange(7)
            core_x = rng.uniform(-0.6, 0.6)
            core_y = rng.uniform(-0.6, 0.6)
            for particle_index in range(multiplicity):
                if particle_index == 0:
                    corsika_id = primary_id
                    frac = 0.35
                else:
                    corsika_id = rng.choice([1, 2, 3, 5, 6, 13, 14])
                    frac = 10.0 ** rng.uniform(-4.0, -0.7)

                energy_gev = max(primary_energy_gev * frac, 1.0e-6)
                x = core_x + rng.gauss(0.0, 0.15)
                y = core_y + rng.gauss(0.0, 0.15)
                z = 1.9

                cos_theta_min = math.cos(theta_min)
                cos_theta_max = math.cos(theta_max)
                cos_theta = rng.uniform(cos_theta_max, cos_theta_min)
                theta = math.acos(cos_theta)
                phi = rng.uniform(phi_min, phi_max)

                px = energy_gev * math.sin(theta) * math.cos(phi)
                py = energy_gev * math.sin(theta) * math.sin(phi)
                pz = -energy_gev * math.cos(theta)
                time = rng.uniform(0.0, 50.0)
                f.write(
                    f"{event_id} {corsika_id} {x:.8g} {y:.8g} {z:.8g} "
                    f"{px:.8g} {py:.8g} {pz:.8g} {time:.8g}\n"
                )

    print(f"wrote {args.events} toy CORSIKA-like shower(s) to {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
