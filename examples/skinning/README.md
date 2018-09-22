# Simple glTF skinning sample with CPU skinning implementation.

Example use CPU implementation of skinning for the explanation of how to process skin property in glTF format.

OpenGL is used to display transformed vertex.

## Build on Linux and macOS

```
$ premake5 gmake
$ make
$ ./bin/native/Debug/skinning simple-skin.gltf
```

## Note on asset

`simple-skin.gltf` is grabbed from gltfTutorial https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_019_SimpleSkin.md
