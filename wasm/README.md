Experimental WASI/WASM build

## Build

Download wasi-sdk https://github.com/WebAssembly/wasi-sdk

Compile tinygltf with C++ exceptions and threads off. See `Makefile` for details
(NOTE: TinyGLTF itself does not use RTTI and threading feature(C++ threads, posix, win32 thread))

## Build examples and Run

Build `loader_example.cc`

```
$ /path/to/wasi-sdk-16.0/bin/clang++ ../loader_example.cc -fno-rtti -fno-exceptions -g -Os -I../ -o loader_example.wasi
```

Tested with wasmtime.  https://github.com/bytecodealliance/wasmtime


Set a folder containing .gltf file to `--dir`

```
$ wasmtime --dir=../models loader_example.wasi ../models/Cube/Cube.gltf
```

## Emscripen

T.B.W. ...

EoL.
