#include "tiny_gltf_v3.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Digest helpers (used to compare v1 vs v3 parses) ===================== */

static uint64_t fnv64(const uint8_t *data, uint64_t n) {
  uint64_t h = 0xcbf29ce484222325ULL;
  uint64_t i;
  for (i = 0; i < n; ++i) { h ^= data[i]; h *= 0x100000001b3ULL; }
  return h;
}

static void d_str(const tg3_str *s) {
  uint32_t i;
  putchar('"');
  if (s && s->data) {
    for (i = 0; i < s->len; ++i) {
      unsigned char c = (unsigned char)s->data[i];
      if (c == '"' || c == '\\') { putchar('\\'); putchar((char)c); }
      else if (c < 0x20 || c >= 0x7f) putchar('?');
      else putchar((char)c);
    }
  }
  putchar('"');
}

static void d_dbl(double v) { printf("%.7g", v); }

static void d_dbl_arr(const double *v, uint32_t n) {
  uint32_t i;
  putchar('[');
  for (i = 0; i < n; ++i) { if (i) putchar(','); d_dbl(v[i]); }
  putchar(']');
}

static int cmp_str_int_pair(const void *a, const void *b) {
  const tg3_str_int_pair *pa = (const tg3_str_int_pair *)a;
  const tg3_str_int_pair *pb = (const tg3_str_int_pair *)b;
  uint32_t la = pa->key.len, lb = pb->key.len;
  uint32_t m = la < lb ? la : lb;
  int r = memcmp(pa->key.data, pb->key.data, m);
  if (r) return r;
  return (la < lb) ? -1 : (la > lb ? 1 : 0);
}

static void d_attrs(const tg3_str_int_pair *attrs, uint32_t n) {
  tg3_str_int_pair *sorted;
  uint32_t i;
  if (n == 0) { fputs("[]", stdout); return; }
  sorted = (tg3_str_int_pair *)malloc(n * sizeof(*sorted));
  memcpy(sorted, attrs, n * sizeof(*sorted));
  qsort(sorted, n, sizeof(*sorted), cmp_str_int_pair);
  putchar('[');
  for (i = 0; i < n; ++i) {
    if (i) putchar(',');
    printf("%.*s:%d", (int)sorted[i].key.len, sorted[i].key.data, sorted[i].value);
  }
  putchar(']');
  free(sorted);
}

static void print_digest(const tg3_model *m) {
  uint32_t i, j;
  printf("DIGEST_BEGIN\n");

  printf("asset version=");
  d_str(&m->asset.version);
  printf(" generator=");
  d_str(&m->asset.generator);
  printf("\n");

  for (i = 0; i < m->buffers_count; ++i) {
    const tg3_buffer *b = &m->buffers[i];
    uint64_t h = b->data.data ? fnv64(b->data.data, b->data.count) : 0;
    printf("buffer %u byte_length=%llu fnv64=0x%016llx\n",
           i, (unsigned long long)b->data.count, (unsigned long long)h);
  }
  for (i = 0; i < m->buffer_views_count; ++i) {
    const tg3_buffer_view *bv = &m->buffer_views[i];
    printf("buffer_view %u buffer=%d byte_offset=%llu byte_length=%llu byte_stride=%u\n",
           i, bv->buffer, (unsigned long long)bv->byte_offset,
           (unsigned long long)bv->byte_length, bv->byte_stride);
  }
  for (i = 0; i < m->accessors_count; ++i) {
    const tg3_accessor *a = &m->accessors[i];
    printf("accessor %u buffer_view=%d byte_offset=%llu component_type=%d count=%llu type=%d normalized=%d min=",
           i, a->buffer_view, (unsigned long long)a->byte_offset, a->component_type,
           (unsigned long long)a->count, a->type, a->normalized);
    d_dbl_arr(a->min_values, a->min_values_count);
    printf(" max=");
    d_dbl_arr(a->max_values, a->max_values_count);
    printf(" sparse=%d\n", a->sparse.is_sparse);
  }
  for (i = 0; i < m->meshes_count; ++i) {
    const tg3_mesh *me = &m->meshes[i];
    printf("mesh %u primitives_count=%u weights_count=%u\n",
           i, me->primitives_count, me->weights_count);
    for (j = 0; j < me->primitives_count; ++j) {
      const tg3_primitive *p = &me->primitives[j];
      printf("prim %u %u indices=%d material=%d mode=%d attrs=", i, j,
             p->indices, p->material, p->mode);
      d_attrs(p->attributes, p->attributes_count);
      printf(" targets_count=%u\n", p->targets_count);
    }
  }
  for (i = 0; i < m->nodes_count; ++i) {
    const tg3_node *n = &m->nodes[i];
    printf("node %u mesh=%d skin=%d camera=%d light=%d children_count=%u has_matrix=%d t=",
           i, n->mesh, n->skin, n->camera, n->light, n->children_count, n->has_matrix);
    d_dbl_arr(n->translation, 3);
    printf(" r=");
    d_dbl_arr(n->rotation, 4);
    printf(" s=");
    d_dbl_arr(n->scale, 3);
    printf(" matrix=");
    d_dbl_arr(n->matrix, 16);
    printf(" weights_count=%u\n", n->weights_count);
  }
  for (i = 0; i < m->materials_count; ++i) {
    const tg3_material *mat = &m->materials[i];
    printf("material %u alpha_mode=", i);
    d_str(&mat->alpha_mode);
    printf(" alpha_cutoff=");
    d_dbl(mat->alpha_cutoff);
    printf(" double_sided=%d emissive=", mat->double_sided);
    d_dbl_arr(mat->emissive_factor, 3);
    printf(" base_color_factor=");
    d_dbl_arr(mat->pbr_metallic_roughness.base_color_factor, 4);
    printf(" metallic=");
    d_dbl(mat->pbr_metallic_roughness.metallic_factor);
    printf(" roughness=");
    d_dbl(mat->pbr_metallic_roughness.roughness_factor);
    printf(" base_color_tex=%d normal_tex=%d occlusion_tex=%d emissive_tex=%d\n",
           mat->pbr_metallic_roughness.base_color_texture.index,
           mat->normal_texture.index,
           mat->occlusion_texture.index,
           mat->emissive_texture.index);
  }
  for (i = 0; i < m->textures_count; ++i) {
    const tg3_texture *t = &m->textures[i];
    printf("texture %u source=%d sampler=%d\n", i, t->source, t->sampler);
  }
  for (i = 0; i < m->samplers_count; ++i) {
    const tg3_sampler *s = &m->samplers[i];
    printf("sampler %u min_filter=%d mag_filter=%d wrap_s=%d wrap_t=%d\n",
           i, s->min_filter, s->mag_filter, s->wrap_s, s->wrap_t);
  }
  for (i = 0; i < m->images_count; ++i) {
    const tg3_image *im = &m->images[i];
    /* mime_type and uri normalization differ between v1/v3 (data URIs,
       extension inference); buffer_view reference is the parse-fidelity bit. */
    printf("image %u buffer_view=%d\n", i, im->buffer_view);
  }
  for (i = 0; i < m->skins_count; ++i) {
    const tg3_skin *s = &m->skins[i];
    printf("skin %u inverse_bind_matrices=%d skeleton=%d joints_count=%u\n",
           i, s->inverse_bind_matrices, s->skeleton, s->joints_count);
  }
  for (i = 0; i < m->animations_count; ++i) {
    const tg3_animation *a = &m->animations[i];
    printf("animation %u channels_count=%u samplers_count=%u\n",
           i, a->channels_count, a->samplers_count);
    for (j = 0; j < a->channels_count; ++j) {
      const tg3_animation_channel *c = &a->channels[j];
      printf("chan %u %u sampler=%d target_node=%d target_path=", i, j,
             c->sampler, c->target.node);
      d_str(&c->target.path);
      printf("\n");
    }
    for (j = 0; j < a->samplers_count; ++j) {
      const tg3_animation_sampler *as = &a->samplers[j];
      printf("samp %u %u input=%d output=%d interpolation=", i, j,
             as->input, as->output);
      d_str(&as->interpolation);
      printf("\n");
    }
  }
  for (i = 0; i < m->cameras_count; ++i) {
    const tg3_camera *c = &m->cameras[i];
    int is_persp = (c->type.len == 11 && memcmp(c->type.data, "perspective", 11) == 0);
    printf("camera %u type=", i);
    d_str(&c->type);
    if (is_persp) {
      printf(" yfov=");
      d_dbl(c->perspective.yfov);
      printf(" znear=");
      d_dbl(c->perspective.znear);
      printf(" zfar=");
      d_dbl(c->perspective.zfar);
      printf(" aspect=");
      d_dbl(c->perspective.aspect_ratio);
    } else {
      printf(" xmag=");
      d_dbl(c->orthographic.xmag);
      printf(" ymag=");
      d_dbl(c->orthographic.ymag);
      printf(" znear=");
      d_dbl(c->orthographic.znear);
      printf(" zfar=");
      d_dbl(c->orthographic.zfar);
    }
    printf("\n");
  }
  for (i = 0; i < m->scenes_count; ++i) {
    const tg3_scene *s = &m->scenes[i];
    printf("scene %u nodes_count=%u\n", i, s->nodes_count);
  }
  printf("DIGEST_END\n");
}

/* ============================================================================ */

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

  err = tg3_parse_file(&model, &errors,
                       "tg3-tester-nonexistent-path.gltf", 32, &opts);
  if (err != TG3_ERR_FS_NOT_AVAILABLE && err != TG3_ERR_FILE_NOT_FOUND) {
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

/* ===== Security regression tests ============================================ */

/* fs read_file callback that records calls into *(int *)user_data and never
 * succeeds — used to verify path-traversal URIs never reach the filesystem. */
static int32_t recording_read_file(uint8_t **out_data, uint64_t *out_size,
                                   const char *path, uint32_t path_len,
                                   void *user_data) {
  (void)out_data; (void)out_size; (void)path; (void)path_len;
  if (user_data) *(int *)user_data += 1;
  return 0;
}

static int check_path_traversal_rejected(void) {
  static const uint8_t json[] =
      "{\"asset\":{\"version\":\"2.0\"},"
      "\"buffers\":[{\"uri\":\"../../etc/passwd\",\"byteLength\":4}]}";
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;
  int fs_calls = 0;
  uint32_t i;
  int saw = 0;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  opts.fs.read_file = recording_read_file;
  opts.fs.user_data = &fs_calls;
  err = tg3_parse(&model, &errors, json, (uint64_t)(sizeof(json) - 1),
                  "/some/base", 10, &opts);
  if (err == TG3_OK) {
    fprintf(stderr, "path traversal NOT rejected\n");
    goto fail;
  }
  if (fs_calls != 0) {
    fprintf(stderr, "fs.read_file called %d times for traversal URI\n", fs_calls);
    goto fail;
  }
  for (i = 0; i < errors.count; ++i) {
    if (errors.entries[i].code == TG3_ERR_INVALID_VALUE) saw = 1;
  }
  if (!saw) {
    fprintf(stderr, "expected TG3_ERR_INVALID_VALUE on traversal\n");
    goto fail;
  }
  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 1;
fail:
  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 0;
}

static int check_absolute_uri_rejected(void) {
  static const uint8_t json[] =
      "{\"asset\":{\"version\":\"2.0\"},"
      "\"buffers\":[{\"uri\":\"/etc/passwd\",\"byteLength\":4}]}";
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;
  int fs_calls = 0;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  opts.fs.read_file = recording_read_file;
  opts.fs.user_data = &fs_calls;
  err = tg3_parse(&model, &errors, json, (uint64_t)(sizeof(json) - 1),
                  "/base", 5, &opts);
  if (err == TG3_OK || fs_calls != 0) {
    fprintf(stderr, "absolute URI not rejected (rc=%d, fs_calls=%d)\n",
            (int)err, fs_calls);
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }
  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 1;
}

static int check_negative_byte_stride_rejected(void) {
  static const uint8_t json[] =
      "{\"asset\":{\"version\":\"2.0\"},"
      "\"buffers\":[{\"byteLength\":4}],"
      "\"bufferViews\":[{\"buffer\":0,\"byteLength\":4,\"byteStride\":-1}]}";
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  err = tg3_parse(&model, &errors, json, (uint64_t)(sizeof(json) - 1), "", 0, &opts);
  if (err == TG3_OK) {
    fprintf(stderr, "negative byteStride NOT rejected\n");
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }
  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 1;
}

static int check_oob_index_rejected(void) {
  static const uint8_t json[] =
      "{\"asset\":{\"version\":\"2.0\"},"
      "\"buffers\":[{\"byteLength\":4}],"
      "\"bufferViews\":[{\"buffer\":0,\"byteLength\":4}],"
      "\"accessors\":[{\"bufferView\":1000000,\"componentType\":5121,"
      "\"count\":1,\"type\":\"SCALAR\"}]}";
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  err = tg3_parse(&model, &errors, json, (uint64_t)(sizeof(json) - 1), "", 0, &opts);
  if (err != TG3_ERR_INVALID_INDEX) {
    fprintf(stderr, "OOB index expected TG3_ERR_INVALID_INDEX, got %d\n", (int)err);
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }
  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 1;
}

static int check_oob_index_opt_in(void) {
  /* When validate_indices=0, parse should accept the same out-of-range index. */
  static const uint8_t json[] =
      "{\"asset\":{\"version\":\"2.0\"},"
      "\"buffers\":[{\"byteLength\":4}],"
      "\"bufferViews\":[{\"buffer\":0,\"byteLength\":4}],"
      "\"accessors\":[{\"bufferView\":1000000,\"componentType\":5121,"
      "\"count\":1,\"type\":\"SCALAR\"}]}";
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  opts.validate_indices = 0;
  err = tg3_parse(&model, &errors, json, (uint64_t)(sizeof(json) - 1), "", 0, &opts);
  if (err != TG3_OK) {
    fprintf(stderr, "validate_indices=0 should accept OOB index, got %d\n", (int)err);
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }
  if (model.accessors_count != 1 || model.accessors[0].buffer_view != 1000000) {
    fprintf(stderr, "OOB index not preserved when validation off\n");
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }
  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 1;
}

static int check_extension_index_oob_rejected(void) {
  /* MSFT_lod and KHR_audio index fields must be validated when
   * validate_indices=1, otherwise downstream consumers can read OOB. */
  static const uint8_t json[] =
      "{\"asset\":{\"version\":\"2.0\"},"
      "\"nodes\":[{\"extensions\":{\"MSFT_lod\":{\"ids\":[99999]}}}]}";
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  err = tg3_parse(&model, &errors, json, (uint64_t)(sizeof(json) - 1), "", 0, &opts);
  if (err != TG3_ERR_INVALID_INDEX) {
    fprintf(stderr, "MSFT_lod OOB index expected TG3_ERR_INVALID_INDEX, got %d\n", (int)err);
    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return 0;
  }
  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 1;
}

static int check_error_messages_survive_parse_failure(void) {
  /* Regression: parse failure must not invalidate arena-allocated error
   * message strings on the user's tg3_error_stack before model_free. */
  static const uint8_t json[] =
      "{\"asset\":{\"version\":\"2.0\"},"
      "\"buffers\":[{\"byteLength\":4}],"
      "\"bufferViews\":[{\"buffer\":0,\"byteLength\":4,\"byteStride\":-1}]}";
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;
  uint32_t i;
  int seen_stride_msg = 0;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  err = tg3_parse(&model, &errors, json, (uint64_t)(sizeof(json) - 1), "", 0, &opts);
  if (err == TG3_OK) goto fail;
  for (i = 0; i < errors.count; ++i) {
    const char *m = errors.entries[i].message;
    if (m && strstr(m, "byteStride")) seen_stride_msg = 1;
  }
  if (!seen_stride_msg) {
    fprintf(stderr, "byteStride error message missing or unreadable after parse\n");
    goto fail;
  }
  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 1;
fail:
  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  return 0;
}

static int parse_file_arg(const char *path) {
  FILE *fp = fopen(path, "rb");
  uint8_t *buf;
  long sz;
  size_t got;
  tg3_model model;
  tg3_error_stack errors;
  tg3_parse_options opts;
  tg3_error_code err;
  int ok;
  const char *slash;
  size_t base_len;

  if (!fp) {
    fprintf(stderr, "open failed: %s\n", path);
    return 0;
  }
  if (fseek(fp, 0, SEEK_END) != 0 || (sz = ftell(fp)) < 0 ||
      fseek(fp, 0, SEEK_SET) != 0) {
    fprintf(stderr, "seek failed: %s\n", path);
    fclose(fp);
    return 0;
  }
  buf = (uint8_t *)malloc((size_t)sz);
  if (!buf) {
    fprintf(stderr, "alloc failed: %s\n", path);
    fclose(fp);
    return 0;
  }
  got = fread(buf, 1, (size_t)sz, fp);
  fclose(fp);
  if (got != (size_t)sz) {
    fprintf(stderr, "short read: %s\n", path);
    free(buf);
    return 0;
  }

  slash = strrchr(path, '/');
  base_len = slash ? (size_t)(slash - path) : 0;

  tg3_error_stack_init(&errors);
  tg3_parse_options_init(&opts);
  err = tg3_parse_auto(&model, &errors, buf, (uint64_t)sz,
                       path, (uint32_t)base_len, &opts);
  ok = (err == TG3_OK);
  if (!ok) {
    uint32_t i;
    fprintf(stderr, "parse failed (%d): %s\n", (int)err, path);
    for (i = 0; i < errors.count; ++i) {
      fprintf(stderr, "  [%u] code=%d sev=%d path=%s msg=%s offset=%lld\n",
              i, (int)errors.entries[i].code, (int)errors.entries[i].severity,
              errors.entries[i].json_path ? errors.entries[i].json_path : "",
              errors.entries[i].message ? errors.entries[i].message : "",
              (long long)errors.entries[i].byte_offset);
    }
  } else {
    printf("COUNTS"
           " accessors=%u animations=%u buffers=%u bufferViews=%u"
           " cameras=%u images=%u materials=%u meshes=%u nodes=%u"
           " samplers=%u scenes=%u skins=%u textures=%u lights=%u\n",
           model.accessors_count, model.animations_count,
           model.buffers_count, model.buffer_views_count,
           model.cameras_count, model.images_count,
           model.materials_count, model.meshes_count,
           model.nodes_count, model.samplers_count,
           model.scenes_count, model.skins_count,
           model.textures_count, model.lights_count);
    print_digest(&model);
  }
  tg3_model_free(&model);
  tg3_error_stack_free(&errors);
  free(buf);
  return ok;
}

int main(int argc, char **argv) {
  if (argc > 1) {
    int i;
    for (i = 1; i < argc; ++i) {
      if (!parse_file_arg(argv[i])) return 1;
    }
    return 0;
  }
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
  if (!check_path_traversal_rejected()) {
    return 1;
  }
  if (!check_absolute_uri_rejected()) {
    return 1;
  }
  if (!check_negative_byte_stride_rejected()) {
    return 1;
  }
  if (!check_oob_index_rejected()) {
    return 1;
  }
  if (!check_oob_index_opt_in()) {
    return 1;
  }
  if (!check_extension_index_oob_rejected()) {
    return 1;
  }
  if (!check_error_messages_survive_parse_failure()) {
    return 1;
  }
  return 0;
}
