/*
 * loader.c — tinygltf v3 C WASM bridge for the three.js demo.
 *
 * Parses glTF/GLB bytes passed in from JavaScript (Emscripten HEAPU8),
 * then exposes flattened per-primitive vertex/index data plus materials,
 * textures and the node hierarchy through EMSCRIPTEN_KEEPALIVE functions.
 *
 * Build with emcc (see Makefile). No filesystem or image decoding is
 * used: all assets must be embedded in the file (GLB chunk or data URI).
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tiny_gltf_v3.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#define TG3W_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define TG3W_EXPORT
#endif

/* ------------------------------------------------------------------ */
/* Exported model info                                                 */
/* ------------------------------------------------------------------ */

typedef struct {
    int32_t  material;     /* material index or -1 */
    int32_t  mode;         /* TG3_MODE_* */
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t pos_offset;   /* offset in tg3w_positions (floats, 3 * vertex_count) */
    int32_t  nrm_offset;   /* -1 = absent; floats, 3 * vertex_count */
    int32_t  uv_offset;    /* -1 = absent; floats, 2 * vertex_count */
    uint32_t idx_offset;   /* offset in tg3w_indices (uint32) */
} tg3w_prim;

typedef struct {
    float    base_color[4];
    float    metallic;
    float    roughness;
    int32_t  base_color_texture; /* texture index or -1 */
    int32_t  alpha_mode;         /* 0 = OPAQUE, 1 = MASK, 2 = BLEND */
    float    alpha_cutoff;
    int32_t  double_sided;
} tg3w_mat;

typedef struct {
    int32_t mesh;  /* mesh index or -1 */
    float   translation[3];
    float   rotation[4];
    float   scale[3];
} tg3w_node;

/* ------------------------------------------------------------------ */
/* Module state (single parse at a time, like a simple loading screen) */
/* ------------------------------------------------------------------ */

static tg3_model       g_model;
static tg3_error_stack g_errors;
static tg3_parse_options g_opts_;
static tg3w_prim     *g_prims = NULL;
static uint32_t       g_prim_count = 0;
static uint32_t      *g_mesh_prim_start = NULL;
static uint32_t      *g_mesh_prim_count = NULL;
static float         *g_positions = NULL;
static float         *g_normals = NULL;
static float         *g_uvs = NULL;
static uint32_t      *g_indices = NULL;
static char           g_last_error[512];

static int g_inited = 0;

/* Free the current model and all flattened buffers. */
TG3W_EXPORT void tg3w_clear(void) {
    tg3_model_free(&g_model);
    free(g_prims);       g_prims = NULL;
    free(g_positions);   g_positions = NULL;
    free(g_normals);     g_normals = NULL;
    free(g_uvs);         g_uvs = NULL;
    free(g_indices);     g_indices = NULL;
    free(g_mesh_prim_start); g_mesh_prim_start = NULL;
    free(g_mesh_prim_count); g_mesh_prim_count = NULL;
    g_prim_count = 0;
}

static void tg3w_init_once(void) {
    if (!g_inited) {
        tg3_parse_options_init(&g_opts_);
        tg3_error_stack_init(&g_errors);
        g_inited = 1;
    }
}

/* ------------------------------------------------------------------ */
/* Accessor helpers                                                    */
/* ------------------------------------------------------------------ */

static uint32_t tg3w_type_components(int32_t type) {
    switch (type) {
    case TG3_TYPE_SCALAR: return 1;
    case TG3_TYPE_VEC2:   return 2;
    case TG3_TYPE_VEC3:   return 3;
    case TG3_TYPE_VEC4:   return 4;
    case TG3_TYPE_MAT4:   return 16;
    default:              return 0;
    }
}

static const uint8_t *tg3w_accessor_ptr(const tg3_model *m,
                                        const tg3_accessor *a,
                                        uint64_t *stride_out) {
    if (a->buffer_view < 0 || a->sparse.is_sparse) {
        return NULL;
    }
    const tg3_buffer_view *bv = &m->buffer_views[a->buffer_view];
    if (bv->buffer < 0) {
        return NULL;
    }
    const tg3_buffer *b = &m->buffers[bv->buffer];
    if (!b->data.data || b->data.count < bv->byte_offset + bv->byte_length) {
        return NULL;
    }
    uint32_t comps = tg3w_type_components(a->type);
    uint64_t elem = 0;
    switch (a->component_type) {
    case TG3_COMPONENT_TYPE_FLOAT:  elem = 4; break;
    case TG3_COMPONENT_TYPE_DOUBLE: elem = 8; break;
    case TG3_COMPONENT_TYPE_UNSIGNED_BYTE: elem = 1; break;
    case TG3_COMPONENT_TYPE_BYTE:   elem = 1; break;
    case TG3_COMPONENT_TYPE_UNSIGNED_SHORT: elem = 2; break;
    case TG3_COMPONENT_TYPE_SHORT:  elem = 2; break;
    case TG3_COMPONENT_TYPE_UNSIGNED_INT: elem = 4; break;
    case TG3_COMPONENT_TYPE_INT:    elem = 4; break;
    default: return NULL;
    }
    uint64_t stride = bv->byte_stride ? bv->byte_stride : elem * comps;
    *stride_out = stride;
    return b->data.data + bv->byte_offset + a->byte_offset;
}

static int tg3w_attr_index(const tg3_primitive *p, const char *name) {
    for (uint32_t i = 0; i < p->attributes_count; i++) {
        const tg3_str_int_pair *a = &p->attributes[i];
        if (a->key.len == (uint32_t)strlen(name) &&
            strncmp(a->key.data, name, a->key.len) == 0) {
            return a->value;
        }
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* Parse + flatten                                                     */
/* ------------------------------------------------------------------ */

TG3W_EXPORT int tg3w_parse(const uint8_t *data, uint32_t size) {
    tg3w_init_once();

    tg3_model_free(&g_model);
    tg3_error_stack_free(&g_errors);
    tg3_error_stack_init(&g_errors);
    tg3_parse_options_init(&g_opts_);

    tg3_error_code err = tg3_parse_auto(&g_model, &g_errors, data, size,
                                        NULL, 0, &g_opts_);
    if (err != TG3_OK) {
        snprintf(g_last_error, sizeof(g_last_error), "parse failed: %d", (int)err);
        return (int)err;
    }

    /* Pass 1: count vertices/indices per primitive. */
    uint32_t prims = 0;
    uint64_t total_pos = 0, total_nrm = 0, total_uv = 0, total_idx = 0;
    for (uint32_t mi = 0; mi < g_model.meshes_count; mi++) {
        const tg3_mesh *mesh = &g_model.meshes[mi];
        for (uint32_t pi = 0; pi < mesh->primitives_count; pi++) {
            const tg3_primitive *p = &mesh->primitives[pi];
            int pos_i = tg3w_attr_index(p, "POSITION");
            int nrm_i = tg3w_attr_index(p, "NORMAL");
            int uv_i  = tg3w_attr_index(p, "TEXCOORD_0");
            uint64_t vcount = 0;
            if (pos_i >= 0) {
                const tg3_accessor *a = &g_model.accessors[pos_i];
                if (a->component_type == TG3_COMPONENT_TYPE_FLOAT &&
                    a->type == TG3_TYPE_VEC3 && !a->sparse.is_sparse) {
                    vcount = a->count;
                }
            }
            uint64_t icount = 0;
            if (p->indices >= 0) {
                const tg3_accessor *a = &g_model.accessors[p->indices];
                if (!a->sparse.is_sparse) {
                    switch (a->component_type) {
                    case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
                    case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
                    case TG3_COMPONENT_TYPE_UNSIGNED_INT:
                        icount = a->count;
                        break;
                    default: break;
                    }
                }
            }
            if (vcount == 0) {
                continue; /* unsupported primitive (e.g. sparse/double) */
            }
            total_pos += vcount * 3;
            if (nrm_i >= 0) {
                const tg3_accessor *a = &g_model.accessors[nrm_i];
                if (a->component_type == TG3_COMPONENT_TYPE_FLOAT &&
                    a->type == TG3_TYPE_VEC3 && !a->sparse.is_sparse) {
                    total_nrm += vcount * 3;
                }
            }
            if (uv_i >= 0) {
                const tg3_accessor *a = &g_model.accessors[uv_i];
                if (a->component_type == TG3_COMPONENT_TYPE_FLOAT &&
                    a->type == TG3_TYPE_VEC2 && !a->sparse.is_sparse) {
                    total_uv += vcount * 2;
                }
            }
            total_idx += icount;
            prims++;
        }
    }

    if (prims == 0) {
        snprintf(g_last_error, sizeof(g_last_error),
                 "no renderable primitives (float VEC3 POSITION required)");
        return -1;
    }

    /* Allocate flattened arrays. */
    free(g_prims); free(g_positions); free(g_normals); free(g_uvs); free(g_indices);
    free(g_mesh_prim_start); free(g_mesh_prim_count);
    g_prims  = (tg3w_prim *)malloc(prims * sizeof(tg3w_prim));
    g_positions = (float *)malloc(total_pos * sizeof(float));
    g_normals   = total_nrm ? (float *)malloc(total_nrm * sizeof(float)) : NULL;
    g_uvs       = total_uv ? (float *)malloc(total_uv * sizeof(float)) : NULL;
    g_indices   = (uint32_t *)malloc(total_idx * sizeof(uint32_t));
    g_mesh_prim_start = (uint32_t *)malloc(g_model.meshes_count * sizeof(uint32_t));
    g_mesh_prim_count = (uint32_t *)malloc(g_model.meshes_count * sizeof(uint32_t));
    g_prim_count = 0;

    if (!g_prims || (total_pos && !g_positions) || (total_nrm && !g_normals) ||
        (total_uv && !g_uvs) || (total_idx && !g_indices) ||
        !g_mesh_prim_start || !g_mesh_prim_count) {
        snprintf(g_last_error, sizeof(g_last_error), "out of memory");
        tg3w_clear();
        return -1;
    }
    memset(g_mesh_prim_start, 0, g_model.meshes_count * sizeof(uint32_t));
    memset(g_mesh_prim_count, 0, g_model.meshes_count * sizeof(uint32_t));

    /* Pass 2: fill. */
    uint32_t p_off = 0, n_off = 0, u_off = 0, i_off = 0;
    for (uint32_t mi = 0; mi < g_model.meshes_count; mi++) {
        const tg3_mesh *mesh = &g_model.meshes[mi];
        g_mesh_prim_start[mi] = g_prim_count;
        for (uint32_t pi = 0; pi < mesh->primitives_count; pi++) {
            const tg3_primitive *p = &mesh->primitives[pi];
            int pos_i = tg3w_attr_index(p, "POSITION");
            uint64_t vcount = 0;
            uint64_t pstride = 0;
            const uint8_t *pp = NULL;
            if (pos_i >= 0) {
                const tg3_accessor *a = &g_model.accessors[pos_i];
                if (a->component_type == TG3_COMPONENT_TYPE_FLOAT &&
                    a->type == TG3_TYPE_VEC3 && !a->sparse.is_sparse) {
                    vcount = a->count;
                    pp = tg3w_accessor_ptr(&g_model, a, &pstride);
                }
            }
            if (!pp || vcount == 0) {
                continue;
            }

            tg3w_prim *out = &g_prims[g_prim_count];
            memset(out, 0, sizeof(*out));
            out->nrm_offset = -1;
            out->uv_offset = -1;
            out->material = p->material;
            out->mode = (p->mode == -1) ? TG3_MODE_TRIANGLES : p->mode;
            out->vertex_count = (uint32_t)vcount;
            out->pos_offset = p_off;

            for (uint64_t v = 0; v < vcount; v++) {
                const float *f = (const float *)(pp + v * pstride);
                g_positions[p_off + (uint32_t)v * 3 + 0] = f[0];
                g_positions[p_off + (uint32_t)v * 3 + 1] = f[1];
                g_positions[p_off + (uint32_t)v * 3 + 2] = f[2];
            }
            p_off += (uint32_t)vcount * 3;

            int nrm_i = tg3w_attr_index(p, "NORMAL");
            if (nrm_i >= 0) {
                const tg3_accessor *a = &g_model.accessors[nrm_i];
                uint64_t stride = 0;
                const uint8_t *np = tg3w_accessor_ptr(&g_model, a, &stride);
                if (np && a->component_type == TG3_COMPONENT_TYPE_FLOAT &&
                    a->type == TG3_TYPE_VEC3 && a->count == vcount) {
                    out->nrm_offset = (int32_t)n_off;
                    for (uint64_t v = 0; v < vcount; v++) {
                        const float *f = (const float *)(np + v * stride);
                        g_normals[n_off + (uint32_t)v * 3 + 0] = f[0];
                        g_normals[n_off + (uint32_t)v * 3 + 1] = f[1];
                        g_normals[n_off + (uint32_t)v * 3 + 2] = f[2];
                    }
                    n_off += (uint32_t)vcount * 3;
                }
            }

            int uv_i = tg3w_attr_index(p, "TEXCOORD_0");
            if (uv_i >= 0) {
                const tg3_accessor *a = &g_model.accessors[uv_i];
                uint64_t stride = 0;
                const uint8_t *up = tg3w_accessor_ptr(&g_model, a, &stride);
                if (up && a->component_type == TG3_COMPONENT_TYPE_FLOAT &&
                    a->type == TG3_TYPE_VEC2 && a->count == vcount) {
                    out->uv_offset = (int32_t)u_off;
                    for (uint64_t v = 0; v < vcount; v++) {
                        const float *f = (const float *)(up + v * stride);
                        g_uvs[u_off + (uint32_t)v * 2 + 0] = f[0];
                        g_uvs[u_off + (uint32_t)v * 2 + 1] = f[1];
                    }
                    u_off += (uint32_t)vcount * 2;
                }
            }

            if (p->indices >= 0) {
                const tg3_accessor *a = &g_model.accessors[p->indices];
                uint64_t stride = 0;
                const uint8_t *ip = tg3w_accessor_ptr(&g_model, a, &stride);
                if (ip && !a->sparse.is_sparse) {
                    out->index_count = (uint32_t)a->count;
                    out->idx_offset = i_off;
                    for (uint64_t v = 0; v < a->count; v++) {
                        uint32_t idx = 0;
                        switch (a->component_type) {
                        case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
                            idx = ((const uint8_t *)ip)[v * stride];
                            break;
                        case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
                            idx = ((const uint16_t *)ip)[v * stride / 2];
                            break;
                        case TG3_COMPONENT_TYPE_UNSIGNED_INT:
                            idx = ((const uint32_t *)ip)[v * stride / 4];
                            break;
                        default: break;
                        }
                        g_indices[i_off + (uint32_t)v] = idx;
                    }
                    i_off += (uint32_t)a->count;
                }
            }
            g_prim_count++;
        }
        g_mesh_prim_count[mi] = g_prim_count - g_mesh_prim_start[mi];
    }

    g_last_error[0] = '\0';
    return 0;
}

/* ------------------------------------------------------------------ */
/* Getters                                                             */
/* ------------------------------------------------------------------ */

TG3W_EXPORT const char *tg3w_last_error(void) { return g_last_error; }

TG3W_EXPORT uint32_t tg3w_error_count(void) { return g_errors.count; }

TG3W_EXPORT const char *tg3w_error_message(uint32_t i) {
    if (i >= g_errors.count || !g_errors.entries[i].message) return "";
    return g_errors.entries[i].message;
}

TG3W_EXPORT int tg3w_error_severity(uint32_t i) {
    if (i >= g_errors.count) return -1;
    return (int)g_errors.entries[i].severity;
}

TG3W_EXPORT uint32_t tg3w_prim_count(void) { return g_prim_count; }

TG3W_EXPORT const tg3w_prim *tg3w_prim_at(uint32_t i) {
    if (i >= g_prim_count) return NULL;
    return &g_prims[i];
}

TG3W_EXPORT const float *tg3w_positions(void) { return g_positions; }
TG3W_EXPORT const float *tg3w_normals(void) { return g_normals; }
TG3W_EXPORT const float *tg3w_uvs(void) { return g_uvs; }
TG3W_EXPORT const uint32_t *tg3w_indices(void) { return g_indices; }

TG3W_EXPORT uint32_t tg3w_mesh_count(void) { return g_model.meshes_count; }

TG3W_EXPORT uint32_t tg3w_mesh_prim_start(uint32_t m) {
    if (m >= g_model.meshes_count) return 0;
    return g_mesh_prim_start[m];
}

TG3W_EXPORT uint32_t tg3w_mesh_prim_count(uint32_t m) {
    if (m >= g_model.meshes_count) return 0;
    return g_mesh_prim_count[m];
}

/* ------------------------------------------------------------------ */
/* Materials                                                           */
/* ------------------------------------------------------------------ */

TG3W_EXPORT uint32_t tg3w_material_count(void) { return g_model.materials_count; }

TG3W_EXPORT const float *tg3w_material_base_color(uint32_t i) {
    static float c[4];
    if (i >= g_model.materials_count) { c[0]=1;c[1]=1;c[2]=1;c[3]=1; return c; }
    const tg3_material *m = &g_model.materials[i];
    for (int k = 0; k < 4; k++) c[k] = (float)m->pbr_metallic_roughness.base_color_factor[k];
    return c;
}

TG3W_EXPORT float tg3w_material_metallic(uint32_t i) {
    if (i >= g_model.materials_count) return 1.0f;
    return (float)g_model.materials[i].pbr_metallic_roughness.metallic_factor;
}

TG3W_EXPORT float tg3w_material_roughness(uint32_t i) {
    if (i >= g_model.materials_count) return 1.0f;
    return (float)g_model.materials[i].pbr_metallic_roughness.roughness_factor;
}

TG3W_EXPORT int tg3w_material_base_color_texture(uint32_t i) {
    if (i >= g_model.materials_count) return -1;
    return g_model.materials[i].pbr_metallic_roughness.base_color_texture.index;
}

TG3W_EXPORT int tg3w_material_alpha_mode(uint32_t i) {
    if (i >= g_model.materials_count) return 0;
    const tg3_str *a = &g_model.materials[i].alpha_mode;
    if (a->len == 5 && strncmp(a->data, "BLEND", 5) == 0) return 2;
    if (a->len == 4 && strncmp(a->data, "MASK", 4) == 0) return 1;
    return 0;
}

TG3W_EXPORT float tg3w_material_alpha_cutoff(uint32_t i) {
    if (i >= g_model.materials_count) return 0.5f;
    return (float)g_model.materials[i].alpha_cutoff;
}

TG3W_EXPORT int tg3w_material_double_sided(uint32_t i) {
    if (i >= g_model.materials_count) return 0;
    return g_model.materials[i].double_sided;
}

/* ------------------------------------------------------------------ */
/* Textures (raw image bytes, decoded client-side)                     */
/* ------------------------------------------------------------------ */

TG3W_EXPORT uint32_t tg3w_texture_count(void) { return g_model.textures_count; }
TG3W_EXPORT uint32_t tg3w_image_count(void) { return g_model.images_count; }

TG3W_EXPORT int tg3w_texture_source(uint32_t i) {
    if (i >= g_model.textures_count) return -1;
    return g_model.textures[i].source;
}

/* Image payload: either a bufferView (GLB / .bin) or a data URI in the
 * glTF JSON. The v3 parser does not decode image bytes itself, so expose
 * both paths to JS. */
TG3W_EXPORT int tg3w_image_buffer_view(uint32_t i) {
    if (i >= g_model.images_count) return -1;
    return g_model.images[i].buffer_view;
}

TG3W_EXPORT const char *tg3w_image_uri(uint32_t i) {
    if (i >= g_model.images_count) return NULL;
    return g_model.images[i].uri.data ? g_model.images[i].uri.data : NULL;
}

TG3W_EXPORT const uint8_t *tg3w_image_bytes(uint32_t i) {
    if (i >= g_model.images_count) return NULL;
    int32_t bv = g_model.images[i].buffer_view;
    if (bv < 0 || bv >= (int32_t)g_model.buffer_views_count) return NULL;
    const tg3_buffer_view *v = &g_model.buffer_views[bv];
    if (v->buffer < 0 || v->buffer >= (int32_t)g_model.buffers_count) return NULL;
    const tg3_buffer *b = &g_model.buffers[v->buffer];
    if (!b->data.data || b->data.count < v->byte_offset + v->byte_length) return NULL;
    return b->data.data + v->byte_offset;
}

TG3W_EXPORT uint64_t tg3w_image_size(uint32_t i) {
    if (i >= g_model.images_count) return 0;
    int32_t bv = g_model.images[i].buffer_view;
    if (bv < 0 || bv >= (int32_t)g_model.buffer_views_count) return 0;
    return g_model.buffer_views[bv].byte_length;
}

TG3W_EXPORT const char *tg3w_image_mime(uint32_t i) {
    if (i >= g_model.images_count) return "";
    return g_model.images[i].mime_type.data ? g_model.images[i].mime_type.data : "";
}

/* ------------------------------------------------------------------ */
/* Scene graph (default scene only)                                    */
/* ------------------------------------------------------------------ */

TG3W_EXPORT uint32_t tg3w_node_count(void) { return g_model.nodes_count; }

TG3W_EXPORT int32_t tg3w_node_mesh(uint32_t i) {
    if (i >= g_model.nodes_count) return -1;
    return g_model.nodes[i].mesh;
}

TG3W_EXPORT uint32_t tg3w_node_child_count(uint32_t i) {
    if (i >= g_model.nodes_count) return 0;
    return g_model.nodes[i].children_count;
}

TG3W_EXPORT const int32_t *tg3w_node_children(uint32_t i) {
    if (i >= g_model.nodes_count) return NULL;
    return g_model.nodes[i].children;
}

TG3W_EXPORT const float *tg3w_node_trs(uint32_t i) {
    static float trs[10];
    if (i >= g_model.nodes_count) { memset(trs, 0, sizeof(trs)); trs[6] = 1.0f; return trs; }
    const tg3_node *n = &g_model.nodes[i];
    for (int k = 0; k < 3; k++) trs[k] = (float)n->translation[k];
    for (int k = 0; k < 4; k++) trs[3 + k] = (float)n->rotation[k];
    for (int k = 0; k < 3; k++) trs[7 + k] = (float)n->scale[k];
    return trs;
}

TG3W_EXPORT int tg3w_default_scene(void) { return g_model.default_scene; }

TG3W_EXPORT uint32_t tg3w_scene_count(void) { return g_model.scenes_count; }

TG3W_EXPORT uint32_t tg3w_scene_node_count(uint32_t i) {
    if (i >= g_model.scenes_count) return 0;
    return g_model.scenes[i].nodes_count;
}

TG3W_EXPORT const int32_t *tg3w_scene_nodes(uint32_t i) {
    if (i >= g_model.scenes_count) return NULL;
    return g_model.scenes[i].nodes;
}
