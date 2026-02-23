#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_PREFIX="${SCRIPT_DIR}/software/install"
BUILD_ROOT="${SCRIPT_DIR}/software/build"

if ! command -v cmake >/dev/null 2>&1; then
    >&2 echo "error: cmake is required but was not found on PATH"
    exit 1
fi

echo "Installing software into ${INSTALL_PREFIX}"
mkdir -p "${BUILD_ROOT}"

build_and_install() {
    local name="$1"
    local source_dir="${SCRIPT_DIR}/software/${name}"
    local build_dir="${BUILD_ROOT}/${name}"

    echo "==> configuring ${name}"
    cmake -S "${source_dir}" -B "${build_dir}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
        -DCMAKE_PREFIX_PATH="${INSTALL_PREFIX}" \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON

    echo "==> building ${name}"
    cmake --build "${build_dir}" --config Release --parallel

    echo "==> installing ${name}"
    cmake --install "${build_dir}" --config Release
}

# Build in dependency order so downstream projects can discover prior installs.
build_and_install "gecode"
build_and_install "minizinc"
build_and_install "chuffed"

echo "Install complete."
