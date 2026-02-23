if [[ "${BASH_SOURCE[0]}" = "${0}" ]]; then
    >&2 echo "Remember: you need to run me as 'source bench_env.sh', not execute it!"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_PREFIX="${SCRIPT_DIR}/software/install"

set -euo pipefail
trap 'return 1' ERR

if ! command -v uv >/dev/null 2>&1; then
    >&2 echo "error: uv is required but was not found on PATH"
    return 1
fi

# Create or activate uv virtual environment.
if [ ! -d "${SCRIPT_DIR}/.venv" ]; then
    uv venv "${SCRIPT_DIR}/.venv"
fi
source "${SCRIPT_DIR}/.venv/bin/activate"

# Build and install local subtree software into software/install.
cmake -S "${SCRIPT_DIR}/software/gecode" -B "${SCRIPT_DIR}/software/gecode/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"
cmake --build "${SCRIPT_DIR}/software/gecode/build" --config Release
cmake --install "${SCRIPT_DIR}/software/gecode/build" --config Release

cmake -S "${SCRIPT_DIR}/software/minizinc" -B "${SCRIPT_DIR}/software/minizinc/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"
cmake --build "${SCRIPT_DIR}/software/minizinc/build" --config Release
cmake --install "${SCRIPT_DIR}/software/minizinc/build" --config Release

cmake -S "${SCRIPT_DIR}/software/chuffed" -B "${SCRIPT_DIR}/software/chuffed/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"
cmake --build "${SCRIPT_DIR}/software/chuffed/build" --config Release
cmake --install "${SCRIPT_DIR}/software/chuffed/build" --config Release

export PATH="${INSTALL_PREFIX}/bin:${PATH}"
export LD_LIBRARY_PATH="${INSTALL_PREFIX}/lib:${INSTALL_PREFIX}/lib64:${LD_LIBRARY_PATH:-}"
