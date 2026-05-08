#include "tiny_gltf_v3.h"

#include <stddef.h>
#include <stdint.h>

static const uint64_t FUZZ_MEMORY_BUDGET = 64ULL * 1024 * 1024;

static void tg3_fuzz_parse_auto(const uint8_t *data, size_t size) {
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  opts.memory.memory_budget = FUZZ_MEMORY_BUDGET;

  tg3_parse_auto(&model, &errors, data, (uint64_t)size, "", 0, &opts);

  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
}

static void tg3_fuzz_parse_json(const uint8_t *data, size_t size) {
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  opts.memory.memory_budget = FUZZ_MEMORY_BUDGET;

  tg3_parse(&model, &errors, data, (uint64_t)size, "", 0, &opts);

  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
}

static void tg3_fuzz_parse_glb(const uint8_t *data, size_t size) {
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  opts.memory.memory_budget = FUZZ_MEMORY_BUDGET;

  tg3_parse_glb(&model, &errors, data, (uint64_t)size, "", 0, &opts);

  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
}

static void tg3_fuzz_parse_float32(const uint8_t *data, size_t size) {
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  opts.memory.memory_budget = FUZZ_MEMORY_BUDGET;
  opts.parse_float32 = 1;

  tg3_parse_auto(&model, &errors, data, (uint64_t)size, "", 0, &opts);

  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  uint8_t selector;
  const uint8_t *payload;
  size_t payload_size;

  if (size == 0) return 0;

  selector = data[0] % 4;
  payload = data + 1;
  payload_size = size - 1;

  switch (selector) {
    case 0:
      tg3_fuzz_parse_auto(payload, payload_size);
      break;
    case 1:
      tg3_fuzz_parse_json(payload, payload_size);
      break;
    case 2:
      tg3_fuzz_parse_glb(payload, payload_size);
      break;
    case 3:
      tg3_fuzz_parse_float32(payload, payload_size);
      break;
  }

  return 0;
}
