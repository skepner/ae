# Acmacs-E

## Dependencies

- clang-13 or g++-11
- ninja
  - macOS: brew install ninja
  - Ubuntu: sudo apt install ninja-build
- meson 0.60+
  - macOS: brew install meson
  - Ubuntu: sudo apt install meson
     ```
     if older version of meson is installed by apt,
     ```
- cmake 3.18+ (to build lexy)
  - macOS: brew install cmake
  - Ubuntu: sudo apt install cmake
     ```
     if older version of cmake is installed by apt
     pip3 install --user meson
     ```
- zlib (brew install zlib)
- libbz2
- liblzma
  - macOS: brew install xz
  - Ubuntu: apt install liblzma-dev)

## Build

./mk
meson compile -C build
