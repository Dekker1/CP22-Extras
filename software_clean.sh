#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/software/build"
INSTALL_DIR="${SCRIPT_DIR}/software/install"

echo "Removing ${BUILD_DIR}"
rm -rf "${BUILD_DIR}"

echo "Removing ${INSTALL_DIR}"
rm -rf "${INSTALL_DIR}"

echo "Clean complete."
