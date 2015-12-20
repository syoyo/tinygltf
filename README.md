# Tiny glTF loader, header only C++ glTF parsing library.

`TinyGLTFLoader` is a header only C++ glTF https://github.com/KhronosGroup/glTF parsing library

![](images/glview_duck.png)

## Features

* Portable C++. C++-98 with STL dependency only.
* Moderate parsing time and memory consumption.
* glTF specification v1.0.0
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

## Limitation

Currently, TinyGLTFLoader only loads nodes and geometry(mesh/buffer) data.

## Examples

* [glview](examples/glview) : Simple glTF geometry viewer.

## TODOs

* [ ] Support multiple scenes in `.gltf`
* [ ] Parse `animation`, `program`, `sampler`, `shader`, `technique`, `texture`
* [ ] Compression/decompression(Open3DGC, etc)
* [ ] Support `extensions` and `extras` property
* [ ] Load `.gltf` from memory.
* [ ] HDR image
* [ ] Binary glTF.

## License

TinyGLTFLoader is licensed under 2-clause BSD.

TinyGLTFLoader uses the following third party libraries.

* picojson.h : Copyright 2009-2010 Cybozu Labs, Inc. Copyright 2011-2014 Kazuho Oku
* base64 : Copyright (C) 2004-2008 René Nyffenegger
* stb_image.h : v2.08 - public domain image loader - http://nothings.org/stb_image.h


## Build and example

Copy `stb_image.h`, picojson.h` and `tiny_gltf_loader.h` to your project.

```
// Define these only in *one* .cc file.
#define TINYGLTF_LOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf_loader.h"

using namespace tinygltf;

Scene scene; 
TinyGLTFLoader loader;
std::string err;
  
bool ret = loader.LoadFromFile(scene, err, argv[1]);
if (!err.empty()) {
  printf("Err: %s\n", err.c_str());
}

if (!ret) {
  printf("Failed to parse glTF\n");
  return -1;
}
```

