# tinygltf v3 — C glTF 2.0 loader/saver

`TinyGLTF v3` is a C11-first implementation of a glTF 2.0
https://github.com/KhronosGroup/glTF loader and writer, with experimental
(spec-unstable) glTF 2.1 support.

- **Pure C POD API** (`tiny_gltf_v3.h` + `tiny_gltf_v3.c` + `tinygltf_json_c.h`) — no STL, no RTTI, no exceptions required; easy to bind to other languages.
- **Arena-based memory management** — all parse-time allocations come from a single arena; a single `tg3_model_free()` frees everything.
- **Structured error reporting** — `tg3_error_stack` provides machine-readable errors with severity levels and source locations.
- **Custom JSON backend** — `tinygltf_json_c.h` is a locale-independent pure-C JSON parser/serializer used by the v3 runtime.
- **Streaming callbacks** — opt-in streaming parse/write via user-supplied callbacks.
- **Opt-in filesystem and image I/O** — `TINYGLTF3_ENABLE_FS` / `TINYGLTF3_ENABLE_STB_IMAGE` are off by default; you control when and how assets are loaded.
- **C++20 coroutine facade** (optional, auto-detected). C17/C++17 default.
- **Hardened against untrusted input** — URI sanitization, post-parse index-bounds and byte-range validation (default-on, opt-out via `tg3_parse_options.validate_indices` / `validate_ranges`), strict numeric range checks; exercised by a libFuzzer harness. See the `Security Considerations` block at the top of `tiny_gltf_v3.h`.
- **Strictness levels** — spec-conformance violations are warnings by default (`TG3_PERMISSIVE`) and errors under `TG3_STRICT`; `tg3_validate()` checks hand-built models with the same rules.
- **Formal specification and machine-checked proofs** — a Lean 4 specification of the validation rules and GLB framing, and a Lean proof that the C GLB framing code is free of undefined behaviour and refines the specification. See [`formal/`](formal/README.md).

## Quick start

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

## Strictness and validation

Validation rules come in two classes (the normative list is
[`formal/GlTF/Validate.lean`](formal/GlTF/Validate.lean)):

| Class | Examples | `TG3_PERMISSIVE` (default) | `TG3_STRICT` |
|---|---|---|---|
| Memory safety | out-of-range indices, accessor/bufferView byte ranges, unknown component types | error | error |
| Spec conformance | `alphaMode`, primitive `mode`, sampler filters, camera parameters, factor ranges, `byteStride` alignment, `extensionsRequired ⊆ extensionsUsed` | warning | error (`TG3_ERR_INVALID_VALUE`) |

```c
tg3_parse_options opts;
tg3_parse_options_init(&opts);
opts.strictness = TG3_STRICT;          /* reject non-conforming files */
...
tg3_validate(&model, TG3_STRICT, &errors);   /* same rules for any model */
```

## glTF 2.1 (spec-unstable)

glTF 2.1 is **not** a ratified Khronos specification. tinygltf follows a draft
and marks every such item `TG3_GLTF21_UNSTABLE` in `tiny_gltf_v3.h`: GLB
container version 3 with 64-bit lengths (read by default, written only with
`tg3_write_options.write_glb_64bit`), `id`, `files`, `thumbnails` and the
promoted-extension names. These may change incompatibly in any release.
Reading a GLB v3 container emits a warning.

## Testing & verification

The v3 C runtime ships with three layers of automated coverage:

- **`tests/tester_v3_c.c`** — internal unit checks plus security regression tests (path traversal, negative `byteStride`, OOB indices, error-message lifetime, …). Build via `make` in `tests/`; run `./tester_v3_c` for the internal suite or `./tester_v3_c <file.gltf|file.glb>` to parse a single asset.
- **`tests/tester_v3_c_v1port.c`** — v3 C port of the legacy unit tests, keeping behavioral parity with the old C++ API.
- **`tests/tester_v3_json_c.c`** and **`tests/tester_v3_freestanding.c`** — JSON backend and freestanding (no libc) checks.
- **`test_runner.py`** — model verifier that runs `tests/tester_v3_c` against every model in `glTF-Sample-Models/2.0` and validates the structured COUNTS/DIGEST output.
- **`formal/`** — Lean 4 specification, proofs, and machine-checked refinement proofs of the verified C core (`cd formal && lake build`; see [`formal/README.md`](formal/README.md)).
- **`tests/v3/fuzzer/`** — libFuzzer harness with ASan + UBSan (`make run` builds and runs `fuzz_gltf_v3_c`). Crafted regression inputs live in `tests/v3/security/` and are seeded into `tests/v3/fuzzer/corpus/`.

### Building

```bash
# Plain make (gcc/clang)
$ make -C tests && make -C tests run

# CMake
$ cmake -B build && cmake --build build && ctest --test-dir build --output-on-failure

# Meson
$ meson setup build && meson compile -C build && meson test -C build

# WebAssembly (requires an Emscripten SDK, defaults to ~/work/emsdk)
$ make -C web && make -C web sample
```

## Web/WASM demo

[`web/`](web/) contains a browser demo that compiles the v3 C runtime with
Emscripten and renders glTF/GLB files with three.js (file picker + drag &
drop). See [`web/README.md`](web/README.md) for build instructions.

## Legacy v1/v2 (C++)

The previous C++ implementation (`tiny_gltf.h`, `tiny_gltf.cc`,
`tinygltf_json.h`, `json.hpp`, `loader_example.cc`, the C++ examples and
tests) is deprecated and has been moved to [`attic/`](attic/) for reference.
It is no longer built, tested, or installed. TinyGLTF v3 C is the mainline.

## Status

TinyGLTF v3 (`tiny_gltf_v3.h` + `tiny_gltf_v3.c`) is the mainline release.
API additions and behavior changes go through the usual review process.

## Builds

![C/C++ CI](https://github.com/syoyo/tinygltf/workflows/C/C++%20CI/badge.svg)

## Licenses

TinyGLTF is licensed under MIT license. See [LICENSE](LICENSE).

TinyGLTF v3 uses the following third party code:

* fast_float (JSON number parsing, in `tinygltf_json_c.h`): Copyright (c) 2021 The fast_float authors. Apache License 2.0 / MIT / Boost 1.0.
* dragonbox (JSON number serialization, in `tinygltf_json_c.h`): Copyright 2020-2024 Junekey Jeon. Apache 2.0 with LLVM exceptions / Boost 1.0.
* stb_image.h (optional, `TINYGLTF3_ENABLE_STB_IMAGE`) : public domain image loader - [Github link](https://github.com/nothings/stb/blob/master/stb_image.h)
