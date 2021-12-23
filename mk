#! /bin/bash

BUILD_DEFAULT_DIR=build
BUILD_DEBUG_DIR=build.debug
BUILD_DIR="${BUILD_DEFAULT_DIR}"
SETUP_ARGS=-Doptimization=3
BUILT=NO

# ----------------------------------------------------------------------

build()
{
    trap 'fail "build failed"' ERR
    if [[ ! -d "${BUILD_DIR}" || ! -f "${BUILD_DIR}/meson-private/coredata.dat" ]]; then
        rm -rf "${BUILD_DIR}"
        find_compiler
        meson setup "${BUILD_DIR}" -Ddebug=true ${SETUP_ARGS}
        # --buildtype debugoptimized
        # -Dcpp_args=-O3
    else
        find_mk_time
    fi
    ${MK_TIME} meson compile -C "${BUILD_DIR}"
    BUILT=YES
}

# ----------------------------------------------------------------------

build_default()
{
    build
}

# ----------------------------------------------------------------------

build_debug()
{
    # run python using ~/bin/python-address-sanitizer
    BUILD_DIR="${BUILD_DEBUG_DIR}"
    SETUP_ARGS="-Doptimization=g -Db_sanitize=address"
    if [[ $(uname) == "Darwin" ]]; then
        # -Db_lundef=false see: https://github.com/mesonbuild/meson/issues/764
        SETUP_ARGS="${SETUP_ARGS} -Db_lundef=false"
    fi
    export CPPFLAGS=-fno-omit-frame-pointer
    build
}

# ----------------------------------------------------------------------

run_test()
{
    meson test -C "${BUILD_DIR}" --print-errorlogs
}

# ----------------------------------------------------------------------

find_compiler()
{
    case $(uname) in
        Darwin)
            export CXX=/usr/local/opt/llvm/bin/clang++
            MK_TIME=gtime
            ;;
        Linux)
            export CXX=$(which g++-11)
            MK_TIME=time
            ;;
        *)
            fail "Unsupported platform: $(name)"
    esac
    if [[ ! -x "${CXX}" ]]; then
        fail "Compiler not found: ${CXX}"
    fi
}

# ----------------------------------------------------------------------

find_mk_time()
{
    case $(uname) in
        Darwin)
            MK_TIME=gtime
            ;;
        Linux)
            MK_TIME=time
            ;;
        *)
            fail "Unsupported platform: $(name)"
    esac
}

# ----------------------------------------------------------------------

clean()
{
    remove "${BUILD_DEFAULT_DIR}" "${BUILD_DEBUG_DIR}"
}

cleanclean()
{
    clean
    for fn in $(cat subprojects/.gitignore); do
        remove subprojects/${fn}
    done
}

# ----------------------------------------------------------------------

remove()
{
    echo "rm -rf $@"
    rm -rf "$@"
}

# ----------------------------------------------------------------------

fail()
{
    echo "> ERROR: $@" >&2
    exit 1
}

# ----------------------------------------------------------------------

trap 'fail "build failed"' ERR
cd "$(dirname $0)"
for arg in "$@"; do
    case "${arg}" in
        clean)
            clean
            ;;
        cleanclean)
            cleanclean
            ;;
        debug)
            build_debug
            ;;
        test)
            run_test
            ;;
        -h)
            fail "Usage: $0 [-h] [clean] [debug] [test]"
            ;;
        *)
            fail "Unknown arg: ${arg}"
    esac
done
if [ $# == 0 ]; then
    build_default
fi
