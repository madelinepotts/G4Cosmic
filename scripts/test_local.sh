#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
THREADS="${2:-1}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

find_exe() {
  local name="$1"
  if [[ -x "${BUILD_DIR}/${name}" ]]; then
    printf '%s\n' "${BUILD_DIR}/${name}"
  elif [[ -x "${BUILD_DIR}/Release/${name}" ]]; then
    printf '%s\n' "${BUILD_DIR}/Release/${name}"
  elif [[ -x "${BUILD_DIR}/Release/${name}.exe" ]]; then
    printf '%s\n' "${BUILD_DIR}/Release/${name}.exe"
  else
    echo "Could not find executable ${name} under ${BUILD_DIR}. Build first." >&2
    exit 1
  fi
}

run_g4cosmic() {
  local exe="$1"
  local macro="$2"
  echo "Running ${exe} ${macro} ${THREADS}"
  "${exe}" "${macro}" "${THREADS}"
}

assert_exists() {
  local path="$1"
  if [[ ! -e "${path}" ]]; then
    echo "Expected output was not created: ${path}" >&2
    exit 1
  fi
}

WARPTRACK_EXE="$(find_exe g4cosmic_warptrack)"
BASIC_EXE="$(find_exe g4cosmic_basic)"

rm -f \
  basic_quick.root basic_quick.json \
  corsika_dat_file.root corsika_dat_file.json \
  corsika_batch.root corsika_batch.json corsika_cache.dat \
  corsika_batch_volume_acceptance.root corsika_batch_volume_acceptance.json corsika_cache_acceptance.dat \
  corsika8_runner.root corsika8_runner.json corsika8_generated_particles.dat

run_g4cosmic "${BASIC_EXE}" "./macros/basic_quick.mac"
assert_exists "basic_quick.root"
assert_exists "basic_quick.json"

run_g4cosmic "${WARPTRACK_EXE}" "./macros/corsika_dat_file_demo.mac"
assert_exists "corsika_dat_file.root"
assert_exists "corsika_dat_file.json"

run_g4cosmic "${WARPTRACK_EXE}" "./macros/corsika_batch_demo.mac"
assert_exists "corsika_batch.root"
assert_exists "corsika_batch.json"
assert_exists "corsika_cache.dat"

run_g4cosmic "${WARPTRACK_EXE}" "./macros/corsika_batch_volume_acceptance.mac"
assert_exists "corsika_batch_volume_acceptance.root"
assert_exists "corsika_batch_volume_acceptance.json"
assert_exists "corsika_cache_acceptance.dat"

OLD_TOY_FALLBACK="${G4COSMIC_CORSIKA8_TOY_FALLBACK-}"
export G4COSMIC_CORSIKA8_TOY_FALLBACK=1
run_g4cosmic "${WARPTRACK_EXE}" "./macros/corsika8_runner_template.mac"
assert_exists "corsika8_runner.root"
assert_exists "corsika8_runner.json"
assert_exists "corsika8_generated_particles.dat"
if [[ -n "${OLD_TOY_FALLBACK}" ]]; then
  export G4COSMIC_CORSIKA8_TOY_FALLBACK="${OLD_TOY_FALLBACK}"
else
  unset G4COSMIC_CORSIKA8_TOY_FALLBACK
fi

echo "G4Cosmic local regression tests passed."
