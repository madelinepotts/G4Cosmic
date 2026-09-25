#!/usr/bin/env bash
# Best-effort installer for CORSIKA 8 under G4Cosmic/external/corsika8.
#
# CORSIKA is a full external air-shower simulator with its own dependencies.
# This script keeps it out of the G4Cosmic source tree and installs it into
# external/corsika8 so G4Cosmic macros can call it through
# /g4cosmic/corsika/inputMode command.
#
# Usage from the repository root:
#   chmod +x ./scripts/install_corsika8.sh
#   ./scripts/install_corsika8.sh
#
# Default tag: corsika8-v1.0-beta1, the first public beta release.
#
# Optional:
#   CORSIKA8_REPO=https://gitlab.iap.kit.edu/AirShowerPhysics/corsika.git ./scripts/install_corsika8.sh
#   CORSIKA8_TAG=corsika8-v1.0-beta1 ./scripts/install_corsika8.sh
#   G4COSMIC_FORCE_CORSIKA8_INSTALL=1 ./scripts/install_corsika8.sh
#   ./scripts/install_corsika8.sh /path/to/external/corsika8

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DESTINATION="${1:-${REPO_ROOT}/external/corsika8}"
REPO_URL="${CORSIKA8_REPO:-https://gitlab.iap.kit.edu/AirShowerPhysics/corsika.git}"
TAG="${CORSIKA8_TAG:-corsika8-v1.0-beta1}"
BUILD_TYPE="${CORSIKA8_BUILD_TYPE:-RelWithDebInfo}"
JOBS="${CORSIKA8_JOBS:-}"

SRC_DIR="${DESTINATION}/src"
BUILD_DIR="${DESTINATION}/build"
INSTALL_DIR="${DESTINATION}/install"

if [[ -d "${INSTALL_DIR}" && "${G4COSMIC_FORCE_CORSIKA8_INSTALL:-0}" != "1" ]]; then
    echo "CORSIKA 8 install directory already exists at ${INSTALL_DIR}"
    echo "Set G4COSMIC_FORCE_CORSIKA8_INSTALL=1 to reinstall."
    exit 0
fi

if ! command -v git >/dev/null 2>&1; then
    echo "ERROR: git is required to install CORSIKA 8." >&2
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "ERROR: cmake is required to build CORSIKA 8." >&2
    exit 1
fi

if [[ "${G4COSMIC_FORCE_CORSIKA8_INSTALL:-0}" == "1" ]]; then
    rm -rf "${DESTINATION}"
fi

mkdir -p "${DESTINATION}"

if [[ ! -d "${SRC_DIR}/.git" ]]; then
    echo "Cloning CORSIKA 8 from ${REPO_URL}"
    if [[ -n "${TAG}" ]]; then
        git clone --recursive --branch "${TAG}" "${REPO_URL}" "${SRC_DIR}"
    else
        git clone --recursive "${REPO_URL}" "${SRC_DIR}"
    fi
else
    echo "Using existing CORSIKA 8 source tree at ${SRC_DIR}"
    git -C "${SRC_DIR}" submodule update --init --recursive
fi

cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"

if [[ -n "${JOBS}" ]]; then
    cmake --build "${BUILD_DIR}" --parallel "${JOBS}"
else
    cmake --build "${BUILD_DIR}" --parallel
fi

cmake --install "${BUILD_DIR}"

cat <<MSG
CORSIKA 8 installed under: ${DESTINATION}

Next step: set your G4Cosmic macro command to call the CORSIKA application or
wrapper that writes G4Cosmic's text shower format, for example:

/g4cosmic/corsika/inputMode command
/g4cosmic/corsika/file corsika_generated_particles.dat
/g4cosmic/corsika/command <your-corsika-wrapper> --events __events__ --output __output__
/g4cosmic/corsika/apply
MSG
