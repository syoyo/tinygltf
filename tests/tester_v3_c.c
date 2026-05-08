#include "tiny_gltf_v3.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int mem_contains(const uint8_t *data, uint64_t size, const char *needle) {
  size_t needle_len = strlen(needle);
  uint64_t i;
  if (needle_len == 0 || size < (uint64_t)needle_len) {
    return 0;
  }
  for (i = 0; i + (uint64_t)needle_len <= size; ++i) {
    if (memcmp(data + i, needle, needle_len) == 0) {
      return 1;
    }
  }
  return 0;
}

static int check_minimal_parse(void) {
  static const uint8_t json[] =
      "{\"asset\":{\"version\":\"2.0\"},"
      "\"scene\":0,"
      "\"scenes\":[{\"nodes\":[0]}],"
      "\"nodes\":[{\"name\":\"root\"}]}";
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);

  err = tg3_parse_auto(&model, &errors, json, (uint64_t)(sizeof(json) - 1), "", 0,
                       &opts);
  if (err != TG3_OK) {
    fprintf(stderr, "tg3_parse_auto failed: %d\n", (int)err);
    tg3_error_stack_free(&errors);
    return 0;
  }

  if (model.default_scene != 0 || model.scenes_count != 1 || model.nodes_count != 1) {
    fprintf(stderr, "unexpected parsed model shape\n");
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }

  if (!model.nodes || !model.nodes[0].name.data ||
      !tg3_str_equals_cstr(model.nodes[0].name, "root")) {
    fprintf(stderr, "node name mismatch\n");
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }

  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 1;
}

static int check_minimal_write_roundtrip(void) {
  static const uint8_t json[] =
      "{\"asset\":{\"version\":\"2.0\"},"
      "\"scene\":0,"
      "\"scenes\":[{\"nodes\":[0]}],"
      "\"nodes\":[{\"name\":\"root\"}]}";
  tg3_model model;
  tg3_model roundtrip;
  tg3_error_stack errors;
  tg3_parse_options parse_opts;
  tg3_write_options write_opts;
  tg3_error_code err;
  uint8_t *out = NULL;
  uint64_t out_size = 0;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&parse_opts);
  tg3_write_options_init(&write_opts);

  err = tg3_parse_auto(&model, &errors, json, (uint64_t)(sizeof(json) - 1), "", 0,
                       &parse_opts);
  if (err != TG3_OK) {
    fprintf(stderr, "initial parse failed: %d\n", (int)err);
    tg3_error_stack_free(&errors);
    return 0;
  }

  err = tg3_write_to_memory(&model, &errors, &out, &out_size, &write_opts);
  if (err != TG3_OK || !out || out_size == 0) {
    fprintf(stderr, "tg3_write_to_memory failed: %d\n", (int)err);
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }

  if (!mem_contains(out, out_size, "\"asset\"") ||
      !mem_contains(out, out_size, "\"root\"")) {
    fprintf(stderr, "serialized JSON missing expected fields\n");
    tg3_write_free(out, &write_opts);
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }

  err = tg3_parse_auto(&roundtrip, &errors, out, out_size, "", 0, &parse_opts);
  tg3_write_free(out, &write_opts);
  if (err != TG3_OK) {
    fprintf(stderr, "roundtrip parse failed: %d\n", (int)err);
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }

  if (roundtrip.default_scene != 0 || roundtrip.nodes_count != 1 ||
      !roundtrip.nodes || !roundtrip.nodes[0].name.data ||
      !tg3_str_equals_cstr(roundtrip.nodes[0].name, "root")) {
    fprintf(stderr, "roundtrip model mismatch\n");
    tg3_model_free(&roundtrip);
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }

  tg3_model_free(&roundtrip);
  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 1;
}

static int check_parse_file_failure_initializes_model(void) {
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;

  memset(&model, 0xA5, sizeof(model));
  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);

  err = tg3_parse_file(&model, &errors, "scene.gltf", 10, &opts);
  if (err != TG3_ERR_FS_NOT_AVAILABLE) {
    fprintf(stderr, "tg3_parse_file unexpected error: %d\n", (int)err);
    tg3_error_stack_free(&errors);
    return 0;
  }

  if (model.default_scene != -1) {
    fprintf(stderr, "tg3_parse_file did not initialize model on failure\n");
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }

  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 1;
}

static int check_non_object_root_rejected(void) {
  static const uint8_t json[] = "\"not an object\"";
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);

  err = tg3_parse(&model, &errors, json, (uint64_t)(sizeof(json) - 1), "", 0, &opts);
  if (err != TG3_ERR_JSON_PARSE) {
    fprintf(stderr, "non-object root returned unexpected error: %d\n", (int)err);
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }

  if (model.default_scene != -1) {
    fprintf(stderr, "non-object root left model in unexpected state\n");
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }

  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 1;
}

static int check_huge_integer_field_rejected(void) {
  static const uint8_t json[] =
      "{\"asset\":{\"version\":\"2.0\"},\"scene\":6.66667e+70}";
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);

  err = tg3_parse(&model, &errors, json, (uint64_t)(sizeof(json) - 1), "", 0, &opts);
  if (err != TG3_ERR_JSON_PARSE) {
    fprintf(stderr, "huge integer-like field returned unexpected error: %d\n", (int)err);
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }

  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 1;
}

int main(void) {
  if (!check_minimal_parse()) {
    return 1;
  }
  if (!check_minimal_write_roundtrip()) {
    return 1;
  }
  if (!check_parse_file_failure_initializes_model()) {
    return 1;
  }
  if (!check_non_object_root_rejected()) {
    return 1;
  }
  if (!check_huge_integer_field_rejected()) {
    return 1;
  }
  return 0;
}
