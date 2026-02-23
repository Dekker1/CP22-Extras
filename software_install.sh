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
echo "Cleaning previous install directory"
rm -rf "${INSTALL_PREFIX}"
mkdir -p "${INSTALL_PREFIX}"
mkdir -p "${BUILD_ROOT}"

build_and_install() {
    local name="$1"
    shift || true
    local source_dir="${SCRIPT_DIR}/software/${name}"
    local build_dir="${BUILD_ROOT}/${name}"

    echo "==> configuring ${name}"
    cmake -S "${source_dir}" -B "${build_dir}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
        -DCMAKE_PREFIX_PATH="${INSTALL_PREFIX}" \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        "$@"

    echo "==> building ${name}"
    cmake --build "${build_dir}" --config Release --parallel

    echo "==> installing ${name}"
    cmake --install "${build_dir}" --config Release
}

build_and_install_chuffed_aekh() {
    local name="chuffed_aekh"
    local minizinc_share_dir="${INSTALL_PREFIX}/share/minizinc"
    local default_bin="${INSTALL_PREFIX}/bin/fzn-chuffed"
    local aekh_bin="${INSTALL_PREFIX}/bin/fzn-chuffed_aekh"
    local solvers_dir="${minizinc_share_dir}/solvers"
    local default_solver_msc="${solvers_dir}/chuffed.msc"
    local aekh_solver_msc="${solvers_dir}/chuffed_aekh.msc"
    local default_mznlib_dir="${minizinc_share_dir}/chuffed"
    local aekh_mznlib_dir="${minizinc_share_dir}/chuffed_aekh"
    local aekh_cumulative_mzn="${aekh_mznlib_dir}/fzn_cumulative.mzn"

    build_and_install "${name}" -DCMAKE_POLICY_VERSION_MINIMUM=3.5

    mkdir -p "${INSTALL_PREFIX}/bin" "${solvers_dir}"
    mv -f "${default_bin}" "${aekh_bin}"
    mv -f "${default_solver_msc}" "${aekh_solver_msc}"
    rm -rf "${aekh_mznlib_dir}"
    mv -f "${default_mznlib_dir}" "${aekh_mznlib_dir}"
    sed -i.bak \
        -e 's/"id": "[^"]*"/"id": "org.chuffed.chuffed_aekh"/' \
        -e 's/"name": "[^"]*"/"name": "Chuffed AlexEk"/' \
        -e 's#"mznlib": "[^"]*"#"mznlib": "../chuffed_aekh"#' \
        -e 's/fzn-chuffed"/fzn-chuffed_aekh"/' \
        "${aekh_solver_msc}"
    rm -f "${aekh_solver_msc}.bak"

    # Patch cumulative builtin signature for FlatZinc compatibility:
    # pass a flattened calendar and declare builtin arg as 1D.
    if [ -f "${aekh_cumulative_mzn}" ]; then
        sed -i.bak \
            -e 's/chuffed_cumulative_cal(s, d, r, b, index1, index2, cal, tc, rho, resCal)/chuffed_cumulative_cal(s, d, r, b, index1, index2, array1d(cal), tc, rho, resCal)/' \
            -e 's/array\[int,int\] of int: cal, array\[int\] of int: tc/array[int] of int: cal, array[int] of int: tc/' \
            "${aekh_cumulative_mzn}"
        rm -f "${aekh_cumulative_mzn}.bak"
    fi
}

# Build in dependency order so downstream projects can discover prior installs.
build_and_install "gecode"
build_and_install "minizinc"
if [ "${INSTALL_CHUFFED_AEKH:-1}" = "1" ]; then
    build_and_install_chuffed_aekh
fi
build_and_install "chuffed"

echo "Install complete."
