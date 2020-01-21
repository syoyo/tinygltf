# Fuzzing test

Do fuzzing test for TinyGLTF API.

## Supported API

* [x] LoadASCIIFromMemory
* [ ] LoadBinaryFromMemory

## Requirements

* meson
* clang with fuzzer support(`-fsanitize=fuzzer`. at least clang 8.0 should work)

## Setup

### Ubuntu 18.04

```
$ sudo apt install clang++-8
$ sudo apt install libfuzzer-8-dev
```

Optionally, if you didn't set `update-alternatives` you can set `clang++` to point to `clang++8`

```
$ sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-8 10
$ sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-8 10
```

## How to compile

```
$ CXX=clang++ CC=clang meson build
$ cd build
$ ninja
```

## How to run

Increase memory limit. e.g. `-rss_limit_mb=50000`

```
$ ./fuzz_gltf -rss_limit_mb=20000 -jobs 4
```

