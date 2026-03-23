/*
 * tiny_gltf_v3.h - Header-only C glTF 2.0 loader and writer (v3)
 *
 * The MIT License (MIT)
 * Copyright (c) 2015 - Present Syoyo Fujita, Aurelien Chatelain and many
 * contributors.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Version: v3.0.0-alpha
 *
 * Ground-up C-centric API rewrite of tinygltf.
 * Uses tinygltf_json.h as the sole JSON backend.
 *
 * Key differences from v2:
 *   - Pure C POD structs (no STL containers in public API)
 *   - Arena-based memory management (single tg3_model_free() frees all)
 *   - Filesystem and image decoding OFF by default (opt-in)
 *   - Structured error reporting via tg3_error_stack
 *   - Streaming parse/write via callbacks
 *   - No RTTI, no exceptions required
 *   - C++20 coroutine facade (optional)
 */

#ifndef TINY_GLTF_V3_H_
#define TINY_GLTF_V3_H_

/* ======================================================================
 * Section 2: Configuration Macros
 * ====================================================================== */

/* Build mode: define in ONE C++ translation unit (.cpp) */
/* #define TINYGLTF3_IMPLEMENTATION */

/* Opt-in features (OFF by default) */
/* #define TINYGLTF3_ENABLE_FS */
/* #define TINYGLTF3_ENABLE_STB_IMAGE */
/* #define TINYGLTF3_ENABLE_STB_IMAGE_WRITE */

/* Opt-out */
/* #define TINYGLTF3_NO_IMAGE_DECODE */

/* C++20 coroutines (auto-detected, or force) */
/* #define TINYGLTF3_ENABLE_COROUTINES */

/* SIMD for JSON parsing (forwarded to tinygltf_json.h) */
/* #define TINYGLTF3_JSON_SIMD_SSE2 */
/* #define TINYGLTF3_JSON_SIMD_AVX2 */
/* #define TINYGLTF3_JSON_SIMD_NEON */

/* Memory limits */
#ifndef TINYGLTF3_MAX_MEMORY_BYTES
#define TINYGLTF3_MAX_MEMORY_BYTES (1ULL << 30) /* 1 GB */
#endif

#ifndef TINYGLTF3_MAX_NESTING_DEPTH
#define TINYGLTF3_MAX_NESTING_DEPTH 512
#endif

#ifndef TINYGLTF3_MAX_STRING_LENGTH
#define TINYGLTF3_MAX_STRING_LENGTH (64 * 1024 * 1024) /* 64 MB */
#endif

/* Linkage control */
#ifndef TINYGLTF3_API
#define TINYGLTF3_API
#endif

/* Assert override */
#ifndef TINYGLTF3_ASSERT
#include <assert.h>
#define TINYGLTF3_ASSERT(x) assert(x)
#endif

/* ======================================================================
 * Section 3: C Includes
 * ====================================================================== */

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/* ======================================================================
 * Section 4: Constants and Enums
 * ====================================================================== */

#ifdef __cplusplus
extern "C" {
#endif

/* Primitive modes */
#define TG3_MODE_POINTS         0
#define TG3_MODE_LINE           1
#define TG3_MODE_LINE_LOOP      2
#define TG3_MODE_LINE_STRIP     3
#define TG3_MODE_TRIANGLES      4
#define TG3_MODE_TRIANGLE_STRIP 5
#define TG3_MODE_TRIANGLE_FAN   6

/* Component types */
#define TG3_COMPONENT_TYPE_BYTE           5120
#define TG3_COMPONENT_TYPE_UNSIGNED_BYTE  5121
#define TG3_COMPONENT_TYPE_SHORT          5122
#define TG3_COMPONENT_TYPE_UNSIGNED_SHORT 5123
#define TG3_COMPONENT_TYPE_INT            5124
#define TG3_COMPONENT_TYPE_UNSIGNED_INT   5125
#define TG3_COMPONENT_TYPE_FLOAT          5126
#define TG3_COMPONENT_TYPE_DOUBLE         5130

/* Accessor types */
#define TG3_TYPE_VEC2   2
#define TG3_TYPE_VEC3   3
#define TG3_TYPE_VEC4   4
#define TG3_TYPE_MAT2   (32 + 2)
#define TG3_TYPE_MAT3   (32 + 3)
#define TG3_TYPE_MAT4   (32 + 4)
#define TG3_TYPE_SCALAR (64 + 1)
#define TG3_TYPE_VECTOR (64 + 4)
#define TG3_TYPE_MATRIX (64 + 16)

/* Texture filter */
#define TG3_TEXTURE_FILTER_NEAREST                9728
#define TG3_TEXTURE_FILTER_LINEAR                 9729
#define TG3_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST 9984
#define TG3_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST  9985
#define TG3_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR  9986
#define TG3_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR   9987

/* Texture wrap */
#define TG3_TEXTURE_WRAP_REPEAT          10497
#define TG3_TEXTURE_WRAP_CLAMP_TO_EDGE   33071
#define TG3_TEXTURE_WRAP_MIRRORED_REPEAT 33648

/* Image format */
#define TG3_IMAGE_FORMAT_JPEG 0
#define TG3_IMAGE_FORMAT_PNG  1
#define TG3_IMAGE_FORMAT_BMP  2
#define TG3_IMAGE_FORMAT_GIF  3

/* Texture format */
#define TG3_TEXTURE_FORMAT_ALPHA           6406
#define TG3_TEXTURE_FORMAT_RGB             6407
#define TG3_TEXTURE_FORMAT_RGBA            6408
#define TG3_TEXTURE_FORMAT_LUMINANCE       6409
#define TG3_TEXTURE_FORMAT_LUMINANCE_ALPHA 6410

/* Texture target / type */
#define TG3_TEXTURE_TARGET_TEXTURE2D    3553
#define TG3_TEXTURE_TYPE_UNSIGNED_BYTE  5121

/* Buffer targets */
#define TG3_TARGET_ARRAY_BUFFER         34962
#define TG3_TARGET_ELEMENT_ARRAY_BUFFER 34963

/* Sentinel for absent index */
#define TG3_INDEX_NONE (-1)

/* Section check flags */
#define TG3_NO_REQUIRE       0x00
#define TG3_REQUIRE_VERSION  0x01
#define TG3_REQUIRE_SCENE    0x02
#define TG3_REQUIRE_SCENES   0x04
#define TG3_REQUIRE_NODES    0x08
#define TG3_REQUIRE_ACCESSORS    0x10
#define TG3_REQUIRE_BUFFERS      0x20
#define TG3_REQUIRE_BUFFER_VIEWS 0x40
#define TG3_REQUIRE_ALL          0x7f

/* Parse strictness */
typedef enum tg3_strictness {
    TG3_PERMISSIVE = 0,
    TG3_STRICT     = 1
} tg3_strictness;

/* ======================================================================
 * Section 5: Foundation Types
 * ====================================================================== */

typedef struct tg3_str {
    const char *data;
    uint32_t    len;
} tg3_str;

typedef struct tg3_span_i32 {
    const int32_t *data;
    uint32_t       count;
} tg3_span_i32;

typedef struct tg3_span_f64 {
    const double *data;
    uint32_t      count;
} tg3_span_f64;

typedef struct tg3_span_u8 {
    const uint8_t *data;
    uint64_t       count;
} tg3_span_u8;

typedef struct tg3_str_int_pair {
    tg3_str  key;
    int32_t  value;
} tg3_str_int_pair;

/* ======================================================================
 * Section 6: Allocator Interface
 * ====================================================================== */

typedef struct tg3_allocator {
    void *(*alloc)(size_t size, void *user_data);
    void *(*realloc)(void *ptr, size_t old_size, size_t new_size, void *user_data);
    void  (*free)(void *ptr, size_t size, void *user_data);
    void  *user_data;
} tg3_allocator;

/* ======================================================================
 * Section 7: Error Reporting
 * ====================================================================== */

typedef enum tg3_severity {
    TG3_SEVERITY_INFO    = 0,
    TG3_SEVERITY_WARNING = 1,
    TG3_SEVERITY_ERROR   = 2
} tg3_severity;

typedef enum tg3_error_code {
    TG3_OK = 0,

    /* I/O errors: 1-9 */
    TG3_ERR_FILE_NOT_FOUND     = 1,
    TG3_ERR_FILE_READ          = 2,
    TG3_ERR_FILE_WRITE         = 3,
    TG3_ERR_FILE_TOO_LARGE     = 4,

    /* JSON errors: 10-19 */
    TG3_ERR_JSON_PARSE         = 10,
    TG3_ERR_JSON_TYPE_MISMATCH = 11,
    TG3_ERR_JSON_MISSING_FIELD = 12,
    TG3_ERR_JSON_INVALID_VALUE = 13,

    /* GLB errors: 20-29 */
    TG3_ERR_GLB_INVALID_MAGIC  = 20,
    TG3_ERR_GLB_INVALID_VERSION = 21,
    TG3_ERR_GLB_INVALID_HEADER = 22,
    TG3_ERR_GLB_CHUNK_ERROR    = 23,
    TG3_ERR_GLB_SIZE_MISMATCH  = 24,

    /* Schema / validation errors: 30-49 */
    TG3_ERR_MISSING_REQUIRED   = 30,
    TG3_ERR_INVALID_INDEX      = 31,
    TG3_ERR_INVALID_TYPE       = 32,
    TG3_ERR_INVALID_VALUE      = 33,
    TG3_ERR_INVALID_ACCESSOR   = 34,
    TG3_ERR_INVALID_BUFFER     = 35,
    TG3_ERR_INVALID_BUFFER_VIEW = 36,
    TG3_ERR_INVALID_IMAGE      = 37,
    TG3_ERR_INVALID_MATERIAL   = 38,
    TG3_ERR_INVALID_MESH       = 39,
    TG3_ERR_INVALID_NODE       = 40,
    TG3_ERR_INVALID_ANIMATION  = 41,
    TG3_ERR_INVALID_SKIN       = 42,
    TG3_ERR_INVALID_CAMERA     = 43,
    TG3_ERR_INVALID_SCENE      = 44,
    TG3_ERR_BUFFER_SIZE_MISMATCH = 45,

    /* Resource errors: 50-59 */
    TG3_ERR_OUT_OF_MEMORY      = 50,
    TG3_ERR_DATA_URI_DECODE    = 51,
    TG3_ERR_BASE64_DECODE      = 52,
    TG3_ERR_EXTERNAL_RESOURCE  = 53,
    TG3_ERR_IMAGE_DECODE       = 54,

    /* Callback errors: 60-69 */
    TG3_ERR_CALLBACK_FAILED    = 60,
    TG3_ERR_FS_NOT_AVAILABLE   = 61,

    /* Streaming errors: 70-79 */
    TG3_ERR_STREAM_ABORTED     = 70,

    /* Writer errors: 80-89 */
    TG3_ERR_WRITE_FAILED       = 80,
    TG3_ERR_SERIALIZE_FAILED   = 81
} tg3_error_code;

typedef struct tg3_error_entry {
    tg3_severity   severity;
    tg3_error_code code;
    const char    *message;     /* Arena-owned, null-terminated */
    const char    *json_path;   /* e.g. "/meshes/0/primitives/1" or NULL */
    int64_t        byte_offset; /* -1 if unknown */
} tg3_error_entry;

typedef struct tg3_error_stack {
    tg3_error_entry *entries;
    uint32_t         count;
    uint32_t         capacity;
    int32_t          has_error; /* 1 if any entry with severity == ERROR */
} tg3_error_stack;

/* Error stack query functions */
TINYGLTF3_API int32_t  tg3_errors_has_error(const tg3_error_stack *es);
TINYGLTF3_API uint32_t tg3_errors_count(const tg3_error_stack *es);
TINYGLTF3_API const tg3_error_entry *tg3_errors_get(const tg3_error_stack *es,
                                                     uint32_t index);

/* ======================================================================
 * Section 8: Generic Value Type (for extras/extensions)
 * ====================================================================== */

typedef enum tg3_value_type {
    TG3_VALUE_NULL   = 0,
    TG3_VALUE_BOOL   = 1,
    TG3_VALUE_INT    = 2,
    TG3_VALUE_REAL   = 3,
    TG3_VALUE_STRING = 4,
    TG3_VALUE_ARRAY  = 5,
    TG3_VALUE_BINARY = 6,
    TG3_VALUE_OBJECT = 7
} tg3_value_type;

typedef struct tg3_kv_pair tg3_kv_pair;

typedef struct tg3_value {
    tg3_value_type type;
    union {
        int32_t  bool_val;
        int64_t  int_val;
        double   real_val;
    };
    tg3_str                string_val;
    const struct tg3_value *array_data;
    uint32_t               array_count;
    const tg3_kv_pair      *object_data;
    uint32_t               object_count;
    tg3_span_u8            binary_val;
} tg3_value;

struct tg3_kv_pair {
    tg3_str   key;
    tg3_value value;
};

typedef struct tg3_extension {
    tg3_str   name;
    tg3_value value;
} tg3_extension;

typedef struct tg3_extras_ext {
    const tg3_value     *extras;           /* NULL if absent */
    const tg3_extension *extensions;       /* Array */
    uint32_t             extensions_count;
    tg3_str              extras_json;      /* Raw JSON if store_original_json */
    tg3_str              extensions_json;
} tg3_extras_ext;

/* ======================================================================
 * Section 9: Core POD Structs
 * ====================================================================== */

/* --- Asset --- */
typedef struct tg3_asset {
    tg3_str       version;    /* Required, e.g. "2.0" */
    tg3_str       generator;
    tg3_str       min_version;
    tg3_str       copyright;
    tg3_extras_ext ext;
} tg3_asset;

/* --- Buffer --- */
typedef struct tg3_buffer {
    tg3_str       name;
    tg3_span_u8   data;
    tg3_str       uri;
    tg3_extras_ext ext;
} tg3_buffer;

/* --- BufferView --- */
typedef struct tg3_buffer_view {
    tg3_str       name;
    int32_t       buffer;       /* Index, required */
    uint64_t      byte_offset;
    uint64_t      byte_length;  /* Required */
    uint32_t      byte_stride;  /* 0 = tightly packed */
    int32_t       target;       /* 0 = unspecified */
    int32_t       draco_decoded;
    tg3_extras_ext ext;
} tg3_buffer_view;

/* --- Accessor Sparse --- */
typedef struct tg3_accessor_sparse_indices {
    uint64_t  byte_offset;
    int32_t   buffer_view; /* Required */
    int32_t   component_type; /* Required */
    tg3_extras_ext ext;
} tg3_accessor_sparse_indices;

typedef struct tg3_accessor_sparse_values {
    int32_t   buffer_view; /* Required */
    uint64_t  byte_offset;
    tg3_extras_ext ext;
} tg3_accessor_sparse_values;

typedef struct tg3_accessor_sparse {
    int32_t  count;      /* Required if sparse */
    int32_t  is_sparse;  /* 0 or 1 */
    tg3_accessor_sparse_indices indices;
    tg3_accessor_sparse_values  values;
    tg3_extras_ext ext;
} tg3_accessor_sparse;

/* --- Accessor --- */
typedef struct tg3_accessor {
    tg3_str       name;
    int32_t       buffer_view;    /* -1 if absent */
    uint64_t      byte_offset;
    int32_t       normalized;     /* 0 or 1 */
    int32_t       component_type; /* Required */
    uint64_t      count;          /* Required */
    int32_t       type;           /* Required: TG3_TYPE_* */
    const double *min_values;
    uint32_t      min_values_count;
    const double *max_values;
    uint32_t      max_values_count;
    tg3_accessor_sparse sparse;
    tg3_extras_ext ext;
} tg3_accessor;

/* --- Image --- */
typedef struct tg3_image {
    tg3_str       name;
    int32_t       width;
    int32_t       height;
    int32_t       component;  /* Channels */
    int32_t       bits;       /* Bits per channel */
    int32_t       pixel_type; /* Component type */
    tg3_span_u8   image;      /* Decoded pixel data (or raw if as_is) */
    int32_t       buffer_view; /* -1 if absent */
    tg3_str       mime_type;
    tg3_str       uri;
    int32_t       as_is;
    tg3_extras_ext ext;
} tg3_image;

/* --- Sampler --- */
typedef struct tg3_sampler {
    tg3_str  name;
    int32_t  min_filter;  /* -1 = unspecified */
    int32_t  mag_filter;  /* -1 = unspecified */
    int32_t  wrap_s;      /* Default: TG3_TEXTURE_WRAP_REPEAT */
    int32_t  wrap_t;      /* Default: TG3_TEXTURE_WRAP_REPEAT */
    tg3_extras_ext ext;
} tg3_sampler;

/* --- Texture --- */
typedef struct tg3_texture {
    tg3_str  name;
    int32_t  sampler;  /* -1 if absent */
    int32_t  source;   /* -1 if absent */
    tg3_extras_ext ext;
} tg3_texture;

/* --- TextureInfo --- */
typedef struct tg3_texture_info {
    int32_t  index;     /* -1 if absent */
    int32_t  tex_coord; /* Default: 0 */
    tg3_extras_ext ext;
} tg3_texture_info;

/* --- NormalTextureInfo --- */
typedef struct tg3_normal_texture_info {
    int32_t  index;
    int32_t  tex_coord;
    double   scale;     /* Default: 1.0 */
    tg3_extras_ext ext;
} tg3_normal_texture_info;

/* --- OcclusionTextureInfo --- */
typedef struct tg3_occlusion_texture_info {
    int32_t  index;
    int32_t  tex_coord;
    double   strength;  /* Default: 1.0 */
    tg3_extras_ext ext;
} tg3_occlusion_texture_info;

/* --- PBR Metallic Roughness --- */
typedef struct tg3_pbr_metallic_roughness {
    double             base_color_factor[4]; /* Default: {1,1,1,1} */
    tg3_texture_info   base_color_texture;
    double             metallic_factor;      /* Default: 1.0 */
    double             roughness_factor;     /* Default: 1.0 */
    tg3_texture_info   metallic_roughness_texture;
    tg3_extras_ext     ext;
} tg3_pbr_metallic_roughness;

/* --- Material --- */
typedef struct tg3_material {
    tg3_str                     name;
    double                      emissive_factor[3]; /* Default: {0,0,0} */
    tg3_str                     alpha_mode;         /* "OPAQUE","MASK","BLEND" */
    double                      alpha_cutoff;       /* Default: 0.5 */
    int32_t                     double_sided;       /* 0 or 1 */
    const int32_t              *lods;
    uint32_t                    lods_count;
    tg3_pbr_metallic_roughness  pbr_metallic_roughness;
    tg3_normal_texture_info     normal_texture;
    tg3_occlusion_texture_info  occlusion_texture;
    tg3_texture_info            emissive_texture;
    tg3_extras_ext              ext;
} tg3_material;

/* --- Primitive --- */
typedef struct tg3_primitive {
    const tg3_str_int_pair *attributes;
    uint32_t                attributes_count;
    int32_t                 material;  /* -1 if absent */
    int32_t                 indices;   /* -1 if absent */
    int32_t                 mode;      /* -1 = default (TRIANGLES) */

    /* Morph targets: array of arrays of attribute pairs */
    const tg3_str_int_pair *const *targets;
    const uint32_t         *target_attribute_counts;
    uint32_t                targets_count;

    tg3_extras_ext ext;
} tg3_primitive;

/* --- Mesh --- */
typedef struct tg3_mesh {
    tg3_str              name;
    const tg3_primitive *primitives;
    uint32_t             primitives_count;
    const double        *weights;
    uint32_t             weights_count;
    tg3_extras_ext       ext;
} tg3_mesh;

/* --- Node --- */
typedef struct tg3_node {
    tg3_str       name;
    int32_t       camera;     /* -1 if absent */
    int32_t       skin;       /* -1 if absent */
    int32_t       mesh;       /* -1 if absent */
    int32_t       light;      /* -1 if absent (KHR_lights_punctual) */
    int32_t       emitter;    /* -1 if absent (KHR_audio) */

    const int32_t *lods;
    uint32_t       lods_count;
    const int32_t *children;
    uint32_t       children_count;

    double         rotation[4];     /* Default: {0,0,0,1} */
    double         scale[3];        /* Default: {1,1,1} */
    double         translation[3];  /* Default: {0,0,0} */
    double         matrix[16];      /* Identity if not set */
    int32_t        has_matrix;      /* 1 if matrix was specified */

    const double  *weights;
    uint32_t       weights_count;

    tg3_extras_ext ext;
} tg3_node;

/* --- Skin --- */
typedef struct tg3_skin {
    tg3_str        name;
    int32_t        inverse_bind_matrices; /* -1 if absent */
    int32_t        skeleton;              /* -1 if absent */
    const int32_t *joints;
    uint32_t       joints_count;
    tg3_extras_ext ext;
} tg3_skin;

/* --- Animation --- */
typedef struct tg3_animation_channel_target {
    int32_t  node;    /* -1 if absent */
    tg3_str  path;    /* "translation","rotation","scale","weights" */
    tg3_extras_ext ext;
} tg3_animation_channel_target;

typedef struct tg3_animation_channel {
    int32_t  sampler; /* Required */
    tg3_animation_channel_target target;
    tg3_extras_ext ext;
} tg3_animation_channel;

typedef struct tg3_animation_sampler {
    int32_t  input;          /* Required */
    int32_t  output;         /* Required */
    tg3_str  interpolation;  /* "LINEAR","STEP","CUBICSPLINE" */
    tg3_extras_ext ext;
} tg3_animation_sampler;

typedef struct tg3_animation {
    tg3_str                      name;
    const tg3_animation_channel *channels;
    uint32_t                     channels_count;
    const tg3_animation_sampler *samplers;
    uint32_t                     samplers_count;
    tg3_extras_ext               ext;
} tg3_animation;

/* --- Camera --- */
typedef struct tg3_perspective_camera {
    double aspect_ratio;
    double yfov;
    double zfar;  /* 0 = infinite */
    double znear;
    tg3_extras_ext ext;
} tg3_perspective_camera;

typedef struct tg3_orthographic_camera {
    double xmag;
    double ymag;
    double zfar;
    double znear;
    tg3_extras_ext ext;
} tg3_orthographic_camera;

typedef struct tg3_camera {
    tg3_str                  name;
    tg3_str                  type; /* "perspective" or "orthographic" */
    tg3_perspective_camera   perspective;
    tg3_orthographic_camera  orthographic;
    tg3_extras_ext           ext;
} tg3_camera;

/* --- Scene --- */
typedef struct tg3_scene {
    tg3_str        name;
    const int32_t *nodes;
    uint32_t       nodes_count;
    const int32_t *audio_emitters;
    uint32_t       audio_emitters_count;
    tg3_extras_ext ext;
} tg3_scene;

/* --- Light (KHR_lights_punctual) --- */
typedef struct tg3_spot_light {
    double inner_cone_angle; /* Default: 0 */
    double outer_cone_angle; /* Default: PI/4 */
    tg3_extras_ext ext;
} tg3_spot_light;

typedef struct tg3_light {
    tg3_str        name;
    double         color[3];    /* Default: {1,1,1} */
    double         intensity;   /* Default: 1.0 */
    tg3_str        type;        /* "directional","point","spot" */
    double         range;       /* Default: 0 (infinite) */
    tg3_spot_light spot;
    tg3_extras_ext ext;
} tg3_light;

/* --- Audio (KHR_audio) --- */
typedef struct tg3_audio_source {
    tg3_str        name;
    tg3_str        uri;
    int32_t        buffer_view; /* -1 if absent */
    tg3_str        mime_type;
    tg3_extras_ext ext;
} tg3_audio_source;

typedef struct tg3_positional_emitter {
    double cone_inner_angle;   /* Default: 2*PI */
    double cone_outer_angle;   /* Default: 2*PI */
    double cone_outer_gain;    /* Default: 0 */
    double max_distance;       /* Default: 100 */
    double ref_distance;       /* Default: 1 */
    double rolloff_factor;     /* Default: 1 */
    tg3_extras_ext ext;
} tg3_positional_emitter;

typedef struct tg3_audio_emitter {
    tg3_str              name;
    double               gain;           /* Default: 1.0 */
    int32_t              loop;           /* Default: 0 */
    int32_t              playing;        /* Default: 0 */
    tg3_str              type;           /* "positional" or "global" */
    tg3_str              distance_model; /* "linear","inverse","exponential" */
    tg3_positional_emitter positional;
    int32_t              source;         /* -1 if absent */
    tg3_extras_ext       ext;
} tg3_audio_emitter;

/* ======================================================================
 * Section 10: Model Container
 * ====================================================================== */

/* Opaque arena type */
struct tg3_arena;

typedef struct tg3_model {
    struct tg3_arena *arena_;  /* Internal, all memory owned here */

    const tg3_accessor      *accessors;      uint32_t accessors_count;
    const tg3_animation     *animations;     uint32_t animations_count;
    const tg3_buffer        *buffers;        uint32_t buffers_count;
    const tg3_buffer_view   *buffer_views;   uint32_t buffer_views_count;
    const tg3_material      *materials;      uint32_t materials_count;
    const tg3_mesh          *meshes;         uint32_t meshes_count;
    const tg3_node          *nodes;          uint32_t nodes_count;
    const tg3_texture       *textures;       uint32_t textures_count;
    const tg3_image         *images;         uint32_t images_count;
    const tg3_skin          *skins;          uint32_t skins_count;
    const tg3_sampler       *samplers;       uint32_t samplers_count;
    const tg3_camera        *cameras;        uint32_t cameras_count;
    const tg3_scene         *scenes;         uint32_t scenes_count;
    const tg3_light         *lights;         uint32_t lights_count;
    const tg3_audio_emitter *audio_emitters; uint32_t audio_emitters_count;
    const tg3_audio_source  *audio_sources;  uint32_t audio_sources_count;

    int32_t    default_scene;
    const tg3_str *extensions_used;      uint32_t extensions_used_count;
    const tg3_str *extensions_required;  uint32_t extensions_required_count;
    tg3_asset  asset;
    tg3_extras_ext ext;
} tg3_model;

/* ======================================================================
 * Section 11: Callback Typedefs
 * ====================================================================== */

/* --- Filesystem Callbacks --- */

typedef int32_t (*tg3_file_exists_fn)(const char *path, uint32_t path_len,
                                      void *user_data);

typedef int32_t (*tg3_read_file_fn)(uint8_t **out_data, uint64_t *out_size,
                                    const char *path, uint32_t path_len,
                                    void *user_data);

typedef void (*tg3_free_file_fn)(uint8_t *data, uint64_t size,
                                  void *user_data);

typedef int32_t (*tg3_write_file_fn)(const char *path, uint32_t path_len,
                                     const uint8_t *data, uint64_t size,
                                     void *user_data);

typedef int32_t (*tg3_resolve_path_fn)(char *out_path, uint32_t out_cap,
                                       uint32_t *out_len,
                                       const char *path, uint32_t path_len,
                                       void *user_data);

typedef int32_t (*tg3_get_file_size_fn)(uint64_t *out_size,
                                        const char *path, uint32_t path_len,
                                        void *user_data);

typedef struct tg3_fs_callbacks {
    tg3_file_exists_fn   file_exists;
    tg3_read_file_fn     read_file;
    tg3_free_file_fn     free_file;
    tg3_write_file_fn    write_file;
    tg3_resolve_path_fn  resolve_path;
    tg3_get_file_size_fn get_file_size;
    void                *user_data;
} tg3_fs_callbacks;

/* --- Image Callbacks --- */

typedef struct tg3_image_request {
    const uint8_t *data;
    uint64_t       data_size;
    int32_t        image_index;
    int32_t        req_width;
    int32_t        req_height;
    const char    *mime_type;
} tg3_image_request;

typedef struct tg3_image_result {
    uint8_t  *pixels;     /* Caller must allocate */
    int32_t   width;
    int32_t   height;
    int32_t   component;
    int32_t   bits;
    int32_t   pixel_type;
} tg3_image_result;

typedef int32_t (*tg3_load_image_fn)(tg3_image_result *result,
                                     const tg3_image_request *request,
                                     void *user_data);

typedef void (*tg3_free_image_fn)(uint8_t *pixels, void *user_data);

typedef struct tg3_image_callbacks {
    tg3_load_image_fn  load_image;
    tg3_free_image_fn  free_image;
    void              *user_data;
} tg3_image_callbacks;

/* --- URI Callbacks --- */

typedef int32_t (*tg3_uri_encode_fn)(char *out, uint32_t out_cap,
                                     uint32_t *out_len,
                                     const char *uri, uint32_t uri_len,
                                     const char *obj_type,
                                     void *user_data);

typedef int32_t (*tg3_uri_decode_fn)(char *out, uint32_t out_cap,
                                     uint32_t *out_len,
                                     const char *uri, uint32_t uri_len,
                                     void *user_data);

typedef struct tg3_uri_callbacks {
    tg3_uri_encode_fn encode;
    tg3_uri_decode_fn decode;
    void             *user_data;
} tg3_uri_callbacks;

/* --- Streaming Callbacks --- */

typedef enum tg3_stream_action {
    TG3_STREAM_CONTINUE = 0,
    TG3_STREAM_ABORT    = 1,
    TG3_STREAM_SKIP     = 2
} tg3_stream_action;

typedef tg3_stream_action (*tg3_on_asset_fn)(const tg3_asset *a, void *ud);
typedef tg3_stream_action (*tg3_on_buffer_fn)(const tg3_buffer *b, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_buffer_view_fn)(const tg3_buffer_view *bv, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_accessor_fn)(const tg3_accessor *a, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_mesh_fn)(const tg3_mesh *m, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_node_fn)(const tg3_node *n, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_material_fn)(const tg3_material *m, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_texture_fn)(const tg3_texture *t, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_image_fn)(const tg3_image *img, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_sampler_fn)(const tg3_sampler *s, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_animation_fn)(const tg3_animation *a, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_skin_fn)(const tg3_skin *s, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_camera_fn)(const tg3_camera *c, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_scene_fn)(const tg3_scene *s, int32_t idx, void *ud);
typedef tg3_stream_action (*tg3_on_light_fn)(const tg3_light *l, int32_t idx, void *ud);

typedef struct tg3_stream_callbacks {
    tg3_on_asset_fn       on_asset;
    tg3_on_buffer_fn      on_buffer;
    tg3_on_buffer_view_fn on_buffer_view;
    tg3_on_accessor_fn    on_accessor;
    tg3_on_mesh_fn        on_mesh;
    tg3_on_node_fn        on_node;
    tg3_on_material_fn    on_material;
    tg3_on_texture_fn     on_texture;
    tg3_on_image_fn       on_image;
    tg3_on_sampler_fn     on_sampler;
    tg3_on_animation_fn   on_animation;
    tg3_on_skin_fn        on_skin;
    tg3_on_camera_fn      on_camera;
    tg3_on_scene_fn       on_scene;
    tg3_on_light_fn       on_light;
    void                 *user_data;
} tg3_stream_callbacks;

/* --- Progress Callback --- */

typedef struct tg3_progress_info {
    uint64_t    bytes_processed;
    uint64_t    bytes_total;
    uint32_t    elements_parsed;
    const char *current_section; /* e.g. "meshes", "nodes" */
} tg3_progress_info;

typedef int32_t (*tg3_progress_fn)(const tg3_progress_info *info,
                                    void *user_data);

/* --- Write Chunk Callback (for streaming writer) --- */

typedef int32_t (*tg3_write_chunk_fn)(const uint8_t *data, uint64_t size,
                                      void *user_data);

/* ======================================================================
 * Section 12: Options Structs
 * ====================================================================== */

typedef struct tg3_memory_config {
    uint64_t       memory_budget;     /* 0 = use TINYGLTF3_MAX_MEMORY_BYTES */
    uint64_t       max_single_alloc;  /* 0 = no limit */
    uint32_t       arena_block_size;  /* 0 = default (256KB) */
    tg3_allocator  allocator;         /* All zero = use malloc/free */
} tg3_memory_config;

typedef struct tg3_parse_options {
    uint32_t             required_sections; /* TG3_REQUIRE_* flags */
    tg3_strictness       strictness;
    tg3_memory_config    memory;

    tg3_fs_callbacks     fs;
    tg3_uri_callbacks    uri;
    tg3_image_callbacks  image;
    tg3_stream_callbacks *stream;  /* NULL = no streaming */
    tg3_progress_fn      progress;
    void                *progress_user_data;

    int32_t  images_as_is;              /* 1 = don't decode images */
    int32_t  preserve_image_channels;   /* 1 = keep original channels */
    int32_t  store_original_json;       /* 1 = store raw JSON strings */
    int32_t  parse_float32;            /* 1 = parse JSON floats as float32 for speed
                                        *     (breaks strict double-precision conformance
                                        *      but sufficient for glTF data which is
                                        *      typically single-precision anyway) */
    uint64_t max_external_file_size;    /* 0 = no limit */
} tg3_parse_options;

typedef struct tg3_write_options {
    int32_t          pretty_print;     /* 1 = indented JSON */
    int32_t          write_binary;     /* 1 = GLB format */
    int32_t          embed_images;     /* 1 = embed as data URIs */
    int32_t          embed_buffers;    /* 1 = embed as data URIs */
    int32_t          serialize_defaults; /* 1 = write default values */
    tg3_fs_callbacks fs;
    tg3_uri_callbacks uri;
    tg3_memory_config memory;
} tg3_write_options;

/* ======================================================================
 * Section 13: Parser API
 * ====================================================================== */

/* Parse JSON glTF from memory */
TINYGLTF3_API tg3_error_code tg3_parse(
    tg3_model *model, tg3_error_stack *errors,
    const uint8_t *json_data, uint64_t json_size,
    const char *base_dir, uint32_t base_dir_len,
    const tg3_parse_options *options);

/* Parse GLB from memory */
TINYGLTF3_API tg3_error_code tg3_parse_glb(
    tg3_model *model, tg3_error_stack *errors,
    const uint8_t *glb_data, uint64_t glb_size,
    const char *base_dir, uint32_t base_dir_len,
    const tg3_parse_options *options);

/* Auto-detect format (JSON or GLB) and parse */
TINYGLTF3_API tg3_error_code tg3_parse_auto(
    tg3_model *model, tg3_error_stack *errors,
    const uint8_t *data, uint64_t size,
    const char *base_dir, uint32_t base_dir_len,
    const tg3_parse_options *options);

/* Parse from file (requires fs callbacks or TINYGLTF3_ENABLE_FS) */
TINYGLTF3_API tg3_error_code tg3_parse_file(
    tg3_model *model, tg3_error_stack *errors,
    const char *filename, uint32_t filename_len,
    const tg3_parse_options *options);

/* Free model and all arena memory */
TINYGLTF3_API void tg3_model_free(tg3_model *model);

/* Initialize options to defaults */
TINYGLTF3_API void tg3_parse_options_init(tg3_parse_options *options);
TINYGLTF3_API void tg3_write_options_init(tg3_write_options *options);

/* Initialize error stack */
TINYGLTF3_API void tg3_error_stack_init(tg3_error_stack *es);
TINYGLTF3_API void tg3_error_stack_free(tg3_error_stack *es);

/* ======================================================================
 * Section 14: Writer API
 * ====================================================================== */

/* Write model to memory buffer */
TINYGLTF3_API tg3_error_code tg3_write_to_memory(
    const tg3_model *model, tg3_error_stack *errors,
    uint8_t **out_data, uint64_t *out_size,
    const tg3_write_options *options);

/* Write model to file */
TINYGLTF3_API tg3_error_code tg3_write_to_file(
    const tg3_model *model, tg3_error_stack *errors,
    const char *filename, uint32_t filename_len,
    const tg3_write_options *options);

/* Free memory from tg3_write_to_memory */
TINYGLTF3_API void tg3_write_free(uint8_t *data, const tg3_write_options *options);

/* --- Streaming Writer --- */

typedef struct tg3_writer tg3_writer;

TINYGLTF3_API tg3_writer *tg3_writer_create(
    tg3_write_chunk_fn chunk_fn, void *user_data,
    const tg3_write_options *options);

TINYGLTF3_API tg3_error_code tg3_writer_begin(tg3_writer *w, const tg3_asset *asset);
TINYGLTF3_API tg3_error_code tg3_writer_add_buffer(tg3_writer *w, const tg3_buffer *buf);
TINYGLTF3_API tg3_error_code tg3_writer_add_buffer_view(tg3_writer *w, const tg3_buffer_view *bv);
TINYGLTF3_API tg3_error_code tg3_writer_add_accessor(tg3_writer *w, const tg3_accessor *acc);
TINYGLTF3_API tg3_error_code tg3_writer_add_mesh(tg3_writer *w, const tg3_mesh *mesh);
TINYGLTF3_API tg3_error_code tg3_writer_add_node(tg3_writer *w, const tg3_node *node);
TINYGLTF3_API tg3_error_code tg3_writer_add_material(tg3_writer *w, const tg3_material *mat);
TINYGLTF3_API tg3_error_code tg3_writer_add_texture(tg3_writer *w, const tg3_texture *tex);
TINYGLTF3_API tg3_error_code tg3_writer_add_image(tg3_writer *w, const tg3_image *img);
TINYGLTF3_API tg3_error_code tg3_writer_add_sampler(tg3_writer *w, const tg3_sampler *samp);
TINYGLTF3_API tg3_error_code tg3_writer_add_animation(tg3_writer *w, const tg3_animation *anim);
TINYGLTF3_API tg3_error_code tg3_writer_add_skin(tg3_writer *w, const tg3_skin *skin);
TINYGLTF3_API tg3_error_code tg3_writer_add_camera(tg3_writer *w, const tg3_camera *cam);
TINYGLTF3_API tg3_error_code tg3_writer_add_scene(tg3_writer *w, const tg3_scene *scene);
TINYGLTF3_API tg3_error_code tg3_writer_add_light(tg3_writer *w, const tg3_light *light);
TINYGLTF3_API tg3_error_code tg3_writer_end(tg3_writer *w);
TINYGLTF3_API void           tg3_writer_destroy(tg3_writer *w);

/* ======================================================================
 * Section 15: Utility Functions
 * ====================================================================== */

/* Get component size in bytes */
TINYGLTF3_API int32_t tg3_component_size(int32_t component_type);

/* Get number of components for a type */
TINYGLTF3_API int32_t tg3_num_components(int32_t type);

/* Compute byte stride for an accessor */
TINYGLTF3_API int32_t tg3_accessor_byte_stride(const tg3_accessor *accessor,
                                                const tg3_buffer_view *buffer_view);

/* Check if a string is a data URI */
TINYGLTF3_API int32_t tg3_is_data_uri(const char *uri, uint32_t len);

/* tg3_str helpers */
TINYGLTF3_API int32_t tg3_str_equals(tg3_str a, tg3_str b);
TINYGLTF3_API int32_t tg3_str_equals_cstr(tg3_str a, const char *b);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* ======================================================================
 * Section 16: C++ Convenience Wrappers
 * ====================================================================== */

#ifdef __cplusplus
namespace tinygltf3 {

/* RAII model wrapper */
class Model {
public:
    Model() { memset(&m_, 0, sizeof(m_)); m_.default_scene = -1; }
    ~Model() { tg3_model_free(&m_); }

    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;
    Model(Model &&o) noexcept : m_(o.m_) { memset(&o.m_, 0, sizeof(o.m_)); }
    Model &operator=(Model &&o) noexcept {
        if (this != &o) { tg3_model_free(&m_); m_ = o.m_; memset(&o.m_, 0, sizeof(o.m_)); }
        return *this;
    }

    tg3_model       *get()       { return &m_; }
    const tg3_model *get() const { return &m_; }
    tg3_model       *operator->()       { return &m_; }
    const tg3_model *operator->() const { return &m_; }

private:
    tg3_model m_;
};

/* RAII error stack wrapper */
class ErrorStack {
public:
    ErrorStack()  { tg3_error_stack_init(&es_); }
    ~ErrorStack() { tg3_error_stack_free(&es_); }

    ErrorStack(const ErrorStack &) = delete;
    ErrorStack &operator=(const ErrorStack &) = delete;

    tg3_error_stack       *get()       { return &es_; }
    const tg3_error_stack *get() const { return &es_; }

    bool has_error() const { return tg3_errors_has_error(&es_) != 0; }
    uint32_t count() const { return tg3_errors_count(&es_); }
    const tg3_error_entry *entry(uint32_t i) const { return tg3_errors_get(&es_, i); }

private:
    tg3_error_stack es_;
};

/* Parse helpers returning error code */
inline tg3_error_code parse_file(Model &model, ErrorStack &errors,
                                  const char *filename,
                                  const tg3_parse_options *options = nullptr) {
    tg3_parse_options opts;
    if (!options) { tg3_parse_options_init(&opts); options = &opts; }
    return tg3_parse_file(model.get(), errors.get(), filename,
                          filename ? (uint32_t)strlen(filename) : 0, options);
}

inline tg3_error_code parse(Model &model, ErrorStack &errors,
                             const uint8_t *data, uint64_t size,
                             const char *base_dir = "",
                             const tg3_parse_options *options = nullptr) {
    tg3_parse_options opts;
    if (!options) { tg3_parse_options_init(&opts); options = &opts; }
    return tg3_parse_auto(model.get(), errors.get(), data, size,
                          base_dir, base_dir ? (uint32_t)strlen(base_dir) : 0,
                          options);
}

} /* namespace tinygltf3 */
#endif /* __cplusplus */

/* ======================================================================
 * Section 17: C++20 Coroutine Facade
 * ====================================================================== */

#ifdef __cplusplus
#if defined(TINYGLTF3_ENABLE_COROUTINES) || \
    (defined(__cpp_impl_coroutine) && __cpp_impl_coroutine >= 201902L && \
     defined(__cpp_lib_coroutine) && __cpp_lib_coroutine >= 201902L)

#include <coroutine>

namespace tinygltf3 {

struct ParsedElement {
    enum Kind {
        Asset, Buffer, BufferView, Accessor, Mesh, Node, Material,
        Texture, Image, Sampler, Animation, Skin, Camera, Scene,
        Light, Done, Error
    };

    Kind    kind;
    int32_t index;

    union {
        const tg3_asset      *asset;
        const tg3_buffer     *buffer;
        const tg3_buffer_view *buffer_view;
        const tg3_accessor   *accessor;
        const tg3_mesh       *mesh;
        const tg3_node       *node;
        const tg3_material   *material;
        const tg3_texture    *texture;
        const tg3_image      *image;
        const tg3_sampler    *sampler;
        const tg3_animation  *animation;
        const tg3_skin       *skin;
        const tg3_camera     *camera;
        const tg3_scene      *scene;
        const tg3_light      *light;
        const void           *ptr;
    };

    tg3_error_code error_code;
};

class ParseGenerator {
public:
    struct promise_type {
        ParsedElement current_;
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        ParseGenerator get_return_object() {
            return ParseGenerator(
                std::coroutine_handle<promise_type>::from_promise(*this));
        }
        std::suspend_always yield_value(ParsedElement elem) noexcept {
            current_ = elem;
            return {};
        }
        void return_void() {}
        void unhandled_exception() {}
    };

    ParseGenerator() : handle_(nullptr) {}
    explicit ParseGenerator(std::coroutine_handle<promise_type> h) : handle_(h) {}
    ~ParseGenerator() { if (handle_) handle_.destroy(); }

    ParseGenerator(const ParseGenerator &) = delete;
    ParseGenerator &operator=(const ParseGenerator &) = delete;
    ParseGenerator(ParseGenerator &&o) noexcept : handle_(o.handle_) { o.handle_ = nullptr; }
    ParseGenerator &operator=(ParseGenerator &&o) noexcept {
        if (this != &o) { if (handle_) handle_.destroy(); handle_ = o.handle_; o.handle_ = nullptr; }
        return *this;
    }

    bool next() {
        if (!handle_ || handle_.done()) return false;
        handle_.resume();
        return !handle_.done();
    }

    const ParsedElement &current() const { return handle_.promise().current_; }
    bool done() const { return !handle_ || handle_.done(); }

private:
    std::coroutine_handle<promise_type> handle_;
};

/* Coroutine parse entry point — declaration only, implemented in TINYGLTF3_IMPLEMENTATION */
ParseGenerator tg3_parse_coro(
    const uint8_t *data, uint64_t size,
    const char *base_dir, uint32_t base_dir_len,
    tg3_model *model, tg3_error_stack *errors,
    const tg3_parse_options *options);

} /* namespace tinygltf3 */

#endif /* coroutines */
#endif /* __cplusplus */

/* ======================================================================
 * Section 18: Implementation
 * ====================================================================== */

#ifdef TINYGLTF3_IMPLEMENTATION

#if !defined(__cplusplus)
#error "TINYGLTF3_IMPLEMENTATION requires a C++ translation unit (compile as .cpp)"
#endif

/* Include JSON parser */
#include "tinygltf_json.h"

#include <stdio.h>
#include <math.h>

/* Implementation uses C++ features from tinygltf_json.h */
#include <string>
#include <algorithm>

/* Forward SIMD macros to tinygltf_json.h */
#ifdef TINYGLTF3_JSON_SIMD_SSE2
#ifndef TINYGLTF_JSON_SIMD_SSE2
#define TINYGLTF_JSON_SIMD_SSE2
#endif
#endif
#ifdef TINYGLTF3_JSON_SIMD_AVX2
#ifndef TINYGLTF_JSON_SIMD_AVX2
#define TINYGLTF_JSON_SIMD_AVX2
#endif
#endif
#ifdef TINYGLTF3_JSON_SIMD_NEON
#ifndef TINYGLTF_JSON_SIMD_NEON
#define TINYGLTF_JSON_SIMD_NEON
#endif
#endif

/* ======================================================================
 * Internal: Arena Allocator
 * ====================================================================== */

#define TG3__ARENA_DEFAULT_BLOCK_SIZE (256u * 1024u) /* 256 KB */
#define TG3__ARENA_ALIGNMENT 8

typedef struct tg3__arena_block {
    struct tg3__arena_block *next;
    uint8_t *base;
    size_t   used;
    size_t   capacity;
} tg3__arena_block;

struct tg3_arena {
    tg3__arena_block *head;
    tg3__arena_block *current;
    size_t            total_allocated;
    size_t            memory_budget;
    size_t            block_size;
    tg3_allocator     alloc;
};

static void *tg3__default_alloc(size_t size, void *ud) {
    (void)ud;
    return malloc(size);
}
static void *tg3__default_realloc(void *ptr, size_t old_size, size_t new_size, void *ud) {
    (void)old_size; (void)ud;
    return realloc(ptr, new_size);
}
static void tg3__default_free(void *ptr, size_t size, void *ud) {
    (void)size; (void)ud;
    free(ptr);
}

static tg3_arena *tg3__arena_create(const tg3_memory_config *config) {
    tg3_allocator alloc;
    if (config && config->allocator.alloc) {
        alloc = config->allocator;
    } else {
        alloc.alloc = tg3__default_alloc;
        alloc.realloc = tg3__default_realloc;
        alloc.free = tg3__default_free;
        alloc.user_data = NULL;
    }

    tg3_arena *arena = (tg3_arena *)alloc.alloc(sizeof(tg3_arena), alloc.user_data);
    if (!arena) return NULL;

    memset(arena, 0, sizeof(tg3_arena));
    arena->alloc = alloc;
    arena->block_size = (config && config->arena_block_size > 0)
        ? config->arena_block_size : TG3__ARENA_DEFAULT_BLOCK_SIZE;
    arena->memory_budget = (config && config->memory_budget > 0)
        ? (size_t)config->memory_budget : (size_t)TINYGLTF3_MAX_MEMORY_BYTES;

    return arena;
}

static tg3__arena_block *tg3__arena_new_block(tg3_arena *arena, size_t min_size) {
    size_t cap = arena->block_size;
    if (cap < min_size) cap = min_size;

    if (arena->total_allocated + sizeof(tg3__arena_block) + cap > arena->memory_budget) {
        return NULL; /* OOM */
    }

    uint8_t *raw = (uint8_t *)arena->alloc.alloc(
        sizeof(tg3__arena_block) + cap, arena->alloc.user_data);
    if (!raw) return NULL;

    tg3__arena_block *block = (tg3__arena_block *)raw;
    block->base = raw + sizeof(tg3__arena_block);
    block->used = 0;
    block->capacity = cap;
    block->next = NULL;

    arena->total_allocated += sizeof(tg3__arena_block) + cap;

    if (arena->current) {
        arena->current->next = block;
    } else {
        arena->head = block;
    }
    arena->current = block;

    return block;
}

static void *tg3__arena_alloc(tg3_arena *arena, size_t size) {
    if (size == 0) return NULL;

    /* Align up */
    size = (size + (TG3__ARENA_ALIGNMENT - 1)) & ~(size_t)(TG3__ARENA_ALIGNMENT - 1);

    tg3__arena_block *block = arena->current;
    if (!block || block->used + size > block->capacity) {
        block = tg3__arena_new_block(arena, size);
        if (!block) return NULL;
    }

    void *ptr = block->base + block->used;
    block->used += size;
    return ptr;
}

static char *tg3__arena_strdup(tg3_arena *arena, const char *s, size_t len) {
    if (!s) return NULL;
    /* Allocate len+1 bytes; when len==0 this produces a 1-byte "\0" buffer so
     * that empty strings (data!=NULL, len==0) remain distinguishable from
     * absent strings (data==NULL, len==0). */
    char *dst = (char *)tg3__arena_alloc(arena, len + 1);
    if (!dst) return NULL;
    if (len > 0) memcpy(dst, s, len);
    dst[len] = '\0';
    return dst;
}

static tg3_str tg3__arena_str(tg3_arena *arena, const char *s, uint32_t len) {
    tg3_str result;
    result.data = tg3__arena_strdup(arena, s, len);
    result.len = result.data ? len : 0;
    return result;
}

static tg3_str tg3__arena_str_from_std(tg3_arena *arena, const std::string &s) {
    return tg3__arena_str(arena, s.c_str(), (uint32_t)s.size());
}

static void tg3__arena_destroy(tg3_arena *arena) {
    if (!arena) return;
    tg3_allocator alloc = arena->alloc;
    tg3__arena_block *block = arena->head;
    while (block) {
        tg3__arena_block *next = block->next;
        size_t block_total = sizeof(tg3__arena_block) + block->capacity;
        alloc.free(block, block_total, alloc.user_data);
        block = next;
    }
    alloc.free(arena, sizeof(tg3_arena), alloc.user_data);
}

/* ======================================================================
 * Internal: Error Stack Implementation
 * ====================================================================== */

static void tg3__error_push(tg3_error_stack *es, tg3_severity sev,
                             tg3_error_code code, const char *msg,
                             const char *json_path, int64_t byte_offset) {
    if (!es) return;

    if (es->count >= es->capacity) {
        uint32_t new_cap = es->capacity ? es->capacity * 2 : 16;
        tg3_error_entry *new_entries = (tg3_error_entry *)realloc(
            es->entries, new_cap * sizeof(tg3_error_entry));
        if (!new_entries) return; /* Drop error on OOM */
        es->entries = new_entries;
        es->capacity = new_cap;
    }

    tg3_error_entry *e = &es->entries[es->count++];
    e->severity = sev;
    e->code = code;
    e->message = msg; /* Caller must ensure lifetime (arena or static) */
    e->json_path = json_path;
    e->byte_offset = byte_offset;

    if (sev == TG3_SEVERITY_ERROR) es->has_error = 1;
}

/* Push an error with a dynamically formatted message allocated from arena */
static void tg3__error_pushf(tg3_error_stack *es, tg3_arena *arena,
                              tg3_severity sev, tg3_error_code code,
                              const char *json_path, const char *fmt, ...) {
    if (!es) return;
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) n = 0;
    if ((size_t)n >= sizeof(buf)) n = (int)(sizeof(buf) - 1);

    const char *msg = buf;
    if (arena) {
        char *dup = tg3__arena_strdup(arena, buf, (size_t)n);
        if (dup) msg = dup;
    }
    tg3__error_push(es, sev, code, msg, json_path, -1);
}

/* ======================================================================
 * Public: Error Stack API
 * ====================================================================== */

TINYGLTF3_API int32_t tg3_errors_has_error(const tg3_error_stack *es) {
    return es ? es->has_error : 0;
}

TINYGLTF3_API uint32_t tg3_errors_count(const tg3_error_stack *es) {
    return es ? es->count : 0;
}

TINYGLTF3_API const tg3_error_entry *tg3_errors_get(const tg3_error_stack *es,
                                                     uint32_t index) {
    if (!es || index >= es->count) return NULL;
    return &es->entries[index];
}

TINYGLTF3_API void tg3_error_stack_init(tg3_error_stack *es) {
    if (!es) return;
    memset(es, 0, sizeof(tg3_error_stack));
}

TINYGLTF3_API void tg3_error_stack_free(tg3_error_stack *es) {
    if (!es) return;
    free(es->entries);
    memset(es, 0, sizeof(tg3_error_stack));
}

/* ======================================================================
 * Public: Options Init
 * ====================================================================== */

TINYGLTF3_API void tg3_parse_options_init(tg3_parse_options *options) {
    if (!options) return;
    memset(options, 0, sizeof(tg3_parse_options));
    options->required_sections = TG3_REQUIRE_VERSION;
    options->strictness = TG3_PERMISSIVE;
}

TINYGLTF3_API void tg3_write_options_init(tg3_write_options *options) {
    if (!options) return;
    memset(options, 0, sizeof(tg3_write_options));
    options->pretty_print = 1;
}

/* ======================================================================
 * Public: Utility Functions
 * ====================================================================== */

TINYGLTF3_API int32_t tg3_component_size(int32_t component_type) {
    switch (component_type) {
        case TG3_COMPONENT_TYPE_BYTE:           return 1;
        case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:  return 1;
        case TG3_COMPONENT_TYPE_SHORT:          return 2;
        case TG3_COMPONENT_TYPE_UNSIGNED_SHORT: return 2;
        case TG3_COMPONENT_TYPE_INT:            return 4;
        case TG3_COMPONENT_TYPE_UNSIGNED_INT:   return 4;
        case TG3_COMPONENT_TYPE_FLOAT:          return 4;
        case TG3_COMPONENT_TYPE_DOUBLE:         return 8;
        default: return -1;
    }
}

TINYGLTF3_API int32_t tg3_num_components(int32_t type) {
    switch (type) {
        case TG3_TYPE_SCALAR: return 1;
        case TG3_TYPE_VEC2:   return 2;
        case TG3_TYPE_VEC3:   return 3;
        case TG3_TYPE_VEC4:   return 4;
        case TG3_TYPE_MAT2:   return 4;
        case TG3_TYPE_MAT3:   return 9;
        case TG3_TYPE_MAT4:   return 16;
        default: return -1;
    }
}

TINYGLTF3_API int32_t tg3_accessor_byte_stride(const tg3_accessor *accessor,
                                                const tg3_buffer_view *bv) {
    if (bv && bv->byte_stride > 0) return (int32_t)bv->byte_stride;
    int32_t comp = tg3_component_size(accessor->component_type);
    int32_t num = tg3_num_components(accessor->type);
    if (comp < 0 || num < 0) return -1;
    return comp * num;
}

TINYGLTF3_API int32_t tg3_str_equals(tg3_str a, tg3_str b) {
    if (a.len != b.len) return 0;
    if (a.len == 0) return 1;
    return memcmp(a.data, b.data, a.len) == 0 ? 1 : 0;
}

TINYGLTF3_API int32_t tg3_str_equals_cstr(tg3_str a, const char *b) {
    if (!b) return a.len == 0 ? 1 : 0;
    uint32_t blen = (uint32_t)strlen(b);
    if (a.len != blen) return 0;
    if (a.len == 0) return 1;
    return memcmp(a.data, b, a.len) == 0 ? 1 : 0;
}

/* ======================================================================
 * Internal: Base64 Encode/Decode
 * ====================================================================== */

static const char tg3__b64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int tg3__b64_is_valid(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=';
}

static int tg3__b64_decode_char(unsigned char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static uint8_t *tg3__b64_decode(const char *input, size_t input_len,
                                 size_t *out_len, tg3_arena *arena) {
    if (input_len == 0) { *out_len = 0; return NULL; }

    /* Strip trailing padding */
    size_t pad = 0;
    while (input_len > 0 && input[input_len - 1] == '=') { ++pad; --input_len; }

    size_t decoded_len = (input_len * 3) / 4;
    uint8_t *out = (uint8_t *)tg3__arena_alloc(arena, decoded_len + 1);
    if (!out) { *out_len = 0; return NULL; }

    size_t j = 0;
    uint32_t accum = 0;
    int bits = 0;

    for (size_t i = 0; i < input_len; ++i) {
        int val = tg3__b64_decode_char((unsigned char)input[i]);
        if (val < 0) continue; /* skip whitespace/invalid */
        accum = (accum << 6) | (uint32_t)val;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[j++] = (uint8_t)((accum >> bits) & 0xFF);
        }
    }

    *out_len = j;
    return out;
}

static char *tg3__b64_encode(const uint8_t *input, size_t input_len,
                              size_t *out_len) {
    size_t enc_len = ((input_len + 2) / 3) * 4;
    char *out = (char *)malloc(enc_len + 1);
    if (!out) { *out_len = 0; return NULL; }

    size_t j = 0;
    for (size_t i = 0; i < input_len; i += 3) {
        uint32_t a = input[i];
        uint32_t b = (i + 1 < input_len) ? input[i + 1] : 0;
        uint32_t c = (i + 2 < input_len) ? input[i + 2] : 0;
        uint32_t triple = (a << 16) | (b << 8) | c;

        out[j++] = tg3__b64_chars[(triple >> 18) & 0x3F];
        out[j++] = tg3__b64_chars[(triple >> 12) & 0x3F];
        out[j++] = (i + 1 < input_len) ? tg3__b64_chars[(triple >> 6) & 0x3F] : '=';
        out[j++] = (i + 2 < input_len) ? tg3__b64_chars[triple & 0x3F] : '=';
    }
    out[j] = '\0';
    *out_len = j;
    return out;
}

/* ======================================================================
 * Internal: Data URI Handling
 * ====================================================================== */

TINYGLTF3_API int32_t tg3_is_data_uri(const char *uri, uint32_t len) {
    if (len < 5) return 0;
    return (memcmp(uri, "data:", 5) == 0) ? 1 : 0;
}

typedef struct tg3__data_uri_result {
    const char *data_start;
    size_t      data_len;
    char        mime_type[64];
} tg3__data_uri_result;

static int tg3__parse_data_uri(const char *uri, uint32_t uri_len,
                                tg3__data_uri_result *result) {
    /* Expected format: data:<mime>;base64,<data> */
    if (uri_len < 5 || memcmp(uri, "data:", 5) != 0) return 0;

    const char *p = uri + 5;
    const char *end = uri + uri_len;

    /* Find semicolon */
    const char *semi = p;
    while (semi < end && *semi != ';') ++semi;
    if (semi >= end) return 0;

    /* Extract MIME type */
    size_t mime_len = (size_t)(semi - p);
    if (mime_len >= sizeof(result->mime_type)) mime_len = sizeof(result->mime_type) - 1;
    memcpy(result->mime_type, p, mime_len);
    result->mime_type[mime_len] = '\0';

    /* Skip ";base64," */
    p = semi + 1;
    if (end - p < 7 || memcmp(p, "base64,", 7) != 0) return 0;
    p += 7;

    result->data_start = p;
    result->data_len = (size_t)(end - p);
    return 1;
}

static uint8_t *tg3__decode_data_uri(tg3_arena *arena, const char *uri,
                                      uint32_t uri_len, size_t *out_len,
                                      char *out_mime, size_t out_mime_cap) {
    tg3__data_uri_result dr;
    if (!tg3__parse_data_uri(uri, uri_len, &dr)) {
        *out_len = 0;
        return NULL;
    }

    if (out_mime && out_mime_cap > 0) {
        size_t mlen = strlen(dr.mime_type);
        if (mlen >= out_mime_cap) mlen = out_mime_cap - 1;
        memcpy(out_mime, dr.mime_type, mlen);
        out_mime[mlen] = '\0';
    }

    return tg3__b64_decode(dr.data_start, dr.data_len, out_len, arena);
}

/* ======================================================================
 * Internal: Parse Context
 * ====================================================================== */

typedef struct tg3__parse_ctx {
    tg3_arena        *arena;
    tg3_error_stack  *errors;
    tg3_parse_options opts;
    const char       *base_dir;
    uint32_t          base_dir_len;

    /* GLB binary chunk */
    const uint8_t    *bin_data;
    uint64_t          bin_size;
    int32_t           is_binary;
} tg3__parse_ctx;

/* ======================================================================
 * Internal: JSON Property Helpers
 * ====================================================================== */

/* Type alias for JSON */
typedef tinygltf_json tg3__json;

static int tg3__json_has(const tg3__json &o, const char *key) {
    auto it = o.find(key);
    return (it != o.end()) ? 1 : 0;
}

static int tg3__parse_string(tg3__parse_ctx *ctx, const tg3__json &o,
                              const char *key, tg3_str *out,
                              int required, const char *parent) {
    auto it = o.find(key);
    if (it == o.end()) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                             TG3_ERR_JSON_MISSING_FIELD, parent,
                             "Missing required field '%s'", key);
            return 0;
        }
        out->data = NULL; out->len = 0;
        return 1;
    }
    if (!it->is_string()) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                         TG3_ERR_JSON_TYPE_MISMATCH, parent,
                         "Field '%s' must be a string", key);
        return 0;
    }
    std::string s = it->get<std::string>();
    *out = tg3__arena_str_from_std(ctx->arena, s);
    return 1;
}

static int tg3__parse_int(tg3__parse_ctx *ctx, const tg3__json &o,
                           const char *key, int32_t *out,
                           int required, const char *parent) {
    auto it = o.find(key);
    if (it == o.end()) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                             TG3_ERR_JSON_MISSING_FIELD, parent,
                             "Missing required field '%s'", key);
            return 0;
        }
        return 1;
    }
    if (!it->is_number()) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                         TG3_ERR_JSON_TYPE_MISMATCH, parent,
                         "Field '%s' must be a number", key);
        return 0;
    }
    if (it->is_number_integer()) {
        int64_t v = it->get<int64_t>();
        if (v < (int64_t)INT32_MIN || v > (int64_t)INT32_MAX) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                             TG3_ERR_JSON_TYPE_MISMATCH, parent,
                             "Field '%s' value %" PRId64 " is out of range for int32", key, v);
            return 0;
        }
        *out = (int32_t)v;
    } else {
        double d = it->get<double>();
        if (d < (double)INT32_MIN || d > (double)INT32_MAX) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                             TG3_ERR_JSON_TYPE_MISMATCH, parent,
                             "Field '%s' value %f is out of range for int32", key, d);
            return 0;
        }
        *out = (int32_t)d;
    }
    return 1;
}

static int tg3__parse_uint64(tg3__parse_ctx *ctx, const tg3__json &o,
                              const char *key, uint64_t *out,
                              int required, const char *parent) {
    auto it = o.find(key);
    if (it == o.end()) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                             TG3_ERR_JSON_MISSING_FIELD, parent,
                             "Missing required field '%s'", key);
            return 0;
        }
        return 1;
    }
    if (!it->is_number()) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                         TG3_ERR_JSON_TYPE_MISMATCH, parent,
                         "Field '%s' must be a number", key);
        return 0;
    }
    if (it->is_number_integer()) {
        int64_t v = it->get<int64_t>();
        *out = (v >= 0) ? (uint64_t)v : 0;
    } else {
        *out = (uint64_t)it->get<double>();
    }
    return 1;
}

static int tg3__parse_double(tg3__parse_ctx *ctx, const tg3__json &o,
                              const char *key, double *out,
                              int required, const char *parent) {
    auto it = o.find(key);
    if (it == o.end()) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                             TG3_ERR_JSON_MISSING_FIELD, parent,
                             "Missing required field '%s'", key);
            return 0;
        }
        return 1;
    }
    if (!it->is_number()) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                         TG3_ERR_JSON_TYPE_MISMATCH, parent,
                         "Field '%s' must be a number", key);
        return 0;
    }
    *out = it->get<double>();
    return 1;
}

static int tg3__parse_bool(tg3__parse_ctx *ctx, const tg3__json &o,
                            const char *key, int32_t *out,
                            int required, const char *parent) {
    auto it = o.find(key);
    if (it == o.end()) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                             TG3_ERR_JSON_MISSING_FIELD, parent,
                             "Missing required field '%s'", key);
            return 0;
        }
        return 1;
    }
    if (!it->is_boolean()) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                         TG3_ERR_JSON_TYPE_MISMATCH, parent,
                         "Field '%s' must be a boolean", key);
        return 0;
    }
    *out = it->get<bool>() ? 1 : 0;
    return 1;
}

static int tg3__parse_number_array(tg3__parse_ctx *ctx, const tg3__json &o,
                                    const char *key, const double **out,
                                    uint32_t *out_count,
                                    int required, const char *parent) {
    auto it = o.find(key);
    if (it == o.end()) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                             TG3_ERR_JSON_MISSING_FIELD, parent,
                             "Missing required field '%s'", key);
            return 0;
        }
        *out = NULL; *out_count = 0;
        return 1;
    }
    if (!it->is_array()) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                         TG3_ERR_JSON_TYPE_MISMATCH, parent,
                         "Field '%s' must be an array", key);
        return 0;
    }
    uint32_t count = (uint32_t)it->size();
    if (count == 0) { *out = NULL; *out_count = 0; return 1; }

    double *arr = (double *)tg3__arena_alloc(ctx->arena, count * sizeof(double));
    if (!arr) {
        tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                        "OOM allocating number array", parent, -1);
        return 0;
    }
    uint32_t i = 0;
    for (auto eit = it->begin(); eit != it->end(); ++eit, ++i) {
        arr[i] = eit->get<double>();
    }
    *out = arr;
    *out_count = count;
    return 1;
}

static int tg3__parse_int_array(tg3__parse_ctx *ctx, const tg3__json &o,
                                 const char *key, const int32_t **out,
                                 uint32_t *out_count,
                                 int required, const char *parent) {
    auto it = o.find(key);
    if (it == o.end()) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                             TG3_ERR_JSON_MISSING_FIELD, parent,
                             "Missing required field '%s'", key);
            return 0;
        }
        *out = NULL; *out_count = 0;
        return 1;
    }
    if (!it->is_array()) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                         TG3_ERR_JSON_TYPE_MISMATCH, parent,
                         "Field '%s' must be an array", key);
        return 0;
    }
    uint32_t count = (uint32_t)it->size();
    if (count == 0) { *out = NULL; *out_count = 0; return 1; }

    int32_t *arr = (int32_t *)tg3__arena_alloc(ctx->arena, count * sizeof(int32_t));
    if (!arr) {
        tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                        "OOM allocating int array", parent, -1);
        return 0;
    }
    uint32_t i = 0;
    for (auto eit = it->begin(); eit != it->end(); ++eit, ++i) {
        arr[i] = eit->get<int>();
    }
    *out = arr;
    *out_count = count;
    return 1;
}

static void tg3__parse_number_to_fixed(const tg3__json &o, const char *key,
                                        double *out, uint32_t max_count) {
    auto it = o.find(key);
    if (it == o.end() || !it->is_array()) return;
    uint32_t i = 0;
    for (auto eit = it->begin(); eit != it->end() && i < max_count; ++eit, ++i) {
        out[i] = eit->get<double>();
    }
}

/* Parse string array */
static int tg3__parse_string_array(tg3__parse_ctx *ctx, const tg3__json &o,
                                    const char *key, const tg3_str **out,
                                    uint32_t *out_count,
                                    int required, const char *parent) {
    auto it = o.find(key);
    if (it == o.end()) {
        if (required) {
            tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                             TG3_ERR_JSON_MISSING_FIELD, parent,
                             "Missing required field '%s'", key);
            return 0;
        }
        *out = NULL; *out_count = 0;
        return 1;
    }
    if (!it->is_array()) return 0;
    uint32_t count = (uint32_t)it->size();
    if (count == 0) { *out = NULL; *out_count = 0; return 1; }

    tg3_str *arr = (tg3_str *)tg3__arena_alloc(ctx->arena, count * sizeof(tg3_str));
    if (!arr) return 0;
    uint32_t i = 0;
    for (auto eit = it->begin(); eit != it->end(); ++eit, ++i) {
        std::string s = eit->get<std::string>();
        arr[i] = tg3__arena_str_from_std(ctx->arena, s);
    }
    *out = arr;
    *out_count = count;
    return 1;
}

/* ======================================================================
 * Internal: Value Conversion (JSON -> tg3_value)
 * ====================================================================== */

static tg3_value tg3__json_to_value(tg3__parse_ctx *ctx, const tg3__json &j) {
    tg3_value v;
    memset(&v, 0, sizeof(v));

    if (j.is_null()) {
        v.type = TG3_VALUE_NULL;
    } else if (j.is_boolean()) {
        v.type = TG3_VALUE_BOOL;
        v.bool_val = j.get<bool>() ? 1 : 0;
    } else if (j.is_number_integer()) {
        v.type = TG3_VALUE_INT;
        v.int_val = j.get<int64_t>();
    } else if (j.is_number_float()) {
        v.type = TG3_VALUE_REAL;
        v.real_val = j.get<double>();
    } else if (j.is_string()) {
        v.type = TG3_VALUE_STRING;
        std::string s = j.get<std::string>();
        v.string_val = tg3__arena_str_from_std(ctx->arena, s);
    } else if (j.is_array()) {
        v.type = TG3_VALUE_ARRAY;
        uint32_t count = (uint32_t)j.size();
        if (count > 0) {
            tg3_value *arr = (tg3_value *)tg3__arena_alloc(
                ctx->arena, count * sizeof(tg3_value));
            if (arr) {
                uint32_t i = 0;
                for (auto it = j.begin(); it != j.end(); ++it, ++i) {
                    arr[i] = tg3__json_to_value(ctx, *it);
                }
                v.array_data = arr;
                v.array_count = count;
            }
        }
    } else if (j.is_object()) {
        v.type = TG3_VALUE_OBJECT;
        uint32_t count = (uint32_t)j.size();
        if (count > 0) {
            tg3_kv_pair *pairs = (tg3_kv_pair *)tg3__arena_alloc(
                ctx->arena, count * sizeof(tg3_kv_pair));
            if (pairs) {
                uint32_t i = 0;
                for (auto it = j.begin(); it != j.end(); ++it, ++i) {
                    std::string k = it.key();
                    pairs[i].key = tg3__arena_str_from_std(ctx->arena, k);
                    pairs[i].value = tg3__json_to_value(ctx, *it);
                }
                v.object_data = pairs;
                v.object_count = count;
            }
        }
    }
    return v;
}

/* ======================================================================
 * Internal: Parse Extras and Extensions
 * ====================================================================== */

static void tg3__init_extras_ext(tg3_extras_ext *ee) {
    memset(ee, 0, sizeof(tg3_extras_ext));
}

static void tg3__parse_extras_and_extensions(tg3__parse_ctx *ctx,
                                              const tg3__json &o,
                                              tg3_extras_ext *ee) {
    /* Extras */
    auto extras_it = o.find("extras");
    if (extras_it != o.end()) {
        tg3_value *ev = (tg3_value *)tg3__arena_alloc(ctx->arena, sizeof(tg3_value));
        if (ev) {
            *ev = tg3__json_to_value(ctx, *extras_it);
            ee->extras = ev;
        }
        if (ctx->opts.store_original_json) {
            std::string raw = extras_it->dump();
            ee->extras_json = tg3__arena_str_from_std(ctx->arena, raw);
        }
    }

    /* Extensions */
    auto ext_it = o.find("extensions");
    if (ext_it != o.end() && ext_it->is_object()) {
        uint32_t count = (uint32_t)ext_it->size();
        if (count > 0) {
            tg3_extension *exts = (tg3_extension *)tg3__arena_alloc(
                ctx->arena, count * sizeof(tg3_extension));
            if (exts) {
                uint32_t i = 0;
                for (auto it = ext_it->begin(); it != ext_it->end(); ++it, ++i) {
                    std::string k = it.key();
                    exts[i].name = tg3__arena_str_from_std(ctx->arena, k);
                    exts[i].value = tg3__json_to_value(ctx, *it);
                }
                ee->extensions = exts;
                ee->extensions_count = count;
            }
        }
        if (ctx->opts.store_original_json) {
            std::string raw = ext_it->dump();
            ee->extensions_json = tg3__arena_str_from_std(ctx->arena, raw);
        }
    }
}

/* ======================================================================
 * Internal: Init functions for default struct values
 * ====================================================================== */

static void tg3__init_texture_info(tg3_texture_info *ti) {
    memset(ti, 0, sizeof(tg3_texture_info));
    ti->index = -1;
    ti->tex_coord = 0;
}

static void tg3__init_normal_texture_info(tg3_normal_texture_info *ti) {
    memset(ti, 0, sizeof(tg3_normal_texture_info));
    ti->index = -1;
    ti->tex_coord = 0;
    ti->scale = 1.0;
}

static void tg3__init_occlusion_texture_info(tg3_occlusion_texture_info *ti) {
    memset(ti, 0, sizeof(tg3_occlusion_texture_info));
    ti->index = -1;
    ti->tex_coord = 0;
    ti->strength = 1.0;
}

static void tg3__init_pbr(tg3_pbr_metallic_roughness *pbr) {
    memset(pbr, 0, sizeof(tg3_pbr_metallic_roughness));
    pbr->base_color_factor[0] = 1.0;
    pbr->base_color_factor[1] = 1.0;
    pbr->base_color_factor[2] = 1.0;
    pbr->base_color_factor[3] = 1.0;
    pbr->metallic_factor = 1.0;
    pbr->roughness_factor = 1.0;
    tg3__init_texture_info(&pbr->base_color_texture);
    tg3__init_texture_info(&pbr->metallic_roughness_texture);
}

static void tg3__init_node(tg3_node *n) {
    memset(n, 0, sizeof(tg3_node));
    n->camera = -1;
    n->skin = -1;
    n->mesh = -1;
    n->light = -1;
    n->emitter = -1;
    n->rotation[3] = 1.0; /* w=1 identity quaternion */
    n->scale[0] = 1.0;
    n->scale[1] = 1.0;
    n->scale[2] = 1.0;
    /* Identity matrix */
    n->matrix[0]  = 1.0;
    n->matrix[5]  = 1.0;
    n->matrix[10] = 1.0;
    n->matrix[15] = 1.0;
}

/* ======================================================================
 * Internal: Entity Parse Functions
 * ====================================================================== */

static int tg3__parse_asset(tg3__parse_ctx *ctx, const tg3__json &o,
                             tg3_asset *asset) {
    memset(asset, 0, sizeof(tg3_asset));
    tg3__parse_string(ctx, o, "version", &asset->version, 0, "/asset");
    tg3__parse_string(ctx, o, "generator", &asset->generator, 0, "/asset");
    tg3__parse_string(ctx, o, "minVersion", &asset->min_version, 0, "/asset");
    tg3__parse_string(ctx, o, "copyright", &asset->copyright, 0, "/asset");
    tg3__parse_extras_and_extensions(ctx, o, &asset->ext);
    return 1;
}

static int tg3__parse_texture_info(tg3__parse_ctx *ctx, const tg3__json &o,
                                    const char *key, tg3_texture_info *ti) {
    tg3__init_texture_info(ti);
    auto it = o.find(key);
    if (it == o.end()) return 1; /* Optional */
    if (!it->is_object()) return 0;
    tg3__parse_int(ctx, *it, "index", &ti->index, 0, key);
    tg3__parse_int(ctx, *it, "texCoord", &ti->tex_coord, 0, key);
    tg3__parse_extras_and_extensions(ctx, *it, &ti->ext);
    return 1;
}

static int tg3__parse_normal_texture_info(tg3__parse_ctx *ctx, const tg3__json &o,
                                           const char *key,
                                           tg3_normal_texture_info *ti) {
    tg3__init_normal_texture_info(ti);
    auto it = o.find(key);
    if (it == o.end()) return 1;
    if (!it->is_object()) return 0;
    tg3__parse_int(ctx, *it, "index", &ti->index, 0, key);
    tg3__parse_int(ctx, *it, "texCoord", &ti->tex_coord, 0, key);
    tg3__parse_double(ctx, *it, "scale", &ti->scale, 0, key);
    tg3__parse_extras_and_extensions(ctx, *it, &ti->ext);
    return 1;
}

static int tg3__parse_occlusion_texture_info(tg3__parse_ctx *ctx,
                                              const tg3__json &o,
                                              const char *key,
                                              tg3_occlusion_texture_info *ti) {
    tg3__init_occlusion_texture_info(ti);
    auto it = o.find(key);
    if (it == o.end()) return 1;
    if (!it->is_object()) return 0;
    tg3__parse_int(ctx, *it, "index", &ti->index, 0, key);
    tg3__parse_int(ctx, *it, "texCoord", &ti->tex_coord, 0, key);
    tg3__parse_double(ctx, *it, "strength", &ti->strength, 0, key);
    tg3__parse_extras_and_extensions(ctx, *it, &ti->ext);
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

static const char *tg3__accessor_type_to_string(int type) {
    switch (type) {
        case TG3_TYPE_SCALAR: return "SCALAR";
        case TG3_TYPE_VEC2:   return "VEC2";
        case TG3_TYPE_VEC3:   return "VEC3";
        case TG3_TYPE_VEC4:   return "VEC4";
        case TG3_TYPE_MAT2:   return "MAT2";
        case TG3_TYPE_MAT3:   return "MAT3";
        case TG3_TYPE_MAT4:   return "MAT4";
        default: return "";
    }
}

static int tg3__parse_accessor_sparse(tg3__parse_ctx *ctx, const tg3__json &o,
                                       tg3_accessor_sparse *sparse) {
    memset(sparse, 0, sizeof(tg3_accessor_sparse));
    sparse->indices.buffer_view = -1;
    sparse->values.buffer_view = -1;

    auto it = o.find("sparse");
    if (it == o.end()) return 1;
    if (!it->is_object()) return 0;

    sparse->is_sparse = 1;
    tg3__parse_int(ctx, *it, "count", &sparse->count, 1, "/sparse");

    auto idx_it = it->find("indices");
    if (idx_it != it->end() && idx_it->is_object()) {
        tg3__parse_int(ctx, *idx_it, "bufferView",
                       &sparse->indices.buffer_view, 1, "/sparse/indices");
        tg3__parse_int(ctx, *idx_it, "componentType",
                       &sparse->indices.component_type, 1, "/sparse/indices");
        uint64_t bo = 0;
        tg3__parse_uint64(ctx, *idx_it, "byteOffset", &bo, 0, "/sparse/indices");
        sparse->indices.byte_offset = bo;
        tg3__parse_extras_and_extensions(ctx, *idx_it, &sparse->indices.ext);
    }

    auto val_it = it->find("values");
    if (val_it != it->end() && val_it->is_object()) {
        tg3__parse_int(ctx, *val_it, "bufferView",
                       &sparse->values.buffer_view, 1, "/sparse/values");
        uint64_t bo = 0;
        tg3__parse_uint64(ctx, *val_it, "byteOffset", &bo, 0, "/sparse/values");
        sparse->values.byte_offset = bo;
        tg3__parse_extras_and_extensions(ctx, *val_it, &sparse->values.ext);
    }

    tg3__parse_extras_and_extensions(ctx, *it, &sparse->ext);
    return 1;
}

static int tg3__parse_accessor(tg3__parse_ctx *ctx, const tg3__json &o,
                                tg3_accessor *acc) {
    memset(acc, 0, sizeof(tg3_accessor));
    acc->buffer_view = -1;
    acc->component_type = -1;
    acc->type = -1;

    tg3__parse_string(ctx, o, "name", &acc->name, 0, "/accessor");
    tg3__parse_int(ctx, o, "bufferView", &acc->buffer_view, 0, "/accessor");

    uint64_t bo = 0;
    tg3__parse_uint64(ctx, o, "byteOffset", &bo, 0, "/accessor");
    acc->byte_offset = bo;

    tg3__parse_bool(ctx, o, "normalized", &acc->normalized, 0, "/accessor");
    tg3__parse_int(ctx, o, "componentType", &acc->component_type, 1, "/accessor");

    uint64_t cnt = 0;
    tg3__parse_uint64(ctx, o, "count", &cnt, 1, "/accessor");
    acc->count = cnt;

    /* Parse type string */
    tg3_str type_str = {0, 0};
    tg3__parse_string(ctx, o, "type", &type_str, 1, "/accessor");
    if (type_str.data) {
        acc->type = tg3__accessor_type_from_string(type_str.data, type_str.len);
    }

    tg3__parse_number_array(ctx, o, "min", &acc->min_values, &acc->min_values_count,
                             0, "/accessor");
    tg3__parse_number_array(ctx, o, "max", &acc->max_values, &acc->max_values_count,
                             0, "/accessor");

    tg3__parse_accessor_sparse(ctx, o, &acc->sparse);
    tg3__parse_extras_and_extensions(ctx, o, &acc->ext);
    return 1;
}

static int tg3__load_external_file(tg3__parse_ctx *ctx, uint8_t **out_data,
                                    uint64_t *out_size, const char *uri,
                                    uint32_t uri_len) {
    if (!ctx->opts.fs.read_file) {
        tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR, TG3_ERR_FS_NOT_AVAILABLE,
                        "No filesystem callbacks available", NULL, -1);
        return 0;
    }

    /* Build full path: base_dir + "/" + uri */
    char path_buf[4096];
    uint32_t path_len = 0;
    if (ctx->base_dir_len > 0) {
        if (ctx->base_dir_len + 1 + uri_len >= sizeof(path_buf)) return 0;
        memcpy(path_buf, ctx->base_dir, ctx->base_dir_len);
        path_len = ctx->base_dir_len;
        if (path_buf[path_len - 1] != '/' && path_buf[path_len - 1] != '\\') {
            path_buf[path_len++] = '/';
        }
    }
    if (path_len + uri_len >= sizeof(path_buf)) return 0;
    memcpy(path_buf + path_len, uri, uri_len);
    path_len += uri_len;
    path_buf[path_len] = '\0';

    int32_t ok = ctx->opts.fs.read_file(out_data, out_size, path_buf, path_len,
                                         ctx->opts.fs.user_data);
    if (!ok) {
        tg3__error_pushf(ctx->errors, ctx->arena, TG3_SEVERITY_ERROR,
                         TG3_ERR_FILE_READ, NULL, "Failed to read file: %s", path_buf);
        return 0;
    }
    return 1;
}

static int tg3__parse_buffer(tg3__parse_ctx *ctx, const tg3__json &o,
                              tg3_buffer *buf, int32_t buf_idx) {
    memset(buf, 0, sizeof(tg3_buffer));
    tg3__parse_string(ctx, o, "name", &buf->name, 0, "/buffer");
    tg3__parse_string(ctx, o, "uri", &buf->uri, 0, "/buffer");

    uint64_t byte_length = 0;
    tg3__parse_uint64(ctx, o, "byteLength", &byte_length, 1, "/buffer");

    /* Load buffer data */
    if (ctx->is_binary && buf_idx == 0 && buf->uri.len == 0) {
        /* GLB: first buffer uses binary chunk */
        if (!ctx->bin_data || ctx->bin_size < byte_length) {
            tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR,
                            TG3_ERR_BUFFER_SIZE_MISMATCH,
                            "GLB BIN chunk missing or smaller than buffer.byteLength",
                            NULL, -1);
            return 0;
        }
        uint8_t *data = (uint8_t *)tg3__arena_alloc(ctx->arena, (size_t)byte_length);
        if (!data) {
            tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR,
                            TG3_ERR_OUT_OF_MEMORY, "OOM for buffer data", NULL, -1);
            return 0;
        }
        memcpy(data, ctx->bin_data, (size_t)byte_length);
        buf->data.data = data;
        buf->data.count = byte_length;
    } else if (buf->uri.len > 0) {
        if (tg3_is_data_uri(buf->uri.data, buf->uri.len)) {
            /* Data URI */
            size_t decoded_len = 0;
            char mime[64] = {0};
            uint8_t *decoded = tg3__decode_data_uri(ctx->arena, buf->uri.data,
                                                     buf->uri.len, &decoded_len,
                                                     mime, sizeof(mime));
            if (!decoded && byte_length > 0) {
                tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR,
                                TG3_ERR_DATA_URI_DECODE,
                                "Failed to decode buffer data URI", NULL, -1);
                return 0;
            }
            buf->data.data = decoded;
            buf->data.count = decoded_len;
        } else {
            /* External file */
            uint8_t *file_data = NULL;
            uint64_t file_size = 0;
            if (tg3__load_external_file(ctx, &file_data, &file_size,
                                         buf->uri.data, buf->uri.len)) {
                /* Copy into arena */
                uint8_t *data = (uint8_t *)tg3__arena_alloc(ctx->arena, (size_t)file_size);
                if (data) {
                    memcpy(data, file_data, (size_t)file_size);
                    buf->data.data = data;
                    buf->data.count = file_size;
                }
                /* Free file data via callback */
                if (ctx->opts.fs.free_file) {
                    ctx->opts.fs.free_file(file_data, file_size,
                                           ctx->opts.fs.user_data);
                }
            }
        }
    }

    tg3__parse_extras_and_extensions(ctx, o, &buf->ext);
    return 1;
}

static int tg3__parse_buffer_view(tg3__parse_ctx *ctx, const tg3__json &o,
                                   tg3_buffer_view *bv) {
    memset(bv, 0, sizeof(tg3_buffer_view));
    bv->buffer = -1;

    tg3__parse_string(ctx, o, "name", &bv->name, 0, "/bufferView");
    tg3__parse_int(ctx, o, "buffer", &bv->buffer, 1, "/bufferView");

    uint64_t val = 0;
    tg3__parse_uint64(ctx, o, "byteOffset", &val, 0, "/bufferView");
    bv->byte_offset = val;
    val = 0;
    tg3__parse_uint64(ctx, o, "byteLength", &val, 1, "/bufferView");
    bv->byte_length = val;

    int32_t stride = 0;
    tg3__parse_int(ctx, o, "byteStride", &stride, 0, "/bufferView");
    bv->byte_stride = (uint32_t)stride;

    tg3__parse_int(ctx, o, "target", &bv->target, 0, "/bufferView");
    tg3__parse_extras_and_extensions(ctx, o, &bv->ext);
    return 1;
}

static int tg3__parse_image(tg3__parse_ctx *ctx, const tg3__json &o,
                             tg3_image *img, int32_t /*img_idx*/) {
    memset(img, 0, sizeof(tg3_image));
    img->width = -1;
    img->height = -1;
    img->component = -1;
    img->bits = -1;
    img->pixel_type = -1;
    img->buffer_view = -1;

    tg3__parse_string(ctx, o, "name", &img->name, 0, "/image");
    tg3__parse_string(ctx, o, "uri", &img->uri, 0, "/image");
    tg3__parse_string(ctx, o, "mimeType", &img->mime_type, 0, "/image");
    tg3__parse_int(ctx, o, "bufferView", &img->buffer_view, 0, "/image");

    if (ctx->opts.images_as_is) {
        img->as_is = 1;
    }

    tg3__parse_extras_and_extensions(ctx, o, &img->ext);
    return 1;
}

static int tg3__parse_sampler(tg3__parse_ctx *ctx, const tg3__json &o,
                               tg3_sampler *samp) {
    memset(samp, 0, sizeof(tg3_sampler));
    samp->min_filter = -1;
    samp->mag_filter = -1;
    samp->wrap_s = TG3_TEXTURE_WRAP_REPEAT;
    samp->wrap_t = TG3_TEXTURE_WRAP_REPEAT;

    tg3__parse_string(ctx, o, "name", &samp->name, 0, "/sampler");
    tg3__parse_int(ctx, o, "minFilter", &samp->min_filter, 0, "/sampler");
    tg3__parse_int(ctx, o, "magFilter", &samp->mag_filter, 0, "/sampler");
    tg3__parse_int(ctx, o, "wrapS", &samp->wrap_s, 0, "/sampler");
    tg3__parse_int(ctx, o, "wrapT", &samp->wrap_t, 0, "/sampler");
    tg3__parse_extras_and_extensions(ctx, o, &samp->ext);
    return 1;
}

static int tg3__parse_texture(tg3__parse_ctx *ctx, const tg3__json &o,
                               tg3_texture *tex) {
    memset(tex, 0, sizeof(tg3_texture));
    tex->sampler = -1;
    tex->source = -1;

    tg3__parse_string(ctx, o, "name", &tex->name, 0, "/texture");
    tg3__parse_int(ctx, o, "sampler", &tex->sampler, 0, "/texture");
    tg3__parse_int(ctx, o, "source", &tex->source, 0, "/texture");
    tg3__parse_extras_and_extensions(ctx, o, &tex->ext);
    return 1;
}

static int tg3__parse_material(tg3__parse_ctx *ctx, const tg3__json &o,
                                tg3_material *mat) {
    memset(mat, 0, sizeof(tg3_material));
    tg3__init_pbr(&mat->pbr_metallic_roughness);
    tg3__init_normal_texture_info(&mat->normal_texture);
    tg3__init_occlusion_texture_info(&mat->occlusion_texture);
    tg3__init_texture_info(&mat->emissive_texture);
    mat->alpha_cutoff = 0.5;

    tg3__parse_string(ctx, o, "name", &mat->name, 0, "/material");

    /* Emissive factor */
    tg3__parse_number_to_fixed(o, "emissiveFactor", mat->emissive_factor, 3);

    /* Alpha mode */
    tg3_str alpha_mode = {0, 0};
    tg3__parse_string(ctx, o, "alphaMode", &alpha_mode, 0, "/material");
    if (alpha_mode.len > 0) {
        mat->alpha_mode = alpha_mode;
    } else {
        mat->alpha_mode = tg3__arena_str(ctx->arena, "OPAQUE", 6);
    }

    tg3__parse_double(ctx, o, "alphaCutoff", &mat->alpha_cutoff, 0, "/material");
    tg3__parse_bool(ctx, o, "doubleSided", &mat->double_sided, 0, "/material");

    /* PBR */
    auto pbr_it = o.find("pbrMetallicRoughness");
    if (pbr_it != o.end() && pbr_it->is_object()) {
        tg3__parse_number_to_fixed(*pbr_it, "baseColorFactor",
                                    mat->pbr_metallic_roughness.base_color_factor, 4);
        tg3__parse_double(ctx, *pbr_it, "metallicFactor",
                          &mat->pbr_metallic_roughness.metallic_factor, 0,
                          "/material/pbrMetallicRoughness");
        tg3__parse_double(ctx, *pbr_it, "roughnessFactor",
                          &mat->pbr_metallic_roughness.roughness_factor, 0,
                          "/material/pbrMetallicRoughness");
        tg3__parse_texture_info(ctx, *pbr_it, "baseColorTexture",
                                &mat->pbr_metallic_roughness.base_color_texture);
        tg3__parse_texture_info(ctx, *pbr_it, "metallicRoughnessTexture",
                                &mat->pbr_metallic_roughness.metallic_roughness_texture);
        tg3__parse_extras_and_extensions(ctx, *pbr_it,
                                          &mat->pbr_metallic_roughness.ext);
    }

    tg3__parse_normal_texture_info(ctx, o, "normalTexture", &mat->normal_texture);
    tg3__parse_occlusion_texture_info(ctx, o, "occlusionTexture",
                                       &mat->occlusion_texture);
    tg3__parse_texture_info(ctx, o, "emissiveTexture", &mat->emissive_texture);

    /* MSFT_lod */
    auto ext_it = o.find("extensions");
    if (ext_it != o.end() && ext_it->is_object()) {
        auto lod_it = ext_it->find("MSFT_lod");
        if (lod_it != ext_it->end() && lod_it->is_object()) {
            tg3__parse_int_array(ctx, *lod_it, "ids",
                                 &mat->lods, &mat->lods_count, 0,
                                 "/material/extensions/MSFT_lod");
        }
    }

    tg3__parse_extras_and_extensions(ctx, o, &mat->ext);
    return 1;
}

static int tg3__parse_primitive(tg3__parse_ctx *ctx, const tg3__json &o,
                                 tg3_primitive *prim) {
    memset(prim, 0, sizeof(tg3_primitive));
    prim->material = -1;
    prim->indices = -1;
    prim->mode = TG3_MODE_TRIANGLES;

    tg3__parse_int(ctx, o, "material", &prim->material, 0, "/primitive");
    tg3__parse_int(ctx, o, "indices", &prim->indices, 0, "/primitive");
    tg3__parse_int(ctx, o, "mode", &prim->mode, 0, "/primitive");

    /* Attributes */
    auto attr_it = o.find("attributes");
    if (attr_it != o.end() && attr_it->is_object()) {
        uint32_t count = (uint32_t)attr_it->size();
        if (count > 0) {
            tg3_str_int_pair *attrs = (tg3_str_int_pair *)tg3__arena_alloc(
                ctx->arena, count * sizeof(tg3_str_int_pair));
            if (attrs) {
                uint32_t i = 0;
                for (auto it = attr_it->begin(); it != attr_it->end(); ++it, ++i) {
                    std::string k = it.key();
                    attrs[i].key = tg3__arena_str_from_std(ctx->arena, k);
                    attrs[i].value = it->get<int>();
                }
                prim->attributes = attrs;
                prim->attributes_count = count;
            }
        }
    }

    /* Morph targets */
    auto targets_it = o.find("targets");
    if (targets_it != o.end() && targets_it->is_array()) {
        uint32_t tcount = (uint32_t)targets_it->size();
        if (tcount > 0) {
            const tg3_str_int_pair **target_arrays =
                (const tg3_str_int_pair **)tg3__arena_alloc(
                    ctx->arena, tcount * sizeof(tg3_str_int_pair *));
            uint32_t *target_counts = (uint32_t *)tg3__arena_alloc(
                ctx->arena, tcount * sizeof(uint32_t));
            if (target_arrays && target_counts) {
                uint32_t ti = 0;
                for (auto tit = targets_it->begin(); tit != targets_it->end(); ++tit, ++ti) {
                    if (!tit->is_object()) {
                        target_arrays[ti] = NULL;
                        target_counts[ti] = 0;
                        continue;
                    }
                    uint32_t acount = (uint32_t)tit->size();
                    tg3_str_int_pair *tattrs = (tg3_str_int_pair *)tg3__arena_alloc(
                        ctx->arena, acount * sizeof(tg3_str_int_pair));
                    if (tattrs) {
                        uint32_t ai = 0;
                        for (auto ait = tit->begin(); ait != tit->end(); ++ait, ++ai) {
                            std::string k = ait.key();
                            tattrs[ai].key = tg3__arena_str_from_std(ctx->arena, k);
                            tattrs[ai].value = ait->get<int>();
                        }
                    }
                    target_arrays[ti] = tattrs;
                    target_counts[ti] = acount;
                }
                prim->targets = target_arrays;
                prim->target_attribute_counts = target_counts;
                prim->targets_count = tcount;
            }
        }
    }

    tg3__parse_extras_and_extensions(ctx, o, &prim->ext);
    return 1;
}

static int tg3__parse_mesh(tg3__parse_ctx *ctx, const tg3__json &o,
                            tg3_mesh *mesh) {
    memset(mesh, 0, sizeof(tg3_mesh));
    tg3__parse_string(ctx, o, "name", &mesh->name, 0, "/mesh");

    /* Primitives */
    auto prim_it = o.find("primitives");
    if (prim_it != o.end() && prim_it->is_array()) {
        uint32_t count = (uint32_t)prim_it->size();
        if (count > 0) {
            tg3_primitive *prims = (tg3_primitive *)tg3__arena_alloc(
                ctx->arena, count * sizeof(tg3_primitive));
            if (prims) {
                uint32_t i = 0;
                for (auto it = prim_it->begin(); it != prim_it->end(); ++it, ++i) {
                    tg3__parse_primitive(ctx, *it, &prims[i]);
                }
                mesh->primitives = prims;
                mesh->primitives_count = count;
            }
        }
    }

    tg3__parse_number_array(ctx, o, "weights", &mesh->weights,
                             &mesh->weights_count, 0, "/mesh");
    tg3__parse_extras_and_extensions(ctx, o, &mesh->ext);
    return 1;
}

static int tg3__parse_node(tg3__parse_ctx *ctx, const tg3__json &o,
                            tg3_node *node) {
    tg3__init_node(node);

    tg3__parse_string(ctx, o, "name", &node->name, 0, "/node");
    tg3__parse_int(ctx, o, "camera", &node->camera, 0, "/node");
    tg3__parse_int(ctx, o, "skin", &node->skin, 0, "/node");
    tg3__parse_int(ctx, o, "mesh", &node->mesh, 0, "/node");

    tg3__parse_int_array(ctx, o, "children", &node->children,
                          &node->children_count, 0, "/node");

    /* TRS */
    if (tg3__json_has(o, "matrix")) {
        tg3__parse_number_to_fixed(o, "matrix", node->matrix, 16);
        node->has_matrix = 1;
    }
    if (tg3__json_has(o, "translation")) {
        tg3__parse_number_to_fixed(o, "translation", node->translation, 3);
    }
    if (tg3__json_has(o, "rotation")) {
        tg3__parse_number_to_fixed(o, "rotation", node->rotation, 4);
    }
    if (tg3__json_has(o, "scale")) {
        tg3__parse_number_to_fixed(o, "scale", node->scale, 3);
    }

    tg3__parse_number_array(ctx, o, "weights", &node->weights,
                             &node->weights_count, 0, "/node");

    /* Extensions: KHR_lights_punctual, KHR_audio, MSFT_lod */
    auto ext_it = o.find("extensions");
    if (ext_it != o.end() && ext_it->is_object()) {
        auto khr_lights = ext_it->find("KHR_lights_punctual");
        if (khr_lights != ext_it->end() && khr_lights->is_object()) {
            tg3__parse_int(ctx, *khr_lights, "light", &node->light, 0,
                          "/node/extensions/KHR_lights_punctual");
        }
        auto khr_audio = ext_it->find("KHR_audio");
        if (khr_audio != ext_it->end() && khr_audio->is_object()) {
            tg3__parse_int(ctx, *khr_audio, "emitter", &node->emitter, 0,
                          "/node/extensions/KHR_audio");
        }
        auto msft_lod = ext_it->find("MSFT_lod");
        if (msft_lod != ext_it->end() && msft_lod->is_object()) {
            tg3__parse_int_array(ctx, *msft_lod, "ids",
                                 &node->lods, &node->lods_count, 0,
                                 "/node/extensions/MSFT_lod");
        }
    }

    tg3__parse_extras_and_extensions(ctx, o, &node->ext);
    return 1;
}

static int tg3__parse_skin(tg3__parse_ctx *ctx, const tg3__json &o,
                            tg3_skin *skin) {
    memset(skin, 0, sizeof(tg3_skin));
    skin->inverse_bind_matrices = -1;
    skin->skeleton = -1;

    tg3__parse_string(ctx, o, "name", &skin->name, 0, "/skin");
    tg3__parse_int(ctx, o, "inverseBindMatrices", &skin->inverse_bind_matrices,
                   0, "/skin");
    tg3__parse_int(ctx, o, "skeleton", &skin->skeleton, 0, "/skin");
    tg3__parse_int_array(ctx, o, "joints", &skin->joints,
                          &skin->joints_count, 1, "/skin");
    tg3__parse_extras_and_extensions(ctx, o, &skin->ext);
    return 1;
}

static int tg3__parse_animation(tg3__parse_ctx *ctx, const tg3__json &o,
                                 tg3_animation *anim) {
    memset(anim, 0, sizeof(tg3_animation));
    tg3__parse_string(ctx, o, "name", &anim->name, 0, "/animation");

    /* Channels */
    auto ch_it = o.find("channels");
    if (ch_it != o.end() && ch_it->is_array()) {
        uint32_t count = (uint32_t)ch_it->size();
        if (count > 0) {
            tg3_animation_channel *channels = (tg3_animation_channel *)tg3__arena_alloc(
                ctx->arena, count * sizeof(tg3_animation_channel));
            if (channels) {
                uint32_t i = 0;
                for (auto it = ch_it->begin(); it != ch_it->end(); ++it, ++i) {
                    memset(&channels[i], 0, sizeof(tg3_animation_channel));
                    channels[i].sampler = -1;
                    channels[i].target.node = -1;

                    tg3__parse_int(ctx, *it, "sampler", &channels[i].sampler,
                                   1, "/animation/channel");

                    auto tgt_it = it->find("target");
                    if (tgt_it != it->end() && tgt_it->is_object()) {
                        tg3__parse_int(ctx, *tgt_it, "node",
                                       &channels[i].target.node, 0,
                                       "/animation/channel/target");
                        tg3__parse_string(ctx, *tgt_it, "path",
                                          &channels[i].target.path, 1,
                                          "/animation/channel/target");
                        tg3__parse_extras_and_extensions(ctx, *tgt_it,
                                                          &channels[i].target.ext);
                    }
                    tg3__parse_extras_and_extensions(ctx, *it, &channels[i].ext);
                }
                anim->channels = channels;
                anim->channels_count = count;
            }
        }
    }

    /* Samplers */
    auto samp_it = o.find("samplers");
    if (samp_it != o.end() && samp_it->is_array()) {
        uint32_t count = (uint32_t)samp_it->size();
        if (count > 0) {
            tg3_animation_sampler *samplers = (tg3_animation_sampler *)tg3__arena_alloc(
                ctx->arena, count * sizeof(tg3_animation_sampler));
            if (samplers) {
                uint32_t i = 0;
                for (auto it = samp_it->begin(); it != samp_it->end(); ++it, ++i) {
                    memset(&samplers[i], 0, sizeof(tg3_animation_sampler));
                    samplers[i].input = -1;
                    samplers[i].output = -1;

                    tg3__parse_int(ctx, *it, "input", &samplers[i].input,
                                   1, "/animation/sampler");
                    tg3__parse_int(ctx, *it, "output", &samplers[i].output,
                                   1, "/animation/sampler");

                    tg3_str interp = {0, 0};
                    tg3__parse_string(ctx, *it, "interpolation", &interp,
                                      0, "/animation/sampler");
                    if (interp.len > 0) {
                        samplers[i].interpolation = interp;
                    } else {
                        samplers[i].interpolation = tg3__arena_str(ctx->arena,
                                                                     "LINEAR", 6);
                    }
                    tg3__parse_extras_and_extensions(ctx, *it, &samplers[i].ext);
                }
                anim->samplers = samplers;
                anim->samplers_count = count;
            }
        }
    }

    tg3__parse_extras_and_extensions(ctx, o, &anim->ext);
    return 1;
}

static int tg3__parse_camera(tg3__parse_ctx *ctx, const tg3__json &o,
                              tg3_camera *cam) {
    memset(cam, 0, sizeof(tg3_camera));
    tg3__parse_string(ctx, o, "name", &cam->name, 0, "/camera");
    tg3__parse_string(ctx, o, "type", &cam->type, 1, "/camera");

    if (cam->type.data && tg3_str_equals_cstr(cam->type, "perspective")) {
        auto p_it = o.find("perspective");
        if (p_it != o.end() && p_it->is_object()) {
            tg3__parse_double(ctx, *p_it, "aspectRatio",
                              &cam->perspective.aspect_ratio, 0, "/camera/perspective");
            tg3__parse_double(ctx, *p_it, "yfov",
                              &cam->perspective.yfov, 1, "/camera/perspective");
            tg3__parse_double(ctx, *p_it, "zfar",
                              &cam->perspective.zfar, 0, "/camera/perspective");
            tg3__parse_double(ctx, *p_it, "znear",
                              &cam->perspective.znear, 1, "/camera/perspective");
            tg3__parse_extras_and_extensions(ctx, *p_it, &cam->perspective.ext);
        }
    } else if (cam->type.data && tg3_str_equals_cstr(cam->type, "orthographic")) {
        auto o_it = o.find("orthographic");
        if (o_it != o.end() && o_it->is_object()) {
            tg3__parse_double(ctx, *o_it, "xmag",
                              &cam->orthographic.xmag, 1, "/camera/orthographic");
            tg3__parse_double(ctx, *o_it, "ymag",
                              &cam->orthographic.ymag, 1, "/camera/orthographic");
            tg3__parse_double(ctx, *o_it, "zfar",
                              &cam->orthographic.zfar, 1, "/camera/orthographic");
            tg3__parse_double(ctx, *o_it, "znear",
                              &cam->orthographic.znear, 1, "/camera/orthographic");
            tg3__parse_extras_and_extensions(ctx, *o_it, &cam->orthographic.ext);
        }
    }

    tg3__parse_extras_and_extensions(ctx, o, &cam->ext);
    return 1;
}

static int tg3__parse_scene(tg3__parse_ctx *ctx, const tg3__json &o,
                             tg3_scene *scene) {
    memset(scene, 0, sizeof(tg3_scene));
    tg3__parse_string(ctx, o, "name", &scene->name, 0, "/scene");
    tg3__parse_int_array(ctx, o, "nodes", &scene->nodes,
                          &scene->nodes_count, 0, "/scene");

    /* KHR_audio emitters */
    auto ext_it = o.find("extensions");
    if (ext_it != o.end() && ext_it->is_object()) {
        auto audio_it = ext_it->find("KHR_audio");
        if (audio_it != ext_it->end() && audio_it->is_object()) {
            tg3__parse_int_array(ctx, *audio_it, "emitters",
                                 &scene->audio_emitters,
                                 &scene->audio_emitters_count, 0,
                                 "/scene/extensions/KHR_audio");
        }
    }

    tg3__parse_extras_and_extensions(ctx, o, &scene->ext);
    return 1;
}

static int tg3__parse_light(tg3__parse_ctx *ctx, const tg3__json &o,
                             tg3_light *light) {
    memset(light, 0, sizeof(tg3_light));
    light->color[0] = 1.0;
    light->color[1] = 1.0;
    light->color[2] = 1.0;
    light->intensity = 1.0;
    light->spot.outer_cone_angle = 0.7853981634;

    tg3__parse_string(ctx, o, "name", &light->name, 0, "/light");
    tg3__parse_string(ctx, o, "type", &light->type, 1, "/light");
    tg3__parse_double(ctx, o, "intensity", &light->intensity, 0, "/light");
    tg3__parse_double(ctx, o, "range", &light->range, 0, "/light");
    tg3__parse_number_to_fixed(o, "color", light->color, 3);

    auto spot_it = o.find("spot");
    if (spot_it != o.end() && spot_it->is_object()) {
        tg3__parse_double(ctx, *spot_it, "innerConeAngle",
                          &light->spot.inner_cone_angle, 0, "/light/spot");
        tg3__parse_double(ctx, *spot_it, "outerConeAngle",
                          &light->spot.outer_cone_angle, 0, "/light/spot");
        tg3__parse_extras_and_extensions(ctx, *spot_it, &light->spot.ext);
    }

    tg3__parse_extras_and_extensions(ctx, o, &light->ext);
    return 1;
}

static int tg3__parse_audio_source(tg3__parse_ctx *ctx, const tg3__json &o,
                                    tg3_audio_source *src) {
    memset(src, 0, sizeof(tg3_audio_source));
    src->buffer_view = -1;

    tg3__parse_string(ctx, o, "name", &src->name, 0, "/audioSource");
    tg3__parse_string(ctx, o, "uri", &src->uri, 0, "/audioSource");
    tg3__parse_int(ctx, o, "bufferView", &src->buffer_view, 0, "/audioSource");
    tg3__parse_string(ctx, o, "mimeType", &src->mime_type, 0, "/audioSource");
    tg3__parse_extras_and_extensions(ctx, o, &src->ext);
    return 1;
}

static int tg3__parse_audio_emitter(tg3__parse_ctx *ctx, const tg3__json &o,
                                     tg3_audio_emitter *emitter) {
    memset(emitter, 0, sizeof(tg3_audio_emitter));
    emitter->gain = 1.0;
    emitter->source = -1;
    emitter->positional.cone_inner_angle = 6.283185307179586;
    emitter->positional.cone_outer_angle = 6.283185307179586;
    emitter->positional.max_distance = 100.0;
    emitter->positional.ref_distance = 1.0;
    emitter->positional.rolloff_factor = 1.0;

    tg3__parse_string(ctx, o, "name", &emitter->name, 0, "/audioEmitter");
    tg3__parse_double(ctx, o, "gain", &emitter->gain, 0, "/audioEmitter");
    tg3__parse_bool(ctx, o, "loop", &emitter->loop, 0, "/audioEmitter");
    tg3__parse_bool(ctx, o, "playing", &emitter->playing, 0, "/audioEmitter");
    tg3__parse_string(ctx, o, "type", &emitter->type, 0, "/audioEmitter");
    tg3__parse_string(ctx, o, "distanceModel", &emitter->distance_model,
                       0, "/audioEmitter");
    tg3__parse_int(ctx, o, "source", &emitter->source, 0, "/audioEmitter");

    auto pos_it = o.find("positional");
    if (pos_it != o.end() && pos_it->is_object()) {
        tg3__parse_double(ctx, *pos_it, "coneInnerAngle",
                          &emitter->positional.cone_inner_angle, 0, "/positional");
        tg3__parse_double(ctx, *pos_it, "coneOuterAngle",
                          &emitter->positional.cone_outer_angle, 0, "/positional");
        tg3__parse_double(ctx, *pos_it, "coneOuterGain",
                          &emitter->positional.cone_outer_gain, 0, "/positional");
        tg3__parse_double(ctx, *pos_it, "maxDistance",
                          &emitter->positional.max_distance, 0, "/positional");
        tg3__parse_double(ctx, *pos_it, "refDistance",
                          &emitter->positional.ref_distance, 0, "/positional");
        tg3__parse_double(ctx, *pos_it, "rolloffFactor",
                          &emitter->positional.rolloff_factor, 0, "/positional");
        tg3__parse_extras_and_extensions(ctx, *pos_it, &emitter->positional.ext);
    }

    tg3__parse_extras_and_extensions(ctx, o, &emitter->ext);
    return 1;
}

/* ======================================================================
 * Internal: Portable variadic-comma helper
 *
 * TG3__COMMA_VA_ARGS(__VA_ARGS__) expands to  , __VA_ARGS__  when the
 * argument list is non-empty, and to nothing when it is empty.
 *
 * - C++20 and later: uses the standard __VA_OPT__(,) token.
 * - C++17 and earlier: falls back to the widely-supported GNU/MSVC
 *   ##__VA_ARGS__ extension.
 * ====================================================================== */
#if __cplusplus >= 202002L
#  define TG3__COMMA_VA_ARGS(...) __VA_OPT__(,) __VA_ARGS__
#else
#  define TG3__COMMA_VA_ARGS(...) , ##__VA_ARGS__
#endif

/* ======================================================================
 * Internal: Array Parse Macro
 * ====================================================================== */

#define TG3__PARSE_ARRAY(ctx, json_doc, json_key, Type, model_field, count_field, parse_fn, ...) \
    do { \
        auto _arr_it = (json_doc).find(json_key); \
        if (_arr_it != (json_doc).end() && _arr_it->is_array()) { \
            uint32_t _count = (uint32_t)_arr_it->size(); \
            if (_count > 0) { \
                Type *_items = (Type *)tg3__arena_alloc((ctx)->arena, \
                    _count * sizeof(Type)); \
                if (_items) { \
                    uint32_t _i = 0; \
                    for (auto _it = _arr_it->begin(); _it != _arr_it->end(); ++_it, ++_i) { \
                        parse_fn((ctx), *_it, &_items[_i] TG3__COMMA_VA_ARGS(__VA_ARGS__)); \
                    } \
                    (model_field) = _items; \
                    (count_field) = _count; \
                } \
            } \
        } \
    } while (0)

/* Variant without extra args and with index param */
#define TG3__PARSE_ARRAY_IDX(ctx, json_doc, json_key, Type, model_field, count_field, parse_fn) \
    do { \
        auto _arr_it = (json_doc).find(json_key); \
        if (_arr_it != (json_doc).end() && _arr_it->is_array()) { \
            uint32_t _count = (uint32_t)_arr_it->size(); \
            if (_count > 0) { \
                Type *_items = (Type *)tg3__arena_alloc((ctx)->arena, \
                    _count * sizeof(Type)); \
                if (_items) { \
                    uint32_t _i = 0; \
                    for (auto _it = _arr_it->begin(); _it != _arr_it->end(); ++_it, ++_i) { \
                        if (!_it->is_object()) { \
                            tg3__error_pushf((ctx)->errors, (ctx)->arena, \
                                TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH, \
                                json_key, "Element %u must be an object", _i); \
                            continue; \
                        } \
                        parse_fn((ctx), *_it, &_items[_i], (int32_t)_i); \
                    } \
                    (model_field) = _items; \
                    (count_field) = _count; \
                } \
            } \
        } \
    } while (0)

/* Simpler variant for entities without index */
#define TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, json_key, Type, model_field, count_field, parse_fn) \
    do { \
        auto _arr_it = (json_doc).find(json_key); \
        if (_arr_it != (json_doc).end() && _arr_it->is_array()) { \
            uint32_t _count = (uint32_t)_arr_it->size(); \
            if (_count > 0) { \
                Type *_items = (Type *)tg3__arena_alloc((ctx)->arena, \
                    _count * sizeof(Type)); \
                if (_items) { \
                    uint32_t _i = 0; \
                    for (auto _it = _arr_it->begin(); _it != _arr_it->end(); ++_it, ++_i) { \
                        if (!_it->is_object()) { \
                            tg3__error_pushf((ctx)->errors, (ctx)->arena, \
                                TG3_SEVERITY_ERROR, TG3_ERR_JSON_TYPE_MISMATCH, \
                                json_key, "Element %u must be an object", _i); \
                            continue; \
                        } \
                        parse_fn((ctx), *_it, &_items[_i]); \
                    } \
                    (model_field) = _items; \
                    (count_field) = _count; \
                } \
            } \
        } \
    } while (0)

/* ======================================================================
 * Internal: Main Parse Orchestrator
 * ====================================================================== */

static tg3_error_code tg3__parse_from_json(tg3__parse_ctx *ctx,
                                            const tg3__json &json_doc,
                                            tg3_model *model) {
    /* Asset */
    auto asset_it = json_doc.find("asset");
    if (asset_it != json_doc.end() && asset_it->is_object()) {
        tg3__parse_asset(ctx, *asset_it, &model->asset);
    } else if (ctx->opts.required_sections & TG3_REQUIRE_VERSION) {
        tg3__error_push(ctx->errors, TG3_SEVERITY_ERROR, TG3_ERR_MISSING_REQUIRED,
                        "Missing required 'asset' property", "/", -1);
        return TG3_ERR_MISSING_REQUIRED;
    }

    /* Extensions used/required */
    tg3__parse_string_array(ctx, json_doc, "extensionsUsed",
                             &model->extensions_used,
                             &model->extensions_used_count, 0, "/");
    tg3__parse_string_array(ctx, json_doc, "extensionsRequired",
                             &model->extensions_required,
                             &model->extensions_required_count, 0, "/");

    /* Default scene */
    model->default_scene = -1;
    tg3__parse_int(ctx, json_doc, "scene", &model->default_scene, 0, "/");

    /* Streaming callback helper macro */
    #define TG3__STREAM_CB(type_name, cb_name, model_arr, model_cnt) \
        if (ctx->opts.stream && ctx->opts.stream->cb_name) { \
            for (uint32_t _si = 0; _si < model_cnt; ++_si) { \
                tg3_stream_action _sa = ctx->opts.stream->cb_name( \
                    &model_arr[_si], (int32_t)_si, ctx->opts.stream->user_data); \
                if (_sa == TG3_STREAM_ABORT) return TG3_ERR_STREAM_ABORTED; \
            } \
        }

    /* Parse all entity arrays */
    TG3__PARSE_ARRAY_IDX(ctx, json_doc, "buffers", tg3_buffer,
                         model->buffers, model->buffers_count, tg3__parse_buffer);
    TG3__STREAM_CB(buffer, on_buffer, model->buffers, model->buffers_count);

    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "bufferViews", tg3_buffer_view,
                            model->buffer_views, model->buffer_views_count,
                            tg3__parse_buffer_view);
    TG3__STREAM_CB(buffer_view, on_buffer_view, model->buffer_views,
                   model->buffer_views_count);

    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "accessors", tg3_accessor,
                            model->accessors, model->accessors_count,
                            tg3__parse_accessor);
    TG3__STREAM_CB(accessor, on_accessor, model->accessors, model->accessors_count);

    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "meshes", tg3_mesh,
                            model->meshes, model->meshes_count, tg3__parse_mesh);
    TG3__STREAM_CB(mesh, on_mesh, model->meshes, model->meshes_count);

    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "nodes", tg3_node,
                            model->nodes, model->nodes_count, tg3__parse_node);
    TG3__STREAM_CB(node, on_node, model->nodes, model->nodes_count);

    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "materials", tg3_material,
                            model->materials, model->materials_count,
                            tg3__parse_material);
    TG3__STREAM_CB(material, on_material, model->materials, model->materials_count);

    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "textures", tg3_texture,
                            model->textures, model->textures_count,
                            tg3__parse_texture);
    TG3__STREAM_CB(texture, on_texture, model->textures, model->textures_count);

    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "samplers", tg3_sampler,
                            model->samplers, model->samplers_count,
                            tg3__parse_sampler);
    TG3__STREAM_CB(sampler, on_sampler, model->samplers, model->samplers_count);

    TG3__PARSE_ARRAY_IDX(ctx, json_doc, "images", tg3_image,
                         model->images, model->images_count, tg3__parse_image);
    TG3__STREAM_CB(image, on_image, model->images, model->images_count);

    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "skins", tg3_skin,
                            model->skins, model->skins_count, tg3__parse_skin);
    TG3__STREAM_CB(skin, on_skin, model->skins, model->skins_count);

    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "animations", tg3_animation,
                            model->animations, model->animations_count,
                            tg3__parse_animation);
    TG3__STREAM_CB(animation, on_animation, model->animations,
                   model->animations_count);

    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "cameras", tg3_camera,
                            model->cameras, model->cameras_count,
                            tg3__parse_camera);
    TG3__STREAM_CB(camera, on_camera, model->cameras, model->cameras_count);

    TG3__PARSE_ARRAY_SIMPLE(ctx, json_doc, "scenes", tg3_scene,
                            model->scenes, model->scenes_count,
                            tg3__parse_scene);
    TG3__STREAM_CB(scene, on_scene, model->scenes, model->scenes_count);

    /* KHR_lights_punctual */
    auto ext_it = json_doc.find("extensions");
    if (ext_it != json_doc.end() && ext_it->is_object()) {
        auto lights_ext = ext_it->find("KHR_lights_punctual");
        if (lights_ext != ext_it->end() && lights_ext->is_object()) {
            TG3__PARSE_ARRAY_SIMPLE(ctx, *lights_ext, "lights", tg3_light,
                                    model->lights, model->lights_count,
                                    tg3__parse_light);
            TG3__STREAM_CB(light, on_light, model->lights, model->lights_count);
        }

        /* KHR_audio */
        auto audio_ext = ext_it->find("KHR_audio");
        if (audio_ext != ext_it->end() && audio_ext->is_object()) {
            TG3__PARSE_ARRAY_SIMPLE(ctx, *audio_ext, "sources", tg3_audio_source,
                                    model->audio_sources, model->audio_sources_count,
                                    tg3__parse_audio_source);
            TG3__PARSE_ARRAY_SIMPLE(ctx, *audio_ext, "emitters", tg3_audio_emitter,
                                    model->audio_emitters, model->audio_emitters_count,
                                    tg3__parse_audio_emitter);
        }
    }

    /* Root extras/extensions */
    tg3__parse_extras_and_extensions(ctx, json_doc, &model->ext);

    #undef TG3__STREAM_CB

    return ctx->errors->has_error ? TG3_ERR_JSON_PARSE : TG3_OK;
}

/* ======================================================================
 * Internal: GLB Parsing
 * ====================================================================== */

static tg3_error_code tg3__parse_glb_header(const uint8_t *data, uint64_t size,
                                             const uint8_t **json_out,
                                             uint64_t *json_size_out,
                                             const uint8_t **bin_out,
                                             uint64_t *bin_size_out,
                                             tg3_error_stack *errors) {
    *json_out = NULL; *json_size_out = 0;
    *bin_out = NULL; *bin_size_out = 0;

    if (size < 12) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_INVALID_HEADER,
                        "GLB data too small for header", NULL, -1);
        return TG3_ERR_GLB_INVALID_HEADER;
    }

    /* Check magic: 'glTF' */
    if (data[0] != 'g' || data[1] != 'l' || data[2] != 'T' || data[3] != 'F') {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_INVALID_MAGIC,
                        "Invalid GLB magic bytes", NULL, -1);
        return TG3_ERR_GLB_INVALID_MAGIC;
    }

    /* Version */
    uint32_t version;
    memcpy(&version, data + 4, 4);
    if (version != 2) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_INVALID_VERSION,
                        "Unsupported GLB version (expected 2)", NULL, -1);
        return TG3_ERR_GLB_INVALID_VERSION;
    }

    /* Total length */
    uint32_t total_length;
    memcpy(&total_length, data + 8, 4);
    if ((uint64_t)total_length > size) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_SIZE_MISMATCH,
                        "GLB total length exceeds data size", NULL, -1);
        return TG3_ERR_GLB_SIZE_MISMATCH;
    }

    if (total_length < 20) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_INVALID_HEADER,
                        "GLB too small for JSON chunk header", NULL, -1);
        return TG3_ERR_GLB_INVALID_HEADER;
    }

    /* Chunk 0: JSON */
    uint32_t chunk0_length, chunk0_type;
    memcpy(&chunk0_length, data + 12, 4);
    memcpy(&chunk0_type, data + 16, 4);

    if (chunk0_type != 0x4E4F534A) { /* 'JSON' in LE */
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_CHUNK_ERROR,
                        "First GLB chunk is not JSON", NULL, -1);
        return TG3_ERR_GLB_CHUNK_ERROR;
    }

    if (20 + (uint64_t)chunk0_length > total_length) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_GLB_CHUNK_ERROR,
                        "JSON chunk length exceeds GLB size", NULL, -1);
        return TG3_ERR_GLB_CHUNK_ERROR;
    }

    *json_out = data + 20;
    *json_size_out = chunk0_length;

    /* Chunk 1: BIN (optional) */
    uint64_t bin_offset = 20 + (uint64_t)chunk0_length;
    /* Align to 4 bytes */
    bin_offset = (bin_offset + 3) & ~(uint64_t)3;

    if (bin_offset + 8 <= total_length) {
        uint32_t chunk1_length, chunk1_type;
        memcpy(&chunk1_length, data + bin_offset, 4);
        memcpy(&chunk1_type, data + bin_offset + 4, 4);

        if (chunk1_type == 0x004E4942) { /* 'BIN\0' in LE */
            if (bin_offset + 8 + chunk1_length <= total_length) {
                *bin_out = data + bin_offset + 8;
                *bin_size_out = chunk1_length;
            }
        }
    }

    return TG3_OK;
}

/* ======================================================================
 * Optional: Default FS Callbacks
 * ====================================================================== */

#ifdef TINYGLTF3_ENABLE_FS

static int32_t tg3__fs_file_exists(const char *path, uint32_t path_len,
                                    void *ud) {
    (void)ud; (void)path_len;
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

static int32_t tg3__fs_read_file(uint8_t **out_data, uint64_t *out_size,
                                  const char *path, uint32_t path_len,
                                  void *ud) {
    (void)ud; (void)path_len;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return 0; }

    uint8_t *data = (uint8_t *)malloc((size_t)sz);
    if (!data) { fclose(f); return 0; }

    size_t read = fread(data, 1, (size_t)sz, f);
    fclose(f);

    if ((long)read != sz) { free(data); return 0; }

    *out_data = data;
    *out_size = (uint64_t)sz;
    return 1;
}

static void tg3__fs_free_file(uint8_t *data, uint64_t size, void *ud) {
    (void)size; (void)ud;
    free(data);
}

static int32_t tg3__fs_write_file(const char *path, uint32_t path_len,
                                   const uint8_t *data, uint64_t size,
                                   void *ud) {
    (void)ud; (void)path_len;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    size_t written = fwrite(data, 1, (size_t)size, f);
    fclose(f);
    return (written == (size_t)size) ? 1 : 0;
}

static void tg3__set_default_fs(tg3_fs_callbacks *fs) {
    if (!fs->read_file) fs->read_file = tg3__fs_read_file;
    if (!fs->free_file) fs->free_file = tg3__fs_free_file;
    if (!fs->file_exists) fs->file_exists = tg3__fs_file_exists;
    if (!fs->write_file) fs->write_file = tg3__fs_write_file;
}

#endif /* TINYGLTF3_ENABLE_FS */

/* ======================================================================
 * Internal: Model Init Helper
 * ====================================================================== */

static void tg3__model_init(tg3_model *model) {
    memset(model, 0, sizeof(tg3_model));
    model->default_scene = -1;
}

/* ======================================================================
 * Public: Parser API Implementation
 * ====================================================================== */

TINYGLTF3_API tg3_error_code tg3_parse(
    tg3_model *model, tg3_error_stack *errors,
    const uint8_t *json_data, uint64_t json_size,
    const char *base_dir, uint32_t base_dir_len,
    const tg3_parse_options *options) {

    tg3_parse_options default_opts;
    if (!options) {
        tg3_parse_options_init(&default_opts);
        options = &default_opts;
    }

    tg3__model_init(model);

    tg3_arena *arena = tg3__arena_create(&options->memory);
    if (!arena) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                        "Failed to create arena", NULL, -1);
        return TG3_ERR_OUT_OF_MEMORY;
    }
    model->arena_ = arena;

    /* Parse JSON */
    tg3__json json_doc = options->parse_float32
        ? tg3__json::parse_float32(
              (const char *)json_data, (const char *)json_data + json_size)
        : tg3__json::parse(
              (const char *)json_data, (const char *)json_data + json_size,
              nullptr, false);

    if (json_doc.is_null()) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_JSON_PARSE,
                        "Failed to parse JSON", NULL, -1);
        return TG3_ERR_JSON_PARSE;
    }

    if (!json_doc.is_object()) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_JSON_PARSE,
                        "JSON root must be an object", NULL, -1);
        return TG3_ERR_JSON_PARSE;
    }

    tg3__parse_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.arena = arena;
    ctx.errors = errors;
    ctx.opts = *options;
    ctx.base_dir = base_dir;
    ctx.base_dir_len = base_dir_len;
    ctx.is_binary = 0;

#ifdef TINYGLTF3_ENABLE_FS
    tg3__set_default_fs(&ctx.opts.fs);
#endif

    return tg3__parse_from_json(&ctx, json_doc, model);
}

TINYGLTF3_API tg3_error_code tg3_parse_glb(
    tg3_model *model, tg3_error_stack *errors,
    const uint8_t *glb_data, uint64_t glb_size,
    const char *base_dir, uint32_t base_dir_len,
    const tg3_parse_options *options) {

    const uint8_t *json_chunk = NULL;
    uint64_t json_chunk_size = 0;
    const uint8_t *bin_chunk = NULL;
    uint64_t bin_chunk_size = 0;

    tg3_error_code err = tg3__parse_glb_header(glb_data, glb_size,
                                                &json_chunk, &json_chunk_size,
                                                &bin_chunk, &bin_chunk_size,
                                                errors);
    if (err != TG3_OK) return err;

    tg3_parse_options default_opts;
    if (!options) {
        tg3_parse_options_init(&default_opts);
        options = &default_opts;
    }

    tg3__model_init(model);

    tg3_arena *arena = tg3__arena_create(&options->memory);
    if (!arena) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                        "Failed to create arena", NULL, -1);
        return TG3_ERR_OUT_OF_MEMORY;
    }
    model->arena_ = arena;

    /* Parse JSON chunk */
    tg3__json json_doc = options->parse_float32
        ? tg3__json::parse_float32(
              (const char *)json_chunk, (const char *)json_chunk + json_chunk_size)
        : tg3__json::parse(
              (const char *)json_chunk, (const char *)json_chunk + json_chunk_size,
              nullptr, false);

    if (json_doc.is_null() || !json_doc.is_object()) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_JSON_PARSE,
                        "Failed to parse GLB JSON chunk", NULL, -1);
        return TG3_ERR_JSON_PARSE;
    }

    tg3__parse_ctx ctx;
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

    return tg3__parse_from_json(&ctx, json_doc, model);
}

TINYGLTF3_API tg3_error_code tg3_parse_auto(
    tg3_model *model, tg3_error_stack *errors,
    const uint8_t *data, uint64_t size,
    const char *base_dir, uint32_t base_dir_len,
    const tg3_parse_options *options) {

    /* Check for GLB magic */
    if (size >= 4 && data[0] == 'g' && data[1] == 'l' &&
        data[2] == 'T' && data[3] == 'F') {
        return tg3_parse_glb(model, errors, data, size,
                              base_dir, base_dir_len, options);
    }
    return tg3_parse(model, errors, data, size,
                      base_dir, base_dir_len, options);
}

TINYGLTF3_API tg3_error_code tg3_parse_file(
    tg3_model *model, tg3_error_stack *errors,
    const char *filename, uint32_t filename_len,
    const tg3_parse_options *options) {

    tg3_parse_options opts;
    if (options) {
        opts = *options;
    } else {
        tg3_parse_options_init(&opts);
    }

#ifdef TINYGLTF3_ENABLE_FS
    tg3__set_default_fs(&opts.fs);
#endif

    if (!opts.fs.read_file) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_FS_NOT_AVAILABLE,
                        "No filesystem callbacks. Define TINYGLTF3_ENABLE_FS "
                        "or provide fs callbacks.", NULL, -1);
        return TG3_ERR_FS_NOT_AVAILABLE;
    }

    /* Read file */
    uint8_t *file_data = NULL;
    uint64_t file_size = 0;
    int32_t ok = opts.fs.read_file(&file_data, &file_size, filename,
                                    filename_len, opts.fs.user_data);
    if (!ok || !file_data) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_FILE_NOT_FOUND,
                        "Failed to read file", NULL, -1);
        return TG3_ERR_FILE_NOT_FOUND;
    }

    /* Extract base directory */
    char base_dir_buf[4096] = {0};
    uint32_t base_dir_len = 0;
    if (filename && filename_len > 0) {
        /* Find last separator */
        const char *last_sep = NULL;
        for (uint32_t i = 0; i < filename_len; ++i) {
            if (filename[i] == '/' || filename[i] == '\\') {
                last_sep = filename + i;
            }
        }
        if (last_sep) {
            base_dir_len = (uint32_t)(last_sep - filename);
            if (base_dir_len >= sizeof(base_dir_buf))
                base_dir_len = (uint32_t)(sizeof(base_dir_buf) - 1);
            memcpy(base_dir_buf, filename, base_dir_len);
            base_dir_buf[base_dir_len] = '\0';
        }
    }

    tg3_error_code result = tg3_parse_auto(model, errors, file_data, file_size,
                                            base_dir_buf, base_dir_len, &opts);

    /* Free file data */
    if (opts.fs.free_file) {
        opts.fs.free_file(file_data, file_size, opts.fs.user_data);
    }

    return result;
}

TINYGLTF3_API void tg3_model_free(tg3_model *model) {
    if (!model) return;
    if (model->arena_) {
        tg3__arena_destroy(model->arena_);
    }
    memset(model, 0, sizeof(tg3_model));
    model->default_scene = -1;
}

/* ======================================================================
 * Internal: JSON Serialization Helpers
 * ====================================================================== */

static void tg3__serialize_str(tg3__json &o, const char *key, tg3_str s) {
    if (s.data && s.len > 0) {
        o[key] = std::string(s.data, s.len);
    }
}

static void tg3__serialize_int(tg3__json &o, const char *key, int32_t val,
                                int32_t default_val, int write_defaults) {
    if (val != default_val || write_defaults) {
        o[key] = val;
    }
}

static void tg3__serialize_uint64(tg3__json &o, const char *key, uint64_t val,
                                   uint64_t default_val, int write_defaults) {
    if (val != default_val || write_defaults) {
        o[key] = (int64_t)val;
    }
}

static void tg3__serialize_double(tg3__json &o, const char *key, double val,
                                   double default_val, int write_defaults) {
    if (fabs(val - default_val) > 1e-12 || write_defaults) {
        o[key] = val;
    }
}

static void tg3__serialize_bool(tg3__json &o, const char *key, int32_t val,
                                 int32_t default_val, int write_defaults) {
    if (val != default_val || write_defaults) {
        o[key] = (val != 0);
    }
}

static void tg3__serialize_double_array(tg3__json &o, const char *key,
                                         const double *arr, uint32_t count) {
    if (!arr || count == 0) return;
    tg3__json jarr;
    jarr.set_array();
    for (uint32_t i = 0; i < count; ++i) {
        jarr.push_back(tg3__json(arr[i]));
    }
    o[key] = static_cast<tg3__json&&>(jarr);
}

static void tg3__serialize_int_array(tg3__json &o, const char *key,
                                      const int32_t *arr, uint32_t count) {
    if (!arr || count == 0) return;
    tg3__json jarr;
    jarr.set_array();
    for (uint32_t i = 0; i < count; ++i) {
        jarr.push_back(tg3__json(arr[i]));
    }
    o[key] = static_cast<tg3__json&&>(jarr);
}

static void tg3__serialize_string_array(tg3__json &o, const char *key,
                                         const tg3_str *arr, uint32_t count) {
    if (!arr || count == 0) return;
    tg3__json jarr;
    jarr.set_array();
    for (uint32_t i = 0; i < count; ++i) {
        if (arr[i].data) {
            jarr.push_back(tg3__json(std::string(arr[i].data, arr[i].len)));
        }
    }
    o[key] = static_cast<tg3__json&&>(jarr);
}

static tg3__json tg3__value_to_json(const tg3_value *v) {
    if (!v) return tg3__json();
    switch (v->type) {
        case TG3_VALUE_NULL:   return tg3__json();
        case TG3_VALUE_BOOL:   return tg3__json(v->bool_val != 0);
        case TG3_VALUE_INT:    return tg3__json(v->int_val);
        case TG3_VALUE_REAL:   return tg3__json(v->real_val);
        case TG3_VALUE_STRING:
            return tg3__json(std::string(v->string_val.data ? v->string_val.data : "",
                                          v->string_val.len));
        case TG3_VALUE_ARRAY: {
            tg3__json arr;
            arr.set_array();
            for (uint32_t i = 0; i < v->array_count; ++i) {
                arr.push_back(tg3__value_to_json(&v->array_data[i]));
            }
            return arr;
        }
        case TG3_VALUE_OBJECT: {
            tg3__json obj = tg3__json::object();
            for (uint32_t i = 0; i < v->object_count; ++i) {
                std::string k(v->object_data[i].key.data ? v->object_data[i].key.data : "",
                              v->object_data[i].key.len);
                obj[k.c_str()] = tg3__value_to_json(&v->object_data[i].value);
            }
            return obj;
        }
        default: return tg3__json();
    }
}

static void tg3__serialize_extras_ext(tg3__json &o, const tg3_extras_ext *ee) {
    if (!ee) return;
    if (ee->extras) {
        o["extras"] = tg3__value_to_json(ee->extras);
    }
    if (ee->extensions && ee->extensions_count > 0) {
        tg3__json exts = tg3__json::object();
        for (uint32_t i = 0; i < ee->extensions_count; ++i) {
            std::string name(ee->extensions[i].name.data ? ee->extensions[i].name.data : "",
                             ee->extensions[i].name.len);
            exts[name.c_str()] = tg3__value_to_json(&ee->extensions[i].value);
        }
        o["extensions"] = static_cast<tg3__json&&>(exts);
    }
}

/* ======================================================================
 * Internal: Entity Serialize Functions
 * ====================================================================== */

static void tg3__serialize_texture_info(tg3__json &parent, const char *key,
                                         const tg3_texture_info *ti, int wd) {
    if (ti->index < 0) return;
    tg3__json o = tg3__json::object();
    o["index"] = ti->index;
    tg3__serialize_int(o, "texCoord", ti->tex_coord, 0, wd);
    tg3__serialize_extras_ext(o, &ti->ext);
    parent[key] = static_cast<tg3__json&&>(o);
}

static void tg3__serialize_normal_texture_info(tg3__json &parent, const char *key,
                                                const tg3_normal_texture_info *ti,
                                                int wd) {
    if (ti->index < 0) return;
    tg3__json o = tg3__json::object();
    o["index"] = ti->index;
    tg3__serialize_int(o, "texCoord", ti->tex_coord, 0, wd);
    tg3__serialize_double(o, "scale", ti->scale, 1.0, wd);
    tg3__serialize_extras_ext(o, &ti->ext);
    parent[key] = static_cast<tg3__json&&>(o);
}

static void tg3__serialize_occlusion_texture_info(tg3__json &parent, const char *key,
                                                   const tg3_occlusion_texture_info *ti,
                                                   int wd) {
    if (ti->index < 0) return;
    tg3__json o = tg3__json::object();
    o["index"] = ti->index;
    tg3__serialize_int(o, "texCoord", ti->tex_coord, 0, wd);
    tg3__serialize_double(o, "strength", ti->strength, 1.0, wd);
    tg3__serialize_extras_ext(o, &ti->ext);
    parent[key] = static_cast<tg3__json&&>(o);
}

static tg3__json tg3__serialize_asset(const tg3_asset *a, int wd) {
    (void)wd;
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "version", a->version);
    tg3__serialize_str(o, "generator", a->generator);
    tg3__serialize_str(o, "minVersion", a->min_version);
    tg3__serialize_str(o, "copyright", a->copyright);
    tg3__serialize_extras_ext(o, &a->ext);
    return o;
}

static tg3__json tg3__serialize_buffer(const tg3_buffer *b, int wd,
                                        int embed) {
    (void)wd;
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", b->name);
    o["byteLength"] = (int64_t)b->data.count;

    if (b->uri.data && b->uri.len > 0) {
        tg3__serialize_str(o, "uri", b->uri);
    } else if (embed && b->data.data && b->data.count > 0) {
        /* Encode as data URI */
        size_t enc_len = 0;
        char *encoded = tg3__b64_encode(b->data.data, (size_t)b->data.count,
                                         &enc_len);
        if (encoded) {
            std::string uri = "data:application/octet-stream;base64,";
            uri.append(encoded, enc_len);
            o["uri"] = uri;
            free(encoded);
        }
    }

    tg3__serialize_extras_ext(o, &b->ext);
    return o;
}

static tg3__json tg3__serialize_buffer_view(const tg3_buffer_view *bv, int wd) {
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", bv->name);
    o["buffer"] = bv->buffer;
    o["byteLength"] = (int64_t)bv->byte_length;
    tg3__serialize_uint64(o, "byteOffset", bv->byte_offset, 0, wd);
    if (bv->byte_stride > 0) o["byteStride"] = (int)bv->byte_stride;
    tg3__serialize_int(o, "target", bv->target, 0, wd);
    tg3__serialize_extras_ext(o, &bv->ext);
    return o;
}

static tg3__json tg3__serialize_accessor(const tg3_accessor *acc, int wd) {
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", acc->name);
    if (acc->buffer_view >= 0) o["bufferView"] = acc->buffer_view;
    tg3__serialize_uint64(o, "byteOffset", acc->byte_offset, 0, wd);
    o["componentType"] = acc->component_type;
    o["count"] = (int64_t)acc->count;
    o["type"] = tg3__accessor_type_to_string(acc->type);
    tg3__serialize_bool(o, "normalized", acc->normalized, 0, wd);

    tg3__serialize_double_array(o, "min", acc->min_values, acc->min_values_count);
    tg3__serialize_double_array(o, "max", acc->max_values, acc->max_values_count);

    if (acc->sparse.is_sparse) {
        tg3__json sparse = tg3__json::object();
        sparse["count"] = acc->sparse.count;

        tg3__json indices = tg3__json::object();
        indices["bufferView"] = acc->sparse.indices.buffer_view;
        indices["componentType"] = acc->sparse.indices.component_type;
        tg3__serialize_uint64(indices, "byteOffset",
                              acc->sparse.indices.byte_offset, 0, wd);
        tg3__serialize_extras_ext(indices, &acc->sparse.indices.ext);
        sparse["indices"] = static_cast<tg3__json&&>(indices);

        tg3__json values = tg3__json::object();
        values["bufferView"] = acc->sparse.values.buffer_view;
        tg3__serialize_uint64(values, "byteOffset",
                              acc->sparse.values.byte_offset, 0, wd);
        tg3__serialize_extras_ext(values, &acc->sparse.values.ext);
        sparse["values"] = static_cast<tg3__json&&>(values);

        tg3__serialize_extras_ext(sparse, &acc->sparse.ext);
        o["sparse"] = static_cast<tg3__json&&>(sparse);
    }

    tg3__serialize_extras_ext(o, &acc->ext);
    return o;
}

static tg3__json tg3__serialize_image(const tg3_image *img, int wd, int embed) {
    (void)wd; (void)embed;
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", img->name);
    tg3__serialize_str(o, "uri", img->uri);
    tg3__serialize_str(o, "mimeType", img->mime_type);
    if (img->buffer_view >= 0) o["bufferView"] = img->buffer_view;
    tg3__serialize_extras_ext(o, &img->ext);
    return o;
}

static tg3__json tg3__serialize_sampler(const tg3_sampler *s, int wd) {
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", s->name);
    if (s->min_filter >= 0) o["minFilter"] = s->min_filter;
    if (s->mag_filter >= 0) o["magFilter"] = s->mag_filter;
    tg3__serialize_int(o, "wrapS", s->wrap_s, TG3_TEXTURE_WRAP_REPEAT, wd);
    tg3__serialize_int(o, "wrapT", s->wrap_t, TG3_TEXTURE_WRAP_REPEAT, wd);
    tg3__serialize_extras_ext(o, &s->ext);
    return o;
}

static tg3__json tg3__serialize_texture(const tg3_texture *t, int wd) {
    (void)wd;
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", t->name);
    if (t->sampler >= 0) o["sampler"] = t->sampler;
    if (t->source >= 0) o["source"] = t->source;
    tg3__serialize_extras_ext(o, &t->ext);
    return o;
}

static tg3__json tg3__serialize_material(const tg3_material *m, int wd) {
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", m->name);

    /* PBR */
    tg3__json pbr = tg3__json::object();
    int has_pbr = 0;

    const tg3_pbr_metallic_roughness *p = &m->pbr_metallic_roughness;
    if (p->base_color_factor[0] != 1.0 || p->base_color_factor[1] != 1.0 ||
        p->base_color_factor[2] != 1.0 || p->base_color_factor[3] != 1.0 || wd) {
        tg3__serialize_double_array(pbr, "baseColorFactor",
                                    p->base_color_factor, 4);
        has_pbr = 1;
    }
    tg3__serialize_double(pbr, "metallicFactor", p->metallic_factor, 1.0, wd);
    if (fabs(p->metallic_factor - 1.0) > 1e-12 || wd) has_pbr = 1;
    tg3__serialize_double(pbr, "roughnessFactor", p->roughness_factor, 1.0, wd);
    if (fabs(p->roughness_factor - 1.0) > 1e-12 || wd) has_pbr = 1;

    if (p->base_color_texture.index >= 0) {
        tg3__serialize_texture_info(pbr, "baseColorTexture",
                                    &p->base_color_texture, wd);
        has_pbr = 1;
    }
    if (p->metallic_roughness_texture.index >= 0) {
        tg3__serialize_texture_info(pbr, "metallicRoughnessTexture",
                                    &p->metallic_roughness_texture, wd);
        has_pbr = 1;
    }
    tg3__serialize_extras_ext(pbr, &p->ext);
    if (has_pbr || wd) {
        o["pbrMetallicRoughness"] = static_cast<tg3__json&&>(pbr);
    }

    tg3__serialize_normal_texture_info(o, "normalTexture", &m->normal_texture, wd);
    tg3__serialize_occlusion_texture_info(o, "occlusionTexture",
                                           &m->occlusion_texture, wd);
    tg3__serialize_texture_info(o, "emissiveTexture", &m->emissive_texture, wd);

    if (m->emissive_factor[0] != 0.0 || m->emissive_factor[1] != 0.0 ||
        m->emissive_factor[2] != 0.0 || wd) {
        tg3__serialize_double_array(o, "emissiveFactor", m->emissive_factor, 3);
    }

    if (m->alpha_mode.data && !tg3_str_equals_cstr(m->alpha_mode, "OPAQUE")) {
        tg3__serialize_str(o, "alphaMode", m->alpha_mode);
    }
    tg3__serialize_double(o, "alphaCutoff", m->alpha_cutoff, 0.5, wd);
    tg3__serialize_bool(o, "doubleSided", m->double_sided, 0, wd);

    tg3__serialize_extras_ext(o, &m->ext);
    return o;
}

static tg3__json tg3__serialize_primitive(const tg3_primitive *p, int wd) {
    tg3__json o = tg3__json::object();

    /* Attributes */
    if (p->attributes && p->attributes_count > 0) {
        tg3__json attrs = tg3__json::object();
        for (uint32_t i = 0; i < p->attributes_count; ++i) {
            std::string k(p->attributes[i].key.data ? p->attributes[i].key.data : "",
                          p->attributes[i].key.len);
            attrs[k.c_str()] = p->attributes[i].value;
        }
        o["attributes"] = static_cast<tg3__json&&>(attrs);
    }

    if (p->indices >= 0) o["indices"] = p->indices;
    if (p->material >= 0) o["material"] = p->material;
    tg3__serialize_int(o, "mode", p->mode, TG3_MODE_TRIANGLES, wd);

    /* Morph targets */
    if (p->targets && p->targets_count > 0) {
        tg3__json targets;
        targets.set_array();
        for (uint32_t t = 0; t < p->targets_count; ++t) {
            tg3__json tgt = tg3__json::object();
            uint32_t acount = p->target_attribute_counts ?
                              p->target_attribute_counts[t] : 0;
            const tg3_str_int_pair *tattrs = p->targets[t];
            for (uint32_t a = 0; a < acount; ++a) {
                std::string k(tattrs[a].key.data ? tattrs[a].key.data : "",
                              tattrs[a].key.len);
                tgt[k.c_str()] = tattrs[a].value;
            }
            targets.push_back(static_cast<tg3__json&&>(tgt));
        }
        o["targets"] = static_cast<tg3__json&&>(targets);
    }

    tg3__serialize_extras_ext(o, &p->ext);
    return o;
}

static tg3__json tg3__serialize_mesh(const tg3_mesh *m, int wd) {
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", m->name);

    if (m->primitives && m->primitives_count > 0) {
        tg3__json prims;
        prims.set_array();
        for (uint32_t i = 0; i < m->primitives_count; ++i) {
            prims.push_back(tg3__serialize_primitive(&m->primitives[i], wd));
        }
        o["primitives"] = static_cast<tg3__json&&>(prims);
    }

    tg3__serialize_double_array(o, "weights", m->weights, m->weights_count);
    tg3__serialize_extras_ext(o, &m->ext);
    return o;
}

static tg3__json tg3__serialize_node(const tg3_node *n, int wd) {
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", n->name);

    if (n->camera >= 0) o["camera"] = n->camera;
    if (n->skin >= 0) o["skin"] = n->skin;
    if (n->mesh >= 0) o["mesh"] = n->mesh;

    tg3__serialize_int_array(o, "children", n->children, n->children_count);

    if (n->has_matrix) {
        tg3__serialize_double_array(o, "matrix", n->matrix, 16);
    } else {
        int has_t = (n->translation[0] != 0.0 || n->translation[1] != 0.0 ||
                     n->translation[2] != 0.0);
        int has_r = (n->rotation[0] != 0.0 || n->rotation[1] != 0.0 ||
                     n->rotation[2] != 0.0 || n->rotation[3] != 1.0);
        int has_s = (n->scale[0] != 1.0 || n->scale[1] != 1.0 ||
                     n->scale[2] != 1.0);

        if (has_t || wd) tg3__serialize_double_array(o, "translation", n->translation, 3);
        if (has_r || wd) tg3__serialize_double_array(o, "rotation", n->rotation, 4);
        if (has_s || wd) tg3__serialize_double_array(o, "scale", n->scale, 3);
    }

    tg3__serialize_double_array(o, "weights", n->weights, n->weights_count);

    /* Extensions for lights / audio / lod */
    int has_ext = (n->light >= 0 || n->emitter >= 0 ||
                   (n->lods && n->lods_count > 0));
    if (has_ext) {
        /* Check if extensions already set by extras_ext */
        auto existing = o.find("extensions");
        tg3__json exts = (existing != o.end()) ?
                         tg3__json(*existing) : tg3__json::object();

        if (n->light >= 0) {
            tg3__json lp = tg3__json::object();
            lp["light"] = n->light;
            exts["KHR_lights_punctual"] = static_cast<tg3__json&&>(lp);
        }
        if (n->emitter >= 0) {
            tg3__json ae = tg3__json::object();
            ae["emitter"] = n->emitter;
            exts["KHR_audio"] = static_cast<tg3__json&&>(ae);
        }
        if (n->lods && n->lods_count > 0) {
            tg3__json lod = tg3__json::object();
            tg3__serialize_int_array(lod, "ids", n->lods, n->lods_count);
            exts["MSFT_lod"] = static_cast<tg3__json&&>(lod);
        }
        o["extensions"] = static_cast<tg3__json&&>(exts);
    }

    tg3__serialize_extras_ext(o, &n->ext);
    return o;
}

static tg3__json tg3__serialize_skin(const tg3_skin *s, int wd) {
    (void)wd;
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", s->name);
    if (s->inverse_bind_matrices >= 0) o["inverseBindMatrices"] = s->inverse_bind_matrices;
    if (s->skeleton >= 0) o["skeleton"] = s->skeleton;
    tg3__serialize_int_array(o, "joints", s->joints, s->joints_count);
    tg3__serialize_extras_ext(o, &s->ext);
    return o;
}

static tg3__json tg3__serialize_animation(const tg3_animation *a, int wd) {
    (void)wd;
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", a->name);

    if (a->channels && a->channels_count > 0) {
        tg3__json channels;
        channels.set_array();
        for (uint32_t i = 0; i < a->channels_count; ++i) {
            tg3__json ch = tg3__json::object();
            ch["sampler"] = a->channels[i].sampler;
            tg3__json tgt = tg3__json::object();
            if (a->channels[i].target.node >= 0)
                tgt["node"] = a->channels[i].target.node;
            tg3__serialize_str(tgt, "path", a->channels[i].target.path);
            tg3__serialize_extras_ext(tgt, &a->channels[i].target.ext);
            ch["target"] = static_cast<tg3__json&&>(tgt);
            tg3__serialize_extras_ext(ch, &a->channels[i].ext);
            channels.push_back(static_cast<tg3__json&&>(ch));
        }
        o["channels"] = static_cast<tg3__json&&>(channels);
    }

    if (a->samplers && a->samplers_count > 0) {
        tg3__json samplers;
        samplers.set_array();
        for (uint32_t i = 0; i < a->samplers_count; ++i) {
            tg3__json s = tg3__json::object();
            s["input"] = a->samplers[i].input;
            s["output"] = a->samplers[i].output;
            tg3__serialize_str(s, "interpolation", a->samplers[i].interpolation);
            tg3__serialize_extras_ext(s, &a->samplers[i].ext);
            samplers.push_back(static_cast<tg3__json&&>(s));
        }
        o["samplers"] = static_cast<tg3__json&&>(samplers);
    }

    tg3__serialize_extras_ext(o, &a->ext);
    return o;
}

static tg3__json tg3__serialize_camera(const tg3_camera *c, int wd) {
    (void)wd;
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", c->name);
    tg3__serialize_str(o, "type", c->type);

    if (c->type.data && tg3_str_equals_cstr(c->type, "perspective")) {
        tg3__json p = tg3__json::object();
        if (c->perspective.aspect_ratio > 0)
            p["aspectRatio"] = c->perspective.aspect_ratio;
        p["yfov"] = c->perspective.yfov;
        if (c->perspective.zfar > 0) p["zfar"] = c->perspective.zfar;
        p["znear"] = c->perspective.znear;
        tg3__serialize_extras_ext(p, &c->perspective.ext);
        o["perspective"] = static_cast<tg3__json&&>(p);
    } else if (c->type.data && tg3_str_equals_cstr(c->type, "orthographic")) {
        tg3__json orth = tg3__json::object();
        orth["xmag"] = c->orthographic.xmag;
        orth["ymag"] = c->orthographic.ymag;
        orth["zfar"] = c->orthographic.zfar;
        orth["znear"] = c->orthographic.znear;
        tg3__serialize_extras_ext(orth, &c->orthographic.ext);
        o["orthographic"] = static_cast<tg3__json&&>(orth);
    }

    tg3__serialize_extras_ext(o, &c->ext);
    return o;
}

static tg3__json tg3__serialize_scene(const tg3_scene *s, int wd) {
    (void)wd;
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", s->name);
    tg3__serialize_int_array(o, "nodes", s->nodes, s->nodes_count);
    tg3__serialize_extras_ext(o, &s->ext);
    return o;
}

static tg3__json tg3__serialize_light(const tg3_light *l, int wd) {
    tg3__json o = tg3__json::object();
    tg3__serialize_str(o, "name", l->name);
    tg3__serialize_str(o, "type", l->type);
    tg3__serialize_double(o, "intensity", l->intensity, 1.0, wd);
    tg3__serialize_double(o, "range", l->range, 0.0, wd);

    if (l->color[0] != 1.0 || l->color[1] != 1.0 || l->color[2] != 1.0 || wd) {
        tg3__serialize_double_array(o, "color", l->color, 3);
    }

    if (l->type.data && tg3_str_equals_cstr(l->type, "spot")) {
        tg3__json spot = tg3__json::object();
        tg3__serialize_double(spot, "innerConeAngle", l->spot.inner_cone_angle, 0.0, wd);
        tg3__serialize_double(spot, "outerConeAngle", l->spot.outer_cone_angle,
                              0.7853981634, wd);
        tg3__serialize_extras_ext(spot, &l->spot.ext);
        o["spot"] = static_cast<tg3__json&&>(spot);
    }

    tg3__serialize_extras_ext(o, &l->ext);
    return o;
}

/* ======================================================================
 * Internal: Main Model Serializer
 * ====================================================================== */

static tg3__json tg3__serialize_model(const tg3_model *model, int wd,
                                       int embed_images, int embed_buffers) {
    tg3__json root = tg3__json::object();

    /* Asset */
    root["asset"] = tg3__serialize_asset(&model->asset, wd);

    /* Default scene */
    if (model->default_scene >= 0) {
        root["scene"] = model->default_scene;
    }

    /* Extensions used/required */
    tg3__serialize_string_array(root, "extensionsUsed",
                                 model->extensions_used,
                                 model->extensions_used_count);
    tg3__serialize_string_array(root, "extensionsRequired",
                                 model->extensions_required,
                                 model->extensions_required_count);

    /* Entity arrays */
    #define TG3__SERIALIZE_ARRAY(key, arr, cnt, fn, ...) \
        if ((arr) && (cnt) > 0) { \
            tg3__json jarr; jarr.set_array(); \
            for (uint32_t _i = 0; _i < (cnt); ++_i) { \
                jarr.push_back(fn(&(arr)[_i] TG3__COMMA_VA_ARGS(__VA_ARGS__))); \
            } \
            root[key] = static_cast<tg3__json&&>(jarr); \
        }

    TG3__SERIALIZE_ARRAY("buffers", model->buffers, model->buffers_count,
                         tg3__serialize_buffer, wd, embed_buffers);
    TG3__SERIALIZE_ARRAY("bufferViews", model->buffer_views,
                         model->buffer_views_count, tg3__serialize_buffer_view, wd);
    TG3__SERIALIZE_ARRAY("accessors", model->accessors, model->accessors_count,
                         tg3__serialize_accessor, wd);
    TG3__SERIALIZE_ARRAY("meshes", model->meshes, model->meshes_count,
                         tg3__serialize_mesh, wd);
    TG3__SERIALIZE_ARRAY("nodes", model->nodes, model->nodes_count,
                         tg3__serialize_node, wd);
    TG3__SERIALIZE_ARRAY("materials", model->materials, model->materials_count,
                         tg3__serialize_material, wd);
    TG3__SERIALIZE_ARRAY("textures", model->textures, model->textures_count,
                         tg3__serialize_texture, wd);
    TG3__SERIALIZE_ARRAY("samplers", model->samplers, model->samplers_count,
                         tg3__serialize_sampler, wd);
    TG3__SERIALIZE_ARRAY("images", model->images, model->images_count,
                         tg3__serialize_image, wd, embed_images);
    TG3__SERIALIZE_ARRAY("skins", model->skins, model->skins_count,
                         tg3__serialize_skin, wd);
    TG3__SERIALIZE_ARRAY("animations", model->animations, model->animations_count,
                         tg3__serialize_animation, wd);
    TG3__SERIALIZE_ARRAY("cameras", model->cameras, model->cameras_count,
                         tg3__serialize_camera, wd);
    TG3__SERIALIZE_ARRAY("scenes", model->scenes, model->scenes_count,
                         tg3__serialize_scene, wd);

    /* KHR_lights_punctual */
    if (model->lights && model->lights_count > 0) {
        tg3__json lights_ext = tg3__json::object();
        tg3__json lights;
        lights.set_array();
        for (uint32_t i = 0; i < model->lights_count; ++i) {
            lights.push_back(tg3__serialize_light(&model->lights[i], wd));
        }
        lights_ext["lights"] = static_cast<tg3__json&&>(lights);

        auto ext_it = root.find("extensions");
        tg3__json exts = (ext_it != root.end()) ?
                         tg3__json(*ext_it) : tg3__json::object();
        exts["KHR_lights_punctual"] = static_cast<tg3__json&&>(lights_ext);
        root["extensions"] = static_cast<tg3__json&&>(exts);
    }

    /* Root extras/extensions */
    tg3__serialize_extras_ext(root, &model->ext);

    #undef TG3__SERIALIZE_ARRAY
    return root;
}

/* ======================================================================
 * Public: Writer API Implementation
 * ====================================================================== */

TINYGLTF3_API tg3_error_code tg3_write_to_memory(
    const tg3_model *model, tg3_error_stack *errors,
    uint8_t **out_data, uint64_t *out_size,
    const tg3_write_options *options) {

    tg3_write_options default_opts;
    if (!options) {
        tg3_write_options_init(&default_opts);
        options = &default_opts;
    }

    int wd = options->serialize_defaults;
    tg3__json root = tg3__serialize_model(model, wd,
                                           options->embed_images,
                                           options->embed_buffers);

    int indent = options->pretty_print ? 2 : -1;
    std::string json_str = root.dump(indent);

    if (options->write_binary) {
        /* GLB format */
        uint32_t json_len = (uint32_t)json_str.size();
        /* Pad JSON to 4-byte alignment with spaces */
        uint32_t json_padded = (json_len + 3) & ~3u;

        /* Collect binary buffer data */
        const uint8_t *bin_data = NULL;
        uint64_t bin_len = 0;
        if (model->buffers_count > 0 && model->buffers[0].data.data) {
            bin_data = model->buffers[0].data.data;
            bin_len = model->buffers[0].data.count;
        }
        uint32_t bin_padded = ((uint32_t)bin_len + 3) & ~3u;

        uint32_t total = 12; /* Header */
        total += 8 + json_padded; /* JSON chunk */
        if (bin_data && bin_len > 0) {
            total += 8 + bin_padded; /* BIN chunk */
        }

        uint8_t *glb = (uint8_t *)malloc(total);
        if (!glb) {
            tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                            "OOM allocating GLB output", NULL, -1);
            return TG3_ERR_OUT_OF_MEMORY;
        }

        /* Header */
        memcpy(glb, "glTF", 4);
        uint32_t version = 2;
        memcpy(glb + 4, &version, 4);
        memcpy(glb + 8, &total, 4);

        /* JSON chunk */
        memcpy(glb + 12, &json_padded, 4);
        uint32_t json_type = 0x4E4F534A;
        memcpy(glb + 16, &json_type, 4);
        memcpy(glb + 20, json_str.c_str(), json_len);
        /* Pad with spaces */
        for (uint32_t i = json_len; i < json_padded; ++i) {
            glb[20 + i] = ' ';
        }

        /* BIN chunk */
        if (bin_data && bin_len > 0) {
            uint32_t bin_off = 20 + json_padded;
            memcpy(glb + bin_off, &bin_padded, 4);
            uint32_t bin_type = 0x004E4942;
            memcpy(glb + bin_off + 4, &bin_type, 4);
            memcpy(glb + bin_off + 8, bin_data, (size_t)bin_len);
            /* Pad with zeros */
            for (uint32_t i = (uint32_t)bin_len; i < bin_padded; ++i) {
                glb[bin_off + 8 + i] = 0;
            }
        }

        *out_data = glb;
        *out_size = total;
    } else {
        /* JSON format */
        uint64_t sz = json_str.size();
        uint8_t *data = (uint8_t *)malloc((size_t)sz);
        if (!data) {
            tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_OUT_OF_MEMORY,
                            "OOM allocating JSON output", NULL, -1);
            return TG3_ERR_OUT_OF_MEMORY;
        }
        memcpy(data, json_str.c_str(), (size_t)sz);
        *out_data = data;
        *out_size = sz;
    }

    return TG3_OK;
}

TINYGLTF3_API tg3_error_code tg3_write_to_file(
    const tg3_model *model, tg3_error_stack *errors,
    const char *filename, uint32_t filename_len,
    const tg3_write_options *options) {

    tg3_write_options opts;
    if (options) {
        opts = *options;
    } else {
        tg3_write_options_init(&opts);
    }

#ifdef TINYGLTF3_ENABLE_FS
    tg3__set_default_fs(&opts.fs);
#endif

    if (!opts.fs.write_file) {
        tg3__error_push(errors, TG3_SEVERITY_ERROR, TG3_ERR_FS_NOT_AVAILABLE,
                        "No filesystem write callback", NULL, -1);
        return TG3_ERR_FS_NOT_AVAILABLE;
    }

    uint8_t *data = NULL;
    uint64_t size = 0;
    tg3_error_code err = tg3_write_to_memory(model, errors, &data, &size, &opts);
    if (err != TG3_OK) return err;

    int32_t ok = opts.fs.write_file(filename, filename_len, data, size,
                                     opts.fs.user_data);
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

/* ======================================================================
 * Streaming Writer (stub implementation)
 * ====================================================================== */

struct tg3_writer {
    tg3_write_chunk_fn chunk_fn;
    void              *user_data;
    tg3_write_options  options;
    tg3__json          root;
    int                begun;
};

TINYGLTF3_API tg3_writer *tg3_writer_create(
    tg3_write_chunk_fn chunk_fn, void *user_data,
    const tg3_write_options *options) {
    tg3_writer *w = new (std::nothrow) tg3_writer();
    if (!w) return NULL;
    w->chunk_fn = chunk_fn;
    w->user_data = user_data;
    if (options) w->options = *options;
    else tg3_write_options_init(&w->options);
    w->root = tg3__json::object();
    return w;
}

TINYGLTF3_API tg3_error_code tg3_writer_begin(tg3_writer *w, const tg3_asset *asset) {
    if (!w) return TG3_ERR_WRITE_FAILED;
    w->root["asset"] = tg3__serialize_asset(asset, w->options.serialize_defaults);
    w->begun = 1;
    return TG3_OK;
}

#define TG3__WRITER_ADD_IMPL(name, Type, json_key, serialize_fn, ...) \
    TINYGLTF3_API tg3_error_code tg3_writer_add_##name(tg3_writer *w, const Type *item) { \
        if (!w || !w->begun) return TG3_ERR_WRITE_FAILED; \
        auto it = w->root.find(json_key); \
        if (it == w->root.end()) { \
            tg3__json arr; arr.set_array(); \
            w->root[json_key] = static_cast<tg3__json&&>(arr); \
        } \
        w->root[json_key].push_back( \
            serialize_fn(item, w->options.serialize_defaults TG3__COMMA_VA_ARGS(__VA_ARGS__))); \
        return TG3_OK; \
    }

#define TG3__WRITER_ADD_SIMPLE(name, Type, json_key, serialize_fn) \
    TINYGLTF3_API tg3_error_code tg3_writer_add_##name(tg3_writer *w, const Type *item) { \
        if (!w || !w->begun) return TG3_ERR_WRITE_FAILED; \
        auto it = w->root.find(json_key); \
        if (it == w->root.end()) { \
            tg3__json arr; arr.set_array(); \
            w->root[json_key] = static_cast<tg3__json&&>(arr); \
        } \
        w->root[json_key].push_back( \
            serialize_fn(item, w->options.serialize_defaults)); \
        return TG3_OK; \
    }

TG3__WRITER_ADD_IMPL(buffer, tg3_buffer, "buffers", tg3__serialize_buffer,
                     w->options.embed_buffers)
TG3__WRITER_ADD_SIMPLE(buffer_view, tg3_buffer_view, "bufferViews",
                       tg3__serialize_buffer_view)
TG3__WRITER_ADD_SIMPLE(accessor, tg3_accessor, "accessors", tg3__serialize_accessor)
TG3__WRITER_ADD_SIMPLE(mesh, tg3_mesh, "meshes", tg3__serialize_mesh)
TG3__WRITER_ADD_SIMPLE(node, tg3_node, "nodes", tg3__serialize_node)
TG3__WRITER_ADD_SIMPLE(material, tg3_material, "materials", tg3__serialize_material)
TG3__WRITER_ADD_SIMPLE(texture, tg3_texture, "textures", tg3__serialize_texture)
TG3__WRITER_ADD_IMPL(image, tg3_image, "images", tg3__serialize_image,
                     w->options.embed_images)
TG3__WRITER_ADD_SIMPLE(sampler, tg3_sampler, "samplers", tg3__serialize_sampler)
TG3__WRITER_ADD_SIMPLE(animation, tg3_animation, "animations", tg3__serialize_animation)
TG3__WRITER_ADD_SIMPLE(skin, tg3_skin, "skins", tg3__serialize_skin)
TG3__WRITER_ADD_SIMPLE(camera, tg3_camera, "cameras", tg3__serialize_camera)
TG3__WRITER_ADD_SIMPLE(scene, tg3_scene, "scenes", tg3__serialize_scene)
TG3__WRITER_ADD_SIMPLE(light, tg3_light, "lights", tg3__serialize_light)

#undef TG3__WRITER_ADD_IMPL
#undef TG3__WRITER_ADD_SIMPLE

TINYGLTF3_API tg3_error_code tg3_writer_end(tg3_writer *w) {
    if (!w || !w->begun || !w->chunk_fn) return TG3_ERR_WRITE_FAILED;

    int indent = w->options.pretty_print ? 2 : -1;
    std::string json_str = w->root.dump(indent);

    int32_t ok = w->chunk_fn((const uint8_t *)json_str.c_str(),
                              json_str.size(), w->user_data);
    return ok ? TG3_OK : TG3_ERR_WRITE_FAILED;
}

TINYGLTF3_API void tg3_writer_destroy(tg3_writer *w) {
    delete w;
}

#endif /* TINYGLTF3_IMPLEMENTATION */

#endif /* TINY_GLTF_V3_H_ */
