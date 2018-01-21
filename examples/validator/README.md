# tinygltf-validator

TinyGLTF validator based on Modern C++ JSON schema validator https://github.com/pboettch/json-schema-validator

## Status

Experimental. W.I.P.

## Requirements

* C++11 compiler
* CMake

## How to build

```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## How to use

```
$ gltf-validator /path/to/file.gltf /path/to/gltf-schema
```

## Third party licenses

* json.hpp https://github.com/nlohmann/json : MIT
* json-schema-validator https://github.com/pboettch/json-schema-validator : MIT
