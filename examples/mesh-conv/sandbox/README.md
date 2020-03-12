# concat_mesh.py

Append(merge) mesh of glTF A to glTF B.

`meshes`, `accessors`, `bufferViews`, `materials` of glTF A is appended to glTF B(index to accessor, bufferViews, etc will be recomputed).

`skin`, `nodes`, etc are not appended(to be merged).

## TODO

* [ ] Support multiple glTFs to merge
* [ ] Support merging skin
* [ ] Support merging different node hierarchies
* [ ] `images`, `textures`

# replace_attrib.py

Replace the accessor id of specified attribute.
