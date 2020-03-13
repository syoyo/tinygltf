# concat_mesh.py

Append(merge) mesh of glTF A to glTF B.

`meshes`, `accessors`, `bufferViews`, `materials` of glTF A is appended to glTF B(index to accessor, bufferViews, etc will be recomputed).

`skin`, `nodes`, etc are not appended(to be merged).

## Usage

concatenate sourceN.gltf to target.gltf and save it to merged.gltf

```
$ python concat_mesh.py source0.gltf <source1.gltf source2.gltf ...> target.gltf merged.gltf
```

## TODO

* [x] Support multiple glTFs to merge(concat)
* [ ] Support merging skin
* [ ] Support merging different node hierarchies
* [x] Support `images`, `textures`, `materials`, `samplers`
* [ ] Support other glTF info

# replace_attrib.py

Replace the accessor id of specified attribute.
