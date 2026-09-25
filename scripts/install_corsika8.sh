#!/usr/bin/env bash
# Stage or build CORSIKA 8 under G4Cosmic/external/corsika8.
#
# G4Cosmic does not link against CORSIKA 8 directly.  Batch mode calls an
# external runner/wrapper that writes G4Cosmic's text .dat shower format.
# This helper keeps the optional CORSIKA tree out of the G4Cosmic source tree.

set -euo pipefail

usage() {
    cat <<'MSG'
Usage:
  scripts/install_corsika8.sh [options] [destination]

Options:
  --stage-only      Clone/update the CORSIKA 8 source tree but do not build.
  --build           Configure/build/install CORSIKA 8 after staging.
  --force           Remove the destination before staging/building.
  --tag TAG         Git tag or branch to checkout. Default: corsika8-v1.0-beta1.
  --repo URL        CORSIKA repository URL.
  --jobs N          Build parallelism.
  --help            Show this help.

Environment overrides:
  CORSIKA8_REPO
  CORSIKA8_TAG
  CORSIKA8_BUILD_TYPE
  CORSIKA8_BUILD=0|1
  CORSIKA8_JOBS
  G4COSMIC_FORCE_CORSIKA8_INSTALL=0|1
MSG
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DESTINATION="${REPO_ROOT}/external/corsika8"
REPO_URL="${CORSIKA8_REPO:-https://gitlab.iap.kit.edu/AirShowerPhysics/corsika.git}"
TAG="${CORSIKA8_TAG:-corsika8-v1.0-beta1}"
BUILD_TYPE="${CORSIKA8_BUILD_TYPE:-RelWithDebInfo}"
JOBS="${CORSIKA8_JOBS:-}"
BUILD="${CORSIKA8_BUILD:-0}"
FORCE="${G4COSMIC_FORCE_CORSIKA8_INSTALL:-0}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --stage-only)
            BUILD=0
            shift
            ;;
        --build)
            BUILD=1
            shift
            ;;
        --force)
            FORCE=1
            shift
            ;;
        --tag)
            TAG="$2"
            shift 2
            ;;
        --repo)
            REPO_URL="$2"
            shift 2
            ;;
        --jobs)
            JOBS="$2"
            shift 2
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            DESTINATION="$1"
            shift
            ;;
    esac
done

SRC_DIR="${DESTINATION}/src"
BUILD_DIR="${DESTINATION}/build"
INSTALL_DIR="${DESTINATION}/install"

command -v git >/dev/null 2>&1 || { echo "ERROR: git is required to stage CORSIKA 8." >&2; exit 1; }
if [[ "${BUILD}" == "1" ]]; then
    command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake is required to build CORSIKA 8." >&2; exit 1; }
fi

if [[ -d "${INSTALL_DIR}" && "${FORCE}" != "1" && "${BUILD}" == "1" ]]; then
    echo "CORSIKA 8 install directory already exists at ${INSTALL_DIR}"
    echo "Use --force or G4COSMIC_FORCE_CORSIKA8_INSTALL=1 to reinstall."
    exit 0
fi

if [[ "${FORCE}" == "1" ]]; then
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

if [[ "${BUILD}" == "1" ]]; then
    cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"

    if [[ -n "${JOBS}" ]]; then
        cmake --build "${BUILD_DIR}" --parallel "${JOBS}"
    else
        cmake --build "${BUILD_DIR}" --parallel
    fi

    cmake --install "${BUILD_DIR}"
    echo "CORSIKA 8 installed under: ${INSTALL_DIR}"
else
    echo "CORSIKA 8 source staged under: ${SRC_DIR}"
    echo "No build was attempted. Use --build or CORSIKA8_BUILD=1 on a supported platform."
fi

cat <<MSG

G4Cosmic batch mode calls an external runner/wrapper that writes G4Cosmic's text shower format:

/g4cosmic/corsika/generationMode batch
/g4cosmic/corsika/cacheFile corsika_generated_particles.dat
/g4cosmic/corsika/runner python
/g4cosmic/corsika/runnerScript examples/corsika/corsika8_runner.py
/g4cosmic/corsika/apply

The runner is cross-platform.  Configure a real CORSIKA command through
G4COSMIC_CORSIKA8_COMMAND or, on Windows+WSL, G4COSMIC_CORSIKA8_WSL_COMMAND.
MSG
