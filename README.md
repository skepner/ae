# Acmacs-E

# Installation instructions 16/10/24 MacOS: ARM64

## Dependencies
- brotli ~= 1.1
- catch2 ~= 3.7
- cmake ~= 3.30
- gnu-time ~= 1.9
- libomp ~= 19.1 (may be included on system)
- llvm ~= 19.1 (clang-18/ Apple Clang 16.0) (may be included on system)
- meson ~= 1.5
- mypy ~= 1.12
- ninja ~= 1.12
- pyenv ~= 2.4
- python ~= 3.13
- unidecode ~= 1.3
- zlib ~= 1.3 (may be included on system)


As of 16/10/24 these versions result in AE building without error. While brew is called with a generic package name, in future version may need to be specified if building fails

## Installing dependencies

To install brew follow the instructions on this page: [Homebrew installation page](https://brew.sh)

Dependencies managed by brew can me installed by running the following line in terminal:
```
brew install brotli catch2 cmake gnu-time libomp llvm meson mypy ninja pyenv zlib
```

### Python and unidecode

This uses pyenv to install python.

To install pyenv run the following in terminal (nb: previous step includes pyenv install so if that has been run you can skip installing it again):
```
brew install pyenv
```
To finish setting up pyenv you need to modify your .zshrc (macOS) (a copy of zshrc)

To open .zshrc:
```
nano ~/.zshrc
```
Add the following lines then save and exit:
```
if command -v pyenv 1>/dev/null 2>&1; then
    eval "$(pyenv init -)"
fi
```
Then run:
```
source ~/.zshrc
```

To install and set python version:
```
pyenv install 3.13 && pyenv local 3.13
```

To install unidecode:
```pip3 install unidecode```

## Building ae

To build ae run the following line n temrinal from the ae directory:
```
./mk
```

### If build fails
Delete the build directory, empty the trash and attempt to build again

## Getting python and system to find ae
Add the following lines to ~/.zshrc (where the paths direct to where ae is), then run ```source ./zshrc```
```
PYTHONPATH=/Users/${USER}/Desktop/pipeline/ae/build:/Users/${USER}/Desktop/pipeline/ae/py:$PYTHONPATH
export PYTHONPATH

PATH=$AE_ROOT/bin:$PATH
export $PATH
```


# Old install instructions


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
- pip3 install mypy

## Installing dependencies on Ubuntu

- sudo apt install g++-11 ninja-build meson cmake libbrotli-dev liblzma-dev

- Check version of meson: meson --version
  If version older than 0.60: pip3 install --user meson

- Check version of cmake: cmake --version
  If version is older than 3.18: ?

## Build

./mk
meson compile -C build
