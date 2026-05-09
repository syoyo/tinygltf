#ifndef TINYGLTF3_SOURCE_INCLUDED_FROM_HEADER
#include "tiny_gltf_v3.h"
#endif
#define TINYGLTF_JSON_C_IMPLEMENTATION
#include "tinygltf_json_c.h"

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TG3__ARENA_DEFAULT_BLOCK_SIZE (256u * 1024u)
#define TG3__ARENA_ALIGNMENT 8u

typedef struct tg3__arena_block {
    struct tg3__arena_block *next;
    uint8_t *base;
    size_t used;
    size_t capacity;
} tg3__arena_block;

typedef struct tg3_arena tg3_arena;

struct tg3_arena {
    tg3__arena_block *head;
    tg3__arena_block *current;
    size_t total_allocated;
    size_t memory_budget;
    size_t max_single_alloc;
    size_t block_size;
    tg3_allocator alloc;
};

typedef struct tg3__parse_ctx {
    tg3_arena *arena;
    tg3_error_stack *errors;
    tg3_parse_options opts;
    const char *base_dir;
    uint32_t base_dir_len;
    const uint8_t *bin_data;
    uint64_t bin_size;
    int32_t is_binary;
} tg3__parse_ctx;

struct tg3_writer {
    tg3_write_chunk_fn chunk_fn;
    void *user_data;
    tg3_write_options options;
    tg3json_value root;
    int begun;
};

static void *tg3__default_alloc(size_t size, void *ud) {
    (void)ud;
    return malloc(size);
}

static void *tg3__default_realloc(void *ptr, size_t old_size, size_t new_size, void *ud) {
    (void)old_size;
    (void)ud;
    return realloc(ptr, new_size);
}

static void tg3__default_free(void *ptr, size_t size, void *ud) {
    (void)size;
    (void)ud;
    free(ptr);
}

static tg3_arena *tg3__arena_create(const tg3_memory_config *config) {
    tg3_allocator alloc;
    tg3_arena *arena;
    if (config && config->allocator.alloc) {
        alloc = config->allocator;
    } else {
        alloc.alloc = tg3__default_alloc;
        alloc.realloc = tg3__default_realloc;
        alloc.free = tg3__default_free;
        alloc.user_data = NULL;
    }

    arena = (tg3_arena *)alloc.alloc(sizeof(tg3_arena), alloc.user_data);
    if (!arena) return NULL;
    memset(arena, 0, sizeof(*arena));
    arena->alloc = alloc;
    arena->block_size = (config && config->arena_block_size > 0)
                            ? (size_t)config->arena_block_size
                            : (size_t)TG3__ARENA_DEFAULT_BLOCK_SIZE;
    arena->memory_budget = (config && config->memory_budget > 0)
                               ? (size_t)config->memory_budget
                               : (size_t)TINYGLTF3_MAX_MEMORY_BYTES;
    arena->max_single_alloc = (config && config->max_single_alloc > 0)
                                  ? (size_t)config->max_single_alloc
                                  : 0;
    return arena;
}

static tg3__arena_block *tg3__arena_new_block(tg3_arena *arena, size_t min_size) {
    size_t cap = arena->block_size;
    void *raw;
    tg3__arena_block *block;
    if (cap < min_size) cap = min_size;
    if (arena->max_single_alloc && cap > arena->max_single_alloc) return NULL;
    if (arena->total_allocated + sizeof(tg3__arena_block) + cap > arena->memory_budget) return NULL;

    raw = arena->alloc.alloc(sizeof(tg3__arena_block) + cap, arena->alloc.user_data);
    if (!raw) return NULL;
    block = (tg3__arena_block *)raw;
    block->next = NULL;
    block->base = (uint8_t *)raw + sizeof(tg3__arena_block);
    block->used = 0;
    block->capacity = cap;
    arena->total_allocated += sizeof(tg3__arena_block) + cap;

    if (arena->current) arena->current->next = block;
    else arena->head = block;
    arena->current = block;
    return block;
}

static void *tg3__arena_alloc(tg3_arena *arena, size_t size) {
    tg3__arena_block *block;
    void *ptr;
    if (!arena || size == 0) return NULL;
    if (arena->max_single_alloc && size > arena->max_single_alloc) return NULL;
    size = (size + (TG3__ARENA_ALIGNMENT - 1u)) & ~(size_t)(TG3__ARENA_ALIGNMENT - 1u);
    block = arena->current;
    if (!block || (block->used + size > block->capacity)) {
        block = tg3__arena_new_block(arena, size);
        if (!block) return NULL;
    }
    ptr = block->base + block->used;
    block->used += size;
    return ptr;
}

static char *tg3__arena_strdup(tg3_arena *arena, const char *s, size_t len) {
    char *dst;
    if (!s) return NULL;
    dst = (char *)tg3__arena_alloc(arena, len + 1);
    if (!dst) return NULL;
    if (len > 0) memcpy(dst, s, len);
    dst[len] = '\0';
    return dst;
}

static tg3_str tg3__arena_str(tg3_arena *arena, const char *s, uint32_t len) {
    tg3_str out;
    out.data = tg3__arena_strdup(arena, s, len);
    out.len = out.data ? len : 0;
    return out;
}

static void tg3__arena_destroy(tg3_arena *arena) {
    tg3_allocator alloc;
    tg3__arena_block *block;
    if (!arena) return;
    alloc = arena->alloc;
    block = arena->head;
    while (block) {
        tg3__arena_block *next = block->next;
        alloc.free(block, sizeof(tg3__arena_block) + block->capacity, alloc.user_data);
        block = next;
    }
    alloc.free(arena, sizeof(*arena), alloc.user_data);
}

static void tg3__error_push(tg3_error_stack *es, tg3_severity sev,
                            tg3_error_code code, const char *msg,
                            const char *json_path, int64_t byte_offset) {
    tg3_error_entry *entry;
    tg3_error_entry *new_entries;
    uint32_t new_cap;
    if (!es) return;
    if (es->count >= es->capacity) {
        new_cap = es->capacity ? es->capacity * 2u : 16u;
        new_entries = (tg3_error_entry *)realloc(es->entries, new_cap * sizeof(tg3_error_entry));
        if (!new_entries) return;
        es->entries = new_entries;
        es->capacity = new_cap;
    }
    entry = &es->entries[es->count++];
    entry->severity = sev;
    entry->code = code;
    entry->message = msg;
    entry->json_path = json_path;
    entry->byte_offset = byte_offset;
    if (sev == TG3_SEVERITY_ERROR) es->has_error = 1;
}

static void tg3__error_pushf(tg3_error_stack *es, tg3_arena *arena,
                             tg3_severity sev, tg3_error_code code,
                             const char *json_path, const char *fmt, ...) {
    char buf[1024];
    int n;
    va_list ap;
    const char *msg = buf;
    if (!es) return;
    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) n = 0;
    if ((size_t)n >= sizeof(buf)) n = (int)(sizeof(buf) - 1u);
    if (arena) {
        char *dup = tg3__arena_strdup(arena, buf, (size_t)n);
        if (dup) msg = dup;
    }
    tg3__error_push(es, sev, code, msg, json_path, -1);
}

TINYGLTF3_API int32_t tg3_errors_has_error(const tg3_error_stack *es) { return es ? es->has_error : 0; }
TINYGLTF3_API uint32_t tg3_errors_count(const tg3_error_stack *es) { return es ? es->count : 0; }
TINYGLTF3_API const tg3_error_entry *tg3_errors_get(const tg3_error_stack *es, uint32_t index) {
    if (!es || index >= es->count) return NULL;
    return &es->entries[index];
}
TINYGLTF3_API void tg3_error_stack_init(tg3_error_stack *es) { if (es) memset(es, 0, sizeof(*es)); }
TINYGLTF3_API void tg3_error_stack_free(tg3_error_stack *es) {
    if (!es) return;
    free(es->entries);
    memset(es, 0, sizeof(*es));
}

TINYGLTF3_API void tg3_parse_options_init(tg3_parse_options *options) {
    if (!options) return;
    memset(options, 0, sizeof(*options));
    options->required_sections = TG3_REQUIRE_VERSION;
    options->strictness = TG3_PERMISSIVE;
    options->memory.memory_budget = TINYGLTF3_MAX_MEMORY_BYTES;
    options->memory.arena_block_size = TG3__ARENA_DEFAULT_BLOCK_SIZE;
    options->max_external_file_size = 0;
    options->validate_indices = 1;
}

TINYGLTF3_API void tg3_write_options_init(tg3_write_options *options) {
    if (!options) return;
    memset(options, 0, sizeof(*options));
    options->pretty_print = 1;
    options->memory.memory_budget = TINYGLTF3_MAX_MEMORY_BYTES;
    options->memory.arena_block_size = TG3__ARENA_DEFAULT_BLOCK_SIZE;
}

TINYGLTF3_API int32_t tg3_component_size(int32_t component_type) {
    switch (component_type) {
        case TG3_COMPONENT_TYPE_BYTE:
        case TG3_COMPONENT_TYPE_UNSIGNED_BYTE: return 1;
        case TG3_COMPONENT_TYPE_SHORT:
        case TG3_COMPONENT_TYPE_UNSIGNED_SHORT: return 2;
        case TG3_COMPONENT_TYPE_INT:
        case TG3_COMPONENT_TYPE_UNSIGNED_INT:
        case TG3_COMPONENT_TYPE_FLOAT: return 4;
        case TG3_COMPONENT_TYPE_DOUBLE: return 8;
        default: return -1;
    }
}

TINYGLTF3_API int32_t tg3_num_components(int32_t type) {
    switch (type) {
        case TG3_TYPE_SCALAR: return 1;
        case TG3_TYPE_VEC2: return 2;
        case TG3_TYPE_VEC3: return 3;
        case TG3_TYPE_VEC4: return 4;
        case TG3_TYPE_MAT2: return 4;
        case TG3_TYPE_MAT3: return 9;
        case TG3_TYPE_MAT4: return 16;
        default: return -1;
    }
}

TINYGLTF3_API int32_t tg3_accessor_byte_stride(const tg3_accessor *accessor, const tg3_buffer_view *bv) {
    int32_t comp;
    int32_t num;
    if (bv && bv->byte_stride > 0) return (int32_t)bv->byte_stride;
    comp = tg3_component_size(accessor->component_type);
    num = tg3_num_components(accessor->type);
    if (comp < 0 || num < 0) return -1;
    return comp * num;
}

TINYGLTF3_API int32_t tg3_str_equals(tg3_str a, tg3_str b) {
    if (a.len != b.len) return 0;
    if (a.len == 0) return 1;
    return memcmp(a.data, b.data, a.len) == 0 ? 1 : 0;
}

TINYGLTF3_API int32_t tg3_str_equals_cstr(tg3_str a, const char *b) {
    uint32_t blen;
    if (!b) return a.len == 0 ? 1 : 0;
    blen = (uint32_t)strlen(b);
    if (a.len != blen) return 0;
    if (a.len == 0) return 1;
    return memcmp(a.data, b, a.len) == 0 ? 1 : 0;
}

static int tg3__b64_decode_char(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static uint8_t *tg3__b64_decode(const char *input, size_t input_len, size_t *out_len, tg3_arena *arena) {
    uint8_t *out;
    size_t decoded_len;
    size_t i;
    size_t j = 0;
    uint32_t accum = 0;
    int bits = 0;
    if (input_len == 0) {
        *out_len = 0;
        return NULL;
    }
    while (input_len > 0 && input[input_len - 1] == '=') --input_len;
    decoded_len = (input_len * 3u) / 4u;
    out = (uint8_t *)tg3__arena_alloc(arena, decoded_len + 1u);
    if (!out) {
        *out_len = 0;
        return NULL;
    }
    for (i = 0; i < input_len; ++i) {
        int val = tg3__b64_decode_char((unsigned char)input[i]);
        if (val < 0) continue;
        accum = (accum << 6) | (uint32_t)val;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[j++] = (uint8_t)((accum >> bits) & 0xFFu);
        }
    }
    *out_len = j;
    return out;
}

TINYGLTF3_API int32_t tg3_is_data_uri(const char *uri, uint32_t len) {
    return (uri && len >= 5u && memcmp(uri, "data:", 5) == 0) ? 1 : 0;
}

typedef struct tg3__data_uri_result {
    const char *data_start;
    size_t data_len;
    char mime_type[64];
} tg3__data_uri_result;

static int tg3__parse_data_uri(const char *uri, uint32_t uri_len, tg3__data_uri_result *result) {
    const char *p;
    const char *end;
    const char *semi;
    size_t mime_len;
    if (!uri || uri_len < 5u || memcmp(uri, "data:", 5) != 0) return 0;
    p = uri + 5;
    end = uri + uri_len;
    semi = p;
    while (semi < end && *semi != ';') ++semi;
    if (semi >= end) return 0;
    mime_len = (size_t)(semi - p);
    if (mime_len >= sizeof(result->mime_type)) mime_len = sizeof(result->mime_type) - 1u;
    memcpy(result->mime_type, p, mime_len);
    result->mime_type[mime_len] = '\0';
    p = semi + 1;
    if ((size_t)(end - p) < 7u || memcmp(p, "base64,", 7) != 0) return 0;
    p += 7;
    result->data_start = p;
    result->data_len = (size_t)(end - p);
    return 1;
}

static uint8_t *tg3__decode_data_uri(tg3_arena *arena, const char *uri, uint32_t uri_len,
                                     size_t *out_len, char *out_mime, size_t out_mime_cap) {
    tg3__data_uri_result dr;
    size_t mlen;
    if (!tg3__parse_data_uri(uri, uri_len, &dr)) {
        *out_len = 0;
        return NULL;
    }
    if (out_mime && out_mime_cap > 0) {
        mlen = strlen(dr.mime_type);
        if (mlen >= out_mime_cap) mlen = out_mime_cap - 1u;
        memcpy(out_mime, dr.mime_type, mlen);
        out_mime[mlen] = '\0';
    }
    return tg3__b64_decode(dr.data_start, dr.data_len, out_len, arena);
}

#ifdef TINYGLTF3_ENABLE_FS
static int32_t tg3__fs_file_exists(const char *path, uint32_t path_len, void *ud) {
    FILE *fp;
    (void)path_len; (void)ud;
    fp = fopen(path, "rb");
    if (!fp) return 0;
    fclose(fp);
    return 1;
}

static int32_t tg3__fs_read_file(uint8_t **out_data, uint64_t *out_size,
                                 const char *path, uint32_t path_len, void *ud) {
    FILE *fp;
    long size;
    uint8_t *data;
    size_t nread;
    (void)path_len; (void)ud;
    *out_data = NULL;
    *out_size = 0;
    fp = fopen(path, "rb");
    if (!fp) return 0;
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return 0; }
    size = ftell(fp);
    if (size < 0) { fclose(fp); return 0; }
    if (fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return 0; }
    data = (uint8_t *)malloc((size_t)size);
    if (!data) { fclose(fp); return 0; }
    nread = fread(data, 1, (size_t)size, fp);
    fclose(fp);
    if (nread != (size_t)size) { free(data); return 0; }
    *out_data = data;
    *out_size = (uint64_t)size;
    return 1;
}

static void tg3__fs_free_file(uint8_t *data, uint64_t size, void *ud) {
    (void)size; (void)ud; free(data);
}

static int32_t tg3__fs_write_file(const char *path, uint32_t path_len,
                                  const uint8_t *data, uint64_t size, void *ud) {
    FILE *fp;
    size_t nwritten;
    (void)path_len; (void)ud;
    fp = fopen(path, "wb");
    if (!fp) return 0;
    nwritten = fwrite(data, 1, (size_t)size, fp);
    fclose(fp);
    return nwritten == (size_t)size ? 1 : 0;
}

static void tg3__set_default_fs(tg3_fs_callbacks *fs) {
    if (!fs->read_file) fs->read_file = tg3__fs_read_file;
    if (!fs->free_file) fs->free_file = tg3__fs_free_file;
    if (!fs->file_exists) fs->file_exists = tg3__fs_file_exists;
    if (!fs->write_file) fs->write_file = tg3__fs_write_file;
}
#endif

static void tg3__model_init(tg3_model *model) {
    memset(model, 0, sizeof(*model));
    model->default_scene = -1;
}

static int tg3__json_is_number(const tg3json_value *v) {
    return v && (v->type == TG3JSON_INT || v->type == TG3JSON_REAL);
}

static int tg3__json_number_to_int32(const tg3json_value *v, int32_t *out) {
    double real;
    int32_t converted;
    if (!tg3__json_is_number(v) || !out) return 0;
    if (v->type == TG3JSON_INT) {
        if (v->u.integer < INT32_MIN || v->u.integer > INT32_MAX) return 0;
        *out = (int32_t)v->u.integer;
        return 1;
    }
    real = v->u.real;
    if (!isfinite(real) || real < (double)INT32_MIN || real > (double)INT32_MAX) {
        return 0;
    }
    converted = (int32_t)real;
    if ((double)converted != real) return 0;
    *out = converted;
    return 1;
}

static int tg3__json_number_to_uint64(const tg3json_value *v, uint64_t *out) {
    double real;
    uint64_t converted;
    /* Doubles have a 53-bit mantissa, so 2^64 itself rounds up and would be UB
       on cast to uint64_t. Cap at the largest representable value strictly
       below 2^64 (== 2^64 - 2^11). */
    const double max_safe_uint64_real = 18446744073709547520.0;
    if (!tg3__json_is_number(v) || !out) return 0;
    if (v->type == TG3JSON_INT) {
        if (v->u.integer < 0) return 0;
        *out = (uint64_t)v->u.integer;
        return 1;
    }
    real = v->u.real;
    if (!isfinite(real) || real < 0.0 || real > max_safe_uint64_real) {
        return 0;
    }
    converted = (uint64_t)real;
    if ((double)converted != real) return 0;
    *out = converted;
    return 1;
}

static double tg3__json_number_to_double(const tg3json_value *v) {
    if (!v) return 0.0;
    if (v->type == TG3JSON_INT) return (double)v->u.integer;
    if (v->type == TG3JSON_REAL) return v->u.real;
    return 0.0;
}

static int tg3__json_is_object(const tg3json_value *v) { return v && v->type == TG3JSON_OBJECT; }
static int tg3__json_is_array(const tg3json_value *v) { return v && v->type == TG3JSON_ARRAY; }

static int tg3__json_has(const tg3json_value *o, const char *key) {
    return tg3json_object_get(o, key) ? 1 : 0;
}

static const tg3json_value *tg3__json_get(const tg3json_value *o, const char *key) {
    return tg3json_object_get(o, key);
}

static int tg3__parse_string(tg3__parse_ctx *ctx, const tg3json_value *o, const char *key,
                             tg3_str *out, int required, const char *parent) {
    const tg3json_value *it = tg3__json_get(o, key);
    if (!it) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_MISSING_FIELD,
                             parent, "Missing required field '%s'", key);
            return 0;
        }
        out->data = NULL;
        out->len = 0;
        return 1;
    }
    if (it->type != TG3JSON_STRING) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH,
                         parent, "Field '%s' must be a string", key);
        return 0;
    }
    *out = tg3__arena_str(ctx->arena, it->u.string.ptr, (uint32_t)it->u.string.len);
    return out->data != NULL || it->u.string.len == 0;
}

static int tg3__parse_int(tg3__parse_ctx *ctx, const tg3json_value *o, const char *key,
                          int32_t *out, int required, const char *parent) {
    const tg3json_value *it = tg3__json_get(o, key);
    if (!it) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_MISSING_FIELD,
                             parent, "Missing required field '%s'", key);
            return 0;
        }
        return 1;
    }
    if (!tg3__json_is_number(it)) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH,
                         parent, "Field '%s' must be a number", key);
        return 0;
    }
    if (!tg3__json_number_to_int32(it, out)) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH,
                         parent, "Field '%s' must be a finite int32-valued number", key);
        return 0;
    }
    return 1;
}

static int tg3__parse_uint64(tg3__parse_ctx *ctx, const tg3json_value *o, const char *key,
                             uint64_t *out, int required, const char *parent) {
    const tg3json_value *it = tg3__json_get(o, key);
    if (!it) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_MISSING_FIELD,
                             parent, "Missing required field '%s'", key);
            return 0;
        }
        return 1;
    }
    if (!tg3__json_is_number(it)) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH,
                         parent, "Field '%s' must be a number", key);
        return 0;
    }
    if (!tg3__json_number_to_uint64(it, out)) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH,
                         parent, "Field '%s' must be a finite uint64-valued number", key);
        return 0;
    }
    return 1;
}

static int tg3__parse_double(tg3__parse_ctx *ctx, const tg3json_value *o, const char *key,
                             double *out, int required, const char *parent) {
    const tg3json_value *it = tg3__json_get(o, key);
    if (!it) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_MISSING_FIELD,
                             parent, "Missing required field '%s'", key);
            return 0;
        }
        return 1;
    }
    if (!tg3__json_is_number(it)) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH,
                         parent, "Field '%s' must be a number", key);
        return 0;
    }
    *out = tg3__json_number_to_double(it);
    return 1;
}

static int tg3__parse_bool(tg3__parse_ctx *ctx, const tg3json_value *o, const char *key,
                           int32_t *out, int required, const char *parent) {
    const tg3json_value *it = tg3__json_get(o, key);
    if (!it) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_MISSING_FIELD,
                             parent, "Missing required field '%s'", key);
            return 0;
        }
        return 1;
    }
    if (it->type != TG3JSON_BOOL) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH,
                         parent, "Field '%s' must be a boolean", key);
        return 0;
    }
    *out = it->u.boolean ? 1 : 0;
    return 1;
}

static int tg3__parse_number_array(tg3__parse_ctx *ctx, const tg3json_value *o, const char *key,
                                   const double **out, uint32_t *out_count,
                                   int required, const char *parent) {
    const tg3json_value *it = tg3__json_get(o, key);
    size_t count;
    size_t i;
    double *arr;
    if (!it) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_MISSING_FIELD,
                             parent, "Missing required field '%s'", key);
            return 0;
        }
        *out = NULL;
        *out_count = 0;
        return 1;
    }
    if (!tg3__json_is_array(it)) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH,
                         parent, "Field '%s' must be an array", key);
        return 0;
    }
    count = tg3json_array_size(it);
    if (count == 0) {
        *out = NULL;
        *out_count = 0;
        return 1;
    }
    arr = (double *)tg3__arena_alloc(ctx->arena, count * sizeof(double));
    if (!arr) {
        tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                        "OOM allocating number array", parent, -1);
        return 0;
    }
    for (i = 0; i < count; ++i) {
        arr[i] = tg3__json_number_to_double(tg3json_array_get(it, i));
    }
    *out = arr;
    *out_count = (uint32_t)count;
    return 1;
}

static int tg3__parse_int_array(tg3__parse_ctx *ctx, const tg3json_value *o, const char *key,
                                const int32_t **out, uint32_t *out_count,
                                int required, const char *parent) {
    const tg3json_value *it = tg3__json_get(o, key);
    size_t count;
    size_t i;
    int32_t *arr;
    if (!it) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_MISSING_FIELD,
                             parent, "Missing required field '%s'", key);
            return 0;
        }
        *out = NULL;
        *out_count = 0;
        return 1;
    }
    if (!tg3__json_is_array(it)) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH,
                         parent, "Field '%s' must be an array", key);
        return 0;
    }
    count = tg3json_array_size(it);
    if (count == 0) {
        *out = NULL;
        *out_count = 0;
        return 1;
    }
    arr = (int32_t *)tg3__arena_alloc(ctx->arena, count * sizeof(int32_t));
    if (!arr) {
        tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                        "OOM allocating int array", parent, -1);
        return 0;
    }
    for (i = 0; i < count; ++i) {
        const tg3json_value *item = tg3json_array_get(it, i);
        if (!item || !tg3__json_number_to_int32(item, &arr[i])) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                             TG3_ERR_JSON_TYPE_MISMATCH, parent,
                             "Field '%s' element %u must be a finite int32-valued number",
                             key, (unsigned)i);
            return 0;
        }
    }
    *out = arr;
    *out_count = (uint32_t)count;
    return 1;
}

static void tg3__parse_number_to_fixed(const tg3json_value *o, const char *key,
                                       double *out, uint32_t max_count) {
    const tg3json_value *it = tg3__json_get(o, key);
    size_t count;
    size_t i;
    if (!tg3__json_is_array(it)) return;
    count = tg3json_array_size(it);
    if (count > max_count) count = max_count;
    for (i = 0; i < count; ++i) {
        const tg3json_value *item = tg3json_array_get(it, i);
        if (!item) continue;
        out[i] = tg3__json_number_to_double(item);
    }
}

static int tg3__parse_string_array(tg3__parse_ctx *ctx, const tg3json_value *o, const char *key,
                                   const tg3_str **out, uint32_t *out_count,
                                   int required, const char *parent) {
    const tg3json_value *it = tg3__json_get(o, key);
    size_t count;
    size_t i;
    tg3_str *arr;
    if (!it) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_MISSING_FIELD,
                             parent, "Missing required field '%s'", key);
            return 0;
        }
        *out = NULL;
        *out_count = 0;
        return 1;
    }
    if (!tg3__json_is_array(it)) return 0;
    count = tg3json_array_size(it);
    if (count == 0) {
        *out = NULL;
        *out_count = 0;
        return 1;
    }
    arr = (tg3_str *)tg3__arena_alloc(ctx->arena, count * sizeof(tg3_str));
    if (!arr) return 0;
    for (i = 0; i < count; ++i) {
        const tg3json_value *item = tg3json_array_get(it, i);
        if (!item || item->type != TG3JSON_STRING) {
            arr[i].data = NULL;
            arr[i].len = 0;
            continue;
        }
        arr[i] = tg3__arena_str(ctx->arena, item->u.string.ptr, (uint32_t)item->u.string.len);
    }
    *out = arr;
    *out_count = (uint32_t)count;
    return 1;
}

static tg3_value tg3__json_to_value(tg3__parse_ctx *ctx, const tg3json_value *j) {
    tg3_value v;
    size_t i;
    memset(&v, 0, sizeof(v));
    if (!j) return v;
    switch (j->type) {
        case TG3JSON_NULL:
            v.type = TG3_VALUE_NULL;
            break;
        case TG3JSON_BOOL:
            v.type = TG3_VALUE_BOOL;
            v.bool_val = j->u.boolean ? 1 : 0;
            break;
        case TG3JSON_INT:
            v.type = TG3_VALUE_INT;
            v.int_val = j->u.integer;
            break;
        case TG3JSON_REAL:
            v.type = TG3_VALUE_REAL;
            v.real_val = j->u.real;
            break;
        case TG3JSON_STRING:
            v.type = TG3_VALUE_STRING;
            v.string_val = tg3__arena_str(ctx->arena, j->u.string.ptr, (uint32_t)j->u.string.len);
            break;
        case TG3JSON_ARRAY:
            v.type = TG3_VALUE_ARRAY;
            if (j->u.array.count > 0) {
                tg3_value *arr = (tg3_value *)tg3__arena_alloc(ctx->arena, j->u.array.count * sizeof(tg3_value));
                if (arr) {
                    for (i = 0; i < j->u.array.count; ++i) arr[i] = tg3__json_to_value(ctx, &j->u.array.items[i]);
                    v.array_data = arr;
                    v.array_count = (uint32_t)j->u.array.count;
                }
            }
            break;
        case TG3JSON_OBJECT:
            v.type = TG3_VALUE_OBJECT;
            if (j->u.object.count > 0) {
                tg3_kv_pair *pairs = (tg3_kv_pair *)tg3__arena_alloc(ctx->arena, j->u.object.count * sizeof(tg3_kv_pair));
                if (pairs) {
                    for (i = 0; i < j->u.object.count; ++i) {
                        pairs[i].key = tg3__arena_str(ctx->arena, j->u.object.items[i].key,
                                                      (uint32_t)j->u.object.items[i].key_len);
                        pairs[i].value = tg3__json_to_value(ctx, j->u.object.items[i].value);
                    }
                    v.object_data = pairs;
                    v.object_count = (uint32_t)j->u.object.count;
                }
            }
            break;
    }
    return v;
}

static void tg3__parse_extras_and_extensions(tg3__parse_ctx *ctx, const tg3json_value *o,
                                             tg3_extras_ext *ee) {
    const tg3json_value *extras_it;
    const tg3json_value *ext_it;
    memset(ee, 0, sizeof(*ee));
    extras_it = tg3__json_get(o, "extras");
    if (extras_it) {
        tg3_value *ev = (tg3_value *)tg3__arena_alloc(ctx->arena, sizeof(tg3_value));
        if (ev) {
            *ev = tg3__json_to_value(ctx, extras_it);
            ee->extras = ev;
        }
        if (ctx->opts.store_original_json) {
            size_t raw_len = 0;
            char *raw = tg3json_stringify(extras_it, &raw_len);
            if (raw) {
                ee->extras_json = tg3__arena_str(ctx->arena, raw, (uint32_t)raw_len);
                free(raw);
            }
        }
    }
    ext_it = tg3__json_get(o, "extensions");
    if (tg3__json_is_object(ext_it)) {
        size_t count = tg3json_object_size(ext_it);
        size_t i;
        if (count > 0) {
            tg3_extension *exts = (tg3_extension *)tg3__arena_alloc(ctx->arena, count * sizeof(tg3_extension));
            if (exts) {
                for (i = 0; i < count; ++i) {
                    const tg3json_object_entry *entry = tg3json_object_at(ext_it, i);
                    exts[i].name = tg3__arena_str(ctx->arena, entry->key, (uint32_t)entry->key_len);
                    exts[i].value = tg3__json_to_value(ctx, entry->value);
                }
                ee->extensions = exts;
                ee->extensions_count = (uint32_t)count;
            }
        }
        if (ctx->opts.store_original_json) {
            size_t raw_len = 0;
            char *raw = tg3json_stringify(ext_it, &raw_len);
            if (raw) {
                ee->extensions_json = tg3__arena_str(ctx->arena, raw, (uint32_t)raw_len);
                free(raw);
            }
        }
    }
}

static void tg3__init_texture_info(tg3_texture_info *ti) { memset(ti, 0, sizeof(*ti)); ti->index = -1; }
static void tg3__init_normal_texture_info(tg3_normal_texture_info *ti) { memset(ti, 0, sizeof(*ti)); ti->index = -1; ti->scale = 1.0; }
static void tg3__init_occlusion_texture_info(tg3_occlusion_texture_info *ti) { memset(ti, 0, sizeof(*ti)); ti->index = -1; ti->strength = 1.0; }
static void tg3__init_pbr(tg3_pbr_metallic_roughness *pbr) {
    memset(pbr, 0, sizeof(*pbr));
    pbr->base_color_factor[0] = 1.0; pbr->base_color_factor[1] = 1.0;
    pbr->base_color_factor[2] = 1.0; pbr->base_color_factor[3] = 1.0;
    pbr->metallic_factor = 1.0; pbr->roughness_factor = 1.0;
    tg3__init_texture_info(&pbr->base_color_texture);
    tg3__init_texture_info(&pbr->metallic_roughness_texture);
}
static void tg3__init_node(tg3_node *n) {
    memset(n, 0, sizeof(*n));
    n->camera = -1; n->skin = -1; n->mesh = -1; n->light = -1; n->emitter = -1;
    n->rotation[3] = 1.0; n->scale[0] = 1.0; n->scale[1] = 1.0; n->scale[2] = 1.0;
    n->matrix[0] = 1.0; n->matrix[5] = 1.0; n->matrix[10] = 1.0; n->matrix[15] = 1.0;
}

static int tg3__parse_asset(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_asset *asset) {
    memset(asset, 0, sizeof(*asset));
    tg3__parse_string(ctx, o, "version", &asset->version, 0, "/asset");
    tg3__parse_string(ctx, o, "generator", &asset->generator, 0, "/asset");
    tg3__parse_string(ctx, o, "minVersion", &asset->min_version, 0, "/asset");
    tg3__parse_string(ctx, o, "copyright", &asset->copyright, 0, "/asset");
    tg3__parse_extras_and_extensions(ctx, o, &asset->ext);
    return 1;
}

static int tg3__parse_texture_info(tg3__parse_ctx *ctx, const tg3json_value *o,
                                   const char *key, tg3_texture_info *ti) {
    const tg3json_value *it = tg3__json_get(o, key);
    tg3__init_texture_info(ti);
    if (!it) return 1;
    if (!tg3__json_is_object(it)) return 0;
    tg3__parse_int(ctx, it, "index", &ti->index, 0, key);
    tg3__parse_int(ctx, it, "texCoord", &ti->tex_coord, 0, key);
    tg3__parse_extras_and_extensions(ctx, it, &ti->ext);
    return 1;
}

static int tg3__parse_normal_texture_info(tg3__parse_ctx *ctx, const tg3json_value *o,
                                          const char *key, tg3_normal_texture_info *ti) {
    const tg3json_value *it = tg3__json_get(o, key);
    tg3__init_normal_texture_info(ti);
    if (!it) return 1;
    if (!tg3__json_is_object(it)) return 0;
    tg3__parse_int(ctx, it, "index", &ti->index, 0, key);
    tg3__parse_int(ctx, it, "texCoord", &ti->tex_coord, 0, key);
    tg3__parse_double(ctx, it, "scale", &ti->scale, 0, key);
    tg3__parse_extras_and_extensions(ctx, it, &ti->ext);
    return 1;
}

static int tg3__parse_occlusion_texture_info(tg3__parse_ctx *ctx, const tg3json_value *o,
                                             const char *key, tg3_occlusion_texture_info *ti) {
    const tg3json_value *it = tg3__json_get(o, key);
    tg3__init_occlusion_texture_info(ti);
    if (!it) return 1;
    if (!tg3__json_is_object(it)) return 0;
    tg3__parse_int(ctx, it, "index", &ti->index, 0, key);
    tg3__parse_int(ctx, it, "texCoord", &ti->tex_coord, 0, key);
    tg3__parse_double(ctx, it, "strength", &ti->strength, 0, key);
    tg3__parse_extras_and_extensions(ctx, it, &ti->ext);
    return 1;
}

static int tg3__accessor_type_from_string(const char *s, size_t len) {
    if (len == 6 && memcmp(s, "SCALAR", 6) == 0) return TG3_TYPE_SCALAR;
    if (len == 4 && memcmp(s, "VEC2", 4) == 0) return TG3_TYPE_VEC2;
    if (len == 4 && memcmp(s, "VEC3", 4) == 0) return TG3_TYPE_VEC3;
    if (len == 4 && memcmp(s, "VEC4", 4) == 0) return TG3_TYPE_VEC4;
    if (len == 4 && memcmp(s, "MAT2", 4) == 0) return TG3_TYPE_MAT2;
    if (len == 4 && memcmp(s, "MAT3", 4) == 0) return TG3_TYPE_MAT3;
    if (len == 4 && memcmp(s, "MAT4", 4) == 0) return TG3_TYPE_MAT4;
    return -1;
}

static int tg3__parse_accessor_sparse(tg3__parse_ctx *ctx, const tg3json_value *o,
                                      tg3_accessor_sparse *sparse) {
    const tg3json_value *it = tg3__json_get(o, "sparse");
    memset(sparse, 0, sizeof(*sparse));
    sparse->indices.buffer_view = -1;
    sparse->values.buffer_view = -1;
    if (!it) return 1;
    if (!tg3__json_is_object(it)) return 0;
    sparse->is_sparse = 1;
    tg3__parse_int(ctx, it, "count", &sparse->count, 1, "/sparse");
    {
        const tg3json_value *idx_it = tg3__json_get(it, "indices");
        if (tg3__json_is_object(idx_it)) {
            uint64_t bo = 0;
            tg3__parse_int(ctx, idx_it, "bufferView", &sparse->indices.buffer_view, 1, "/sparse/indices");
            tg3__parse_int(ctx, idx_it, "componentType", &sparse->indices.component_type, 1, "/sparse/indices");
            tg3__parse_uint64(ctx, idx_it, "byteOffset", &bo, 0, "/sparse/indices");
            sparse->indices.byte_offset = bo;
            tg3__parse_extras_and_extensions(ctx, idx_it, &sparse->indices.ext);
        }
    }
    {
        const tg3json_value *val_it = tg3__json_get(it, "values");
        if (tg3__json_is_object(val_it)) {
            uint64_t bo = 0;
            tg3__parse_int(ctx, val_it, "bufferView", &sparse->values.buffer_view, 1, "/sparse/values");
            tg3__parse_uint64(ctx, val_it, "byteOffset", &bo, 0, "/sparse/values");
            sparse->values.byte_offset = bo;
            tg3__parse_extras_and_extensions(ctx, val_it, &sparse->values.ext);
        }
    }
    tg3__parse_extras_and_extensions(ctx, it, &sparse->ext);
    return 1;
}

static int tg3__parse_accessor(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_accessor *acc) {
    tg3_str type_str;
    uint64_t bo = 0;
    uint64_t cnt = 0;
    memset(acc, 0, sizeof(*acc));
    acc->buffer_view = -1;
    acc->component_type = -1;
    acc->type = -1;
    tg3__parse_string(ctx, o, "name", &acc->name, 0, "/accessor");
    tg3__parse_int(ctx, o, "bufferView", &acc->buffer_view, 0, "/accessor");
    tg3__parse_uint64(ctx, o, "byteOffset", &bo, 0, "/accessor");
    acc->byte_offset = bo;
    tg3__parse_bool(ctx, o, "normalized", &acc->normalized, 0, "/accessor");
    tg3__parse_int(ctx, o, "componentType", &acc->component_type, 1, "/accessor");
    tg3__parse_uint64(ctx, o, "count", &cnt, 1, "/accessor");
    acc->count = cnt;
    type_str.data = NULL; type_str.len = 0;
    tg3__parse_string(ctx, o, "type", &type_str, 1, "/accessor");
    if (type_str.data) acc->type = tg3__accessor_type_from_string(type_str.data, type_str.len);
    tg3__parse_number_array(ctx, o, "min", &acc->min_values, &acc->min_values_count, 0, "/accessor");
    tg3__parse_number_array(ctx, o, "max", &acc->max_values, &acc->max_values_count, 0, "/accessor");
    tg3__parse_accessor_sparse(ctx, o, &acc->sparse);
    tg3__parse_extras_and_extensions(ctx, o, &acc->ext);
    return 1;
}

/* Reject URIs that could escape base_dir or smuggle a different path through
 * path-resolution layers. Returns 1 if the URI is safe to concatenate with
 * base_dir, 0 if it must be rejected. */
static int tg3__uri_is_safe(const char *uri, uint32_t uri_len) {
    uint32_t i;
    if (!uri || uri_len == 0) return 0;
    /* No NUL bytes — fopen would truncate; protects against smuggling. */
    if (memchr(uri, '\0', uri_len) != NULL) return 0;
    /* Reject absolute paths (POSIX and Windows). */
    if (uri[0] == '/' || uri[0] == '\\') return 0;
    /* Reject Windows drive prefixes like "C:". */
    if (uri_len >= 2 && uri[1] == ':' &&
        ((uri[0] >= 'A' && uri[0] <= 'Z') || (uri[0] >= 'a' && uri[0] <= 'z'))) {
        return 0;
    }
    /* Reject any ".." segment, separator-aware on both / and \. */
    for (i = 0; i + 1 < uri_len; ++i) {
        if (uri[i] == '.' && uri[i + 1] == '.') {
            int at_start = (i == 0) || uri[i - 1] == '/' || uri[i - 1] == '\\';
            int at_end = (i + 2 == uri_len) || uri[i + 2] == '/' || uri[i + 2] == '\\';
            if (at_start && at_end) return 0;
        }
    }
    return 1;
}

static int tg3__load_external_file(tg3__parse_ctx *ctx, uint8_t **out_data, uint64_t *out_size,
                                   const char *uri, uint32_t uri_len) {
    char path_buf[4096];
    uint32_t path_len = 0;
    int32_t ok;
    if (!ctx->opts.fs.read_file) {
        tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR, TG3_ERR_FS_NOT_AVAILABLE,
                        "No filesystem callbacks available", NULL, -1);
        return 0;
    }
    if (!tg3__uri_is_safe(uri, uri_len)) {
        tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR, TG3_ERR_INVALID_VALUE,
                        "External URI rejected (absolute path, '..' segment, or NUL byte)",
                        NULL, -1);
        return 0;
    }
    /* Saturating bounds check: path_buf is fixed-size; subtract instead of add
     * so 32-bit size_t cannot wrap. */
    if (uri_len >= sizeof(path_buf)) return 0;
    if (ctx->base_dir_len > 0) {
        if (ctx->base_dir_len >= sizeof(path_buf) - uri_len - 1u) return 0;
        memcpy(path_buf, ctx->base_dir, ctx->base_dir_len);
        path_len = ctx->base_dir_len;
        if (path_len > 0 && path_buf[path_len - 1u] != '/' && path_buf[path_len - 1u] != '\\') {
            path_buf[path_len++] = '/';
        }
    }
    if (path_len + uri_len >= sizeof(path_buf)) return 0;
    memcpy(path_buf + path_len, uri, uri_len);
    path_len += uri_len;
    path_buf[path_len] = '\0';
    ok = ctx->opts.fs.read_file(out_data, out_size, path_buf, path_len, ctx->opts.fs.user_data);
    if (!ok) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, TG3_ERR_FILE_READ,
                         NULL, "Failed to read file: %s", path_buf);
        return 0;
    }
    if (ctx->opts.max_external_file_size > 0 &&
        *out_size > ctx->opts.max_external_file_size) {
        if (ctx->opts.fs.free_file) {
            ctx->opts.fs.free_file(*out_data, *out_size, ctx->opts.fs.user_data);
        }
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                         TG3_ERR_FILE_TOO_LARGE, NULL,
                         "External file %s exceeds max_external_file_size (%llu > %llu)",
                         path_buf, (unsigned long long)*out_size,
                         (unsigned long long)ctx->opts.max_external_file_size);
        *out_data = NULL;
        *out_size = 0;
        return 0;
    }
    return 1;
}

static int tg3__parse_buffer(tg3__parse_ctx *ctx, const tg3json_value *o,
                             tg3_buffer *buf, int32_t buf_idx) {
    uint64_t byte_length = 0;
    memset(buf, 0, sizeof(*buf));
    tg3__parse_string(ctx, o, "name", &buf->name, 0, "/buffer");
    tg3__parse_string(ctx, o, "uri", &buf->uri, 0, "/buffer");
    tg3__parse_uint64(ctx, o, "byteLength", &byte_length, 1, "/buffer");
    if (ctx->is_binary && buf_idx == 0 && buf->uri.len == 0) {
        uint8_t *data;
        if (!ctx->bin_data || ctx->bin_size < byte_length) {
            tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR, TG3_ERR_BUFFER_SIZE_MISMATCH,
                            "GLB BIN chunk missing or smaller than buffer.byteLength", NULL, -1);
            return 0;
        }
        data = (uint8_t *)tg3__arena_alloc(ctx->arena, (size_t)byte_length);
        if (!data) {
            tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                            "OOM for buffer data", NULL, -1);
            return 0;
        }
        memcpy(data, ctx->bin_data, (size_t)byte_length);
        buf->data.data = data;
        buf->data.count = byte_length;
    } else if (buf->uri.len > 0) {
        if (tg3_is_data_uri(buf->uri.data, buf->uri.len)) {
            size_t decoded_len = 0;
            char mime[64];
            uint8_t *decoded;
            memset(mime, 0, sizeof(mime));
            decoded = tg3__decode_data_uri(ctx->arena, buf->uri.data, buf->uri.len, &decoded_len, mime, sizeof(mime));
            if (!decoded && byte_length > 0) {
                tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR, TG3_ERR_DATA_URI_DECODE,
                                "Failed to decode buffer data URI", NULL, -1);
                return 0;
            }
            buf->data.data = decoded;
            buf->data.count = decoded_len;
        } else {
            uint8_t *file_data = NULL;
            uint64_t file_size = 0;
            if (tg3__load_external_file(ctx, &file_data, &file_size, buf->uri.data, buf->uri.len)) {
                uint8_t *data = (uint8_t *)tg3__arena_alloc(ctx->arena, (size_t)file_size);
                if (data) {
                    memcpy(data, file_data, (size_t)file_size);
                    buf->data.data = data;
                    buf->data.count = file_size;
                }
                if (ctx->opts.fs.free_file) ctx->opts.fs.free_file(file_data, file_size, ctx->opts.fs.user_data);
            }
        }
    }
    tg3__parse_extras_and_extensions(ctx, o, &buf->ext);
    return 1;
}

static int tg3__parse_buffer_view(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_buffer_view *bv) {
    uint64_t val = 0;
    int32_t stride = 0;
    memset(bv, 0, sizeof(*bv));
    bv->buffer = -1;
    tg3__parse_string(ctx, o, "name", &bv->name, 0, "/bufferView");
    tg3__parse_int(ctx, o, "buffer", &bv->buffer, 1, "/bufferView");
    tg3__parse_uint64(ctx, o, "byteOffset", &val, 0, "/bufferView");
    bv->byte_offset = val;
    val = 0;
    tg3__parse_uint64(ctx, o, "byteLength", &val, 1, "/bufferView");
    bv->byte_length = val;
    tg3__parse_int(ctx, o, "byteStride", &stride, 0, "/bufferView");
    /* glTF spec: byteStride is 0 (tightly packed) or in [4, 252]. Reject
     * negatives so the (uint32_t) cast cannot wrap to 2^32-1 and propagate
     * into downstream size math; reject 1..3 so consumers that pre-allocate
     * `count * stride` cannot underallocate against a spec-non-conforming
     * stride. */
    if (stride != 0 && (stride < 4 || stride > 252)) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                         TG3_ERR_INVALID_VALUE, "/bufferView",
                         "byteStride %d must be 0 or in [4, 252]", stride);
        return 0;
    }
    bv->byte_stride = (uint32_t)stride;
    tg3__parse_int(ctx, o, "target", &bv->target, 0, "/bufferView");
    tg3__parse_extras_and_extensions(ctx, o, &bv->ext);
    return 1;
}

static int tg3__parse_image(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_image *img, int32_t img_idx) {
    (void)img_idx;
    memset(img, 0, sizeof(*img));
    img->width = -1; img->height = -1; img->component = -1; img->bits = -1; img->pixel_type = -1; img->buffer_view = -1;
    tg3__parse_string(ctx, o, "name", &img->name, 0, "/image");
    tg3__parse_string(ctx, o, "uri", &img->uri, 0, "/image");
    tg3__parse_string(ctx, o, "mimeType", &img->mime_type, 0, "/image");
    tg3__parse_int(ctx, o, "bufferView", &img->buffer_view, 0, "/image");
    if (ctx->opts.images_as_is) img->as_is = 1;
    tg3__parse_extras_and_extensions(ctx, o, &img->ext);
    return 1;
}

static int tg3__parse_sampler(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_sampler *samp) {
    memset(samp, 0, sizeof(*samp));
    samp->min_filter = -1; samp->mag_filter = -1;
    samp->wrap_s = TG3_TEXTURE_WRAP_REPEAT; samp->wrap_t = TG3_TEXTURE_WRAP_REPEAT;
    tg3__parse_string(ctx, o, "name", &samp->name, 0, "/sampler");
    tg3__parse_int(ctx, o, "minFilter", &samp->min_filter, 0, "/sampler");
    tg3__parse_int(ctx, o, "magFilter", &samp->mag_filter, 0, "/sampler");
    tg3__parse_int(ctx, o, "wrapS", &samp->wrap_s, 0, "/sampler");
    tg3__parse_int(ctx, o, "wrapT", &samp->wrap_t, 0, "/sampler");
    tg3__parse_extras_and_extensions(ctx, o, &samp->ext);
    return 1;
}

static int tg3__parse_texture(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_texture *tex) {
    memset(tex, 0, sizeof(*tex));
    tex->sampler = -1; tex->source = -1;
    tg3__parse_string(ctx, o, "name", &tex->name, 0, "/texture");
    tg3__parse_int(ctx, o, "sampler", &tex->sampler, 0, "/texture");
    tg3__parse_int(ctx, o, "source", &tex->source, 0, "/texture");
    tg3__parse_extras_and_extensions(ctx, o, &tex->ext);
    return 1;
}

static int tg3__parse_material(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_material *mat) {
    const tg3json_value *pbr_it;
    const tg3json_value *ext_it;
    memset(mat, 0, sizeof(*mat));
    tg3__init_pbr(&mat->pbr_metallic_roughness);
    tg3__init_normal_texture_info(&mat->normal_texture);
    tg3__init_occlusion_texture_info(&mat->occlusion_texture);
    tg3__init_texture_info(&mat->emissive_texture);
    mat->alpha_cutoff = 0.5;
    tg3__parse_string(ctx, o, "name", &mat->name, 0, "/material");
    tg3__parse_number_to_fixed(o, "emissiveFactor", mat->emissive_factor, 3);
    {
        tg3_str alpha_mode;
        alpha_mode.data = NULL; alpha_mode.len = 0;
        tg3__parse_string(ctx, o, "alphaMode", &alpha_mode, 0, "/material");
        mat->alpha_mode = alpha_mode.len > 0 ? alpha_mode : tg3__arena_str(ctx->arena, "OPAQUE", 6);
    }
    tg3__parse_double(ctx, o, "alphaCutoff", &mat->alpha_cutoff, 0, "/material");
    tg3__parse_bool(ctx, o, "doubleSided", &mat->double_sided, 0, "/material");
    pbr_it = tg3__json_get(o, "pbrMetallicRoughness");
    if (tg3__json_is_object(pbr_it)) {
        tg3__parse_number_to_fixed(pbr_it, "baseColorFactor", mat->pbr_metallic_roughness.base_color_factor, 4);
        tg3__parse_double(ctx, pbr_it, "metallicFactor", &mat->pbr_metallic_roughness.metallic_factor, 0, "/material/pbrMetallicRoughness");
        tg3__parse_double(ctx, pbr_it, "roughnessFactor", &mat->pbr_metallic_roughness.roughness_factor, 0, "/material/pbrMetallicRoughness");
        tg3__parse_texture_info(ctx, pbr_it, "baseColorTexture", &mat->pbr_metallic_roughness.base_color_texture);
        tg3__parse_texture_info(ctx, pbr_it, "metallicRoughnessTexture", &mat->pbr_metallic_roughness.metallic_roughness_texture);
        tg3__parse_extras_and_extensions(ctx, pbr_it, &mat->pbr_metallic_roughness.ext);
    }
    tg3__parse_normal_texture_info(ctx, o, "normalTexture", &mat->normal_texture);
    tg3__parse_occlusion_texture_info(ctx, o, "occlusionTexture", &mat->occlusion_texture);
    tg3__parse_texture_info(ctx, o, "emissiveTexture", &mat->emissive_texture);
    ext_it = tg3__json_get(o, "extensions");
    if (tg3__json_is_object(ext_it)) {
        const tg3json_value *lod_it = tg3json_object_get(ext_it, "MSFT_lod");
        if (tg3__json_is_object(lod_it)) {
            tg3__parse_int_array(ctx, lod_it, "ids", &mat->lods, &mat->lods_count, 0, "/material/extensions/MSFT_lod");
        }
    }
    tg3__parse_extras_and_extensions(ctx, o, &mat->ext);
    return 1;
}

static int tg3__parse_primitive(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_primitive *prim) {
    const tg3json_value *attr_it;
    const tg3json_value *targets_it;
    memset(prim, 0, sizeof(*prim));
    prim->material = -1; prim->indices = -1; prim->mode = TG3_MODE_TRIANGLES;
    tg3__parse_int(ctx, o, "material", &prim->material, 0, "/primitive");
    tg3__parse_int(ctx, o, "indices", &prim->indices, 0, "/primitive");
    tg3__parse_int(ctx, o, "mode", &prim->mode, 0, "/primitive");
    attr_it = tg3__json_get(o, "attributes");
    if (tg3__json_is_object(attr_it)) {
        size_t count = tg3json_object_size(attr_it);
        if (count > 0) {
            size_t i;
            tg3_str_int_pair *attrs = (tg3_str_int_pair *)tg3__arena_alloc(ctx->arena, count * sizeof(tg3_str_int_pair));
            if (attrs) {
                for (i = 0; i < count; ++i) {
                    const tg3json_object_entry *entry = tg3json_object_at(attr_it, i);
                    attrs[i].key = tg3__arena_str(ctx->arena, entry->key, (uint32_t)entry->key_len);
                    if (!tg3__json_number_to_int32(entry->value, &attrs[i].value)) {
                        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                                         TG3_ERR_JSON_TYPE_MISMATCH, "/primitive/attributes",
                                         "Attribute '%s' must be a finite int32-valued number",
                                         entry->key ? entry->key : "");
                        attrs[i].value = 0;
                    }
                }
                prim->attributes = attrs;
                prim->attributes_count = (uint32_t)count;
            }
        }
    }
    targets_it = tg3__json_get(o, "targets");
    if (tg3__json_is_array(targets_it)) {
        size_t tcount = tg3json_array_size(targets_it);
        if (tcount > 0) {
            size_t ti;
            const tg3_str_int_pair **target_arrays = (const tg3_str_int_pair **)tg3__arena_alloc(ctx->arena, tcount * sizeof(tg3_str_int_pair *));
            uint32_t *target_counts = (uint32_t *)tg3__arena_alloc(ctx->arena, tcount * sizeof(uint32_t));
            if (target_arrays && target_counts) {
                for (ti = 0; ti < tcount; ++ti) {
                    const tg3json_value *target_obj = tg3json_array_get(targets_it, ti);
                    size_t acount;
                    size_t ai;
                    tg3_str_int_pair *tattrs;
                    if (!tg3__json_is_object(target_obj)) {
                        target_arrays[ti] = NULL;
                        target_counts[ti] = 0;
                        continue;
                    }
                    acount = tg3json_object_size(target_obj);
                    tattrs = (tg3_str_int_pair *)tg3__arena_alloc(ctx->arena, acount * sizeof(tg3_str_int_pair));
                    if (tattrs) {
                        for (ai = 0; ai < acount; ++ai) {
                            const tg3json_object_entry *entry = tg3json_object_at(target_obj, ai);
                            tattrs[ai].key = tg3__arena_str(ctx->arena, entry->key, (uint32_t)entry->key_len);
                            if (!tg3__json_number_to_int32(entry->value, &tattrs[ai].value)) {
                                tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                                                 TG3_ERR_JSON_TYPE_MISMATCH, "/primitive/targets",
                                                 "Target attribute '%s' must be a finite int32-valued number",
                                                 entry->key ? entry->key : "");
                                tattrs[ai].value = 0;
                            }
                        }
                    }
                    target_arrays[ti] = tattrs;
                    /* On arena OOM tattrs may be NULL; keep count consistent so
                     * the index validator and downstream consumers don't deref. */
                    target_counts[ti] = tattrs ? (uint32_t)acount : 0u;
                }
                prim->targets = target_arrays;
                prim->target_attribute_counts = target_counts;
                prim->targets_count = (uint32_t)tcount;
            }
        }
    }
    tg3__parse_extras_and_extensions(ctx, o, &prim->ext);
    return 1;
}

static int tg3__parse_mesh(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_mesh *mesh) {
    const tg3json_value *prim_it = tg3__json_get(o, "primitives");
    memset(mesh, 0, sizeof(*mesh));
    tg3__parse_string(ctx, o, "name", &mesh->name, 0, "/mesh");
    if (tg3__json_is_array(prim_it)) {
        size_t count = tg3json_array_size(prim_it);
        if (count > 0) {
            size_t i;
            tg3_primitive *prims = (tg3_primitive *)tg3__arena_alloc(ctx->arena, count * sizeof(tg3_primitive));
            if (prims) {
                for (i = 0; i < count; ++i) tg3__parse_primitive(ctx, tg3json_array_get(prim_it, i), &prims[i]);
                mesh->primitives = prims;
                mesh->primitives_count = (uint32_t)count;
            }
        }
    }
    tg3__parse_number_array(ctx, o, "weights", &mesh->weights, &mesh->weights_count, 0, "/mesh");
    tg3__parse_extras_and_extensions(ctx, o, &mesh->ext);
    return 1;
}

static int tg3__parse_node(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_node *node) {
    const tg3json_value *ext_it;
    tg3__init_node(node);
    tg3__parse_string(ctx, o, "name", &node->name, 0, "/node");
    tg3__parse_int(ctx, o, "camera", &node->camera, 0, "/node");
    tg3__parse_int(ctx, o, "skin", &node->skin, 0, "/node");
    tg3__parse_int(ctx, o, "mesh", &node->mesh, 0, "/node");
    tg3__parse_int_array(ctx, o, "children", &node->children, &node->children_count, 0, "/node");
    if (tg3__json_has(o, "matrix")) { tg3__parse_number_to_fixed(o, "matrix", node->matrix, 16); node->has_matrix = 1; }
    if (tg3__json_has(o, "translation")) tg3__parse_number_to_fixed(o, "translation", node->translation, 3);
    if (tg3__json_has(o, "rotation")) tg3__parse_number_to_fixed(o, "rotation", node->rotation, 4);
    if (tg3__json_has(o, "scale")) tg3__parse_number_to_fixed(o, "scale", node->scale, 3);
    tg3__parse_number_array(ctx, o, "weights", &node->weights, &node->weights_count, 0, "/node");
    ext_it = tg3__json_get(o, "extensions");
    if (tg3__json_is_object(ext_it)) {
        const tg3json_value *khr_lights = tg3json_object_get(ext_it, "KHR_lights_punctual");
        const tg3json_value *khr_audio = tg3json_object_get(ext_it, "KHR_audio");
        const tg3json_value *msft_lod = tg3json_object_get(ext_it, "MSFT_lod");
        if (tg3__json_is_object(khr_lights)) tg3__parse_int(ctx, khr_lights, "light", &node->light, 0, "/node/extensions/KHR_lights_punctual");
        if (tg3__json_is_object(khr_audio)) tg3__parse_int(ctx, khr_audio, "emitter", &node->emitter, 0, "/node/extensions/KHR_audio");
        if (tg3__json_is_object(msft_lod)) tg3__parse_int_array(ctx, msft_lod, "ids", &node->lods, &node->lods_count, 0, "/node/extensions/MSFT_lod");
    }
    tg3__parse_extras_and_extensions(ctx, o, &node->ext);
    return 1;
}

static int tg3__parse_skin(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_skin *skin) {
    memset(skin, 0, sizeof(*skin));
    skin->inverse_bind_matrices = -1; skin->skeleton = -1;
    tg3__parse_string(ctx, o, "name", &skin->name, 0, "/skin");
    tg3__parse_int(ctx, o, "inverseBindMatrices", &skin->inverse_bind_matrices, 0, "/skin");
    tg3__parse_int(ctx, o, "skeleton", &skin->skeleton, 0, "/skin");
    tg3__parse_int_array(ctx, o, "joints", &skin->joints, &skin->joints_count, 1, "/skin");
    tg3__parse_extras_and_extensions(ctx, o, &skin->ext);
    return 1;
}

static int tg3__parse_animation(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_animation *anim) {
    const tg3json_value *ch_it = tg3__json_get(o, "channels");
    const tg3json_value *samp_it = tg3__json_get(o, "samplers");
    memset(anim, 0, sizeof(*anim));
    tg3__parse_string(ctx, o, "name", &anim->name, 0, "/animation");
    if (tg3__json_is_array(ch_it)) {
        size_t count = tg3json_array_size(ch_it);
        if (count > 0) {
            size_t i;
            tg3_animation_channel *channels = (tg3_animation_channel *)tg3__arena_alloc(ctx->arena, count * sizeof(tg3_animation_channel));
            if (channels) {
                for (i = 0; i < count; ++i) {
                    const tg3json_value *item = tg3json_array_get(ch_it, i);
                    const tg3json_value *tgt_it;
                    memset(&channels[i], 0, sizeof(channels[i]));
                    channels[i].sampler = -1;
                    channels[i].target.node = -1;
                    tg3__parse_int(ctx, item, "sampler", &channels[i].sampler, 1, "/animation/channel");
                    tgt_it = tg3__json_get(item, "target");
                    if (tg3__json_is_object(tgt_it)) {
                        tg3__parse_int(ctx, tgt_it, "node", &channels[i].target.node, 0, "/animation/channel/target");
                        tg3__parse_string(ctx, tgt_it, "path", &channels[i].target.path, 1, "/animation/channel/target");
                        tg3__parse_extras_and_extensions(ctx, tgt_it, &channels[i].target.ext);
                    }
                    tg3__parse_extras_and_extensions(ctx, item, &channels[i].ext);
                }
                anim->channels = channels;
                anim->channels_count = (uint32_t)count;
            }
        }
    }
    if (tg3__json_is_array(samp_it)) {
        size_t count = tg3json_array_size(samp_it);
        if (count > 0) {
            size_t i;
            tg3_animation_sampler *samplers = (tg3_animation_sampler *)tg3__arena_alloc(ctx->arena, count * sizeof(tg3_animation_sampler));
            if (samplers) {
                for (i = 0; i < count; ++i) {
                    const tg3json_value *item = tg3json_array_get(samp_it, i);
                    tg3_str interp;
                    memset(&samplers[i], 0, sizeof(samplers[i]));
                    samplers[i].input = -1; samplers[i].output = -1;
                    tg3__parse_int(ctx, item, "input", &samplers[i].input, 1, "/animation/sampler");
                    tg3__parse_int(ctx, item, "output", &samplers[i].output, 1, "/animation/sampler");
                    interp.data = NULL; interp.len = 0;
                    tg3__parse_string(ctx, item, "interpolation", &interp, 0, "/animation/sampler");
                    samplers[i].interpolation = interp.len > 0 ? interp : tg3__arena_str(ctx->arena, "LINEAR", 6);
                    tg3__parse_extras_and_extensions(ctx, item, &samplers[i].ext);
                }
                anim->samplers = samplers;
                anim->samplers_count = (uint32_t)count;
            }
        }
    }
    tg3__parse_extras_and_extensions(ctx, o, &anim->ext);
    return 1;
}

static int tg3__parse_camera(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_camera *cam) {
    const tg3json_value *it;
    memset(cam, 0, sizeof(*cam));
    tg3__parse_string(ctx, o, "name", &cam->name, 0, "/camera");
    tg3__parse_string(ctx, o, "type", &cam->type, 1, "/camera");
    if (cam->type.data && tg3_str_equals_cstr(cam->type, "perspective")) {
        it = tg3__json_get(o, "perspective");
        if (tg3__json_is_object(it)) {
            tg3__parse_double(ctx, it, "aspectRatio", &cam->perspective.aspect_ratio, 0, "/camera/perspective");
            tg3__parse_double(ctx, it, "yfov", &cam->perspective.yfov, 1, "/camera/perspective");
            tg3__parse_double(ctx, it, "zfar", &cam->perspective.zfar, 0, "/camera/perspective");
            tg3__parse_double(ctx, it, "znear", &cam->perspective.znear, 1, "/camera/perspective");
            tg3__parse_extras_and_extensions(ctx, it, &cam->perspective.ext);
        }
    } else if (cam->type.data && tg3_str_equals_cstr(cam->type, "orthographic")) {
        it = tg3__json_get(o, "orthographic");
        if (tg3__json_is_object(it)) {
            tg3__parse_double(ctx, it, "xmag", &cam->orthographic.xmag, 1, "/camera/orthographic");
            tg3__parse_double(ctx, it, "ymag", &cam->orthographic.ymag, 1, "/camera/orthographic");
            tg3__parse_double(ctx, it, "zfar", &cam->orthographic.zfar, 1, "/camera/orthographic");
            tg3__parse_double(ctx, it, "znear", &cam->orthographic.znear, 1, "/camera/orthographic");
            tg3__parse_extras_and_extensions(ctx, it, &cam->orthographic.ext);
        }
    }
    tg3__parse_extras_and_extensions(ctx, o, &cam->ext);
    return 1;
}

static int tg3__parse_scene(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_scene *scene) {
    const tg3json_value *ext_it;
    memset(scene, 0, sizeof(*scene));
    tg3__parse_string(ctx, o, "name", &scene->name, 0, "/scene");
    tg3__parse_int_array(ctx, o, "nodes", &scene->nodes, &scene->nodes_count, 0, "/scene");
    ext_it = tg3__json_get(o, "extensions");
    if (tg3__json_is_object(ext_it)) {
        const tg3json_value *audio_it = tg3json_object_get(ext_it, "KHR_audio");
        if (tg3__json_is_object(audio_it)) {
            tg3__parse_int_array(ctx, audio_it, "emitters", &scene->audio_emitters, &scene->audio_emitters_count, 0, "/scene/extensions/KHR_audio");
        }
    }
    tg3__parse_extras_and_extensions(ctx, o, &scene->ext);
    return 1;
}

static int tg3__parse_light(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_light *light) {
    const tg3json_value *spot_it;
    memset(light, 0, sizeof(*light));
    light->color[0] = 1.0; light->color[1] = 1.0; light->color[2] = 1.0;
    light->intensity = 1.0; light->spot.outer_cone_angle = 0.7853981634;
    tg3__parse_string(ctx, o, "name", &light->name, 0, "/light");
    tg3__parse_string(ctx, o, "type", &light->type, 1, "/light");
    tg3__parse_double(ctx, o, "intensity", &light->intensity, 0, "/light");
    tg3__parse_double(ctx, o, "range", &light->range, 0, "/light");
    tg3__parse_number_to_fixed(o, "color", light->color, 3);
    spot_it = tg3__json_get(o, "spot");
    if (tg3__json_is_object(spot_it)) {
        tg3__parse_double(ctx, spot_it, "innerConeAngle", &light->spot.inner_cone_angle, 0, "/light/spot");
        tg3__parse_double(ctx, spot_it, "outerConeAngle", &light->spot.outer_cone_angle, 0, "/light/spot");
        tg3__parse_extras_and_extensions(ctx, spot_it, &light->spot.ext);
    }
    tg3__parse_extras_and_extensions(ctx, o, &light->ext);
    return 1;
}

static int tg3__parse_audio_source(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_audio_source *src) {
    memset(src, 0, sizeof(*src));
    src->buffer_view = -1;
    tg3__parse_string(ctx, o, "name", &src->name, 0, "/audioSource");
    tg3__parse_string(ctx, o, "uri", &src->uri, 0, "/audioSource");
    tg3__parse_int(ctx, o, "bufferView", &src->buffer_view, 0, "/audioSource");
    tg3__parse_string(ctx, o, "mimeType", &src->mime_type, 0, "/audioSource");
    tg3__parse_extras_and_extensions(ctx, o, &src->ext);
    return 1;
}

static int tg3__parse_audio_emitter(tg3__parse_ctx *ctx, const tg3json_value *o, tg3_audio_emitter *emitter) {
    const tg3json_value *pos_it;
    memset(emitter, 0, sizeof(*emitter));
    emitter->gain = 1.0; emitter->source = -1;
    emitter->positional.cone_inner_angle = 6.283185307179586;
    emitter->positional.cone_outer_angle = 6.283185307179586;
    emitter->positional.max_distance = 100.0; emitter->positional.ref_distance = 1.0; emitter->positional.rolloff_factor = 1.0;
    tg3__parse_string(ctx, o, "name", &emitter->name, 0, "/audioEmitter");
    tg3__parse_double(ctx, o, "gain", &emitter->gain, 0, "/audioEmitter");
    tg3__parse_bool(ctx, o, "loop", &emitter->loop, 0, "/audioEmitter");
    tg3__parse_bool(ctx, o, "playing", &emitter->playing, 0, "/audioEmitter");
    tg3__parse_string(ctx, o, "type", &emitter->type, 0, "/audioEmitter");
    tg3__parse_string(ctx, o, "distanceModel", &emitter->distance_model, 0, "/audioEmitter");
    tg3__parse_int(ctx, o, "source", &emitter->source, 0, "/audioEmitter");
    pos_it = tg3__json_get(o, "positional");
    if (tg3__json_is_object(pos_it)) {
        tg3__parse_double(ctx, pos_it, "coneInnerAngle", &emitter->positional.cone_inner_angle, 0, "/positional");
        tg3__parse_double(ctx, pos_it, "coneOuterAngle", &emitter->positional.cone_outer_angle, 0, "/positional");
        tg3__parse_double(ctx, pos_it, "coneOuterGain", &emitter->positional.cone_outer_gain, 0, "/positional");
        tg3__parse_double(ctx, pos_it, "maxDistance", &emitter->positional.max_distance, 0, "/positional");
        tg3__parse_double(ctx, pos_it, "refDistance", &emitter->positional.ref_distance, 0, "/positional");
        tg3__parse_double(ctx, pos_it, "rolloffFactor", &emitter->positional.rolloff_factor, 0, "/positional");
        tg3__parse_extras_and_extensions(ctx, pos_it, &emitter->positional.ext);
    }
    tg3__parse_extras_and_extensions(ctx, o, &emitter->ext);
    return 1;
}

#define TG3__STREAM_CB(cb_name, model_arr, model_cnt) \
    do { \
        if ((ctx)->opts.stream && (ctx)->opts.stream->cb_name) { \
            uint32_t _si; \
            for (_si = 0; _si < (model_cnt); ++_si) { \
                tg3_stream_action _sa = (ctx)->opts.stream->cb_name(&(model_arr)[_si], (int32_t)_si, (ctx)->opts.stream->user_data); \
                if (_sa == TG3_STREAM_ABORT) return TG3_ERR_STREAM_ABORTED; \
            } \
        } \
    } while (0)

#define TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, json_key, Type, model_field, count_field, parse_fn) \
    do { \
        const tg3json_value *_arr = tg3json_object_get((json_doc), (json_key)); \
        if (tg3__json_is_array(_arr)) { \
            size_t _count = tg3json_array_size(_arr); \
            if (_count > 0) { \
                size_t _i; \
                Type *_items = (Type *)tg3__arena_alloc((ctx)->arena, _count * sizeof(Type)); \
                if (_items) { \
                    for (_i = 0; _i < _count; ++_i) { \
                        const tg3json_value *_it = tg3json_array_get(_arr, _i); \
                        if (!tg3__json_is_object(_it)) { \
                            tg3__error_pushf((ctx)->errors, (ctx)->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH, (json_key), "Element %u must be an object", (unsigned)_i); \
                            continue; \
                        } \
                        parse_fn((ctx), _it, &_items[_i]); \
                    } \
                    (model_field) = _items; \
                    (count_field) = (uint32_t)_count; \
                } \
            } \
        } \
    } while (0)

#define TG3__PARSE_ARRAY_IDX(ctx, json_doc, json_key, Type, model_field, count_field, parse_fn) \
    do { \
        const tg3json_value *_arr = tg3json_object_get((json_doc), (json_key)); \
        if (tg3__json_is_array(_arr)) { \
            size_t _count = tg3json_array_size(_arr); \
            if (_count > 0) { \
                size_t _i; \
                Type *_items = (Type *)tg3__arena_alloc((ctx)->arena, _count * sizeof(Type)); \
                if (_items) { \
                    for (_i = 0; _i < _count; ++_i) { \
                        const tg3json_value *_it = tg3json_array_get(_arr, _i); \
                        if (!tg3__json_is_object(_it)) { \
                            tg3__error_pushf((ctx)->errors, (ctx)->arena, TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH, (json_key), "Element %u must be an object", (unsigned)_i); \
                            continue; \
                        } \
                        parse_fn((ctx), _it, &_items[_i], (int32_t)_i); \
                    } \
                    (model_field) = _items; \
                    (count_field) = (uint32_t)_count; \
                } \
            } \
        } \
    } while (0)

/* Post-parse index validation. Walks every int32_t index field populated from
 * JSON and rejects out-of-range values so naive consumers cannot dereference
 * attacker-controlled indices into model arrays. Caps reported errors so a
 * pathological input does not flood the error stack. Returns 1 on success. */
#define TG3__IDX_ERR_CAP 64
static int tg3__validate_indices(tg3__parse_ctx *ctx, const tg3_model *m) {
    uint32_t i, j;
    int errs = 0;
    /* Helper macros — push with format and bump error counter. Every call
     * site passes at least one variadic arg, so plain __VA_ARGS__ (C11) is
     * sufficient and avoids the GNU `, ##__VA_ARGS__` extension. */
    #define TG3__IDX_BAD(path_str, fmt, ...) do { \
        if (errs < TG3__IDX_ERR_CAP) { \
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR, \
                             TG3_ERR_INVALID_INDEX, (path_str), fmt, __VA_ARGS__); \
        } \
        ++errs; \
    } while (0)
    #define TG3__CHECK_REQ(idx, max, path_str, fmt, ...) do { \
        if ((idx) < 0 || (uint32_t)(idx) >= (max)) { \
            TG3__IDX_BAD(path_str, fmt, __VA_ARGS__); \
        } \
    } while (0)
    #define TG3__CHECK_OPT(idx, max, path_str, fmt, ...) do { \
        if ((idx) != -1 && ((idx) < 0 || (uint32_t)(idx) >= (max))) { \
            TG3__IDX_BAD(path_str, fmt, __VA_ARGS__); \
        } \
    } while (0)

    if (m->default_scene != -1) {
        TG3__CHECK_OPT(m->default_scene, m->scenes_count, "/scene",
                       "default scene %d out of range [0,%u)",
                       m->default_scene, m->scenes_count);
    }
    for (i = 0; i < m->buffer_views_count && errs < TG3__IDX_ERR_CAP; ++i) {
        TG3__CHECK_REQ(m->buffer_views[i].buffer, m->buffers_count,
                       "/bufferViews", "bufferViews[%u].buffer %d out of range [0,%u)",
                       i, m->buffer_views[i].buffer, m->buffers_count);
    }
    for (i = 0; i < m->accessors_count && errs < TG3__IDX_ERR_CAP; ++i) {
        const tg3_accessor *a = &m->accessors[i];
        TG3__CHECK_OPT(a->buffer_view, m->buffer_views_count, "/accessors",
                       "accessors[%u].bufferView %d out of range [0,%u)",
                       i, a->buffer_view, m->buffer_views_count);
        if (a->sparse.is_sparse) {
            TG3__CHECK_REQ(a->sparse.indices.buffer_view, m->buffer_views_count,
                           "/accessors", "accessors[%u].sparse.indices.bufferView %d out of range [0,%u)",
                           i, a->sparse.indices.buffer_view, m->buffer_views_count);
            TG3__CHECK_REQ(a->sparse.values.buffer_view, m->buffer_views_count,
                           "/accessors", "accessors[%u].sparse.values.bufferView %d out of range [0,%u)",
                           i, a->sparse.values.buffer_view, m->buffer_views_count);
        }
    }
    for (i = 0; i < m->meshes_count && errs < TG3__IDX_ERR_CAP; ++i) {
        const tg3_mesh *me = &m->meshes[i];
        for (j = 0; j < me->primitives_count && errs < TG3__IDX_ERR_CAP; ++j) {
            const tg3_primitive *p = &me->primitives[j];
            uint32_t ai;
            TG3__CHECK_OPT(p->indices, m->accessors_count, "/meshes",
                           "meshes[%u].primitives[%u].indices %d out of range [0,%u)",
                           i, j, p->indices, m->accessors_count);
            TG3__CHECK_OPT(p->material, m->materials_count, "/meshes",
                           "meshes[%u].primitives[%u].material %d out of range [0,%u)",
                           i, j, p->material, m->materials_count);
            for (ai = 0; ai < p->attributes_count && errs < TG3__IDX_ERR_CAP; ++ai) {
                TG3__CHECK_REQ(p->attributes[ai].value, m->accessors_count, "/meshes",
                               "meshes[%u].primitives[%u].attributes[%u] %d out of range [0,%u)",
                               i, j, ai, p->attributes[ai].value, m->accessors_count);
            }
            {
                uint32_t ti;
                for (ti = 0; ti < p->targets_count && errs < TG3__IDX_ERR_CAP; ++ti) {
                    uint32_t tk;
                    uint32_t tcount = p->target_attribute_counts ? p->target_attribute_counts[ti] : 0;
                    const tg3_str_int_pair *tarr = p->targets ? p->targets[ti] : NULL;
                    if (!tarr) continue;
                    for (tk = 0; tk < tcount && errs < TG3__IDX_ERR_CAP; ++tk) {
                        TG3__CHECK_REQ(tarr[tk].value, m->accessors_count, "/meshes",
                                       "meshes[%u].primitives[%u].targets[%u][%u] %d out of range [0,%u)",
                                       i, j, ti, tk, tarr[tk].value, m->accessors_count);
                    }
                }
            }
        }
    }
    for (i = 0; i < m->nodes_count && errs < TG3__IDX_ERR_CAP; ++i) {
        const tg3_node *n = &m->nodes[i];
        uint32_t k;
        TG3__CHECK_OPT(n->mesh,   m->meshes_count,   "/nodes", "nodes[%u].mesh %d out of range [0,%u)",   i, n->mesh,   m->meshes_count);
        TG3__CHECK_OPT(n->skin,   m->skins_count,    "/nodes", "nodes[%u].skin %d out of range [0,%u)",   i, n->skin,   m->skins_count);
        TG3__CHECK_OPT(n->camera, m->cameras_count,  "/nodes", "nodes[%u].camera %d out of range [0,%u)", i, n->camera, m->cameras_count);
        TG3__CHECK_OPT(n->light,  m->lights_count,   "/nodes", "nodes[%u].light %d out of range [0,%u)",  i, n->light,  m->lights_count);
        for (k = 0; k < n->children_count && errs < TG3__IDX_ERR_CAP; ++k) {
            TG3__CHECK_REQ(n->children[k], m->nodes_count, "/nodes",
                           "nodes[%u].children[%u] %d out of range [0,%u)",
                           i, k, n->children[k], m->nodes_count);
        }
    }
    for (i = 0; i < m->textures_count && errs < TG3__IDX_ERR_CAP; ++i) {
        TG3__CHECK_OPT(m->textures[i].source,  m->images_count,   "/textures",
                       "textures[%u].source %d out of range [0,%u)",  i, m->textures[i].source,  m->images_count);
        TG3__CHECK_OPT(m->textures[i].sampler, m->samplers_count, "/textures",
                       "textures[%u].sampler %d out of range [0,%u)", i, m->textures[i].sampler, m->samplers_count);
    }
    for (i = 0; i < m->images_count && errs < TG3__IDX_ERR_CAP; ++i) {
        TG3__CHECK_OPT(m->images[i].buffer_view, m->buffer_views_count, "/images",
                       "images[%u].bufferView %d out of range [0,%u)",
                       i, m->images[i].buffer_view, m->buffer_views_count);
    }
    for (i = 0; i < m->skins_count && errs < TG3__IDX_ERR_CAP; ++i) {
        const tg3_skin *s = &m->skins[i];
        uint32_t k;
        TG3__CHECK_OPT(s->inverse_bind_matrices, m->accessors_count, "/skins",
                       "skins[%u].inverseBindMatrices %d out of range [0,%u)",
                       i, s->inverse_bind_matrices, m->accessors_count);
        TG3__CHECK_OPT(s->skeleton, m->nodes_count, "/skins",
                       "skins[%u].skeleton %d out of range [0,%u)",
                       i, s->skeleton, m->nodes_count);
        for (k = 0; k < s->joints_count && errs < TG3__IDX_ERR_CAP; ++k) {
            TG3__CHECK_REQ(s->joints[k], m->nodes_count, "/skins",
                           "skins[%u].joints[%u] %d out of range [0,%u)",
                           i, k, s->joints[k], m->nodes_count);
        }
    }
    for (i = 0; i < m->animations_count && errs < TG3__IDX_ERR_CAP; ++i) {
        const tg3_animation *an = &m->animations[i];
        uint32_t k;
        for (k = 0; k < an->channels_count && errs < TG3__IDX_ERR_CAP; ++k) {
            TG3__CHECK_REQ(an->channels[k].sampler, an->samplers_count, "/animations",
                           "animations[%u].channels[%u].sampler %d out of range [0,%u)",
                           i, k, an->channels[k].sampler, an->samplers_count);
            TG3__CHECK_OPT(an->channels[k].target.node, m->nodes_count, "/animations",
                           "animations[%u].channels[%u].target.node %d out of range [0,%u)",
                           i, k, an->channels[k].target.node, m->nodes_count);
        }
        for (k = 0; k < an->samplers_count && errs < TG3__IDX_ERR_CAP; ++k) {
            TG3__CHECK_REQ(an->samplers[k].input,  m->accessors_count, "/animations",
                           "animations[%u].samplers[%u].input %d out of range [0,%u)",
                           i, k, an->samplers[k].input,  m->accessors_count);
            TG3__CHECK_REQ(an->samplers[k].output, m->accessors_count, "/animations",
                           "animations[%u].samplers[%u].output %d out of range [0,%u)",
                           i, k, an->samplers[k].output, m->accessors_count);
        }
    }
    for (i = 0; i < m->scenes_count && errs < TG3__IDX_ERR_CAP; ++i) {
        uint32_t k;
        const tg3_scene *s = &m->scenes[i];
        for (k = 0; k < s->nodes_count && errs < TG3__IDX_ERR_CAP; ++k) {
            TG3__CHECK_REQ(s->nodes[k], m->nodes_count, "/scenes",
                           "scenes[%u].nodes[%u] %d out of range [0,%u)",
                           i, k, s->nodes[k], m->nodes_count);
        }
        for (k = 0; k < s->audio_emitters_count && errs < TG3__IDX_ERR_CAP; ++k) {
            TG3__CHECK_REQ(s->audio_emitters[k], m->audio_emitters_count, "/scenes",
                           "scenes[%u].audio_emitters[%u] %d out of range [0,%u)",
                           i, k, s->audio_emitters[k], m->audio_emitters_count);
        }
    }
    /* Extension index fields (KHR_audio, MSFT_lod). */
    for (i = 0; i < m->nodes_count && errs < TG3__IDX_ERR_CAP; ++i) {
        const tg3_node *n = &m->nodes[i];
        uint32_t k;
        TG3__CHECK_OPT(n->emitter, m->audio_emitters_count, "/nodes",
                       "nodes[%u].emitter %d out of range [0,%u)",
                       i, n->emitter, m->audio_emitters_count);
        for (k = 0; k < n->lods_count && errs < TG3__IDX_ERR_CAP; ++k) {
            TG3__CHECK_REQ(n->lods[k], m->nodes_count, "/nodes",
                           "nodes[%u].lods[%u] %d out of range [0,%u)",
                           i, k, n->lods[k], m->nodes_count);
        }
    }
    for (i = 0; i < m->materials_count && errs < TG3__IDX_ERR_CAP; ++i) {
        const tg3_material *mat = &m->materials[i];
        uint32_t k;
        for (k = 0; k < mat->lods_count && errs < TG3__IDX_ERR_CAP; ++k) {
            TG3__CHECK_REQ(mat->lods[k], m->materials_count, "/materials",
                           "materials[%u].lods[%u] %d out of range [0,%u)",
                           i, k, mat->lods[k], m->materials_count);
        }
    }
    for (i = 0; i < m->audio_sources_count && errs < TG3__IDX_ERR_CAP; ++i) {
        TG3__CHECK_OPT(m->audio_sources[i].buffer_view, m->buffer_views_count,
                       "/extensions/KHR_audio/sources",
                       "audio_sources[%u].bufferView %d out of range [0,%u)",
                       i, m->audio_sources[i].buffer_view, m->buffer_views_count);
    }
    for (i = 0; i < m->audio_emitters_count && errs < TG3__IDX_ERR_CAP; ++i) {
        TG3__CHECK_OPT(m->audio_emitters[i].source, m->audio_sources_count,
                       "/extensions/KHR_audio/emitters",
                       "audio_emitters[%u].source %d out of range [0,%u)",
                       i, m->audio_emitters[i].source, m->audio_sources_count);
    }

    #undef TG3__CHECK_OPT
    #undef TG3__CHECK_REQ
    #undef TG3__IDX_BAD
    return errs == 0;
}

static tg3_error_code tg3__parse_from_json(tg3__parse_ctx *ctx, const tg3json_value *json_doc, tg3_model *model) {
    const tg3json_value *asset_it = tg3json_object_get(json_doc, "asset");
    const tg3json_value *ext_it;
    if (tg3__json_is_object(asset_it)) {
        tg3__parse_asset(ctx, asset_it, &model->asset);
    } else if (ctx->opts.required_sections & TG3_REQUIRE_VERSION) {
        tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR, TG3_ERR_MISSING_REQUIRED,
                        "Missing required 'asset' property", "/", -1);
        return TG3_ERR_MISSING_REQUIRED;
    }
    if (ctx->opts.stream && ctx->opts.stream->on_asset) {
        tg3_stream_action sa = ctx->opts.stream->on_asset(&model->asset, ctx->opts.stream->user_data);
        if (sa == TG3_STREAM_ABORT) return TG3_ERR_STREAM_ABORTED;
    }
    tg3__parse_string_array(ctx, json_doc, "extensionsUsed", &model->extensions_used, &model->extensions_used_count, 0, "/");
    tg3__parse_string_array(ctx, json_doc, "extensionsRequired", &model->extensions_required, &model->extensions_required_count, 0, "/");
    model->default_scene = -1;
    tg3__parse_int(ctx, json_doc, "scene", &model->default_scene, 0, "/");

    TG3__PARSE_ARRAY_IDX(ctx, json_doc, "buffers", tg3_buffer, model->buffers, model->buffers_count, tg3__parse_buffer);
    TG3__STREAM_CB(on_buffer, model->buffers, model->buffers_count);
    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "bufferViews", tg3_buffer_view, model->buffer_views, model->buffer_views_count, tg3__parse_buffer_view);
    TG3__STREAM_CB(on_buffer_view, model->buffer_views, model->buffer_views_count);
    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "accessors", tg3_accessor, model->accessors, model->accessors_count, tg3__parse_accessor);
    TG3__STREAM_CB(on_accessor, model->accessors, model->accessors_count);
    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "meshes", tg3_mesh, model->meshes, model->meshes_count, tg3__parse_mesh);
    TG3__STREAM_CB(on_mesh, model->meshes, model->meshes_count);
    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "nodes", tg3_node, model->nodes, model->nodes_count, tg3__parse_node);
    TG3__STREAM_CB(on_node, model->nodes, model->nodes_count);
    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "materials", tg3_material, model->materials, model->materials_count, tg3__parse_material);
    TG3__STREAM_CB(on_material, model->materials, model->materials_count);
    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "textures", tg3_texture, model->textures, model->textures_count, tg3__parse_texture);
    TG3__STREAM_CB(on_texture, model->textures, model->textures_count);
    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "samplers", tg3_sampler, model->samplers, model->samplers_count, tg3__parse_sampler);
    TG3__STREAM_CB(on_sampler, model->samplers, model->samplers_count);
    TG3__PARSE_ARRAY_IDX(ctx, json_doc, "images", tg3_image, model->images, model->images_count, tg3__parse_image);
    TG3__STREAM_CB(on_image, model->images, model->images_count);
    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "skins", tg3_skin, model->skins, model->skins_count, tg3__parse_skin);
    TG3__STREAM_CB(on_skin, model->skins, model->skins_count);
    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "animations", tg3_animation, model->animations, model->animations_count, tg3__parse_animation);
    TG3__STREAM_CB(on_animation, model->animations, model->animations_count);
    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "cameras", tg3_camera, model->cameras, model->cameras_count, tg3__parse_camera);
    TG3__STREAM_CB(on_camera, model->cameras, model->cameras_count);
    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "scenes", tg3_scene, model->scenes, model->scenes_count, tg3__parse_scene);
    TG3__STREAM_CB(on_scene, model->scenes, model->scenes_count);

    ext_it = tg3json_object_get(json_doc, "extensions");
    if (tg3__json_is_object(ext_it)) {
        const tg3json_value *lights_ext = tg3json_object_get(ext_it, "KHR_lights_punctual");
        const tg3json_value *audio_ext = tg3json_object_get(ext_it, "KHR_audio");
        if (tg3__json_is_object(lights_ext)) {
            TG3__PARSE_ARRAY_SIMPLE(ctx, lights_ext, "lights", tg3_light, model->lights, model->lights_count, tg3__parse_light);
            TG3__STREAM_CB(on_light, model->lights, model->lights_count);
        }
        if (tg3__json_is_object(audio_ext)) {
            TG3__PARSE_ARRAY_SIMPLE(ctx, audio_ext, "sources", tg3_audio_source, model->audio_sources, model->audio_sources_count, tg3__parse_audio_source);
            TG3__PARSE_ARRAY_SIMPLE(ctx, audio_ext, "emitters", tg3_audio_emitter, model->audio_emitters, model->audio_emitters_count, tg3__parse_audio_emitter);
        }
    }
    tg3__parse_extras_and_extensions(ctx, json_doc, &model->ext);
    if (ctx->opts.validate_indices) {
        if (!tg3__validate_indices(ctx, model)) {
            return TG3_ERR_INVALID_INDEX;
        }
    }
    return (ctx->errors && ctx->errors->has_error) ? TG3_ERR_JSON_PARSE : TG3_OK;
}

static tg3_error_code tg3__parse_glb_header(const uint8_t *data, uint64_t size,
                                            const uint8_t **json_out, uint64_t *json_size_out,
                                            const uint8_t **bin_out, uint64_t *bin_size_out,
                                            tg3_error_stack *errors) {
    uint32_t version;
    uint32_t total_length;
    uint64_t offset = 12;
    *json_out = NULL; *json_size_out = 0; *bin_out = NULL; *bin_size_out = 0;
    if (size < 12) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_INVALID_HEADER,
                        "GLB data too small for header", NULL, -1);
        return TG3_ERR_GLB_INVALID_HEADER;
    }
    if (data[0] != 'g' || data[1] != 'l' || data[2] != 'T' || data[3] != 'F') {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_INVALID_MAGIC,
                        "Invalid GLB magic bytes", NULL, -1);
        return TG3_ERR_GLB_INVALID_MAGIC;
    }
    memcpy(&version, data + 4, 4);
    if (version != 2) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_INVALID_VERSION,
                        "Unsupported GLB version (expected 2)", NULL, -1);
        return TG3_ERR_GLB_INVALID_VERSION;
    }
    memcpy(&total_length, data + 8, 4);
    if ((uint64_t)total_length > size) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_SIZE_MISMATCH,
                        "GLB total length exceeds data size", NULL, -1);
        return TG3_ERR_GLB_SIZE_MISMATCH;
    }
    while (offset + 8 <= size) {
        uint32_t chunk_length;
        uint32_t chunk_type;
        memcpy(&chunk_length, data + offset, 4); offset += 4;
        memcpy(&chunk_type, data + offset, 4); offset += 4;
        if (offset + chunk_length > size) {
            tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_CHUNK_ERROR,
                            "GLB chunk exceeds data size", NULL, -1);
            return TG3_ERR_GLB_CHUNK_ERROR;
        }
        if (chunk_type == 0x4E4F534Au) {
            *json_out = data + offset;
            *json_size_out = chunk_length;
        } else if (chunk_type == 0x004E4942u) {
            *bin_out = data + offset;
            *bin_size_out = chunk_length;
        }
        offset += chunk_length;
        while ((offset & 3u) != 0u && offset < size) ++offset;
    }
    if (!*json_out) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_CHUNK_ERROR,
                        "GLB missing JSON chunk", NULL, -1);
        return TG3_ERR_GLB_CHUNK_ERROR;
    }
    return TG3_OK;
}

TINYGLTF3_API tg3_error_code tg3_parse(tg3_model *model, tg3_error_stack *errors,
                                       const uint8_t *json_data, uint64_t json_size,
                                       const char *base_dir, uint32_t base_dir_len,
                                       const tg3_parse_options *options) {
    tg3_parse_options default_opts;
    tg3_arena *arena;
    tg3__parse_ctx ctx;
    tg3json_value json_doc;
    const char *error_pos = NULL;
    tg3_error_code ret;
    int parsed_ok;
    if (!options) { tg3_parse_options_init(&default_opts); options = &default_opts; }
    tg3__model_init(model);
    arena = tg3__arena_create(&options->memory);
    if (!arena) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY, "Failed to create arena", NULL, -1);
        return TG3_ERR_OUT_OF_MEMORY;
    }
    model->arena_ = arena;
    parsed_ok = tg3json_parse_n((const char *)json_data, (size_t)json_size,
                                TINYGLTF3_MAX_NESTING_DEPTH, &json_doc, &error_pos);
    if (!parsed_ok || json_doc.type != TG3JSON_OBJECT) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_JSON_PARSE, "Failed to parse JSON", NULL,
                        error_pos ? (int64_t)(error_pos - (const char *)json_data) : -1);
        if (parsed_ok) tg3json_value_free(&json_doc);
        /* Keep arena alive so error messages (arena-allocated) stay valid for
         * the caller; tg3_model_free is the sole arena owner on the error path. */
        return TG3_ERR_JSON_PARSE;
    }
    memset(&ctx, 0, sizeof(ctx));
    ctx.arena = arena;
    ctx.errors = errors;
    ctx.opts = *options;
    ctx.base_dir = base_dir;
    ctx.base_dir_len = base_dir_len;
#ifdef TINYGLTF3_ENABLE_FS
    tg3__set_default_fs(&ctx.opts.fs);
#endif
    ret = tg3__parse_from_json(&ctx, &json_doc, model);
    tg3json_value_free(&json_doc);
    /* Arena stays attached to model->arena_ on both success and failure;
     * tg3_model_free reclaims it. Destroying here would dangle arena-allocated
     * error messages on the user-facing tg3_error_stack. */
    return ret;
}

TINYGLTF3_API tg3_error_code tg3_parse_glb(tg3_model *model, tg3_error_stack *errors,
                                           const uint8_t *glb_data, uint64_t glb_size,
                                           const char *base_dir, uint32_t base_dir_len,
                                           const tg3_parse_options *options) {
    const uint8_t *json_chunk = NULL;
    uint64_t json_chunk_size = 0;
    const uint8_t *bin_chunk = NULL;
    uint64_t bin_chunk_size = 0;
    tg3_parse_options default_opts;
    tg3_arena *arena;
    tg3__parse_ctx ctx;
    tg3json_value json_doc;
    const char *error_pos = NULL;
    tg3_error_code err;
    int parsed_ok;
    /* Initialize model before any failure-return so callers can safely call
     * tg3_model_free() on the error path; the GLB header parse must not run
     * against a model whose arena_ field is uninitialized garbage. */
    tg3__model_init(model);
    err = tg3__parse_glb_header(glb_data, glb_size, &json_chunk, &json_chunk_size, &bin_chunk, &bin_chunk_size, errors);
    if (err != TG3_OK) return err;
    if (!options) { tg3_parse_options_init(&default_opts); options = &default_opts; }
    arena = tg3__arena_create(&options->memory);
    if (!arena) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY, "Failed to create arena", NULL, -1);
        return TG3_ERR_OUT_OF_MEMORY;
    }
    model->arena_ = arena;
    parsed_ok = tg3json_parse_n((const char *)json_chunk, (size_t)json_chunk_size,
                                TINYGLTF3_MAX_NESTING_DEPTH, &json_doc, &error_pos);
    if (!parsed_ok || json_doc.type != TG3JSON_OBJECT) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_JSON_PARSE, "Failed to parse GLB JSON chunk", NULL,
                        error_pos ? (int64_t)(error_pos - (const char *)json_chunk) : -1);
        if (parsed_ok) tg3json_value_free(&json_doc);
        /* Keep arena alive so error messages stay valid; model_free owns it. */
        return TG3_ERR_JSON_PARSE;
    }
    memset(&ctx, 0, sizeof(ctx));
    ctx.arena = arena;
    ctx.errors = errors;
    ctx.opts = *options;
    ctx.base_dir = base_dir;
    ctx.base_dir_len = base_dir_len;
    ctx.is_binary = 1;
    ctx.bin_data = bin_chunk;
    ctx.bin_size = bin_chunk_size;
#ifdef TINYGLTF3_ENABLE_FS
    tg3__set_default_fs(&ctx.opts.fs);
#endif
    err = tg3__parse_from_json(&ctx, &json_doc, model);
    tg3json_value_free(&json_doc);
    /* Arena stays attached to model->arena_ on both paths; model_free owns it. */
    return err;
}

TINYGLTF3_API tg3_error_code tg3_parse_auto(tg3_model *model, tg3_error_stack *errors,
                                            const uint8_t *data, uint64_t size,
                                            const char *base_dir, uint32_t base_dir_len,
                                            const tg3_parse_options *options) {
    if (size >= 4 && data[0] == 'g' && data[1] == 'l' && data[2] == 'T' && data[3] == 'F') {
        return tg3_parse_glb(model, errors, data, size, base_dir, base_dir_len, options);
    }
    return tg3_parse(model, errors, data, size, base_dir, base_dir_len, options);
}

TINYGLTF3_API tg3_error_code tg3_parse_file(tg3_model *model, tg3_error_stack *errors,
                                            const char *filename, uint32_t filename_len,
                                            const tg3_parse_options *options) {
    tg3_parse_options opts;
    uint8_t *file_data = NULL;
    uint64_t file_size = 0;
    char base_dir_buf[4096];
    uint32_t base_dir_len = 0;
    tg3_error_code result;
    uint32_t i;
    if (options) opts = *options;
    else tg3_parse_options_init(&opts);
    tg3__model_init(model);
#ifdef TINYGLTF3_ENABLE_FS
    tg3__set_default_fs(&opts.fs);
#endif
    if (!opts.fs.read_file) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_FS_NOT_AVAILABLE,
                        "No filesystem callbacks. Define TINYGLTF3_ENABLE_FS or provide fs callbacks.", NULL, -1);
        return TG3_ERR_FS_NOT_AVAILABLE;
    }
    if (!opts.fs.read_file(&file_data, &file_size, filename, filename_len, opts.fs.user_data) || !file_data) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_FILE_NOT_FOUND,
                        "Failed to read file", NULL, -1);
        return TG3_ERR_FILE_NOT_FOUND;
    }
    memset(base_dir_buf, 0, sizeof(base_dir_buf));
    for (i = 0; i < filename_len; ++i) {
        if (filename[i] == '/' || filename[i] == '\\') base_dir_len = i;
    }
    if (base_dir_len > 0) {
        memcpy(base_dir_buf, filename, base_dir_len);
        base_dir_buf[base_dir_len] = '\0';
    }
    result = tg3_parse_auto(model, errors, file_data, file_size, base_dir_buf, base_dir_len, &opts);
    if (opts.fs.free_file) opts.fs.free_file(file_data, file_size, opts.fs.user_data);
    return result;
}

TINYGLTF3_API void tg3_model_free(tg3_model *model) {
    if (!model) return;
    if (model->arena_) tg3__arena_destroy(model->arena_);
    memset(model, 0, sizeof(*model));
    model->default_scene = -1;
}

static char *tg3__b64_encode(const uint8_t *input, size_t input_len, size_t *out_len) {
    static const char chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t enc_len = ((input_len + 2u) / 3u) * 4u;
    char *out = (char *)malloc(enc_len + 1u);
    size_t i;
    size_t j = 0;
    if (!out) return NULL;
    for (i = 0; i < input_len; i += 3u) {
        uint32_t a = input[i];
        uint32_t b = (i + 1u < input_len) ? input[i + 1u] : 0u;
        uint32_t c = (i + 2u < input_len) ? input[i + 2u] : 0u;
        uint32_t triple = (a << 16u) | (b << 8u) | c;
        out[j++] = chars[(triple >> 18u) & 0x3Fu];
        out[j++] = chars[(triple >> 12u) & 0x3Fu];
        out[j++] = (i + 1u < input_len) ? chars[(triple >> 6u) & 0x3Fu] : '=';
        out[j++] = (i + 2u < input_len) ? chars[triple & 0x3Fu] : '=';
    }
    out[j] = '\0';
    if (out_len) *out_len = enc_len;
    return out;
}

static const char *tg3__accessor_type_to_string(int type) {
    switch (type) {
        case TG3_TYPE_SCALAR: return "SCALAR";
        case TG3_TYPE_VEC2: return "VEC2";
        case TG3_TYPE_VEC3: return "VEC3";
        case TG3_TYPE_VEC4: return "VEC4";
        case TG3_TYPE_MAT2: return "MAT2";
        case TG3_TYPE_MAT3: return "MAT3";
        case TG3_TYPE_MAT4: return "MAT4";
        default: return "";
    }
}

static int tg3__json_set_take(tg3json_value *obj, const char *key, tg3json_value *value) {
    if (!tg3json_object_set_take(obj, key, value)) {
        tg3json_value_free(value);
        return 0;
    }
    return 1;
}

static int tg3__json_push_take(tg3json_value *arr, tg3json_value *value) {
    if (!tg3json_array_append_take(arr, value)) {
        tg3json_value_free(value);
        return 0;
    }
    return 1;
}

static int tg3__json_set_int(tg3json_value *obj, const char *key, int64_t value) {
    tg3json_value tmp;
    tg3json_value_init_int(&tmp, value);
    return tg3__json_set_take(obj, key, &tmp);
}

static int tg3__json_set_real(tg3json_value *obj, const char *key, double value) {
    tg3json_value tmp;
    tg3json_value_init_real(&tmp, value);
    return tg3__json_set_take(obj, key, &tmp);
}

static int tg3__json_set_bool(tg3json_value *obj, const char *key, int value) {
    tg3json_value tmp;
    tg3json_value_init_bool(&tmp, value);
    return tg3__json_set_take(obj, key, &tmp);
}

static int tg3__json_set_string_n(tg3json_value *obj, const char *key, const char *str, size_t len) {
    tg3json_value tmp;
    if (!tg3json_value_init_string_n(&tmp, str, len)) return 0;
    return tg3__json_set_take(obj, key, &tmp);
}

static int tg3__json_set_string(tg3json_value *obj, const char *key, const char *str) {
    return tg3__json_set_string_n(obj, key, str, str ? strlen(str) : 0);
}

static int tg3__json_array_append_int(tg3json_value *arr, int64_t value) {
    tg3json_value tmp;
    tg3json_value_init_int(&tmp, value);
    return tg3__json_push_take(arr, &tmp);
}

static int tg3__json_array_append_real(tg3json_value *arr, double value) {
    tg3json_value tmp;
    tg3json_value_init_real(&tmp, value);
    return tg3__json_push_take(arr, &tmp);
}

static int tg3__json_ensure_object(tg3json_value *obj, const char *key, tg3json_value **out_child) {
    tg3json_value *existing = tg3json_object_get_mut(obj, key);
    if (existing) {
        if (existing->type != TG3JSON_OBJECT) return 0;
        *out_child = existing;
        return 1;
    }
    {
        tg3json_value tmp;
        tg3json_value_init_object(&tmp);
        if (!tg3__json_set_take(obj, key, &tmp)) return 0;
    }
    existing = tg3json_object_get_mut(obj, key);
    if (!existing || existing->type != TG3JSON_OBJECT) return 0;
    *out_child = existing;
    return 1;
}

static int tg3__json_ensure_array(tg3json_value *obj, const char *key, tg3json_value **out_child) {
    tg3json_value *existing = tg3json_object_get_mut(obj, key);
    if (existing) {
        if (existing->type != TG3JSON_ARRAY) return 0;
        *out_child = existing;
        return 1;
    }
    {
        tg3json_value tmp;
        tg3json_value_init_array(&tmp);
        if (!tg3__json_set_take(obj, key, &tmp)) return 0;
    }
    existing = tg3json_object_get_mut(obj, key);
    if (!existing || existing->type != TG3JSON_ARRAY) return 0;
    *out_child = existing;
    return 1;
}

static int tg3__serialize_str(tg3json_value *o, const char *key, tg3_str val) {
    if (!val.data || val.len == 0) return 1;
    return tg3__json_set_string_n(o, key, val.data, val.len);
}

static int tg3__serialize_int(tg3json_value *o, const char *key, int32_t val,
                              int32_t default_val, int write_defaults) {
    if (val == default_val && !write_defaults) return 1;
    return tg3__json_set_int(o, key, val);
}

static int tg3__serialize_uint64(tg3json_value *o, const char *key, uint64_t val,
                                 uint64_t default_val, int write_defaults) {
    if (val == default_val && !write_defaults) return 1;
    return tg3__json_set_int(o, key, (int64_t)val);
}

static int tg3__serialize_double(tg3json_value *o, const char *key, double val,
                                 double default_val, int write_defaults) {
    if (fabs(val - default_val) <= 1e-12 && !write_defaults) return 1;
    return tg3__json_set_real(o, key, val);
}

static int tg3__serialize_bool(tg3json_value *o, const char *key, int32_t val,
                               int32_t default_val, int write_defaults) {
    if (val == default_val && !write_defaults) return 1;
    return tg3__json_set_bool(o, key, val != 0);
}

static int tg3__json_from_double_array(const double *arr, uint32_t count, tg3json_value *out) {
    uint32_t i;
    tg3json_value_init_array(out);
    for (i = 0; i < count; ++i) {
        if (!tg3__json_array_append_real(out, arr[i])) {
            tg3json_value_free(out);
            return 0;
        }
    }
    return 1;
}

static int tg3__json_from_int_array(const int32_t *arr, uint32_t count, tg3json_value *out) {
    uint32_t i;
    tg3json_value_init_array(out);
    for (i = 0; i < count; ++i) {
        if (!tg3__json_array_append_int(out, arr[i])) {
            tg3json_value_free(out);
            return 0;
        }
    }
    return 1;
}

static int tg3__json_from_string_array(const tg3_str *arr, uint32_t count, tg3json_value *out) {
    uint32_t i;
    tg3json_value_init_array(out);
    for (i = 0; i < count; ++i) {
        tg3json_value tmp;
        if (!arr[i].data) continue;
        if (!tg3json_value_init_string_n(&tmp, arr[i].data, arr[i].len)) {
            tg3json_value_free(out);
            return 0;
        }
        if (!tg3__json_push_take(out, &tmp)) {
            tg3json_value_free(out);
            return 0;
        }
    }
    return 1;
}

static int tg3__serialize_double_array(tg3json_value *o, const char *key,
                                       const double *arr, uint32_t count) {
    tg3json_value tmp;
    if (!arr || count == 0) return 1;
    if (!tg3__json_from_double_array(arr, count, &tmp)) return 0;
    return tg3__json_set_take(o, key, &tmp);
}

static int tg3__serialize_int_array(tg3json_value *o, const char *key,
                                    const int32_t *arr, uint32_t count) {
    tg3json_value tmp;
    if (!arr || count == 0) return 1;
    if (!tg3__json_from_int_array(arr, count, &tmp)) return 0;
    return tg3__json_set_take(o, key, &tmp);
}

static int tg3__serialize_string_array(tg3json_value *o, const char *key,
                                       const tg3_str *arr, uint32_t count) {
    tg3json_value tmp;
    if (!arr || count == 0) return 1;
    if (!tg3__json_from_string_array(arr, count, &tmp)) return 0;
    return tg3__json_set_take(o, key, &tmp);
}

static int tg3__value_to_json(const tg3_value *v, tg3json_value *out) {
    uint32_t i;
    tg3json_value_init_null(out);
    if (!v) return 1;
    switch (v->type) {
        case TG3_VALUE_NULL:
            return 1;
        case TG3_VALUE_BOOL:
            tg3json_value_init_bool(out, v->bool_val != 0);
            return 1;
        case TG3_VALUE_INT:
            tg3json_value_init_int(out, v->int_val);
            return 1;
        case TG3_VALUE_REAL:
            tg3json_value_init_real(out, v->real_val);
            return 1;
        case TG3_VALUE_STRING:
            return tg3json_value_init_string_n(out,
                v->string_val.data ? v->string_val.data : "",
                v->string_val.data ? v->string_val.len : 0);
        case TG3_VALUE_ARRAY:
            tg3json_value_init_array(out);
            for (i = 0; i < v->array_count; ++i) {
                tg3json_value item;
                if (!tg3__value_to_json(&v->array_data[i], &item)) {
                    tg3json_value_free(out);
                    return 0;
                }
                if (!tg3__json_push_take(out, &item)) {
                    tg3json_value_free(out);
                    return 0;
                }
            }
            return 1;
        case TG3_VALUE_OBJECT:
            tg3json_value_init_object(out);
            for (i = 0; i < v->object_count; ++i) {
                tg3json_value item;
                if (!tg3__value_to_json(&v->object_data[i].value, &item)) {
                    tg3json_value_free(out);
                    return 0;
                }
                if (!tg3json_object_set_take_n(out,
                                               v->object_data[i].key.data ? v->object_data[i].key.data : "",
                                               v->object_data[i].key.data ? v->object_data[i].key.len : 0,
                                               &item)) {
                    tg3json_value_free(&item);
                    tg3json_value_free(out);
                    return 0;
                }
            }
            return 1;
        case TG3_VALUE_BINARY:
            /* Binary blobs cannot be represented as JSON; emit null. */
            return 1;
    }
    return 0; /* unreachable: all enum cases handled above. */
}

static int tg3__serialize_extras_ext(tg3json_value *o, const tg3_extras_ext *ee) {
    uint32_t i;
    if (!ee) return 1;
    if (ee->extras) {
        tg3json_value extras;
        if (!tg3__value_to_json(ee->extras, &extras)) return 0;
        if (!tg3__json_set_take(o, "extras", &extras)) return 0;
    }
    if (ee->extensions && ee->extensions_count > 0) {
        tg3json_value *exts = NULL;
        if (!tg3__json_ensure_object(o, "extensions", &exts)) return 0;
        for (i = 0; i < ee->extensions_count; ++i) {
            tg3json_value item;
            if (!tg3__value_to_json(&ee->extensions[i].value, &item)) return 0;
            if (!tg3json_object_set_take_n(exts,
                                           ee->extensions[i].name.data ? ee->extensions[i].name.data : "",
                                           ee->extensions[i].name.data ? ee->extensions[i].name.len : 0,
                                           &item)) {
                tg3json_value_free(&item);
                return 0;
            }
        }
    }
    return 1;
}

static int tg3__serialize_texture_info(tg3json_value *parent, const char *key,
                                       const tg3_texture_info *ti, int wd) {
    tg3json_value o;
    if (ti->index < 0) return 1;
    tg3json_value_init_object(&o);
    if (!tg3__json_set_int(&o, "index", ti->index) ||
        !tg3__serialize_int(&o, "texCoord", ti->tex_coord, 0, wd) ||
        !tg3__serialize_extras_ext(&o, &ti->ext) ||
        !tg3__json_set_take(parent, key, &o)) {
        tg3json_value_free(&o);
        return 0;
    }
    return 1;
}

static int tg3__serialize_normal_texture_info(tg3json_value *parent, const char *key,
                                              const tg3_normal_texture_info *ti, int wd) {
    tg3json_value o;
    if (ti->index < 0) return 1;
    tg3json_value_init_object(&o);
    if (!tg3__json_set_int(&o, "index", ti->index) ||
        !tg3__serialize_int(&o, "texCoord", ti->tex_coord, 0, wd) ||
        !tg3__serialize_double(&o, "scale", ti->scale, 1.0, wd) ||
        !tg3__serialize_extras_ext(&o, &ti->ext) ||
        !tg3__json_set_take(parent, key, &o)) {
        tg3json_value_free(&o);
        return 0;
    }
    return 1;
}

static int tg3__serialize_occlusion_texture_info(tg3json_value *parent, const char *key,
                                                 const tg3_occlusion_texture_info *ti, int wd) {
    tg3json_value o;
    if (ti->index < 0) return 1;
    tg3json_value_init_object(&o);
    if (!tg3__json_set_int(&o, "index", ti->index) ||
        !tg3__serialize_int(&o, "texCoord", ti->tex_coord, 0, wd) ||
        !tg3__serialize_double(&o, "strength", ti->strength, 1.0, wd) ||
        !tg3__serialize_extras_ext(&o, &ti->ext) ||
        !tg3__json_set_take(parent, key, &o)) {
        tg3json_value_free(&o);
        return 0;
    }
    return 1;
}

static int tg3__serialize_asset(const tg3_asset *a, int wd, tg3json_value *out) {
    (void)wd;
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "version", a->version) ||
        !tg3__serialize_str(out, "generator", a->generator) ||
        !tg3__serialize_str(out, "minVersion", a->min_version) ||
        !tg3__serialize_str(out, "copyright", a->copyright) ||
        !tg3__serialize_extras_ext(out, &a->ext)) {
        tg3json_value_free(out);
        return 0;
    }
    return 1;
}

static int tg3__serialize_buffer(const tg3_buffer *b, int wd, int embed, tg3json_value *out) {
    (void)wd;
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", b->name) ||
        !tg3__json_set_int(out, "byteLength", (int64_t)b->data.count)) {
        tg3json_value_free(out);
        return 0;
    }
    if (b->uri.data && b->uri.len > 0) {
        if (!tg3__serialize_str(out, "uri", b->uri)) {
            tg3json_value_free(out);
            return 0;
        }
    } else if (embed && b->data.data && b->data.count > 0) {
        size_t enc_len = 0;
        char *encoded = tg3__b64_encode(b->data.data, (size_t)b->data.count, &enc_len);
        const char prefix[] = "data:application/octet-stream;base64,";
        size_t prefix_len = sizeof(prefix) - 1u;
        char *uri;
        if (!encoded) {
            tg3json_value_free(out);
            return 0;
        }
        uri = (char *)malloc(prefix_len + enc_len + 1u);
        if (!uri) {
            free(encoded);
            tg3json_value_free(out);
            return 0;
        }
        memcpy(uri, prefix, prefix_len);
        memcpy(uri + prefix_len, encoded, enc_len);
        uri[prefix_len + enc_len] = '\0';
        free(encoded);
        if (!tg3__json_set_string_n(out, "uri", uri, prefix_len + enc_len)) {
            free(uri);
            tg3json_value_free(out);
            return 0;
        }
        free(uri);
    }
    if (!tg3__serialize_extras_ext(out, &b->ext)) {
        tg3json_value_free(out);
        return 0;
    }
    return 1;
}

static int tg3__serialize_buffer_view(const tg3_buffer_view *bv, int wd, tg3json_value *out) {
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", bv->name) ||
        !tg3__json_set_int(out, "buffer", bv->buffer) ||
        !tg3__json_set_int(out, "byteLength", (int64_t)bv->byte_length) ||
        !tg3__serialize_uint64(out, "byteOffset", bv->byte_offset, 0, wd)) {
        tg3json_value_free(out);
        return 0;
    }
    if (bv->byte_stride > 0 && !tg3__json_set_int(out, "byteStride", bv->byte_stride)) {
        tg3json_value_free(out);
        return 0;
    }
    if (!tg3__serialize_int(out, "target", bv->target, 0, wd) ||
        !tg3__serialize_extras_ext(out, &bv->ext)) {
        tg3json_value_free(out);
        return 0;
    }
    return 1;
}

static int tg3__serialize_accessor(const tg3_accessor *acc, int wd, tg3json_value *out) {
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", acc->name) ||
        !tg3__serialize_uint64(out, "byteOffset", acc->byte_offset, 0, wd) ||
        !tg3__json_set_int(out, "componentType", acc->component_type) ||
        !tg3__json_set_int(out, "count", (int64_t)acc->count) ||
        !tg3__json_set_string(out, "type", tg3__accessor_type_to_string(acc->type)) ||
        !tg3__serialize_bool(out, "normalized", acc->normalized, 0, wd) ||
        !tg3__serialize_double_array(out, "min", acc->min_values, acc->min_values_count) ||
        !tg3__serialize_double_array(out, "max", acc->max_values, acc->max_values_count)) {
        tg3json_value_free(out);
        return 0;
    }
    if (acc->buffer_view >= 0 && !tg3__json_set_int(out, "bufferView", acc->buffer_view)) {
        tg3json_value_free(out);
        return 0;
    }
    if (acc->sparse.is_sparse) {
        tg3json_value sparse, indices, values;
        tg3json_value_init_object(&sparse);
        tg3json_value_init_object(&indices);
        tg3json_value_init_object(&values);
        if (!tg3__json_set_int(&sparse, "count", acc->sparse.count) ||
            !tg3__json_set_int(&indices, "bufferView", acc->sparse.indices.buffer_view) ||
            !tg3__json_set_int(&indices, "componentType", acc->sparse.indices.component_type) ||
            !tg3__serialize_uint64(&indices, "byteOffset", acc->sparse.indices.byte_offset, 0, wd) ||
            !tg3__serialize_extras_ext(&indices, &acc->sparse.indices.ext) ||
            !tg3__json_set_take(&sparse, "indices", &indices) ||
            !tg3__json_set_int(&values, "bufferView", acc->sparse.values.buffer_view) ||
            !tg3__serialize_uint64(&values, "byteOffset", acc->sparse.values.byte_offset, 0, wd) ||
            !tg3__serialize_extras_ext(&values, &acc->sparse.values.ext) ||
            !tg3__json_set_take(&sparse, "values", &values) ||
            !tg3__serialize_extras_ext(&sparse, &acc->sparse.ext) ||
            !tg3__json_set_take(out, "sparse", &sparse)) {
            tg3json_value_free(&indices);
            tg3json_value_free(&values);
            tg3json_value_free(&sparse);
            tg3json_value_free(out);
            return 0;
        }
    }
    if (!tg3__serialize_extras_ext(out, &acc->ext)) {
        tg3json_value_free(out);
        return 0;
    }
    return 1;
}

static int tg3__serialize_image(const tg3_image *img, int wd, int embed, tg3json_value *out) {
    (void)wd;
    (void)embed;
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", img->name) ||
        !tg3__serialize_str(out, "uri", img->uri) ||
        !tg3__serialize_str(out, "mimeType", img->mime_type) ||
        !tg3__serialize_extras_ext(out, &img->ext)) {
        tg3json_value_free(out);
        return 0;
    }
    if (img->buffer_view >= 0 && !tg3__json_set_int(out, "bufferView", img->buffer_view)) {
        tg3json_value_free(out);
        return 0;
    }
    return 1;
}

static int tg3__serialize_sampler(const tg3_sampler *s, int wd, tg3json_value *out) {
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", s->name) ||
        !tg3__serialize_int(out, "wrapS", s->wrap_s, TG3_TEXTURE_WRAP_REPEAT, wd) ||
        !tg3__serialize_int(out, "wrapT", s->wrap_t, TG3_TEXTURE_WRAP_REPEAT, wd) ||
        !tg3__serialize_extras_ext(out, &s->ext)) {
        tg3json_value_free(out);
        return 0;
    }
    if (s->min_filter >= 0 && !tg3__json_set_int(out, "minFilter", s->min_filter)) {
        tg3json_value_free(out);
        return 0;
    }
    if (s->mag_filter >= 0 && !tg3__json_set_int(out, "magFilter", s->mag_filter)) {
        tg3json_value_free(out);
        return 0;
    }
    return 1;
}

static int tg3__serialize_texture(const tg3_texture *t, int wd, tg3json_value *out) {
    (void)wd;
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", t->name) ||
        !tg3__serialize_extras_ext(out, &t->ext)) {
        tg3json_value_free(out);
        return 0;
    }
    if (t->sampler >= 0 && !tg3__json_set_int(out, "sampler", t->sampler)) {
        tg3json_value_free(out);
        return 0;
    }
    if (t->source >= 0 && !tg3__json_set_int(out, "source", t->source)) {
        tg3json_value_free(out);
        return 0;
    }
    return 1;
}

static int tg3__serialize_material(const tg3_material *m, int wd, tg3json_value *out) {
    tg3json_value pbr;
    int has_pbr = 0;
    const tg3_pbr_metallic_roughness *p = &m->pbr_metallic_roughness;
    tg3json_value_init_object(out);
    tg3json_value_init_object(&pbr);
    if (!tg3__serialize_str(out, "name", m->name)) goto fail;
    if (p->base_color_factor[0] != 1.0 || p->base_color_factor[1] != 1.0 ||
        p->base_color_factor[2] != 1.0 || p->base_color_factor[3] != 1.0 || wd) {
        if (!tg3__serialize_double_array(&pbr, "baseColorFactor", p->base_color_factor, 4)) goto fail;
        has_pbr = 1;
    }
    if (!tg3__serialize_double(&pbr, "metallicFactor", p->metallic_factor, 1.0, wd)) goto fail;
    if (fabs(p->metallic_factor - 1.0) > 1e-12 || wd) has_pbr = 1;
    if (!tg3__serialize_double(&pbr, "roughnessFactor", p->roughness_factor, 1.0, wd)) goto fail;
    if (fabs(p->roughness_factor - 1.0) > 1e-12 || wd) has_pbr = 1;
    if (p->base_color_texture.index >= 0) {
        if (!tg3__serialize_texture_info(&pbr, "baseColorTexture", &p->base_color_texture, wd)) goto fail;
        has_pbr = 1;
    }
    if (p->metallic_roughness_texture.index >= 0) {
        if (!tg3__serialize_texture_info(&pbr, "metallicRoughnessTexture", &p->metallic_roughness_texture, wd)) goto fail;
        has_pbr = 1;
    }
    if (!tg3__serialize_extras_ext(&pbr, &p->ext)) goto fail;
    if ((has_pbr || wd) && !tg3__json_set_take(out, "pbrMetallicRoughness", &pbr)) goto fail;
    if (!tg3__serialize_normal_texture_info(out, "normalTexture", &m->normal_texture, wd) ||
        !tg3__serialize_occlusion_texture_info(out, "occlusionTexture", &m->occlusion_texture, wd) ||
        !tg3__serialize_texture_info(out, "emissiveTexture", &m->emissive_texture, wd)) goto fail;
    if ((m->emissive_factor[0] != 0.0 || m->emissive_factor[1] != 0.0 ||
         m->emissive_factor[2] != 0.0 || wd) &&
        !tg3__serialize_double_array(out, "emissiveFactor", m->emissive_factor, 3)) goto fail;
    if (m->alpha_mode.data && !tg3_str_equals_cstr(m->alpha_mode, "OPAQUE") &&
        !tg3__serialize_str(out, "alphaMode", m->alpha_mode)) goto fail;
    if (!tg3__serialize_double(out, "alphaCutoff", m->alpha_cutoff, 0.5, wd) ||
        !tg3__serialize_bool(out, "doubleSided", m->double_sided, 0, wd) ||
        !tg3__serialize_extras_ext(out, &m->ext)) goto fail;
    return 1;
fail:
    tg3json_value_free(&pbr);
    tg3json_value_free(out);
    return 0;
}

static int tg3__serialize_primitive(const tg3_primitive *p, int wd, tg3json_value *out) {
    tg3json_value attrs, targets;
    uint32_t i;
    tg3json_value_init_object(out);
    if (p->attributes && p->attributes_count > 0) {
        tg3json_value_init_object(&attrs);
        for (i = 0; i < p->attributes_count; ++i) {
            tg3json_value item;
            tg3json_value_init_int(&item, p->attributes[i].value);
            if (!tg3json_object_set_take_n(&attrs,
                                           p->attributes[i].key.data ? p->attributes[i].key.data : "",
                                           p->attributes[i].key.data ? p->attributes[i].key.len : 0,
                                           &item)) {
                tg3json_value_free(&item);
                tg3json_value_free(&attrs);
                goto fail;
            }
        }
        if (!tg3__json_set_take(out, "attributes", &attrs)) {
            tg3json_value_free(&attrs);
            goto fail;
        }
    }
    if (p->indices >= 0 && !tg3__json_set_int(out, "indices", p->indices)) goto fail;
    if (p->material >= 0 && !tg3__json_set_int(out, "material", p->material)) goto fail;
    if (!tg3__serialize_int(out, "mode", p->mode, TG3_MODE_TRIANGLES, wd)) goto fail;
    if (p->targets && p->targets_count > 0) {
        uint32_t t;
        tg3json_value_init_array(&targets);
        for (t = 0; t < p->targets_count; ++t) {
            tg3json_value tgt;
            uint32_t acount = p->target_attribute_counts ? p->target_attribute_counts[t] : 0;
            tg3json_value_init_object(&tgt);
            for (i = 0; i < acount; ++i) {
                tg3json_value item;
                tg3json_value_init_int(&item, p->targets[t][i].value);
                if (!tg3json_object_set_take_n(&tgt,
                                               p->targets[t][i].key.data ? p->targets[t][i].key.data : "",
                                               p->targets[t][i].key.data ? p->targets[t][i].key.len : 0,
                                               &item)) {
                    tg3json_value_free(&item);
                    tg3json_value_free(&tgt);
                    tg3json_value_free(&targets);
                    goto fail;
                }
            }
            if (!tg3__json_push_take(&targets, &tgt)) {
                tg3json_value_free(&tgt);
                tg3json_value_free(&targets);
                goto fail;
            }
        }
        if (!tg3__json_set_take(out, "targets", &targets)) {
            tg3json_value_free(&targets);
            goto fail;
        }
    }
    if (!tg3__serialize_extras_ext(out, &p->ext)) goto fail;
    return 1;
fail:
    tg3json_value_free(out);
    return 0;
}

static int tg3__serialize_mesh(const tg3_mesh *m, int wd, tg3json_value *out) {
    uint32_t i;
    tg3json_value prims;
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", m->name)) goto fail;
    if (m->primitives && m->primitives_count > 0) {
        tg3json_value_init_array(&prims);
        for (i = 0; i < m->primitives_count; ++i) {
            tg3json_value prim;
            if (!tg3__serialize_primitive(&m->primitives[i], wd, &prim) ||
                !tg3__json_push_take(&prims, &prim)) {
                tg3json_value_free(&prim);
                tg3json_value_free(&prims);
                goto fail;
            }
        }
        if (!tg3__json_set_take(out, "primitives", &prims)) {
            tg3json_value_free(&prims);
            goto fail;
        }
    }
    if (!tg3__serialize_double_array(out, "weights", m->weights, m->weights_count) ||
        !tg3__serialize_extras_ext(out, &m->ext)) goto fail;
    return 1;
fail:
    tg3json_value_free(out);
    return 0;
}

static int tg3__serialize_node(const tg3_node *n, int wd, tg3json_value *out) {
    tg3json_value *exts = NULL;
    tg3json_value tmp;
    int has_t, has_r, has_s;
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", n->name) ||
        !tg3__serialize_int_array(out, "children", n->children, n->children_count)) goto fail;
    if (n->camera >= 0 && !tg3__json_set_int(out, "camera", n->camera)) goto fail;
    if (n->skin >= 0 && !tg3__json_set_int(out, "skin", n->skin)) goto fail;
    if (n->mesh >= 0 && !tg3__json_set_int(out, "mesh", n->mesh)) goto fail;
    if (n->has_matrix) {
        if (!tg3__serialize_double_array(out, "matrix", n->matrix, 16)) goto fail;
    } else {
        has_t = (n->translation[0] != 0.0 || n->translation[1] != 0.0 || n->translation[2] != 0.0);
        has_r = (n->rotation[0] != 0.0 || n->rotation[1] != 0.0 || n->rotation[2] != 0.0 || n->rotation[3] != 1.0);
        has_s = (n->scale[0] != 1.0 || n->scale[1] != 1.0 || n->scale[2] != 1.0);
        if ((has_t || wd) && !tg3__serialize_double_array(out, "translation", n->translation, 3)) goto fail;
        if ((has_r || wd) && !tg3__serialize_double_array(out, "rotation", n->rotation, 4)) goto fail;
        if ((has_s || wd) && !tg3__serialize_double_array(out, "scale", n->scale, 3)) goto fail;
    }
    if (!tg3__serialize_double_array(out, "weights", n->weights, n->weights_count) ||
        !tg3__serialize_extras_ext(out, &n->ext)) goto fail;
    if (n->light >= 0 || n->emitter >= 0 || (n->lods && n->lods_count > 0)) {
        if (!tg3__json_ensure_object(out, "extensions", &exts)) goto fail;
        if (n->light >= 0) {
            tg3json_value_init_object(&tmp);
            if (!tg3__json_set_int(&tmp, "light", n->light) ||
                !tg3__json_set_take(exts, "KHR_lights_punctual", &tmp)) {
                tg3json_value_free(&tmp);
                goto fail;
            }
        }
        if (n->emitter >= 0) {
            tg3json_value_init_object(&tmp);
            if (!tg3__json_set_int(&tmp, "emitter", n->emitter) ||
                !tg3__json_set_take(exts, "KHR_audio", &tmp)) {
                tg3json_value_free(&tmp);
                goto fail;
            }
        }
        if (n->lods && n->lods_count > 0) {
            tg3json_value_init_object(&tmp);
            if (!tg3__serialize_int_array(&tmp, "ids", n->lods, n->lods_count) ||
                !tg3__json_set_take(exts, "MSFT_lod", &tmp)) {
                tg3json_value_free(&tmp);
                goto fail;
            }
        }
    }
    return 1;
fail:
    tg3json_value_free(out);
    return 0;
}

static int tg3__serialize_skin(const tg3_skin *s, int wd, tg3json_value *out) {
    (void)wd;
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", s->name) ||
        !tg3__serialize_int_array(out, "joints", s->joints, s->joints_count) ||
        !tg3__serialize_extras_ext(out, &s->ext)) {
        tg3json_value_free(out);
        return 0;
    }
    if (s->inverse_bind_matrices >= 0 &&
        !tg3__json_set_int(out, "inverseBindMatrices", s->inverse_bind_matrices)) {
        tg3json_value_free(out);
        return 0;
    }
    if (s->skeleton >= 0 && !tg3__json_set_int(out, "skeleton", s->skeleton)) {
        tg3json_value_free(out);
        return 0;
    }
    return 1;
}

static int tg3__serialize_animation(const tg3_animation *a, int wd, tg3json_value *out) {
    uint32_t i;
    tg3json_value channels, samplers;
    (void)wd;
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", a->name)) goto fail;
    if (a->channels && a->channels_count > 0) {
        tg3json_value_init_array(&channels);
        for (i = 0; i < a->channels_count; ++i) {
            tg3json_value ch, tgt;
            tg3json_value_init_object(&ch);
            tg3json_value_init_object(&tgt);
            if (a->channels[i].target.node >= 0 &&
                !tg3__json_set_int(&tgt, "node", a->channels[i].target.node)) {
                tg3json_value_free(&tgt);
                tg3json_value_free(&ch);
                tg3json_value_free(&channels);
                goto fail;
            }
            if (!tg3__json_set_int(&ch, "sampler", a->channels[i].sampler) ||
                !tg3__serialize_str(&tgt, "path", a->channels[i].target.path) ||
                !tg3__serialize_extras_ext(&tgt, &a->channels[i].target.ext) ||
                !tg3__json_set_take(&ch, "target", &tgt) ||
                !tg3__serialize_extras_ext(&ch, &a->channels[i].ext) ||
                !tg3__json_push_take(&channels, &ch)) {
                tg3json_value_free(&tgt);
                tg3json_value_free(&ch);
                tg3json_value_free(&channels);
                goto fail;
            }
        }
        if (!tg3__json_set_take(out, "channels", &channels)) {
            tg3json_value_free(&channels);
            goto fail;
        }
    }
    if (a->samplers && a->samplers_count > 0) {
        tg3json_value_init_array(&samplers);
        for (i = 0; i < a->samplers_count; ++i) {
            tg3json_value s;
            tg3json_value_init_object(&s);
            if (!tg3__json_set_int(&s, "input", a->samplers[i].input) ||
                !tg3__json_set_int(&s, "output", a->samplers[i].output) ||
                !tg3__serialize_str(&s, "interpolation", a->samplers[i].interpolation) ||
                !tg3__serialize_extras_ext(&s, &a->samplers[i].ext) ||
                !tg3__json_push_take(&samplers, &s)) {
                tg3json_value_free(&s);
                tg3json_value_free(&samplers);
                goto fail;
            }
        }
        if (!tg3__json_set_take(out, "samplers", &samplers)) {
            tg3json_value_free(&samplers);
            goto fail;
        }
    }
    if (!tg3__serialize_extras_ext(out, &a->ext)) goto fail;
    return 1;
fail:
    tg3json_value_free(out);
    return 0;
}

static int tg3__serialize_camera(const tg3_camera *c, int wd, tg3json_value *out) {
    tg3json_value tmp;
    (void)wd;
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", c->name) ||
        !tg3__serialize_str(out, "type", c->type)) goto fail;
    if (c->type.data && tg3_str_equals_cstr(c->type, "perspective")) {
        tg3json_value_init_object(&tmp);
        if ((c->perspective.aspect_ratio > 0.0 &&
             !tg3__json_set_real(&tmp, "aspectRatio", c->perspective.aspect_ratio)) ||
            !tg3__json_set_real(&tmp, "yfov", c->perspective.yfov) ||
            (c->perspective.zfar > 0.0 &&
             !tg3__json_set_real(&tmp, "zfar", c->perspective.zfar)) ||
            !tg3__json_set_real(&tmp, "znear", c->perspective.znear) ||
            !tg3__serialize_extras_ext(&tmp, &c->perspective.ext) ||
            !tg3__json_set_take(out, "perspective", &tmp)) {
            tg3json_value_free(&tmp);
            goto fail;
        }
    } else if (c->type.data && tg3_str_equals_cstr(c->type, "orthographic")) {
        tg3json_value_init_object(&tmp);
        if (!tg3__json_set_real(&tmp, "xmag", c->orthographic.xmag) ||
            !tg3__json_set_real(&tmp, "ymag", c->orthographic.ymag) ||
            !tg3__json_set_real(&tmp, "zfar", c->orthographic.zfar) ||
            !tg3__json_set_real(&tmp, "znear", c->orthographic.znear) ||
            !tg3__serialize_extras_ext(&tmp, &c->orthographic.ext) ||
            !tg3__json_set_take(out, "orthographic", &tmp)) {
            tg3json_value_free(&tmp);
            goto fail;
        }
    }
    if (!tg3__serialize_extras_ext(out, &c->ext)) goto fail;
    return 1;
fail:
    tg3json_value_free(out);
    return 0;
}

static int tg3__serialize_scene(const tg3_scene *s, int wd, tg3json_value *out) {
    (void)wd;
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", s->name) ||
        !tg3__serialize_int_array(out, "nodes", s->nodes, s->nodes_count) ||
        !tg3__serialize_extras_ext(out, &s->ext)) {
        tg3json_value_free(out);
        return 0;
    }
    return 1;
}

static int tg3__serialize_light(const tg3_light *l, int wd, tg3json_value *out) {
    tg3json_value spot;
    tg3json_value_init_object(out);
    if (!tg3__serialize_str(out, "name", l->name) ||
        !tg3__serialize_str(out, "type", l->type) ||
        !tg3__serialize_double(out, "intensity", l->intensity, 1.0, wd) ||
        !tg3__serialize_double(out, "range", l->range, 0.0, wd)) goto fail;
    if ((l->color[0] != 1.0 || l->color[1] != 1.0 || l->color[2] != 1.0 || wd) &&
        !tg3__serialize_double_array(out, "color", l->color, 3)) goto fail;
    if (l->type.data && tg3_str_equals_cstr(l->type, "spot")) {
        tg3json_value_init_object(&spot);
        if (!tg3__serialize_double(&spot, "innerConeAngle", l->spot.inner_cone_angle, 0.0, wd) ||
            !tg3__serialize_double(&spot, "outerConeAngle", l->spot.outer_cone_angle, 0.7853981634, wd) ||
            !tg3__serialize_extras_ext(&spot, &l->spot.ext) ||
            !tg3__json_set_take(out, "spot", &spot)) {
            tg3json_value_free(&spot);
            goto fail;
        }
    }
    if (!tg3__serialize_extras_ext(out, &l->ext)) goto fail;
    return 1;
fail:
    tg3json_value_free(out);
    return 0;
}

static int tg3__serialize_model(const tg3_model *model, int wd,
                                int embed_images, int embed_buffers,
                                tg3json_value *out) {
    tg3json_value asset, arr, item, lights_ext, lights, *exts;
    uint32_t i;
    tg3json_value_init_object(out);
    if (!tg3__serialize_asset(&model->asset, wd, &asset) ||
        !tg3__json_set_take(out, "asset", &asset)) goto fail;
    if (model->default_scene >= 0 && !tg3__json_set_int(out, "scene", model->default_scene)) goto fail;
    if (!tg3__serialize_string_array(out, "extensionsUsed", model->extensions_used, model->extensions_used_count) ||
        !tg3__serialize_string_array(out, "extensionsRequired", model->extensions_required, model->extensions_required_count)) goto fail;

#define TG3__SERIALIZE_ARRAY_FIELD(json_key, arr_ptr, arr_count, fn, ...) \
    do { \
        if ((arr_ptr) && (arr_count) > 0) { \
            tg3json_value_init_array(&arr); \
            for (i = 0; i < (arr_count); ++i) { \
                tg3json_value_init_null(&item); \
                if (!fn(&(arr_ptr)[i], __VA_ARGS__, &item) || !tg3__json_push_take(&arr, &item)) { \
                    tg3json_value_free(&item); \
                    tg3json_value_free(&arr); \
                    goto fail; \
                } \
            } \
            if (!tg3__json_set_take(out, json_key, &arr)) { \
                tg3json_value_free(&arr); \
                goto fail; \
            } \
        } \
    } while (0)

    TG3__SERIALIZE_ARRAY_FIELD("buffers", model->buffers, model->buffers_count, tg3__serialize_buffer, wd, embed_buffers);
    TG3__SERIALIZE_ARRAY_FIELD("bufferViews", model->buffer_views, model->buffer_views_count, tg3__serialize_buffer_view, wd);
    TG3__SERIALIZE_ARRAY_FIELD("accessors", model->accessors, model->accessors_count, tg3__serialize_accessor, wd);
    TG3__SERIALIZE_ARRAY_FIELD("meshes", model->meshes, model->meshes_count, tg3__serialize_mesh, wd);
    TG3__SERIALIZE_ARRAY_FIELD("nodes", model->nodes, model->nodes_count, tg3__serialize_node, wd);
    TG3__SERIALIZE_ARRAY_FIELD("materials", model->materials, model->materials_count, tg3__serialize_material, wd);
    TG3__SERIALIZE_ARRAY_FIELD("textures", model->textures, model->textures_count, tg3__serialize_texture, wd);
    TG3__SERIALIZE_ARRAY_FIELD("samplers", model->samplers, model->samplers_count, tg3__serialize_sampler, wd);
    TG3__SERIALIZE_ARRAY_FIELD("images", model->images, model->images_count, tg3__serialize_image, wd, embed_images);
    TG3__SERIALIZE_ARRAY_FIELD("skins", model->skins, model->skins_count, tg3__serialize_skin, wd);
    TG3__SERIALIZE_ARRAY_FIELD("animations", model->animations, model->animations_count, tg3__serialize_animation, wd);
    TG3__SERIALIZE_ARRAY_FIELD("cameras", model->cameras, model->cameras_count, tg3__serialize_camera, wd);
    TG3__SERIALIZE_ARRAY_FIELD("scenes", model->scenes, model->scenes_count, tg3__serialize_scene, wd);

#undef TG3__SERIALIZE_ARRAY_FIELD

    if (model->lights && model->lights_count > 0) {
        tg3json_value_init_object(&lights_ext);
        tg3json_value_init_array(&lights);
        for (i = 0; i < model->lights_count; ++i) {
            tg3json_value_init_null(&item);
            if (!tg3__serialize_light(&model->lights[i], wd, &item) ||
                !tg3__json_push_take(&lights, &item)) {
                tg3json_value_free(&item);
                tg3json_value_free(&lights);
                tg3json_value_free(&lights_ext);
                goto fail;
            }
        }
        if (!tg3__json_set_take(&lights_ext, "lights", &lights) ||
            !tg3__json_ensure_object(out, "extensions", &exts) ||
            !tg3__json_set_take(exts, "KHR_lights_punctual", &lights_ext)) {
            tg3json_value_free(&lights);
            tg3json_value_free(&lights_ext);
            goto fail;
        }
    }
    if (!tg3__serialize_extras_ext(out, &model->ext)) goto fail;
    return 1;
fail:
    tg3json_value_free(out);
    return 0;
}

TINYGLTF3_API tg3_error_code tg3_write_to_memory(const tg3_model *model, tg3_error_stack *errors,
                                                 uint8_t **out_data, uint64_t *out_size,
                                                 const tg3_write_options *options) {
    tg3_write_options default_opts;
    tg3json_value root;
    char *json_str;
    size_t json_len;
    if (out_data) *out_data = NULL;
    if (out_size) *out_size = 0;
    if (!model || !out_data || !out_size) return TG3_ERR_WRITE_FAILED;
    if (!options) {
        tg3_write_options_init(&default_opts);
        options = &default_opts;
    }
    if (!tg3__serialize_model(model, options->serialize_defaults,
                              options->embed_images, options->embed_buffers, &root)) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                        "Failed to serialize model to JSON tree", NULL, -1);
        return TG3_ERR_OUT_OF_MEMORY;
    }
    json_str = options->pretty_print
        ? tg3json_stringify_pretty(&root, 2, &json_len)
        : tg3json_stringify(&root, &json_len);
    tg3json_value_free(&root);
    if (!json_str) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                        "Failed to stringify JSON output", NULL, -1);
        return TG3_ERR_OUT_OF_MEMORY;
    }
    if (options->write_binary) {
        uint32_t json_padded = ((uint32_t)json_len + 3u) & ~3u;
        const uint8_t *bin_data = NULL;
        uint64_t bin_len = 0;
        uint32_t bin_padded;
        uint32_t total;
        uint8_t *glb;
        if (model->buffers_count > 0 && model->buffers && model->buffers[0].data.data) {
            bin_data = model->buffers[0].data.data;
            bin_len = model->buffers[0].data.count;
        }
        bin_padded = ((uint32_t)bin_len + 3u) & ~3u;
        total = 12u + 8u + json_padded + ((bin_data && bin_len > 0) ? (8u + bin_padded) : 0u);
        glb = (uint8_t *)malloc(total);
        if (!glb) {
            free(json_str);
            tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                            "OOM allocating GLB output", NULL, -1);
            return TG3_ERR_OUT_OF_MEMORY;
        }
        memcpy(glb, "glTF", 4);
        {
            uint32_t version = 2u;
            uint32_t json_type = 0x4E4F534Au;
            memcpy(glb + 4, &version, 4);
            memcpy(glb + 8, &total, 4);
            memcpy(glb + 12, &json_padded, 4);
            memcpy(glb + 16, &json_type, 4);
            memcpy(glb + 20, json_str, json_len);
        }
        while ((uint32_t)json_len < json_padded) {
            glb[20u + json_len] = ' ';
            ++json_len;
        }
        if (bin_data && bin_len > 0) {
            uint32_t bin_off = 20u + json_padded;
            uint32_t bin_type = 0x004E4942u;
            uint32_t i;
            memcpy(glb + bin_off, &bin_padded, 4);
            memcpy(glb + bin_off + 4u, &bin_type, 4);
            memcpy(glb + bin_off + 8u, bin_data, (size_t)bin_len);
            for (i = (uint32_t)bin_len; i < bin_padded; ++i) glb[bin_off + 8u + i] = 0;
        }
        free(json_str);
        *out_data = glb;
        *out_size = total;
        return TG3_OK;
    } else {
        uint8_t *data = (uint8_t *)malloc(json_len);
        if (!data) {
            free(json_str);
            tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                            "OOM allocating JSON output", NULL, -1);
            return TG3_ERR_OUT_OF_MEMORY;
        }
        memcpy(data, json_str, json_len);
        free(json_str);
        *out_data = data;
        *out_size = (uint64_t)json_len;
        return TG3_OK;
    }
}

TINYGLTF3_API tg3_error_code tg3_write_to_file(const tg3_model *model, tg3_error_stack *errors,
                                               const char *filename, uint32_t filename_len,
                                               const tg3_write_options *options) {
    tg3_write_options opts;
    uint8_t *data = NULL;
    uint64_t size = 0;
    tg3_error_code err;
    int32_t ok;
    if (options) opts = *options;
    else tg3_write_options_init(&opts);
#ifdef TINYGLTF3_ENABLE_FS
    tg3__set_default_fs(&opts.fs);
#endif
    if (!opts.fs.write_file) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_FS_NOT_AVAILABLE,
                        "No filesystem write callback", NULL, -1);
        return TG3_ERR_FS_NOT_AVAILABLE;
    }
    err = tg3_write_to_memory(model, errors, &data, &size, &opts);
    if (err != TG3_OK) return err;
    ok = opts.fs.write_file(filename, filename_len, data, size, opts.fs.user_data);
    free(data);
    if (!ok) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_FILE_WRITE,
                        "Failed to write file", NULL, -1);
        return TG3_ERR_FILE_WRITE;
    }
    return TG3_OK;
}

TINYGLTF3_API void tg3_write_free(uint8_t *data, const tg3_write_options *options) {
    (void)options;
    free(data);
}

TINYGLTF3_API tg3_writer *tg3_writer_create(tg3_write_chunk_fn chunk_fn, void *user_data,
                                            const tg3_write_options *options) {
    tg3_writer *w = (tg3_writer *)malloc(sizeof(tg3_writer));
    if (!w) return NULL;
    memset(w, 0, sizeof(*w));
    w->chunk_fn = chunk_fn;
    w->user_data = user_data;
    if (options) w->options = *options;
    else tg3_write_options_init(&w->options);
    tg3json_value_init_object(&w->root);
    return w;
}

TINYGLTF3_API tg3_error_code tg3_writer_begin(tg3_writer *w, const tg3_asset *asset) {
    tg3json_value asset_json;
    if (!w || !asset) return TG3_ERR_WRITE_FAILED;
    tg3json_value_init_null(&asset_json);
    if (!tg3__serialize_asset(asset, w->options.serialize_defaults, &asset_json) ||
        !tg3__json_set_take(&w->root, "asset", &asset_json)) {
        tg3json_value_free(&asset_json);
        return TG3_ERR_OUT_OF_MEMORY;
    }
    w->begun = 1;
    return TG3_OK;
}

static tg3_error_code tg3__writer_add_item(tg3_writer *w, const char *json_key, const tg3json_value *item) {
    tg3json_value *arr = NULL;
    if (!w || !w->begun) return TG3_ERR_WRITE_FAILED;
    if (!tg3__json_ensure_array(&w->root, json_key, &arr) ||
        !tg3json_array_append_copy(arr, item)) {
        return TG3_ERR_OUT_OF_MEMORY;
    }
    return TG3_OK;
}

#define TG3__WRITER_ADD_IMPL(name, Type, json_key, serialize_fn, ...) \
    TINYGLTF3_API tg3_error_code tg3_writer_add_##name(tg3_writer *w, const Type *item) { \
        tg3json_value tmp; \
        if (!item) return TG3_ERR_WRITE_FAILED; \
        if (!serialize_fn(item, w->options.serialize_defaults, __VA_ARGS__, &tmp)) return TG3_ERR_OUT_OF_MEMORY; \
        { tg3_error_code err = tg3__writer_add_item(w, json_key, &tmp); tg3json_value_free(&tmp); return err; } \
    }

#define TG3__WRITER_ADD_SIMPLE(name, Type, json_key, serialize_fn) \
    TINYGLTF3_API tg3_error_code tg3_writer_add_##name(tg3_writer *w, const Type *item) { \
        tg3json_value tmp; \
        if (!item) return TG3_ERR_WRITE_FAILED; \
        if (!serialize_fn(item, w->options.serialize_defaults, &tmp)) return TG3_ERR_OUT_OF_MEMORY; \
        { tg3_error_code err = tg3__writer_add_item(w, json_key, &tmp); tg3json_value_free(&tmp); return err; } \
    }

TG3__WRITER_ADD_IMPL(buffer, tg3_buffer, "buffers", tg3__serialize_buffer, w->options.embed_buffers)
TG3__WRITER_ADD_SIMPLE(buffer_view, tg3_buffer_view, "bufferViews", tg3__serialize_buffer_view)
TG3__WRITER_ADD_SIMPLE(accessor, tg3_accessor, "accessors", tg3__serialize_accessor)
TG3__WRITER_ADD_SIMPLE(mesh, tg3_mesh, "meshes", tg3__serialize_mesh)
TG3__WRITER_ADD_SIMPLE(node, tg3_node, "nodes", tg3__serialize_node)
TG3__WRITER_ADD_SIMPLE(material, tg3_material, "materials", tg3__serialize_material)
TG3__WRITER_ADD_SIMPLE(texture, tg3_texture, "textures", tg3__serialize_texture)
TG3__WRITER_ADD_IMPL(image, tg3_image, "images", tg3__serialize_image, w->options.embed_images)
TG3__WRITER_ADD_SIMPLE(sampler, tg3_sampler, "samplers", tg3__serialize_sampler)
TG3__WRITER_ADD_SIMPLE(animation, tg3_animation, "animations", tg3__serialize_animation)
TG3__WRITER_ADD_SIMPLE(skin, tg3_skin, "skins", tg3__serialize_skin)
TG3__WRITER_ADD_SIMPLE(camera, tg3_camera, "cameras", tg3__serialize_camera)
TG3__WRITER_ADD_SIMPLE(scene, tg3_scene, "scenes", tg3__serialize_scene)
TG3__WRITER_ADD_SIMPLE(light, tg3_light, "lights", tg3__serialize_light)

#undef TG3__WRITER_ADD_IMPL
#undef TG3__WRITER_ADD_SIMPLE

TINYGLTF3_API tg3_error_code tg3_writer_end(tg3_writer *w) {
    char *json_str;
    size_t json_len;
    int32_t ok;
    if (!w || !w->begun || !w->chunk_fn) return TG3_ERR_WRITE_FAILED;
    json_str = w->options.pretty_print
        ? tg3json_stringify_pretty(&w->root, 2, &json_len)
        : tg3json_stringify(&w->root, &json_len);
    if (!json_str) return TG3_ERR_OUT_OF_MEMORY;
    ok = w->chunk_fn((const uint8_t *)json_str, (uint64_t)json_len, w->user_data);
    free(json_str);
    return ok ? TG3_OK : TG3_ERR_WRITE_FAILED;
}

TINYGLTF3_API void tg3_writer_destroy(tg3_writer *w) {
    if (!w) return;
    tg3json_value_free(&w->root);
    free(w);
}
