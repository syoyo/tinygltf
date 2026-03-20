/*
 * bench_v3.cpp — Benchmark tinygltf v3 parser: parse speed & memory.
 *
 * Measures:
 *   - File read time
 *   - JSON parse + model build time
 *   - Peak arena memory usage
 *   - Throughput (MB/s)
 *
 * Usage:
 *   bench_v3 <file.gltf|file.glb> [--iterations N] [--warmup N] [--quiet]
 *   bench_v3 --batch <file1> <file2> ... [--iterations N]
 */

#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS
#include "tiny_gltf_v3.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>

#if defined(__linux__)
#include <sys/resource.h>
#endif

/* ------------------------------------------------------------------ */
/* Timing helpers                                                     */
/* ------------------------------------------------------------------ */

using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;

static double elapsed_ms(TimePoint start, TimePoint end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

/* ------------------------------------------------------------------ */
/* Memory tracking allocator                                          */
/* ------------------------------------------------------------------ */

struct MemTracker {
    size_t current;
    size_t peak;
    size_t total_allocs;
    size_t total_frees;
};

static void *tracked_alloc(size_t size, void *ud) {
    MemTracker *mt = (MemTracker *)ud;
    void *ptr = malloc(size);
    if (ptr) {
        mt->current += size;
        if (mt->current > mt->peak) mt->peak = mt->current;
        mt->total_allocs++;
    }
    return ptr;
}

static void *tracked_realloc(void *ptr, size_t old_size, size_t new_size, void *ud) {
    MemTracker *mt = (MemTracker *)ud;
    void *new_ptr = realloc(ptr, new_size);
    if (new_ptr) {
        mt->current -= old_size;
        mt->current += new_size;
        if (mt->current > mt->peak) mt->peak = mt->current;
    }
    return new_ptr;
}

static void tracked_free(void *ptr, size_t size, void *ud) {
    MemTracker *mt = (MemTracker *)ud;
    if (ptr) {
        mt->current -= size;
        mt->total_frees++;
        free(ptr);
    }
}

/* ------------------------------------------------------------------ */
/* RSS measurement (Linux)                                            */
/* ------------------------------------------------------------------ */

static size_t get_rss_bytes() {
#if defined(__linux__)
    FILE *f = fopen("/proc/self/statm", "r");
    if (!f) return 0;
    long pages = 0;
    if (fscanf(f, "%*s %ld", &pages) != 1) pages = 0;
    fclose(f);
    return (size_t)pages * 4096;
#else
    return 0;
#endif
}

/* ------------------------------------------------------------------ */
/* Benchmark result                                                   */
/* ------------------------------------------------------------------ */

struct BenchResult {
    std::string filename;
    uint64_t    file_size;
    int         iterations;

    /* Parse timing (ms) */
    double parse_min;
    double parse_max;
    double parse_avg;
    double parse_median;

    /* Memory */
    size_t arena_peak;     /* Peak arena allocation */
    size_t rss_before;
    size_t rss_after;

    /* Model stats */
    uint32_t meshes;
    uint32_t nodes;
    uint32_t accessors;
    uint32_t materials;
    uint32_t animations;
    uint32_t buffers;
    uint32_t buffer_views;
    uint32_t images;
    uint32_t textures;

    /* Derived */
    double throughput_mbs; /* MB/s based on median */
};

/* ------------------------------------------------------------------ */
/* Run benchmark for a single file                                    */
/* ------------------------------------------------------------------ */

static BenchResult bench_file(const char *filename, int iterations, int warmup,
                               bool quiet, int float32_mode = 0) {
    BenchResult r = {};
    r.filename = filename;
    r.iterations = iterations;

    /* Read file into memory */
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: Cannot open '%s'\n", filename);
        return r;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return r; }

    std::vector<uint8_t> data((size_t)sz);
    size_t rd = fread(data.data(), 1, (size_t)sz, f);
    fclose(f);
    if ((long)rd != sz) { return r; }

    r.file_size = (uint64_t)sz;

    /* Extract base dir */
    std::string path(filename);
    std::string base_dir;
    size_t sep = path.find_last_of("/\\");
    if (sep != std::string::npos) base_dir = path.substr(0, sep);

    /* Warmup iterations (not measured) */
    for (int i = 0; i < warmup; ++i) {
        tg3_model model;
        tg3_error_stack errors;
        tg3_error_stack_init(&errors);
        tg3_parse_auto(&model, &errors, data.data(), data.size(),
                       base_dir.c_str(), (uint32_t)base_dir.size(), NULL);
        tg3_model_free(&model);
        tg3_error_stack_free(&errors);
    }

    /* Benchmark iterations */
    std::vector<double> times;
    times.reserve(iterations);

    MemTracker tracker_best;
    memset(&tracker_best, 0, sizeof(tracker_best));

    r.rss_before = get_rss_bytes();

    for (int i = 0; i < iterations; ++i) {
        MemTracker tracker;
        memset(&tracker, 0, sizeof(tracker));

        tg3_parse_options opts;
        tg3_parse_options_init(&opts);
        opts.memory.allocator.alloc = tracked_alloc;
        opts.memory.allocator.realloc = tracked_realloc;
        opts.memory.allocator.free = tracked_free;
        opts.memory.allocator.user_data = &tracker;
        opts.parse_float32 = float32_mode;

        tg3_model model;
        tg3_error_stack errors;
        tg3_error_stack_init(&errors);

        TimePoint t0 = Clock::now();

        tg3_error_code err = tg3_parse_auto(&model, &errors,
                                             data.data(), data.size(),
                                             base_dir.c_str(),
                                             (uint32_t)base_dir.size(),
                                             &opts);

        TimePoint t1 = Clock::now();
        double ms = elapsed_ms(t0, t1);
        times.push_back(ms);

        /* Capture model stats on first successful iteration */
        if (i == 0 && err == TG3_OK) {
            r.meshes = model.meshes_count;
            r.nodes = model.nodes_count;
            r.accessors = model.accessors_count;
            r.materials = model.materials_count;
            r.animations = model.animations_count;
            r.buffers = model.buffers_count;
            r.buffer_views = model.buffer_views_count;
            r.images = model.images_count;
            r.textures = model.textures_count;
        }

        if (tracker.peak > tracker_best.peak) {
            tracker_best = tracker;
        }

        tg3_model_free(&model);
        tg3_error_stack_free(&errors);

        if (err != TG3_OK && !quiet) {
            fprintf(stderr, "  Parse error on iteration %d: code %d\n", i, (int)err);
        }
    }

    r.rss_after = get_rss_bytes();
    r.arena_peak = tracker_best.peak;

    /* Compute stats */
    std::sort(times.begin(), times.end());
    r.parse_min = times.front();
    r.parse_max = times.back();
    double sum = 0;
    for (double t : times) sum += t;
    r.parse_avg = sum / times.size();
    r.parse_median = times[times.size() / 2];

    /* Throughput: file_size / median_time */
    if (r.parse_median > 0) {
        r.throughput_mbs = ((double)r.file_size / (1024.0 * 1024.0)) /
                           (r.parse_median / 1000.0);
    }

    return r;
}

/* ------------------------------------------------------------------ */
/* Print results                                                      */
/* ------------------------------------------------------------------ */

static const char *human_bytes(size_t bytes, char *buf, size_t buf_sz) {
    if (bytes >= 1024ULL * 1024 * 1024)
        snprintf(buf, buf_sz, "%.2f GB", (double)bytes / (1024.0 * 1024 * 1024));
    else if (bytes >= 1024 * 1024)
        snprintf(buf, buf_sz, "%.2f MB", (double)bytes / (1024.0 * 1024));
    else if (bytes >= 1024)
        snprintf(buf, buf_sz, "%.2f KB", (double)bytes / 1024.0);
    else
        snprintf(buf, buf_sz, "%zu B", bytes);
    return buf;
}

static void print_result(const BenchResult &r) {
    char buf1[64], buf2[64];

    printf("┌─────────────────────────────────────────────────────────────────┐\n");
    printf("│ %-63s │\n", r.filename.c_str());
    printf("├─────────────────────────────────────────────────────────────────┤\n");
    printf("│ File size:      %-47s │\n", human_bytes((size_t)r.file_size, buf1, sizeof(buf1)));
    printf("│ Iterations:     %-47d │\n", r.iterations);
    printf("│                                                                 │\n");
    printf("│ Parse time (ms):                                                │\n");
    printf("│   min:    %10.3f                                            │\n", r.parse_min);
    printf("│   max:    %10.3f                                            │\n", r.parse_max);
    printf("│   avg:    %10.3f                                            │\n", r.parse_avg);
    printf("│   median: %10.3f                                            │\n", r.parse_median);
    printf("│                                                                 │\n");
    printf("│ Throughput:     %-47s │\n",
           (snprintf(buf1, sizeof(buf1), "%.2f MB/s", r.throughput_mbs), buf1));
    printf("│ Arena peak:     %-47s │\n", human_bytes(r.arena_peak, buf1, sizeof(buf1)));
    if (r.rss_before > 0) {
        printf("│ RSS before:     %-47s │\n", human_bytes(r.rss_before, buf1, sizeof(buf1)));
        printf("│ RSS after:      %-47s │\n", human_bytes(r.rss_after, buf2, sizeof(buf2)));
    }
    printf("│                                                                 │\n");
    printf("│ Model: %u meshes, %u nodes, %u accessors, %u materials",
           r.meshes, r.nodes, r.accessors, r.materials);
    printf("        │\n");
    printf("│        %u animations, %u buffers, %u images",
           r.animations, r.buffers, r.images);
    printf("                       │\n");
    printf("└─────────────────────────────────────────────────────────────────┘\n");
}

static void print_csv_header() {
    printf("file,size_bytes,iterations,parse_min_ms,parse_max_ms,parse_avg_ms,"
           "parse_median_ms,throughput_mbs,arena_peak_bytes,"
           "meshes,nodes,accessors,materials,animations\n");
}

static void print_csv_row(const BenchResult &r) {
    printf("%s,%lu,%d,%.3f,%.3f,%.3f,%.3f,%.2f,%zu,%u,%u,%u,%u,%u\n",
           r.filename.c_str(), (unsigned long)r.file_size, r.iterations,
           r.parse_min, r.parse_max, r.parse_avg, r.parse_median,
           r.throughput_mbs, r.arena_peak,
           r.meshes, r.nodes, r.accessors, r.materials, r.animations);
}

/* ------------------------------------------------------------------ */
/* Main                                                               */
/* ------------------------------------------------------------------ */

static void usage() {
    fprintf(stderr,
        "Usage:\n"
        "  bench_v3 <file> [--iterations N] [--warmup N] [--csv] [--quiet]\n"
        "  bench_v3 --batch <file1> [file2] ... [--iterations N] [--csv]\n"
        "\n"
        "Options:\n"
        "  --iterations N   Number of timed parse iterations (default: 10)\n"
        "  --warmup N       Number of warmup iterations (default: 2)\n"
        "  --csv            Output in CSV format\n"
        "  --quiet          Suppress per-iteration error messages\n"
        "  --batch          Benchmark multiple files\n"
        "  --float32        Parse JSON floats as float32 (faster, less precise)\n");
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 1; }

    int iterations = 10;
    int warmup = 2;
    bool csv = false;
    bool quiet = false;
    int float32_mode = 0;
    std::vector<std::string> files;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            iterations = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            warmup = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--csv") == 0) {
            csv = true;
        } else if (strcmp(argv[i], "--quiet") == 0) {
            quiet = true;
        } else if (strcmp(argv[i], "--float32") == 0) {
            float32_mode = 1;
        } else if (strcmp(argv[i], "--batch") == 0) {
            /* batch mode: just collect files */
        } else if (argv[i][0] != '-') {
            files.push_back(argv[i]);
        }
    }

    if (files.empty()) { usage(); return 1; }

    if (csv) print_csv_header();

    for (const auto &file : files) {
        if (!csv && !quiet) {
            printf("Benchmarking: %s (%d iterations, %d warmup%s)\n",
                   file.c_str(), iterations, warmup,
                   float32_mode ? ", float32" : "");
        }

        BenchResult r = bench_file(file.c_str(), iterations, warmup, quiet, float32_mode);

        if (csv) {
            print_csv_row(r);
        } else {
            print_result(r);
            printf("\n");
        }
    }

    return 0;
}
