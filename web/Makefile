# Build the tinygltf v3 C WASM module for the three.js demo.
#
# Requires an Emscripten SDK. The default EMSDK path is $(HOME)/work/emsdk;
# override with `make EMSDK=/path/to/emsdk`.

EMSDK      ?= $(HOME)/work/emsdk
ROOT       := $(abspath ../)

SHELL      := /bin/bash
EMCC       := source $(EMSDK)/emsdk_env.sh && emcc

CFLAGS     := -O3 -std=c11 -Wall -Wextra -I$(ROOT)
EMFLAGS    := -sMODULARIZE=1 \
              -sEXPORT_ES6=1 \
              -sEXPORT_NAME=createTinyGLTF \
              -sENVIRONMENT=web \
              -sALLOW_MEMORY_GROWTH=1 \
              -sSTACK_SIZE=1048576 \
              -sFILESYSTEM=0 \
              -sEXPORTED_FUNCTIONS=_malloc,_free \
              -sEXPORTED_RUNTIME_METHODS=getValue,setValue,UTF8ToString,HEAP32,HEAPU32,HEAPF32,HEAPU8

SRC        := $(ROOT)/tiny_gltf_v3.c loader.c
OUT_JS     := tinygltf_v3.js
OUT_WASM   := tinygltf_v3.wasm
SAMPLE     := Cube.glb

.PHONY: all sample serve clean

all: $(OUT_JS)

$(OUT_JS): loader.c Makefile $(ROOT)/tiny_gltf_v3.h $(ROOT)/tiny_gltf_v3.c $(ROOT)/tinygltf_json_c.h
	$(EMCC) $(CFLAGS) $(EMFLAGS) -o $(OUT_JS) $(SRC)

# Generate a small self-contained textured sample GLB (Cube.glb).
sample:
	python3 gen_sample.py

# Serve the demo locally (http://localhost:8000)
serve:
	python3 -m http.server 8000

clean:
	rm -f $(OUT_JS) $(OUT_WASM) $(SAMPLE)
