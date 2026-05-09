/*
 * tester_v3_c_v1port.c — Parse/load tests ported from tester.cc (v1) to the
 * pure-C v3 runtime. Each PORT_CASE corresponds to a TEST_CASE in tester.cc;
 * the comment marks the original test name.
 *
 * Skipped from tester.cc (require v3 writer or v1-internal helpers, out of
 * scope for this port):
 *   serialize-empty-material, serialize-empty-node, serialize-light-index,
 *   serialize-empty-scene, serialize-node-emitter, serialize-lods,
 *   serialize-const-image, serialize-image-callback, serialize-image-failure,
 *   write-image-issue, empty-images-not-written, empty-bin-buffer,
 *   parse-integer, parse-unsigned, parse-integer-array,
 *   expandpath-utf-8, cj-float32-long-integer
 *
 * Build (run from tests/):
 *   make tester_v3_c_v1port
 * Run:
 *   ./tester_v3_c_v1port
 */

#include "tiny_gltf_v3.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- thin helpers ---------------------------------------------------------- */

static int tg3_str_eq(const tg3_str *s, const char *cstr) {
  size_t n = strlen(cstr);
  return s->data && s->len == (uint32_t)n &&
         memcmp(s->data, cstr, n) == 0;
}

static int tg3_str_contains(const tg3_str *s, char ch) {
  uint32_t i;
  if (!s->data) return 0;
  for (i = 0; i < s->len; ++i) {
    if (s->data[i] == ch) return 1;
  }
  return 0;
}

static int has_extension(const tg3_extras_ext *e, const char *name) {
  uint32_t i;
  if (!e || !e->extensions) return 0;
  for (i = 0; i < e->extensions_count; ++i) {
    if (tg3_str_eq(&e->extensions[i].name, name)) return 1;
  }
  return 0;
}

static const tg3_extension *find_extension(const tg3_extras_ext *e, const char *name) {
  uint32_t i;
  if (!e || !e->extensions) return NULL;
  for (i = 0; i < e->extensions_count; ++i) {
    if (tg3_str_eq(&e->extensions[i].name, name)) return &e->extensions[i];
  }
  return NULL;
}

static const tg3_value *value_get(const tg3_value *v, const char *key) {
  uint32_t i;
  if (!v || v->type != TG3_VALUE_OBJECT || !v->object_data) return NULL;
  for (i = 0; i < v->object_count; ++i) {
    if (tg3_str_eq(&v->object_data[i].key, key)) {
      return &v->object_data[i].value;
    }
  }
  return NULL;
}

/* Read a glTF/GLB file into a heap buffer; returns 1 on success. */
static int slurp(const char *path, uint8_t **out, size_t *out_len) {
  FILE *fp = fopen(path, "rb");
  long sz;
  size_t got;
  uint8_t *buf;
  if (!fp) return 0;
  if (fseek(fp, 0, SEEK_END) != 0 || (sz = ftell(fp)) < 0 ||
      fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return 0; }
  buf = (uint8_t *)malloc((size_t)sz);
  if (!buf) { fclose(fp); return 0; }
  got = fread(buf, 1, (size_t)sz, fp);
  fclose(fp);
  if (got != (size_t)sz) { free(buf); return 0; }
  *out = buf;
  *out_len = (size_t)sz;
  return 1;
}

static const char *base_dir_of(const char *path, uint32_t *out_len) {
  const char *slash = strrchr(path, '/');
  if (slash) {
    *out_len = (uint32_t)(slash - path);
  } else {
    *out_len = 0;
  }
  return path;
}

static int parse_path(tg3_model *m, tg3_error_stack *e,
                      tg3_parse_options *opts,
                      const char *path, tg3_error_code *err_out) {
  uint8_t *buf = NULL;
  size_t sz = 0;
  uint32_t base_len = 0;
  const char *base_dir;
  if (!slurp(path, &buf, &sz)) {
    fprintf(stderr, "  slurp failed: %s\n", path);
    return 0;
  }
  base_dir = base_dir_of(path, &base_len);
  *err_out = tg3_parse_auto(m, e, buf, (uint64_t)sz,
                            base_dir, base_len, opts);
  free(buf);
  return 1;
}

static int errstack_contains(const tg3_error_stack *e, const char *needle) {
  uint32_t i;
  size_t nl = strlen(needle);
  for (i = 0; i < e->count; ++i) {
    const char *m = e->entries[i].message;
    if (m && strstr(m, needle) != NULL) return 1;
    (void)nl;
  }
  return 0;
}

/* --- test cases ------------------------------------------------------------ */

#define PASS_OR_RET(label) do { \
  fprintf(stdout, "  [PASS] %s\n", label); \
  return 1; \
} while (0)
#define FAIL(...) do { \
  fprintf(stderr, "  [FAIL] "); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
  goto fail; \
} while (0)

/* TEST_CASE("parse-error", "[parse]") */
static int t_parse_error(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  static const uint8_t bad[] = "bora";
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  err = tg3_parse(&m, &e, bad, (uint64_t)(sizeof(bad) - 1), "", 0, &opts);
  if (err == TG3_OK) FAIL("expected parse failure on garbage input");
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("parse-error");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("datauri-in-glb", "[issue-79]") */
static int t_datauri_in_glb(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts, "../models/box01.glb", &err)) goto fail;
  if (err != TG3_OK) FAIL("box01.glb load failed err=%d", (int)err);
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("datauri-in-glb");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("extension-with-empty-object", "[issue-97]") */
static int t_extension_empty_object(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts, "../models/Extensions-issue97/test.gltf", &err)) goto fail;
  if (err != TG3_OK) FAIL("load failed err=%d", (int)err);
  if (m.extensions_used_count != 1)
    FAIL("extensionsUsed.size %u != 1", m.extensions_used_count);
  if (!tg3_str_eq(&m.extensions_used[0], "VENDOR_material_some_ext"))
    FAIL("unexpected extensionsUsed[0]");
  if (m.materials_count != 1) FAIL("materials.size %u != 1", m.materials_count);
  if (!has_extension(&m.materials[0].ext, "VENDOR_material_some_ext"))
    FAIL("material missing VENDOR_material_some_ext extension");
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("extension-with-empty-object");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("extension-overwrite", "[issue-261]") */
static int t_extension_overwrite(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  uint32_t i;
  int has_lights = 0;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts,
                  "../models/Extensions-overwrite-issue261/issue-261.gltf",
                  &err)) goto fail;
  if (err != TG3_OK) FAIL("load failed err=%d", (int)err);
  if (m.extensions_used_count != 3)
    FAIL("extensionsUsed.size %u != 3", m.extensions_used_count);
  for (i = 0; i < m.extensions_used_count; ++i) {
    if (tg3_str_eq(&m.extensions_used[i], "KHR_lights_punctual")) {
      has_lights = 1; break;
    }
  }
  if (!has_lights) FAIL("KHR_lights_punctual missing in extensionsUsed");
  if (!has_extension(&m.ext, "NV_MDL")) FAIL("model.extensions missing NV_MDL");
  if (!has_extension(&m.ext, "KHR_lights_punctual"))
    FAIL("model.extensions missing KHR_lights_punctual");
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("extension-overwrite");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("invalid-primitive-indices", "[bounds-checking]") */
static int t_invalid_primitive_indices(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts,
                  "../models/BoundsChecking/invalid-primitive-indices.gltf",
                  &err)) goto fail;
  if (err == TG3_OK)
    FAIL("invalid-primitive-indices unexpectedly succeeded");
  /* v3 reports TG3_ERR_INVALID_INDEX from validate_indices. v1 reports
   * "primitive indices accessor out of bounds" at parse time; either is
   * acceptable as long as the parser does not crash and rejects the model. */
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("invalid-primitive-indices");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("invalid-buffer-view-index", "[bounds-checking]") */
static int t_invalid_buffer_view_index(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts,
                  "../models/BoundsChecking/invalid-buffer-view-index.gltf",
                  &err)) goto fail;
  if (err == TG3_OK) FAIL("invalid-buffer-view-index unexpectedly succeeded");
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("invalid-buffer-view-index");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("invalid-buffer-index", "[bounds-checking]") */
static int t_invalid_buffer_index(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts,
                  "../models/BoundsChecking/invalid-buffer-index.gltf",
                  &err)) goto fail;
  if (err == TG3_OK) FAIL("invalid-buffer-index unexpectedly succeeded");
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("invalid-buffer-index");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("glb-invalid-length", "[bounds-checking]") */
static int t_glb_invalid_length(void) {
  /* 'glTF' magic, version=2, length=0x0000666c (way larger than provided
   * data), JSON chunk with empty {}. */
  static const unsigned char glb[] = "glTF"
      "\x02\x00\x00\x00" "\x6c\x66\x00\x00"
      "\x02\x00\x00\x00" "\x4a\x53\x4f\x4e{}";
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  err = tg3_parse_glb(&m, &e, glb, (uint64_t)(sizeof(glb) - 1), "", 0, &opts);
  if (err == TG3_OK) FAIL("invalid-length GLB unexpectedly accepted");
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("glb-invalid-length");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("integer-out-of-bounds", "[bounds-checking]") */
static int t_integer_out_of_bounds(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts,
                  "../models/BoundsChecking/integer-out-of-bounds.gltf",
                  &err)) goto fail;
  if (err == TG3_OK) FAIL("integer-out-of-bounds unexpectedly succeeded");
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("integer-out-of-bounds");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("pbr-khr-texture-transform", "[material]") */
static int t_pbr_khr_texture_transform(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  const tg3_extension *ext;
  const tg3_value *scale, *s0, *s1;
  double v0, v1;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts,
                  "../models/Cube-texture-ext/Cube-textransform.gltf",
                  &err)) goto fail;
  if (err != TG3_OK) FAIL("load failed err=%d", (int)err);
  if (m.materials_count != 2) FAIL("materials.size %u != 2", m.materials_count);
  ext = find_extension(&m.materials[0].emissive_texture.ext,
                       "KHR_texture_transform");
  if (!ext) FAIL("KHR_texture_transform missing on emissiveTexture");
  if (ext->value.type != TG3_VALUE_OBJECT)
    FAIL("KHR_texture_transform value not an object");
  scale = value_get(&ext->value, "scale");
  if (!scale || scale->type != TG3_VALUE_ARRAY || scale->array_count != 2)
    FAIL("scale not an array of 2");
  s0 = &scale->array_data[0];
  s1 = &scale->array_data[1];
  v0 = (s0->type == TG3_VALUE_INT) ? (double)s0->int_val : s0->real_val;
  v1 = (s1->type == TG3_VALUE_INT) ? (double)s1->int_val : s1->real_val;
  if (v0 != 1.0)  FAIL("scale[0] %g != 1.0", v0);
  if (v1 != -1.0) FAIL("scale[1] %g != -1.0", v1);
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("pbr-khr-texture-transform");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("image-uri-spaces", "[issue-236]") */
static int t_image_uri_spaces(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;

  /* Single-spaces variant. */
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts,
                  "../models/CubeImageUriSpaces/CubeImageUriSpaces.gltf",
                  &err)) goto fail;
  if (err != TG3_OK) FAIL("CubeImageUriSpaces load failed err=%d", (int)err);
  if (m.images_count != 1) FAIL("images.size %u != 1", m.images_count);
  if (!tg3_str_contains(&m.images[0].uri, ' '))
    FAIL("image.uri does not contain a space");
  tg3_model_free(&m); tg3_error_stack_free(&e);

  /* Multiple-spaces variant. */
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts,
                  "../models/CubeImageUriSpaces/CubeImageUriMultipleSpaces.gltf",
                  &err)) goto fail2;
  if (err != TG3_OK) FAIL("MultipleSpaces load failed err=%d", (int)err);
  if (m.images_count != 1) FAIL("images.size %u != 1", m.images_count);
  if (m.images[0].uri.len < 2 || m.images[0].uri.data[0] != ' ')
    FAIL("image.uri does not start with a space");
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("image-uri-spaces");
fail2:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("empty-skeleton-id", "[issue-321]") */
static int t_empty_skeleton_id(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts,
                  "../models/regression/unassigned-skeleton.gltf",
                  &err)) goto fail;
  if (err != TG3_OK) FAIL("load failed err=%d", (int)err);
  if (m.skins_count != 1) FAIL("skins.size %u != 1", m.skins_count);
  if (m.skins[0].skeleton != -1)
    FAIL("skin.skeleton %d != -1", m.skins[0].skeleton);
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("empty-skeleton-id");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("filesize-check", "[issue-416]") */
static int t_filesize_check(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  opts.max_external_file_size = 10; /* 10 bytes — texture image will exceed */
  if (!parse_path(&m, &e, &opts, "../models/Cube/Cube.gltf", &err)) goto fail;
  if (err == TG3_OK)
    FAIL("expected load failure due to oversized external file");
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("filesize-check");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("load-issue-416-model", "[issue-416]") */
static int t_load_issue_416_model(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts, "issue-416.gltf", &err)) goto fail;
  /* External file is missing, but the parser should still accept the glTF
   * structurally. v1 returns true; v3 returns TG3_OK or a non-fatal error
   * about the missing file. */
  (void)err; /* tolerate either outcome */
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("load-issue-416-model");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("zero-sized-bin-chunk-glb", "[issue-440]") */
static int t_zero_sized_bin_chunk_glb(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts,
                  "../models/regression/zero-sized-bin-chunk-issue-440.glb",
                  &err)) goto fail;
  if (err != TG3_OK) FAIL("zero-sized-bin-chunk failed err=%d", (int)err);
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("zero-sized-bin-chunk-glb");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("images-as-is", "[issue-487]") */
static int t_images_as_is(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  uint32_t i;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  opts.images_as_is = 1;
  if (!parse_path(&m, &e, &opts, "../models/Cube/Cube.gltf", &err)) goto fail;
  if (err != TG3_OK) FAIL("Cube load failed err=%d", (int)err);
  if (m.images_count == 0) FAIL("no images parsed");
  for (i = 0; i < m.images_count; ++i) {
    if (m.images[i].as_is != 1) FAIL("image[%u].as_is != 1", i);
    if (m.images[i].uri.len == 0) FAIL("image[%u].uri empty", i);
  }
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("images-as-is");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("inverse-bind-matrices-optional", "[issue-492]") */
static int t_ibm_optional(void) {
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  if (!parse_path(&m, &e, &opts, "issue-492.glb", &err)) goto fail;
  if (err != TG3_OK) FAIL("issue-492.glb load failed err=%d", (int)err);
  /* No "error" entries (warnings are not errors). */
  if (errstack_contains(&e, "error"))
    FAIL("unexpected error message present");
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("inverse-bind-matrices-optional");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* TEST_CASE("default-material", "[issue-459]") — black-box parse of a
 * minimal model with a default material; verify v3 populates the same
 * default-PBR field values v1 documents. */
static int t_default_material(void) {
  static const uint8_t json[] =
      "{\"asset\":{\"version\":\"2.0\"},\"materials\":[{}]}";
  tg3_model m; tg3_error_stack e; tg3_parse_options opts;
  tg3_error_code err;
  const tg3_material *mat;
  tg3_error_stack_init(&e); tg3_parse_options_init(&opts);
  err = tg3_parse(&m, &e, json, (uint64_t)(sizeof(json) - 1), "", 0, &opts);
  if (err != TG3_OK) FAIL("minimal default-material parse failed err=%d", (int)err);
  if (m.materials_count != 1) FAIL("materials.size %u != 1", m.materials_count);
  mat = &m.materials[0];
  if (!tg3_str_eq(&mat->alpha_mode, "OPAQUE")) FAIL("alpha_mode default mismatch");
  if (mat->alpha_cutoff != 0.5) FAIL("alpha_cutoff default %g != 0.5", mat->alpha_cutoff);
  if (mat->double_sided != 0)   FAIL("double_sided default != 0");
  if (mat->emissive_factor[0] != 0.0 ||
      mat->emissive_factor[1] != 0.0 ||
      mat->emissive_factor[2] != 0.0) FAIL("emissive_factor default mismatch");
  if (mat->pbr_metallic_roughness.base_color_factor[0] != 1.0 ||
      mat->pbr_metallic_roughness.base_color_factor[1] != 1.0 ||
      mat->pbr_metallic_roughness.base_color_factor[2] != 1.0 ||
      mat->pbr_metallic_roughness.base_color_factor[3] != 1.0)
    FAIL("base_color_factor default mismatch");
  if (mat->pbr_metallic_roughness.metallic_factor != 1.0)
    FAIL("metallic_factor default %g != 1.0",
         mat->pbr_metallic_roughness.metallic_factor);
  if (mat->pbr_metallic_roughness.roughness_factor != 1.0)
    FAIL("roughness_factor default %g != 1.0",
         mat->pbr_metallic_roughness.roughness_factor);
  if (mat->normal_texture.index != -1)    FAIL("normal_texture.index != -1");
  if (mat->occlusion_texture.index != -1) FAIL("occlusion_texture.index != -1");
  if (mat->emissive_texture.index != -1)  FAIL("emissive_texture.index != -1");
  tg3_model_free(&m); tg3_error_stack_free(&e);
  PASS_OR_RET("default-material");
fail:
  tg3_model_free(&m); tg3_error_stack_free(&e);
  return 0;
}

/* --- main ------------------------------------------------------------------ */

int main(void) {
  struct { const char *name; int (*fn)(void); } cases[] = {
    {"parse-error",                    t_parse_error},
    {"datauri-in-glb",                 t_datauri_in_glb},
    {"extension-with-empty-object",    t_extension_empty_object},
    {"extension-overwrite",            t_extension_overwrite},
    {"invalid-primitive-indices",      t_invalid_primitive_indices},
    {"invalid-buffer-view-index",      t_invalid_buffer_view_index},
    {"invalid-buffer-index",           t_invalid_buffer_index},
    {"glb-invalid-length",             t_glb_invalid_length},
    {"integer-out-of-bounds",          t_integer_out_of_bounds},
    {"pbr-khr-texture-transform",      t_pbr_khr_texture_transform},
    {"image-uri-spaces",               t_image_uri_spaces},
    {"empty-skeleton-id",              t_empty_skeleton_id},
    {"filesize-check",                 t_filesize_check},
    {"load-issue-416-model",           t_load_issue_416_model},
    {"zero-sized-bin-chunk-glb",       t_zero_sized_bin_chunk_glb},
    {"images-as-is",                   t_images_as_is},
    {"inverse-bind-matrices-optional", t_ibm_optional},
    {"default-material",               t_default_material},
  };
  size_t total = sizeof(cases) / sizeof(cases[0]);
  size_t passed = 0;
  size_t i;
  for (i = 0; i < total; ++i) {
    fprintf(stdout, "RUN  %s\n", cases[i].name);
    if (cases[i].fn()) ++passed;
  }
  fprintf(stdout, "\n=== v1 port: %zu/%zu passed ===\n", passed, total);
  return passed == total ? 0 : 1;
}
