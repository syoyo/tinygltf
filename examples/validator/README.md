# tinygltf-validator

TinyGLTF validator based on Modern C++ JSON schema validator https://github.com/pboettch/json-schema-validator

## Status

Experimental. W.I.P.

## Requirements

* C++11 compiler
* CMake 3.2 or later

## How to build

```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## How to run

First clone glTF repo somewhere to get glTF schema JSON files.

```
$ cd ~/work # Choose your favorite dir.
$ git clone https://github.com/KhronosGroup/glTF
$ export GLTF_DIR=~/work/glTF
```

Then, specify the directory of glTF 2.0 schema JSONs and a glTF file.

```
$ ./tinygltf-validator $GLTF_DIR/specification/2.0/schema/ ../../models/Cube/Cube.gltf
```

## Third party licenses

* json.hpp https://github.com/nlohmann/json : MIT
* json-schema-validator https://github.com/pboettch/json-schema-validator : MIT
