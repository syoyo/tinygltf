#include <stddef.h>
#include <stdint.h>

static union {
  uint64_t align;
  unsigned char bytes[512 * 1024];
} test_heap;
static size_t test_heap_used;

static void *test_malloc(size_t size) {
  size_t total = (size + sizeof(size_t) + 7u) & ~(size_t)7u;
  if (test_heap_used + total > sizeof(test_heap.bytes)) return 0;
  {
    unsigned char *base = test_heap.bytes + test_heap_used;
    *((size_t *)base) = size;
    test_heap_used += total;
    return base + sizeof(size_t);
  }
}

static void *test_realloc(void *ptr, size_t size) {
  unsigned char *old_base;
  size_t old_size;
  unsigned char *new_ptr;
  size_t n;
  size_t i;
  if (!ptr) return test_malloc(size);
  old_base = (unsigned char *)ptr - sizeof(size_t);
  old_size = *((size_t *)old_base);
  new_ptr = (unsigned char *)test_malloc(size);
  if (!new_ptr) return 0;
  n = old_size < size ? old_size : size;
  for (i = 0; i < n; ++i) new_ptr[i] = ((unsigned char *)ptr)[i];
  return new_ptr;
}

static void test_free(void *ptr) {
  (void)ptr;
}

#define TINYGLTF3_NO_STDLIB
#define TINYGLTF3_MALLOC(sz) test_malloc(sz)
#define TINYGLTF3_REALLOC(ptr, sz) test_realloc((ptr), (sz))
#define TINYGLTF3_FREE(ptr) test_free(ptr)
#define TINYGLTF3_IMPLEMENTATION
#include "tiny_gltf_v3.h"

static int streq(tg3_str s, const char *lit, uint32_t len) {
  uint32_t i;
  if (!s.data || s.len != len) return 0;
  for (i = 0; i < len; ++i) {
    if (s.data[i] != lit[i]) return 0;
  }
  return 1;
}

int main(void) {
  static const uint8_t json[] =
      "{\"asset\":{\"version\":\"2.0\"},\"nodes\":[{\"name\":\"free\"}]}";
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  err = tg3_parse_auto(&model, &errors, json, (uint64_t)(sizeof(json) - 1),
                       "", 0, &opts);
  if (err != TG3_OK) return 1;
  if (model.nodes_count != 1) return 3;
  if (!streq(model.nodes[0].name, "free", 4)) return 4;
  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 0;
}
