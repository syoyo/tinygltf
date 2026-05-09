# Header only C++ tiny glTF library(loader/saver).

`TinyGLTF` is a header only C++ glTF 2.0 https://github.com/KhronosGroup/glTF library.

## TinyGLTF v3 (new major release)

**`tiny_gltf_v3.h`** is the new major version of TinyGLTF.
The new C implementation (`tiny_gltf_v3.c` + `tinygltf_json_c.h`) is currently **experimental**.

### What's new in v3

v3 is a ground-up rewrite with a C-centric, low-overhead design:

- **Pure C POD structs** — no STL containers in the public API; easy to bind to other languages.
- **Arena-based memory management** — all parse-time allocations come from a single arena; a single `tg3_model_free()` frees everything.
- **Structured error reporting** — `tg3_error_stack` provides machine-readable errors with severity levels and source locations.
- **Custom JSON backend** — backed by `tinygltf_json_c.h`, a locale-independent pure-C JSON parser/serializer used by the v3 runtime.
- **Streaming callbacks** — opt-in streaming parse/write via user-supplied callbacks.
- **No RTTI, no exceptions required** — suitable for embedded and game-engine use.
- **Opt-in filesystem and image I/O** — `TINYGLTF3_ENABLE_FS` / `TINYGLTF3_ENABLE_STB_IMAGE` are off by default; you control when and how assets are loaded.
- **C++20 coroutine facade** (optional, auto-detected). C17/C++17 default.
- **Hardened against untrusted input** — URI sanitization, post-parse index-bounds validation (default-on, opt-out via `tg3_parse_options.validate_indices = 0`), strict numeric range checks; exercised by a libFuzzer harness and by a cross-version verifier that compares parsed output against the v1 C++ reference loader. See *Security model* below and the `Security Considerations` block at the top of `tiny_gltf_v3.h`.

### Quick start (v3)

Copy `tiny_gltf_v3.h`, `tiny_gltf_v3.c`, and `tinygltf_json_c.h` to your project.
Compile `tiny_gltf_v3.c` as C11 or newer. Define `TINYGLTF3_ENABLE_FS` when
building `tiny_gltf_v3.c` if you want `tg3_parse_file()` to use stdio-backed
filesystem helpers. The legacy `TINYGLTF3_IMPLEMENTATION` include path remains
available for compatibility.

```c
#include "tiny_gltf_v3.h"
```

Loading a glTF file:

```c
tg3_parse_options opts;
tg3_error_stack errors;
tg3_model model;

tg3_parse_options_init(&opts);
tg3_error_stack_init(&errors);

tg3_error_code err = tg3_parse_file(&model, &errors, "scene.gltf", 10, &opts);
if (err != TG3_OK) {
    for (uint32_t i = 0; i < errors.count; i++) {
        fprintf(stderr, "[%d] %s\n", (int)errors.entries[i].severity,
                errors.entries[i].message ? errors.entries[i].message : "(null)");
    }
}
// ... use model ...
tg3_model_free(&model);
tg3_error_stack_free(&errors);
```

### Security model (v3 C runtime)

The v3 C runtime is built for processing **untrusted glTF/GLB input** (server-side asset pipelines, user uploads, etc.) and ships hardened by default:

- **URI sanitization** — external buffer/image URIs are rejected before any filesystem call if they are empty, contain NUL bytes, begin with `/` or `\`, look like a Windows drive prefix (`X:`), or contain a `..` segment. Production callers SHOULD still provide a custom `tg3_fs_callbacks.read_file` that confines reads to a known directory (e.g. via `openat` plus a `realpath` prefix check) when the input is attacker-controlled.
- **Index bounds validation** — every `int32_t` index field populated from JSON (accessor.bufferView, primitive.indices/material/attributes, scene.nodes[], skin.joints[], animation channel/sampler refs, KHR_audio + MSFT_lod refs, …) is checked after the structural parse. Out-of-range indices produce `TG3_ERR_INVALID_INDEX`. Default `tg3_parse_options.validate_indices = 1`; set to `0` only when you need raw round-trip and have your own validator.
- **Strict numeric range checks** — JSON numbers feeding integer fields go through finite/round-trip-validated coercion (`tg3__json_number_to_int32` / `_uint64`). `byteStride` is restricted to 0 or [4, 252].
- **Memory budget** — the arena is capped at `TINYGLTF3_MAX_MEMORY_BYTES` (1 GB by default; configurable via `tg3_memory_config`).
- **Image decoding off by default** — the parser does not decode image bytes; use `tg3_parse_options.images_as_is = 1` to skip any decoder entirely when handling untrusted input.
- **Error message lifetime** — error strings on `tg3_error_stack` are arena-allocated and remain valid until `tg3_model_free()`. Read or copy them BEFORE freeing the model.

See the `Security Considerations` block at the top of `tiny_gltf_v3.h` for the authoritative threat-model summary.

### Testing & verification

The v3 C runtime ships with three layers of automated coverage:

- **`tests/tester_v3_c.c`** — internal unit checks plus security regression tests (path traversal, negative `byteStride`, OOB indices, error-message lifetime, …). Build via `make` in `tests/`; run `./tester_v3_c` for the internal suite or `./tester_v3_c <file.gltf|file.glb>` to parse a single asset.
- **`test_runner.py`** — a cross-version verifier that runs the v1 C++ reference loader (`loader_example`) and the v3 C tester against every model in `glTF-Sample-Models/2.0`, then diffs a structured DIGEST block (buffer FNV64 hashes, accessor/bufferView fields, primitive attribute maps, node TRS, material PBR factors, skin/animation/scene topology, …). v1 is the ground truth.
- **`tests/v3/fuzzer/`** — libFuzzer harness with ASan + UBSan (`make run` builds and runs `fuzz_gltf_v3_c`). Crafted regression inputs live in `tests/v3/security/` and are seeded into `tests/v3/fuzzer/corpus/`.

## Status

> ⚠️ **v2 deprecation notice:** `tiny_gltf.h` (v2) remains fully functional and is still supported,
> but it is now in **maintenance mode only** — no new features will be added.
> v2 will be **sunset after mid-2026**. `tiny_gltf_v3.h` is the intended successor, but the new C v3 runtime is still **experimental**.

TinyGLTF v3's C runtime (`tiny_gltf_v3.h` + `tiny_gltf_v3.c`) is available for evaluation and early adoption,
but its API/behavior may still change while the implementation matures.

Currently TinyGLTF v2 is stable and in maintenance mode. No drastic changes and feature additions planned.
 - v2.9.0 Various fixes and improvements. Filesystem callback API change.
 - v2.8.0 Add URICallbacks for custom URI handling in Buffer and Image. PR#397
 - v2.7.0 Change WriteImageDataFunction user callback function signature. PR#393
 - v2.6.0 Support serializing sparse accessor(Thanks to @fynv).
 - v2.5.0 Add SetPreserveImageChannels() option to load image data as is.
 - v2.4.0 Experimental RapidJSON support. Experimental C++14 support(C++14 may give better performance)
 - v2.3.0 Modified Material representation according to glTF 2.0 schema(and introduced TextureInfo class)
 - v2.2.0 release(Support loading 16bit PNG. Sparse accessor support)
 - v2.1.0 release(Draco decoding support)
 - v2.0.0 release(22 Aug, 2018)!

### Branches

* `sajson` : Use sajson to parse JSON. Parsing only but faster compile time(2x reduction compared to json.hpp and RapidJson), but not well maintained.

## Builds

![C/C++ CI](https://github.com/syoyo/tinygltf/workflows/C/C++%20CI/badge.svg)

## Features

Probably mostly feature-complete. Last missing feature is Draco encoding: https://github.com/syoyo/tinygltf/issues/207

* Written in portable C++. C++-11 with STL dependency only.
  * [x] macOS + clang(LLVM)
  * [x] iOS + clang
  * [x] Linux + gcc/clang
  * [x] Windows + MinGW
  * [x] Windows + Visual Studio 2015 Update 3 or later.
    * Visual Studio 2013 is not supported since they have limited C++11 support and failed to compile `json.hpp`.
  * [x] Android NDK
  * [x] Android + CrystaX(NDK drop-in replacement) GCC
  * [x] Web using Emscripten(LLVM)
* Moderate parsing time and memory consumption.
* glTF specification v2.0.0
  * [x] ASCII glTF
    * [x] Load
    * [x] Save
  * [x] Binary glTF(GLB)
    * [x] Load
    * [x] Save(.bin embedded .glb)
* Buffers
  * [x] Parse BASE64 encoded embedded buffer data(DataURI).
  * [x] Load `.bin` file.
* Image(Using stb_image)
  * [x] Parse BASE64 encoded embedded image data(DataURI).
  * [x] Load external image file.
  * [x] Load PNG(8bit and 16bit)
  * [x] Load JPEG(8bit only)
  * [x] Load BMP
  * [x] Load GIF
  * [x] Custom Image decoder callback(e.g. for decoding OpenEXR image)
* Morph traget
  * [x] Sparse accessor
* Load glTF from memory
* Custom callback handler
  * [x] Image load
  * [x] Image save
* Extensions
  * [x] Draco mesh decoding
  * [ ] Draco mesh encoding

## Note on extension property

In extension(`ExtensionMap`), JSON number value is parsed as int or float(number) and stored as `tinygltf::Value` object. If you want a floating point value from `tinygltf::Value`, use `GetNumberAsDouble()` method.

`IsNumber()` returns true if the underlying value is an int value or a floating point value.

## Examples

* [glview](examples/glview) : Simple glTF geometry viewer.
* [validator](examples/validator) : Simple glTF validator with JSON schema.
* [basic](examples/basic) : Basic glTF viewer with texturing support.
* [build-gltf](examples/build-gltf) : Build simple glTF scene from a scratch.

### WASI/WASM build

Users who want to run TinyGLTF securely and safely(e.g. need to handle malcious glTF file to serve online glTF conver),
I recommend to build TinyGLTF for WASM target.
WASI build example is located in [wasm](wasm) .


## TODOs

* [ ] Robust URI decoding/encoding. https://github.com/syoyo/tinygltf/issues/369
* [ ] Mesh Compression/decompression(Open3DGC, etc)
  * [x] Load Draco compressed mesh
  * [ ] Save Draco compressed mesh
  * [ ] Open3DGC?
* [x] Support `extensions` and `extras` property
* [ ] HDR image?
  * [ ] OpenEXR extension through TinyEXR.
* [ ] 16bit PNG support in Serialization
* [ ] Write example and tests for `animation` and `skin`

### Optional

* [ ] Write C++ code generator which emits C++ code from JSON schema for robust parsing?

## Licenses

TinyGLTF is licensed under MIT license.

TinyGLTF uses the following third party libraries.

* json.hpp : Copyright (c) 2013-2017 Niels Lohmann. MIT license.
* base64 : Copyright (C) 2004-2008 René Nyffenegger
* stb_image.h : v2.08 - public domain image loader - [Github link](https://github.com/nothings/stb/blob/master/stb_image.h)
* stb_image_write.h : v1.09 - public domain image writer - [Github link](https://github.com/nothings/stb/blob/master/stb_image_write.h)


## Build and example

Copy `stb_image.h`, `stb_image_write.h`, `json.hpp` and `tiny_gltf.h` to your project.

### Loading glTF 2.0 model

```c++
// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

using namespace tinygltf;

Model model;
TinyGLTF loader;
std::string err;
std::string warn;
std::string filename = "input.gltf";

bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
//bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename); // for binary glTF(.glb)

if (!warn.empty()) {
  printf("Warn: %s\n", warn.c_str());
}

if (!err.empty()) {
  printf("Err: %s\n", err.c_str());
}

if (!ret) {
  printf("Failed to parse glTF: %s\n", filename.c_str());
}
```

#### Loader options

* `TinyGLTF::SetPreserveimageChannels(bool onoff)`. `true` to preserve image channels as stored in image file for loaded image. `false` by default for backward compatibility(image channels are widen to `RGBA` 4 channels). Effective only when using builtin image loader(STB image loader).

## Compile options

* `TINYGLTF_NOEXCEPTION` : Disable C++ exception in JSON parsing. You can use `-fno-exceptions` or by defining the symbol `JSON_NOEXCEPTION` and `TINYGLTF_NOEXCEPTION`  to fully remove C++ exception codes when compiling TinyGLTF.
* `TINYGLTF_NO_STB_IMAGE` : Do not load images with stb_image. Instead use `TinyGLTF::SetImageLoader(LoadimageDataFunction LoadImageData, void *user_data)` to set a callback for loading images.
* `TINYGLTF_NO_STB_IMAGE_WRITE` : Do not write images with stb_image_write. Instead use `TinyGLTF::SetImageWriter(WriteimageDataFunction WriteImageData, void *user_data)` to set a callback for writing images.
* `TINYGLTF_NO_EXTERNAL_IMAGE` : Do not try to load external image file. This option would be helpful if you do not want to load image files during glTF parsing.
* `TINYGLTF_ANDROID_LOAD_FROM_ASSETS`: Load all files from packaged app assets instead of the regular file system. **Note:** You must pass a valid asset manager from your android app to `tinygltf::asset_manager` beforehand.
* `TINYGLTF_ENABLE_DRACO`: Enable Draco compression. User must provide include path and link correspnding libraries in your project file.
* `TINYGLTF_NO_INCLUDE_JSON `: Disable including `json.hpp` from within `tiny_gltf.h` because it has been already included before or you want to include it using custom path before including `tiny_gltf.h`.
* `TINYGLTF_NO_INCLUDE_RAPIDJSON `: Disable including RapidJson's header files from within `tiny_gltf.h` because it has been already included before or you want to include it using custom path before including `tiny_gltf.h`.
* `TINYGLTF_NO_INCLUDE_STB_IMAGE `: Disable including `stb_image.h` from within `tiny_gltf.h` because it has been already included before or you want to include it using custom path before including `tiny_gltf.h`.
* `TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE `: Disable including `stb_image_write.h` from within `tiny_gltf.h` because it has been already included before or you want to include it using custom path before including `tiny_gltf.h`.
* `TINYGLTF_USE_RAPIDJSON` : Use RapidJSON as a JSON parser/serializer. RapidJSON files are not included in TinyGLTF repo. Please set an include path to RapidJSON if you enable this feature.


## CMake options

You can add tinygltf using `add_subdirectory` feature.
If you add tinygltf to your project using `add_subdirectory`, it would be better to set `TINYGLTF_HEADER_ONLY` on(just add an include path to tinygltf) and `TINYGLTF_INSTALL` off(Which does not install tinygltf files).

```
// Your project's CMakeLists.txt
...

set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(/path/to/tinygltf)
```

NOTE: Using tinygltf as a submodule doesn't automatically add the headers to your include path (as standard for many libraries). To get this functionality, add the following to the CMakeLists.txt file from above:

```
target_include_directories(${PROJECT_NAME} PRIVATE "/path/to/tinygltf")
```

### Saving gltTF 2.0 model

* Buffers.
  * [x] To file
  * [x] Embedded
  * [ ] Draco compressed?
* [x] Images
  * [x] To file
  * [x] Embedded
* Binary(.glb)
  * [x] .bin embedded single .glb
  * [ ] External .bin

## Running tests.

### glTF parsing test

#### Setup

Python required.
Git clone https://github.com/KhronosGroup/glTF-Sample-Models to your local dir.

#### Run parsing test

After building `loader_example`, edit `test_runner.py`, then,

```bash
$ python test_runner.py
```

### Unit tests

```bash
$ cd tests
$ make
$ ./tester
$ ./tester_noexcept
```

### Fuzzing tests

See `tests/fuzzer` for details.

After running fuzzer on Ryzen9 3950X a week, at least `LoadASCIIFromString` looks safe except for out-of-memory error in Fuzzer.
We may be better to introduce bounded memory size checking when parsing glTF data.

## Third party licenses

* json.hpp : Licensed under the MIT License <http://opensource.org/licenses/MIT>. Copyright (c) 2013-2017 Niels Lohmann <http://nlohmann.me>.
* stb_image : Public domain.
* catch : Copyright (c) 2012 Two Blue Cubes Ltd. All rights reserved. Distributed under the Boost Software License, Version 1.0.
* RapidJSON : Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip. All rights reserved. http://rapidjson.org/
* dlib(uridecode, uriencode) : Copyright (C) 2003  Davis E. King Boost Software License 1.0. http://dlib.net/dlib/server/server_http.cpp.html
