/*
 * gen_synthetic.cpp — Generate synthetic glTF 2.0 scenes for benchmarking.
 *
 * Produces .gltf (ASCII) and .glb (binary) files with configurable:
 *   - Number of meshes, each with N vertices/triangles
 *   - Number of nodes (flat hierarchy)
 *   - Number of materials
 *   - Number of animations with M keyframes
 *
 * Usage:
 *   gen_synthetic [--meshes N] [--verts-per-mesh N] [--nodes N]
 *                 [--materials N] [--animations N] [--keyframes N]
 *                 [--prefix NAME]
 *
 * Outputs: <prefix>_<label>.gltf and <prefix>_<label>.glb
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <cstdint>

/* ------------------------------------------------------------------ */
/* Tiny JSON writer (no dependencies)                                 */
/* ------------------------------------------------------------------ */

struct JsonWriter {
    std::string buf;
    int indent;
    bool need_comma;
    std::vector<bool> stack; /* true = array context */

    JsonWriter() : indent(0), need_comma(false) {}

    void comma() {
        if (need_comma) buf += ",";
        buf += "\n";
        for (int i = 0; i < indent; ++i) buf += "  ";
    }

    void begin_obj() {
        if (!stack.empty()) comma();
        buf += "{";
        indent++;
        need_comma = false;
        stack.push_back(false);
    }
    void end_obj() {
        indent--;
        buf += "\n";
        for (int i = 0; i < indent; ++i) buf += "  ";
        buf += "}";
        stack.pop_back();
        need_comma = true;
    }

    void begin_arr() {
        if (!stack.empty() && !need_comma) { /* first elem */ }
        buf += "[";
        indent++;
        need_comma = false;
        stack.push_back(true);
    }
    void end_arr() {
        indent--;
        buf += "\n";
        for (int i = 0; i < indent; ++i) buf += "  ";
        buf += "]";
        stack.pop_back();
        need_comma = true;
    }

    void key(const char *k) {
        comma();
        buf += "\"";
        buf += k;
        buf += "\": ";
        need_comma = false;
    }

    void val_str(const char *v) {
        if (stack.back()) comma();
        buf += "\"";
        buf += v;
        buf += "\"";
        need_comma = true;
    }
    void val_int(int64_t v) {
        if (stack.back()) comma();
        buf += std::to_string(v);
        need_comma = true;
    }
    void val_double(double v) {
        if (stack.back()) comma();
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "%.6g", v);
        buf += tmp;
        need_comma = true;
    }
    void val_bool(bool v) {
        if (stack.back()) comma();
        buf += v ? "true" : "false";
        need_comma = true;
    }

    void kv_str(const char *k, const char *v) { key(k); val_str(v); need_comma = true; }
    void kv_int(const char *k, int64_t v) { key(k); val_int(v); need_comma = true; }
    void kv_double(const char *k, double v) { key(k); val_double(v); need_comma = true; }
    void kv_bool(const char *k, bool v) { key(k); val_bool(v); need_comma = true; }
};

/* ------------------------------------------------------------------ */
/* Binary buffer builder                                              */
/* ------------------------------------------------------------------ */

struct BinBuffer {
    std::vector<uint8_t> data;

    size_t offset() const { return data.size(); }

    void push_float(float v) {
        const uint8_t *p = reinterpret_cast<const uint8_t*>(&v);
        data.insert(data.end(), p, p + 4);
    }
    void push_u16(uint16_t v) {
        const uint8_t *p = reinterpret_cast<const uint8_t*>(&v);
        data.insert(data.end(), p, p + 2);
    }
    void push_u32(uint32_t v) {
        const uint8_t *p = reinterpret_cast<const uint8_t*>(&v);
        data.insert(data.end(), p, p + 4);
    }
    void align4() {
        while (data.size() % 4 != 0) data.push_back(0);
    }
};

/* ------------------------------------------------------------------ */
/* Scene config                                                       */
/* ------------------------------------------------------------------ */

struct SceneConfig {
    int num_meshes;
    int verts_per_mesh;
    int num_nodes;
    int num_materials;
    int num_animations;
    int keyframes;
};

/* ------------------------------------------------------------------ */
/* Generate the scene                                                 */
/* ------------------------------------------------------------------ */

struct AccessorInfo {
    int buffer_view;
    int component_type;
    int count;
    const char *type;
    float min_vals[3];
    float max_vals[3];
    int min_max_components; /* 0 = none, 1 = scalar, 3 = vec3 */
};

static void generate_scene(const SceneConfig &cfg,
                            std::string &out_json,
                            std::vector<uint8_t> &out_bin) {
    BinBuffer bin;

    /* Pre-compute sizes */
    int tris_per_mesh = cfg.verts_per_mesh / 3;
    if (tris_per_mesh < 1) tris_per_mesh = 1;
    int actual_verts = tris_per_mesh * 3;

    /*
     * For each mesh:
     *   - positions: actual_verts * 3 floats
     *   - normals:   actual_verts * 3 floats
     *   - indices:   tris_per_mesh * 3 uint16 (or uint32 if >65535)
     *
     * For each animation:
     *   - time keys:  keyframes floats
     *   - translation values: keyframes * 3 floats
     */

    /* Track buffer views and accessors */
    std::vector<size_t> bv_offsets;
    std::vector<size_t> bv_lengths;
    std::vector<AccessorInfo> accessors;
    int bv_idx = 0;

    bool use_u32_indices = (actual_verts > 65535);

    /* === Mesh data === */
    for (int m = 0; m < cfg.num_meshes; ++m) {
        float mesh_offset_x = (float)m * 5.0f;

        /* Positions */
        size_t pos_off = bin.offset();
        float pmin[3] = {1e30f, 1e30f, 1e30f};
        float pmax[3] = {-1e30f, -1e30f, -1e30f};
        for (int v = 0; v < actual_verts; ++v) {
            float angle = (float)v / (float)actual_verts * 6.2831853f;
            float r = 1.0f + 0.3f * sinf(angle * 5.0f);
            float x = mesh_offset_x + r * cosf(angle);
            float y = r * sinf(angle);
            float z = 0.5f * sinf(angle * 3.0f + (float)m);
            bin.push_float(x); bin.push_float(y); bin.push_float(z);
            if (x < pmin[0]) pmin[0] = x;
            if (x > pmax[0]) pmax[0] = x;
            if (y < pmin[1]) pmin[1] = y;
            if (y > pmax[1]) pmax[1] = y;
            if (z < pmin[2]) pmin[2] = z;
            if (z > pmax[2]) pmax[2] = z;
        }
        size_t pos_len = bin.offset() - pos_off;
        bin.align4();
        bv_offsets.push_back(pos_off); bv_lengths.push_back(pos_len);
        int pos_bv = bv_idx++;

        AccessorInfo pos_acc;
        pos_acc.buffer_view = pos_bv;
        pos_acc.component_type = 5126; /* FLOAT */
        pos_acc.count = actual_verts;
        pos_acc.type = "VEC3";
        memcpy(pos_acc.min_vals, pmin, sizeof(pmin));
        memcpy(pos_acc.max_vals, pmax, sizeof(pmax));
        pos_acc.min_max_components = 3;
        accessors.push_back(pos_acc);

        /* Normals */
        size_t norm_off = bin.offset();
        for (int v = 0; v < actual_verts; ++v) {
            float angle = (float)v / (float)actual_verts * 6.2831853f;
            float nx = cosf(angle), ny = sinf(angle), nz = 0.0f;
            float len = sqrtf(nx*nx + ny*ny + nz*nz);
            if (len > 0) { nx /= len; ny /= len; nz /= len; }
            bin.push_float(nx); bin.push_float(ny); bin.push_float(nz);
        }
        size_t norm_len = bin.offset() - norm_off;
        bin.align4();
        bv_offsets.push_back(norm_off); bv_lengths.push_back(norm_len);
        int norm_bv = bv_idx++;

        AccessorInfo norm_acc;
        norm_acc.buffer_view = norm_bv;
        norm_acc.component_type = 5126;
        norm_acc.count = actual_verts;
        norm_acc.type = "VEC3";
        norm_acc.min_max_components = 0;
        accessors.push_back(norm_acc);

        /* Indices */
        size_t idx_off = bin.offset();
        for (int t = 0; t < tris_per_mesh; ++t) {
            if (use_u32_indices) {
                bin.push_u32((uint32_t)(t * 3));
                bin.push_u32((uint32_t)(t * 3 + 1));
                bin.push_u32((uint32_t)(t * 3 + 2));
            } else {
                bin.push_u16((uint16_t)(t * 3));
                bin.push_u16((uint16_t)(t * 3 + 1));
                bin.push_u16((uint16_t)(t * 3 + 2));
            }
        }
        size_t idx_len = bin.offset() - idx_off;
        bin.align4();
        bv_offsets.push_back(idx_off); bv_lengths.push_back(idx_len);
        int idx_bv = bv_idx++;

        AccessorInfo idx_acc;
        idx_acc.buffer_view = idx_bv;
        idx_acc.component_type = use_u32_indices ? 5125 : 5123; /* UINT or USHORT */
        idx_acc.count = tris_per_mesh * 3;
        idx_acc.type = "SCALAR";
        idx_acc.min_max_components = 0;
        accessors.push_back(idx_acc);
    }

    /* === Animation data === */
    int anim_time_accessor_start = (int)accessors.size();
    for (int a = 0; a < cfg.num_animations; ++a) {
        /* Time keys */
        size_t time_off = bin.offset();
        float tmin = 0.0f, tmax = 0.0f;
        for (int k = 0; k < cfg.keyframes; ++k) {
            float t = (float)k / (float)(cfg.keyframes - 1) * 10.0f;
            bin.push_float(t);
            if (k == 0) tmin = t;
            tmax = t;
        }
        size_t time_len = bin.offset() - time_off;
        bin.align4();
        bv_offsets.push_back(time_off); bv_lengths.push_back(time_len);
        int time_bv = bv_idx++;

        AccessorInfo time_acc;
        time_acc.buffer_view = time_bv;
        time_acc.component_type = 5126;
        time_acc.count = cfg.keyframes;
        time_acc.type = "SCALAR";
        time_acc.min_vals[0] = tmin;
        time_acc.max_vals[0] = tmax;
        time_acc.min_max_components = 1;
        accessors.push_back(time_acc);

        /* Translation values */
        size_t val_off = bin.offset();
        for (int k = 0; k < cfg.keyframes; ++k) {
            float t = (float)k / (float)(cfg.keyframes - 1) * 10.0f;
            float x = sinf(t * 0.5f + (float)a) * 2.0f;
            float y = cosf(t * 0.3f) * 1.5f;
            float z = sinf(t * 0.7f + (float)a * 0.5f);
            bin.push_float(x); bin.push_float(y); bin.push_float(z);
        }
        size_t val_len = bin.offset() - val_off;
        bin.align4();
        bv_offsets.push_back(val_off); bv_lengths.push_back(val_len);
        int val_bv = bv_idx++;

        AccessorInfo val_acc;
        val_acc.buffer_view = val_bv;
        val_acc.component_type = 5126;
        val_acc.count = cfg.keyframes;
        val_acc.type = "VEC3";
        val_acc.min_max_components = 0;
        accessors.push_back(val_acc);
    }

    size_t total_bin = bin.data.size();

    /* === Build JSON === */
    JsonWriter w;
    w.begin_obj();

    /* asset */
    w.key("asset"); w.begin_obj();
    w.kv_str("version", "2.0");
    w.kv_str("generator", "tinygltf_benchmark_gen");
    w.end_obj();

    /* scene */
    w.kv_int("scene", 0);

    /* scenes */
    w.key("scenes"); w.begin_arr();
    w.begin_obj();
    w.kv_str("name", "BenchmarkScene");
    w.key("nodes"); w.begin_arr();
    for (int n = 0; n < cfg.num_nodes; ++n) w.val_int(n);
    w.end_arr();
    w.end_obj();
    w.end_arr();

    /* nodes */
    w.key("nodes"); w.begin_arr();
    for (int n = 0; n < cfg.num_nodes; ++n) {
        w.begin_obj();
        w.kv_str("name", ("Node_" + std::to_string(n)).c_str());
        if (n < cfg.num_meshes) {
            w.kv_int("mesh", n);
        }
        w.key("translation"); w.begin_arr();
        w.val_double((double)n * 3.0);
        w.val_double(0.0);
        w.val_double(0.0);
        w.end_arr();
        w.end_obj();
    }
    w.end_arr();

    /* meshes */
    w.key("meshes"); w.begin_arr();
    for (int m = 0; m < cfg.num_meshes; ++m) {
        int base_acc = m * 3; /* pos, norm, idx per mesh */
        w.begin_obj();
        w.kv_str("name", ("Mesh_" + std::to_string(m)).c_str());
        w.key("primitives"); w.begin_arr();
        w.begin_obj();
        w.key("attributes"); w.begin_obj();
        w.kv_int("POSITION", base_acc);
        w.kv_int("NORMAL", base_acc + 1);
        w.end_obj();
        w.kv_int("indices", base_acc + 2);
        w.kv_int("material", m % cfg.num_materials);
        w.kv_int("mode", 4);
        w.end_obj();
        w.end_arr();
        w.end_obj();
    }
    w.end_arr();

    /* materials */
    w.key("materials"); w.begin_arr();
    for (int m = 0; m < cfg.num_materials; ++m) {
        w.begin_obj();
        w.kv_str("name", ("Material_" + std::to_string(m)).c_str());
        w.key("pbrMetallicRoughness"); w.begin_obj();
        w.key("baseColorFactor"); w.begin_arr();
        float hue = (float)m / (float)cfg.num_materials;
        w.val_double(0.5 + 0.5 * sin(hue * 6.28));
        w.val_double(0.5 + 0.5 * sin(hue * 6.28 + 2.09));
        w.val_double(0.5 + 0.5 * sin(hue * 6.28 + 4.19));
        w.val_double(1.0);
        w.end_arr();
        w.kv_double("metallicFactor", 0.2 + 0.6 * ((double)m / cfg.num_materials));
        w.kv_double("roughnessFactor", 0.3 + 0.5 * ((double)(cfg.num_materials - m) / cfg.num_materials));
        w.end_obj();
        w.end_obj();
    }
    w.end_arr();

    /* accessors */
    w.key("accessors"); w.begin_arr();
    for (size_t i = 0; i < accessors.size(); ++i) {
        const AccessorInfo &a = accessors[i];
        w.begin_obj();
        w.kv_int("bufferView", a.buffer_view);
        w.kv_int("componentType", a.component_type);
        w.kv_int("count", a.count);
        w.kv_str("type", a.type);
        if (a.min_max_components == 1) {
            w.key("min"); w.begin_arr(); w.val_double(a.min_vals[0]); w.end_arr();
            w.key("max"); w.begin_arr(); w.val_double(a.max_vals[0]); w.end_arr();
        } else if (a.min_max_components == 3) {
            w.key("min"); w.begin_arr();
            w.val_double(a.min_vals[0]); w.val_double(a.min_vals[1]); w.val_double(a.min_vals[2]);
            w.end_arr();
            w.key("max"); w.begin_arr();
            w.val_double(a.max_vals[0]); w.val_double(a.max_vals[1]); w.val_double(a.max_vals[2]);
            w.end_arr();
        }
        w.end_obj();
    }
    w.end_arr();

    /* bufferViews */
    w.key("bufferViews"); w.begin_arr();
    for (int i = 0; i < bv_idx; ++i) {
        w.begin_obj();
        w.kv_int("buffer", 0);
        w.kv_int("byteOffset", (int64_t)bv_offsets[i]);
        w.kv_int("byteLength", (int64_t)bv_lengths[i]);
        w.end_obj();
    }
    w.end_arr();

    /* buffers */
    w.key("buffers"); w.begin_arr();
    w.begin_obj();
    w.kv_int("byteLength", (int64_t)total_bin);
    /* URI will be set by caller for .gltf, omitted for .glb */
    w.end_obj();
    w.end_arr();

    /* animations */
    if (cfg.num_animations > 0) {
        w.key("animations"); w.begin_arr();
        for (int a = 0; a < cfg.num_animations; ++a) {
            int time_acc = anim_time_accessor_start + a * 2;
            int val_acc = time_acc + 1;
            /* Target node: cycle through available nodes */
            int target_node = a % cfg.num_nodes;

            w.begin_obj();
            w.kv_str("name", ("Anim_" + std::to_string(a)).c_str());

            w.key("channels"); w.begin_arr();
            w.begin_obj();
            w.kv_int("sampler", 0);
            w.key("target"); w.begin_obj();
            w.kv_int("node", target_node);
            w.kv_str("path", "translation");
            w.end_obj();
            w.end_obj();
            w.end_arr();

            w.key("samplers"); w.begin_arr();
            w.begin_obj();
            w.kv_int("input", time_acc);
            w.kv_int("output", val_acc);
            w.kv_str("interpolation", "LINEAR");
            w.end_obj();
            w.end_arr();

            w.end_obj();
        }
        w.end_arr();
    }

    w.end_obj();

    out_json = w.buf;
    out_bin = bin.data;
}

/* ------------------------------------------------------------------ */
/* Write .gltf + .bin                                                 */
/* ------------------------------------------------------------------ */

static void write_gltf(const std::string &prefix, const std::string &label,
                        const std::string &json_str,
                        const std::vector<uint8_t> &bin_data) {
    std::string bin_name = prefix + "_" + label + ".bin";
    std::string gltf_name = prefix + "_" + label + ".gltf";

    /* Inject "uri" into the buffer object in JSON */
    std::string json_patched = json_str;
    /* Find the buffers array and add uri before the closing } of the buffer */
    size_t pos = json_patched.find("\"byteLength\"");
    if (pos != std::string::npos) {
        /* Find the line end after byteLength value */
        size_t line_end = json_patched.find('\n', pos);
        if (line_end != std::string::npos) {
            /* Extract just the filename for uri */
            std::string bin_filename = prefix + "_" + label + ".bin";
            std::string uri_line = ",\n      \"uri\": \"" + bin_filename + "\"";
            json_patched.insert(line_end, uri_line);
        }
    }

    /* Write .bin */
    FILE *f = fopen(bin_name.c_str(), "wb");
    if (f) {
        fwrite(bin_data.data(), 1, bin_data.size(), f);
        fclose(f);
    }

    /* Write .gltf */
    f = fopen(gltf_name.c_str(), "w");
    if (f) {
        fwrite(json_patched.c_str(), 1, json_patched.size(), f);
        fclose(f);
    }

    printf("  Written: %s (%zu bytes JSON) + %s (%zu bytes binary)\n",
           gltf_name.c_str(), json_patched.size(),
           bin_name.c_str(), bin_data.size());
}

/* ------------------------------------------------------------------ */
/* Write .glb                                                         */
/* ------------------------------------------------------------------ */

static void write_glb(const std::string &prefix, const std::string &label,
                       const std::string &json_str,
                       const std::vector<uint8_t> &bin_data) {
    std::string glb_name = prefix + "_" + label + ".glb";

    uint32_t json_len = (uint32_t)json_str.size();
    uint32_t json_padded = (json_len + 3) & ~3u;
    uint32_t bin_len = (uint32_t)bin_data.size();
    uint32_t bin_padded = (bin_len + 3) & ~3u;

    uint32_t total = 12 + 8 + json_padded + 8 + bin_padded;

    FILE *f = fopen(glb_name.c_str(), "wb");
    if (!f) return;

    /* Header */
    fwrite("glTF", 1, 4, f);
    uint32_t version = 2;
    fwrite(&version, 4, 1, f);
    fwrite(&total, 4, 1, f);

    /* JSON chunk */
    uint32_t json_type = 0x4E4F534A;
    fwrite(&json_padded, 4, 1, f);
    fwrite(&json_type, 4, 1, f);
    fwrite(json_str.c_str(), 1, json_len, f);
    for (uint32_t i = json_len; i < json_padded; ++i) {
        char sp = ' ';
        fwrite(&sp, 1, 1, f);
    }

    /* BIN chunk */
    uint32_t bin_type = 0x004E4942;
    fwrite(&bin_padded, 4, 1, f);
    fwrite(&bin_type, 4, 1, f);
    fwrite(bin_data.data(), 1, bin_len, f);
    for (uint32_t i = bin_len; i < bin_padded; ++i) {
        char z = 0;
        fwrite(&z, 1, 1, f);
    }

    fclose(f);
    printf("  Written: %s (%u bytes)\n", glb_name.c_str(), total);
}

/* ------------------------------------------------------------------ */
/* Preset configurations                                              */
/* ------------------------------------------------------------------ */

struct Preset {
    const char *label;
    SceneConfig cfg;
};

static Preset presets[] = {
    {"tiny",   {1,   100,    2,  1, 0,   0}},
    {"small",  {5,   1000,  10,  3, 2,  50}},
    {"medium", {20,  5000,  50, 10, 5, 200}},
    {"large",  {100, 10000, 200, 20, 10, 500}},
    {"huge",   {500, 50000, 1000, 50, 50, 1000}},
};

static const int num_presets = (int)(sizeof(presets) / sizeof(presets[0]));

/* ------------------------------------------------------------------ */
/* Generate float-heavy scene (~500MB of ASCII float values in JSON)  */
/* ------------------------------------------------------------------ */

static void generate_float_heavy(const std::string &prefix, size_t target_mb) {
    std::string gltf_name = prefix + "_float_heavy.gltf";
    FILE *f = fopen(gltf_name.c_str(), "w");
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", gltf_name.c_str());
        return;
    }

    /* Write minimal valid glTF with massive extras float array */
    fprintf(f, "{\n");
    fprintf(f, "  \"asset\": {\"version\": \"2.0\", \"generator\": \"tinygltf_benchmark_gen\"},\n");
    fprintf(f, "  \"scene\": 0,\n");
    fprintf(f, "  \"scenes\": [{\"name\": \"FloatHeavy\", \"nodes\": [0]}],\n");
    fprintf(f, "  \"nodes\": [{\"name\": \"Root\"}],\n");
    fprintf(f, "  \"extras\": {\n");

    size_t target_bytes = target_mb * 1024ULL * 1024ULL;
    size_t total_written = 0;
    int num_channels = 10;
    size_t per_channel = target_bytes / (size_t)num_channels;

    for (int ch = 0; ch < num_channels; ++ch) {
        fprintf(f, "    \"channel_%d\": [\n      ", ch);
        size_t ch_written = 0;
        size_t count = 0;
        uint64_t seed = (uint64_t)ch * 7919ULL + 1;
        bool first = true;

        while (ch_written < per_channel) {
            /* Comma before every value except the first */
            if (!first) {
                fwrite(",\n      ", 1, 8, f);
                ch_written += 8;
            }
            first = false;

            /* Generate varied float values: mix of magnitudes and precisions */
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            double raw = (double)(int64_t)seed / (double)INT64_MAX;

            double val;
            int kind = (int)(count % 5);
            switch (kind) {
                case 0: val = raw * 1000.0; break;           /* large: -999.xxx */
                case 1: val = raw * 0.001; break;            /* small: 0.000xxx */
                case 2: val = raw * 3.14159265358979; break; /* medium: -3.14..3.14 */
                case 3: val = raw * 1e6; break;              /* very large */
                case 4: val = raw * 1e-6; break;             /* very small */
                default: val = raw; break;
            }

            char buf[64];
            int len = snprintf(buf, sizeof(buf), "%.8g", val);
            fwrite(buf, 1, (size_t)len, f);
            ch_written += (size_t)len;
            count++;
        }

        total_written += ch_written;

        if (ch < num_channels - 1) {
            fprintf(f, "\n    ],\n");
        } else {
            fprintf(f, "\n    ]\n");
        }
    }

    fprintf(f, "  }\n");
    fprintf(f, "}\n");
    fclose(f);

    /* Report actual file size */
    f = fopen(gltf_name.c_str(), "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fclose(f);
        printf("  Written: %s (%.1f MB, ~%zu float values across %d channels)\n",
               gltf_name.c_str(), (double)sz / (1024.0 * 1024.0),
               total_written / 12, num_channels);
    }
}

/* ------------------------------------------------------------------ */
/* Main                                                               */
/* ------------------------------------------------------------------ */

int main(int argc, char **argv) {
    std::string prefix = "synthetic";

    /* Parse args */
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--prefix") == 0 && i + 1 < argc) {
            prefix = argv[++i];
        }
    }

    printf("Generating synthetic glTF benchmark scenes...\n\n");

    for (int p = 0; p < num_presets; ++p) {
        const Preset &pr = presets[p];

        printf("[%s] meshes=%d verts/mesh=%d nodes=%d materials=%d "
               "animations=%d keyframes=%d\n",
               pr.label, pr.cfg.num_meshes, pr.cfg.verts_per_mesh,
               pr.cfg.num_nodes, pr.cfg.num_materials,
               pr.cfg.num_animations, pr.cfg.keyframes);

        std::string json;
        std::vector<uint8_t> bin;
        generate_scene(pr.cfg, json, bin);

        write_gltf(prefix, pr.label, json, bin);
        write_glb(prefix, pr.label, json, bin);
        printf("\n");
    }

    /* Float-heavy scene: ~500MB of ASCII floats in JSON */
    printf("[float_heavy] ~500MB of ASCII float values in JSON extras\n");
    generate_float_heavy(prefix, 500);
    printf("\n");

    printf("Done.\n");
    return 0;
}
