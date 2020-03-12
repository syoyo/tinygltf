# Mesh modify experiment

Sometimes we want to tweak mesh attributes(e.g. vertex position, uv coord, etc).
glTF itself does not allow ASCII representation of such data so we need to write a converter.

This example show how to

- Export mesh data from .bin to .obj
- Import mesh data to .bin(update corresponding buffer data) from .obj

## Features

* Support skin weights(`JOINTS_N`, `WEIGHTS_N`)

## Supported attributes

* [x] POSITION
* [x] NORMAL
* [x] TANGENT
* [ ] COLOR  (vertex color)
* [x] TEXCOORD_N
  * Only single texcoord(uv set) is supported
  * Specify `--uvset 1` to specify which UV to use.
* [x] WEIGHTS_N, JOINTS_N

## Usage

### Wavefront .obj to glTF

```
$ mesh-modify --op=obj2gltf input.obj
```

All shapes in .obj are concatenated and create single glTF mesh.
(tinyobjloader's skin weight extension `vw` supported)

#### Limitation

Buffer is stored as external file(`.bin`)

### glTF to Wavefront .obj

```
$ mesh-modify --op=gltf2obj input.gltf
```

.obj will be created for each glTF Mesh.
(Skin weight data is exported to .obj using tinyobjloader's skin weight extension `vw`)

#### Limitation

Buffer is stored as external file(`.bin`)

## TODO

* [ ] obj2gltf: Assign glTF material from .mtl
* [ ] modify/patch mesh geometry in glTF
  * By using JSON Patch or JSON Merge feature
