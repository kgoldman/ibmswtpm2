# ibmswtpm2
This project is an implementation of the TCG TPM 2.0 specification. It is based on the TPM specification Parts 3 and 4 source code donated by Microsoft, with additional files to complete the implementation.

See the companion IBM TSS at https://sourceforge.net/projects/ibmtpm20tss/

# Building
## Dependencies
* For all platforms
* * cmake - http://cmake.org
* * openssl ( See conan instructions for Mac and Windows easy deps )
* Mac ( XCode and brew clang/gcc ) and Windows ( MSVC )
* * conan - http://conan.io

## Easy openssl dependency for Mac and Windows with Conan
* After install conan do:
```
mkdir build
cd build
conan profile new default --detect
conan install ..
```
On Windows Conan has preset binary dependencies for openssl 64 bits and MSVC 16 and up
If you get an error of non availability of openssl for your old MSVC build, you can execute
```
conan install .. --build *
```
This will rebuild the dependencies for your compiler

## Simple build instructions for Linux and Mac
Building is straightforward, as long you installed all dependencies:
```
mkdir build
cd build
cmake ..
make
```

## Simple build instructions for Windows Visual Studio
Build for Windows Visual Studio must be "almost" straightforward.
Builds are tested with MSVC
```
mkdir build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## Install
* Linux
On Linux, binary is installed in default sbin dir and an extra systemd service file is installed to start and stop the service.
No firewall exclusion files are included, so remember to open the required ports

* Mac and Windows
Install works, but only mac is guaranteed to end up in proper path

# Troubleshoot

## Windows has symbols / 32/64 bits mismatch
Only 64 bits compilations are tested with conan deps. If you installed openssl by your own, some possible issues:
* Your compiler is 64 bits and you have 32 bits deps, configure this way:
```
cmake -DCMAKE_BUILD_TYPE=Release -A win32 ..
```

## Mac You are using gcc or clang from Homebrew,ad Connan detected XCode Clang as default compiler
* Read profile section of Conan - https://docs.conan.io/en/latest/reference/profiles.html
