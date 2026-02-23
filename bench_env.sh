if [[ "${BASH_SOURCE[0]}" = "${0}" ]]; then
    >&2 echo "Remember: you need to run me as 'source bench_env.sh', not execute it!"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_PREFIX="${SCRIPT_DIR}/software/install"

__bench_old_set_opts="$(set +o)"
__bench_restore_shell_opts() {
    trap - RETURN
    if [[ -n "${__bench_old_set_opts-}" ]]; then
        eval "${__bench_old_set_opts}"
        unset __bench_old_set_opts
    fi
}
trap '__bench_restore_shell_opts' RETURN

set -euo pipefail

if ! command -v uv >/dev/null 2>&1; then
    >&2 echo "error: uv is required but was not found on PATH"
    return 1
fi

# Create or activate uv virtual environment.
if [ ! -d "${SCRIPT_DIR}/.venv" ]; then
    uv venv "${SCRIPT_DIR}/.venv"
fi
source "${SCRIPT_DIR}/.venv/bin/activate"

export PATH="${INSTALL_PREFIX}/bin:${PATH}"
export LD_LIBRARY_PATH="${INSTALL_PREFIX}/lib:${INSTALL_PREFIX}/lib64:${LD_LIBRARY_PATH:-}"

EXPECTED_MINIZINC="${INSTALL_PREFIX}/bin/minizinc"
if [ ! -x "${EXPECTED_MINIZINC}" ]; then
    >&2 echo "error: ${EXPECTED_MINIZINC} not found."
    >&2 echo "run ./software_install.sh first"
    return 1
fi

ACTIVE_MINIZINC="$(command -v minizinc || true)"
if [ "${ACTIVE_MINIZINC}" != "${EXPECTED_MINIZINC}" ]; then
    >&2 echo "error: active minizinc is '${ACTIVE_MINIZINC}', expected '${EXPECTED_MINIZINC}'"
    return 1
fi

SOLVERS_JSON="$(minizinc --solvers-json 2>/dev/null || true)"
if [ -z "${SOLVERS_JSON}" ]; then
    >&2 echo "error: failed to query minizinc --solvers-json"
    return 1
fi

if ! printf '%s\n' "${SOLVERS_JSON}" | grep -Fq "\"${INSTALL_PREFIX}/bin/fzn-gecode\""; then
    >&2 echo "error: minizinc --solvers-json does not reference ${INSTALL_PREFIX}/bin/fzn-gecode"
    return 1
fi
if ! printf '%s\n' "${SOLVERS_JSON}" | grep -Fq "\"${INSTALL_PREFIX}/bin/fzn-chuffed\""; then
    >&2 echo "error: minizinc --solvers-json does not reference ${INSTALL_PREFIX}/bin/fzn-chuffed"
    return 1
fi
