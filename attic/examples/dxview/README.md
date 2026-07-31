# DirectX glTF Viewer

## Overview

This project was motivated by a lack of sample code demonstrating the graphics API agnostic nature of the glTF specification. The sample code is written using modern C++ and DirectX 12 for the client application.

## Features

* [x] DirectX 12
* [ ] Loader
* [ ] Animation
* [ ] Morph Target
* [ ] Physical Base Rendering
* [ ] Environment Map

## Dependencies

* [CMake](https://github.com/Kitware/CMake)
* [Vcpkg](https://github.com/Microsoft/vcpkg)
* [GLFW](https://github.com/glfw/glfw)
* [spdlog](https://github.com/gabime/spdlog)

## Building

### Install dependencies

```
vcpkg install glfw3:x64-windows
vcpkg install spdlog:x64-windows
```

### Generate Project Files

```
mkdir build
cmake . -B build -DCMAKE_TOOLCHAIN_FILE=${VCPKG_DIR}/script/buildsystem/vcpkg.cmake
```
