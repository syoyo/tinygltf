# Header only C++ tiny glTF library(loader/saver).

`TinyGLTF` is a header only C++ glTF 2.0 https://github.com/KhronosGroup/glTF library.

## Status

Work in process(`devel` branch). Very near to release, but need more tests and examples.

## Builds

[![Build Status](https://travis-ci.org/syoyo/tinygltf.svg?branch=devel)](https://travis-ci.org/syoyo/tinygltf)

[![Build status](https://ci.appveyor.com/api/projects/status/warngenu9wjjhlm8?svg=true)](https://ci.appveyor.com/project/syoyo/tinygltf)

## Features

* Written in portable C++. C++-03 with STL dependency only.
  * [x] macOS + clang(LLVM)
  * [x] iOS + clang
  * [x] Linux + gcc/clang
  * [x] Windows + MinGW
  * [x] Android + CrystaX(NDK drop-in replacement) GCC
  * [x] Web using Emscripten(LLVM)
* Moderate parsing time and memory consumption.
* glTF specification v2.0.0
  * [x] ASCII glTF
  * [x] Binary glTF(GLB)
  * [x] PBR material description
* Buffers
  * [x] Parse BASE64 encoded embedded buffer fata(DataURI).
  * [x] Load `.bin` file.
* Image(Using stb_image)
  * [x] Parse BASE64 encoded embedded image fata(DataURI).
  * [x] Load external image file.
  * [x] PNG(8bit only)
  * [x] JPEG(8bit only)
  * [x] BMP
  * [x] GIF

## Examples

* [glview](examples/glview) : Simple glTF geometry viewer.

## TODOs

* [ ] Write C++ code generator from json schema for robust parsing.
* [ ] Serialization
* [ ] Compression/decompression(Open3DGC, etc)
* [ ] Support `extensions` and `extras` property
* [ ] HDR image?
* [ ] Write tests for `animation` and `skin` 

## Licenses

TinyGLTF is licensed under MIT license.

TinyGLTF uses the following third party libraries.

* picojson.h : Copyright 2009-2010 Cybozu Labs, Inc. Copyright 2011-2014 Kazuho Oku
* base64 : Copyright (C) 2004-2008 René Nyffenegger
* stb_image.h : v2.08 - public domain image loader - http://nothings.org/stb_image.h


## Build and example

Copy `stb_image.h`, `picojson.h` and `tiny_gltf.h` to your project.

### Loading glTF 2.0 model

```
// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf.h"

using namespace tinygltf;

Model model; 
TinyGLTF loader;
std::string err;
  
bool ret = loader.LoadASCIIFromFile(&model, &err, argv[1]);
//bool ret = loader.LoadBinaryFromFile(&model, &err, argv[1]); // for binary glTF(.glb) 
if (!err.empty()) {
  printf("Err: %s\n", err.c_str());
}

if (!ret) {
  printf("Failed to parse glTF\n");
  return -1;
}
```

### Saving gltTF 2.0 model

T.B.W.

## Running tests.

### Setup

Python 2.6 or 2.7 required.
Git clone https://github.com/KhronosGroup/glTF-Sample-Models to your local dir.

### Run test

After building `loader_example`, edit `test_runner.py`, then,

    $ python test_runner.py
