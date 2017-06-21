# Header only C++ tiny glTF library(loader/saver).

**NOTICE! Tiny glTF is still in development and does not provide official release at the morment. Please take a look at `devel` branch**
 
`TinyGLTF` is a header only C++ glTF 2.0 https://github.com/KhronosGroup/glTF library.

![](images/glview_duck.png)

[![Build Status](https://travis-ci.org/syoyo/tinygltfloader.svg?branch=master)](https://travis-ci.org/syoyo/tinygltfloader)

[![Build status](https://ci.appveyor.com/api/projects/status/i5ku97hf0r0quti3?svg=true)](https://ci.appveyor.com/project/syoyo/tinygltfloader)

## Features

* Portable C++. C++-03 with STL dependency only.
* Moderate parsing time and memory consumption.
* glTF specification v2.0.0
  * [x] ASCII glTF
  * [x] Binary glTF(https://github.com/KhronosGroup/glTF/tree/master/extensions/Khronos/KHR_binary_glTF)
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
* [writer](examples/writer) : Simple glTF writer(serialize `tinygltf::Model` class) 
* [alembic_to_gltf](examples/alembic_to_gltf) : Simple Alembic to glTF converter. 
* [cyhair_to_gltf](examples/cyhair_to_gltf) : Simple CyHair(hair curves) to glTF converter.

## TODOs

* [ ] Write C++ code generator from json schema for robust parsing.
* [ ] Write serialization example.
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

Copy `stb_image.h`, `picojson.h` and `tiny_gltf_loader.h` to your project.

### Loading glTF 2.0 model

```
// Define these only in *one* .cc file.
#define TINYGLTF_LOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf_loader.h"

using namespace tinygltf;

Scene scene; 
TinyGLTFLoader loader;
std::string err;
  
bool ret = loader.LoadASCIIFromFile(scene, err, argv[1]);
//bool ret = loader.LoadBinaryFromFile(scene, err, argv[1]); // for binary glTF(.glb) 
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
