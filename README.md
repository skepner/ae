# Acmacs-E

## Dependencies (general description)

- clang-14 or g++-11
- ninja
- meson 0.60+
- cmake 3.18+ (to build lexy)
- libomp
- brotli
- zlib
- libbz2
- liblzma
- catch2

## Installing dependencies on macOS

- install homebrew https://brew.sh
- brew install llvm ninja meson libomp cmake brotli zlib xz gnu-time catch2

## Installing dependencies on Ubuntu

- sudo apt install g++-11 ninja-build meson cmake libbrotli-dev liblzma-dev

- Check version of meson: meson --version
  If version older than 0.60: pip3 install --user meson

- Check version of cmake: cmake --version
  If version is older than 3.18: ?

## Build

./mk
meson compile -C build
