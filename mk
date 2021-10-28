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
    if [[ ! -d "${BUILD_DIR}" ]]; then
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
    # run python using
    # env DYLD_INSERT_LIBRARIES=/usr/local/opt/llvm/lib/clang/13.0.0/lib/darwin/libclang_rt.asan_osx_dynamic.dylib /usr/local/Cellar/python@3.9/3.9.7_1/Frameworks/Python.framework/Versions/3.9/Resources/Python.app/Contents/MacOS/Python
    BUILD_DIR="${BUILD_DEBUG_DIR}"
    # -Db_lundef=false see: https://github.com/mesonbuild/meson/issues/764
    SETUP_ARGS="-Doptimization=g -Db_sanitize=address -Db_lundef=false"
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
            export CXX=g++-11
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
