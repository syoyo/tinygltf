/*
 * fuzz_gltf_v3.cc — libFuzzer harness for tinygltf v3 parser.
 *
 * Fuzz targets:
 *   - Auto-detect (GLB or JSON) parse from arbitrary bytes
 *   - Exercises JSON parser, GLB header parsing, arena allocator,
 *     error stack, and all glTF entity parsing paths.
 *
 * Build (clang with libFuzzer):
 *   clang++ -g -O1 -fsanitize=fuzzer,address,undefined \
 *       -std=c++17 -fno-rtti -fno-exceptions \
 *       -I../../.. -o fuzz_gltf_v3 fuzz_gltf_v3.cc
 *
 * Run:
 *   ./fuzz_gltf_v3 corpus/ -max_len=65536
 *
 * Seed corpus: place valid .gltf and .glb files in corpus/
 */

#define TINYGLTF3_IMPLEMENTATION
#include "tiny_gltf_v3.h"

#include <cstdint>
#include <cstddef>

/* Memory budget to prevent OOM during fuzzing */
static const uint64_t FUZZ_MEMORY_BUDGET = 64ULL * 1024 * 1024; /* 64 MB */

static void fuzz_parse_auto(const uint8_t *data, size_t size) {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    tg3_parse_options opts;
    tg3_parse_options_init(&opts);
    opts.memory.memory_budget = FUZZ_MEMORY_BUDGET;

    tg3_parse_auto(&model, &errors, data, (uint64_t)size,
                   "", 0, &opts);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

static void fuzz_parse_json(const uint8_t *data, size_t size) {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    tg3_parse_options opts;
    tg3_parse_options_init(&opts);
    opts.memory.memory_budget = FUZZ_MEMORY_BUDGET;

    tg3_parse(&model, &errors, data, (uint64_t)size,
              "", 0, &opts);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

static void fuzz_parse_glb(const uint8_t *data, size_t size) {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    tg3_parse_options opts;
    tg3_parse_options_init(&opts);
    opts.memory.memory_budget = FUZZ_MEMORY_BUDGET;

    tg3_parse_glb(&model, &errors, data, (uint64_t)size,
                  "", 0, &opts);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

static void fuzz_parse_float32(const uint8_t *data, size_t size) {
    tg3_model model;
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    tg3_parse_options opts;
    tg3_parse_options_init(&opts);
    opts.memory.memory_budget = FUZZ_MEMORY_BUDGET;
    opts.parse_float32 = 1;

    tg3_parse_auto(&model, &errors, data, (uint64_t)size,
                   "", 0, &opts);

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0) return 0;

    /* Use first byte to select parse path, rest is the payload */
    uint8_t selector = data[0] % 4;
    const uint8_t *payload = data + 1;
    size_t payload_size = size - 1;

    switch (selector) {
    case 0: fuzz_parse_auto(payload, payload_size);    break;
    case 1: fuzz_parse_json(payload, payload_size);    break;
    case 2: fuzz_parse_glb(payload, payload_size);     break;
    case 3: fuzz_parse_float32(payload, payload_size); break;
    }

    return 0;
}
