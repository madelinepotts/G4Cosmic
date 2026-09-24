#!/usr/bin/env bash
# Install CRY into G4Cosmic/external/cry.
#
# This follows the original WarpTrack layout: CRY lives outside the simulation
# source under external/cry, and CMake builds CRY from its src/*.cc files.
#
# Usage from the repository root:
#   ./scripts/install_cry.sh
#
# Optional:
#   CRY_URL=https://example.com/cry.zip ./scripts/install_cry.sh
#   ./scripts/install_cry.sh /path/to/external/cry

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DESTINATION="${1:-${REPO_ROOT}/external/cry}"
URL="${CRY_URL:-https://github.com/PKMuon/cry/archive/refs/heads/master.zip}"

if [[ -f "${DESTINATION}/src/CRYGenerator.cc" && "${G4COSMIC_FORCE_CRY_INSTALL:-0}" != "1" ]]; then
    echo "CRY already exists at ${DESTINATION}"
    echo "Set G4COSMIC_FORCE_CRY_INSTALL=1 to reinstall."
    exit 0
fi

TMP_ROOT="${REPO_ROOT}/.download"
TMP_DIR="${TMP_ROOT}/cry"
ZIP_PATH="${TMP_ROOT}/cry.zip"

rm -rf "${TMP_DIR}"
mkdir -p "${TMP_DIR}"
mkdir -p "$(dirname "${ZIP_PATH}")"

if command -v curl >/dev/null 2>&1; then
    echo "Downloading CRY from: ${URL}"
    curl -L "${URL}" -o "${ZIP_PATH}"
elif command -v wget >/dev/null 2>&1; then
    echo "Downloading CRY from: ${URL}"
    wget -O "${ZIP_PATH}" "${URL}"
else
    echo "ERROR: install curl or wget, or download CRY manually into external/cry." >&2
    exit 1
fi

echo "Extracting CRY..."
python3 - "${ZIP_PATH}" "${TMP_DIR}" <<'PYZIP'
import sys
import zipfile
zip_path, out_dir = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(zip_path) as z:
    z.extractall(out_dir)
PYZIP

CRY_ROOT="$(find "${TMP_DIR}" -path '*/src/CRYGenerator.cc' -type f -print -quit | sed 's#/src/CRYGenerator.cc$##')"

if [[ -z "${CRY_ROOT}" ]]; then
    echo "ERROR: could not find src/CRYGenerator.cc in downloaded CRY archive." >&2
    exit 1
fi

rm -rf "${DESTINATION}"
mkdir -p "$(dirname "${DESTINATION}")"
cp -R "${CRY_ROOT}" "${DESTINATION}"

if [[ ! -f "${DESTINATION}/src/CRYGenerator.cc" ]]; then
    echo "ERROR: CRY install failed: missing src/CRYGenerator.cc at ${DESTINATION}" >&2
    exit 1
fi

if [[ ! -d "${DESTINATION}/data" ]]; then
    echo "ERROR: CRY install failed: missing data directory at ${DESTINATION}/data" >&2
    exit 1
fi

echo "CRY installed at: ${DESTINATION}"
echo "You can now configure G4Cosmic with CMake."
