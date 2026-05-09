#include "tiny_gltf_v3.h"

#include <stddef.h>
#include <stdint.h>

static const uint64_t FUZZ_MEMORY_BUDGET = 64ULL * 1024 * 1024;

typedef tg3_error_code (*tg3_fuzz_parse_fn)(tg3_model *, tg3_error_stack *,
    const uint8_t *, uint64_t, const char *, uint32_t, const tg3_parse_options *);

static void tg3_fuzz_run(tg3_fuzz_parse_fn fn, int parse_float32,
                         const uint8_t *data, size_t size) {
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  opts.memory.memory_budget = FUZZ_MEMORY_BUDGET;
  opts.parse_float32 = parse_float32;

  fn(&model, &errors, data, (uint64_t)size, "", 0, &opts);

  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size == 0) return 0;

  switch (data[0] % 4) {
    case 0: tg3_fuzz_run(tg3_parse_auto, 0, data + 1, size - 1); break;
    case 1: tg3_fuzz_run(tg3_parse,      0, data + 1, size - 1); break;
    case 2: tg3_fuzz_run(tg3_parse_glb,  0, data + 1, size - 1); break;
    case 3: tg3_fuzz_run(tg3_parse_auto, 1, data + 1, size - 1); break;
  }

  return 0;
}
