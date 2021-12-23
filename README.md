# Acmacs-E

## Dependencies

- clang-13 or g++-11
- ninja
  - macOS: brew install ninja
  - Ubuntu: sudo apt install ninja-build
- meson
  - macOS: brew install meson
  - Ubuntu: sudo apt install meson
- cmake 3.18+ (to build lexy)
  - macOS: brew install cmake
  - Ubuntu: sudo apt install cmake
     `if older version of cmake is installed, use snap
     (sudo apt install snapd)
     sudo snap install cmake
     sudo ln -s /snap/bin/cmake /usr/local/bin`
- zlib (brew install zlib)
- libbz2
- liblzma
  - macOS: brew install xz
  - Ubuntu: apt install liblzma-dev)

## Build

./mk
meson compile -C build
