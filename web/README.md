# tinygltf v3 C — Web/WASM demo (three.js)

A minimal single-page demo that parses glTF/GLB in the browser using the
tinygltf v3 C runtime compiled to WebAssembly, then renders the result with
[three.js](https://threejs.org).

```
web/
  loader.c        — C bridge: parse bytes, flatten primitives/materials/textures/nodes for JS
  index.html      — demo page (file picker + drag & drop)
  main.js         — three.js viewer consuming the WASM exports
  style.css
  Makefile        — Emscripten build
  gen_sample.py   — generates Cube.glb (small self-contained textured cube)
  Cube.glb        — sample model, generated with `make sample`
```

## Requirements

* An Emscripten SDK. The Makefile defaults to `~/work/emsdk`
  (override with `make EMSDK=/path/to/emsdk`).

## Build

```bash
$ cd web
$ make            # produces tinygltf_v3.js + tinygltf_v3.wasm
$ make sample     # generate Cube.glb (self-contained textured cube)
$ make serve      # python3 -m http.server 8000
```

Then open http://localhost:8000 and drop a `.glb` / `.gltf` file onto the
page, or click **Load sample**.

## Notes

* Assets must be **self-contained**: GLB binary chunk or embedded data-URI
  buffers/images. External `.bin` / image file references are not resolved
  (no filesystem is linked into the module: `-sFILESYSTEM=0`).
* Primitives must use `float` `VEC3` positions (the common case); sparse
  accessors and `double` attributes are skipped with a warning message.
* Image decoding happens client-side: raw image bytes are passed to
  `createImageBitmap()` and wrapped in a `THREE.CanvasTexture`.

## C exports (loader.c)

All functions are exported as `_tg3w_*` on the Emscripten module:

| Function | Description |
| --- | --- |
| `tg3w_parse(ptr, size)` | Parse GLB/glTF bytes (auto-detect), flatten model |
| `tg3w_clear()` | Free the current model and flattened buffers |
| `tg3w_last_error()` / `tg3w_error_message(i)` / `tg3w_error_count()` | Parse diagnostics |
| `tg3w_prim(i)` | Primitive record (material, mode, counts, buffer offsets; `nrm_offset`/`uv_offset` are -1 when absent) |
| `tg3w_positions()/normals()/uvs()/indices()` | Flattened vertex/index arrays |
| `tg3w_mesh_prim_start/count(m)` | Primitive range of a mesh |
| `tg3w_material_*` | PBR factors, alpha mode, base color texture |
| `tg3w_texture_source(i)`, `tg3w_image_bytes/size/mime(i)` | Texture image bytes |
| `tg3w_node_*`, `tg3w_scene_*`, `tg3w_default_scene()` | Scene graph |
