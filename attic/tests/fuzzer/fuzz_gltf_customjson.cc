/*
 * LLVM libFuzzer harness for tinygltf with the custom JSON backend
 * (tinygltf_json.h).
 *
 * Exercises:
 *   1. LoadASCIIFromString  – glTF JSON parsing
 *   2. LoadBinaryFromMemory – GLB binary parsing
 *
 * Build (clang with libFuzzer):
 *   clang++ -std=c++11 -fsanitize=address,fuzzer \
 *           -DTINYGLTF_USE_CUSTOM_JSON           \
 *           -I../../ fuzz_gltf_customjson.cc      \
 *           -o fuzz_gltf_customjson
 *
 * Run:
 *   ./fuzz_gltf_customjson -rss_limit_mb=20000 -jobs 4
 */

#include <cstdint>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#ifndef TINYGLTF_USE_CUSTOM_JSON
#define TINYGLTF_USE_CUSTOM_JSON
#endif
#include "tiny_gltf.h"

/* Fuzz the ASCII (JSON) parser path */
static void fuzz_ascii(const uint8_t *data, size_t size) {
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  const char *str = reinterpret_cast<const char *>(data);

  bool ret =
      ctx.LoadASCIIFromString(&model, &err, &warn, str,
                              static_cast<unsigned int>(size), /* base_dir */ "");
  (void)ret;
}

/* Fuzz the binary (GLB) parser path */
static void fuzz_binary(const uint8_t *data, size_t size) {
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  bool ret = ctx.LoadBinaryFromMemory(&model, &err, &warn, data,
                                      static_cast<unsigned int>(size),
                                      /* base_dir */ "");
  (void)ret;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size == 0) return 0;

  /* Use the lowest bit of the first byte to select the parse path.
   * The remaining bits are left for the fuzzer engine to explore;
   * additional paths (e.g. LoadASCIIFromFile, check_sections flags)
   * can be added here in the future using more selector bits. */
  uint8_t selector = data[0];
  const uint8_t *payload = data + 1;
  size_t payload_size = size - 1;

  if (selector & 1) {
    fuzz_binary(payload, payload_size);
  } else {
    fuzz_ascii(payload, payload_size);
  }

  return 0;
}
