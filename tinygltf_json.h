/*
 * tinygltf_json.h - Fast JSON parser for tinygltf
 *
 * The MIT License (MIT)
 * Copyright (c) 2015 - Present Syoyo Fujita, Aurelien Chatelain and many
 * contributors.
 *
 * A custom JSON parser optimized for glTF processing.
 *
 * Design goals:
 *   - C-style implementation core (structs, raw pointers, malloc/free)
 *   - Minimal C++ wrappers for tinygltf interface compatibility
 *   - SIMD-accelerated whitespace skipping and string scanning
 *   - Flat storage arrays for cache-friendly memory layout
 *
 * SIMD activation (default: SIMD disabled):
 *   Define TINYGLTF_JSON_USE_SIMD to auto-detect CPU SIMD support, OR
 *   define one or more of the following explicitly:
 *     TINYGLTF_JSON_SIMD_SSE2   - Enable SSE2 (x86/x86-64)
 *     TINYGLTF_JSON_SIMD_AVX2   - Enable AVX2 (x86-64, implies SSE2)
 *     TINYGLTF_JSON_SIMD_NEON   - Enable ARM NEON
 */

#ifndef TINYGLTF_JSON_H_
#define TINYGLTF_JSON_H_

/* C standard headers (keep these first for C compatibility) */
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* C++ headers (minimal) */
#include <string>
#include <stdexcept>

/* ======================================================================
 * SIMD detection
 * ====================================================================== */

#ifdef TINYGLTF_JSON_USE_SIMD
#  if defined(__AVX2__)
#    define TINYGLTF_JSON_SIMD_AVX2
#  endif
#  if defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) || \
      (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#    define TINYGLTF_JSON_SIMD_SSE2
#  endif
#  if defined(__ARM_NEON) || defined(__ARM_NEON__)
#    define TINYGLTF_JSON_SIMD_NEON
#  endif
#endif

#ifdef TINYGLTF_JSON_SIMD_AVX2
#  include <immintrin.h>
#elif defined(TINYGLTF_JSON_SIMD_SSE2)
#  include <emmintrin.h>
#endif

#ifdef TINYGLTF_JSON_SIMD_NEON
#  include <arm_neon.h>
#endif

/* ======================================================================
 * JSON VALUE TYPE CONSTANTS (C-style integer constants)
 * ====================================================================== */

#define CJ_NULL    0
#define CJ_BOOL    1
#define CJ_INT     2
#define CJ_REAL    3
#define CJ_STRING  4
#define CJ_ARRAY   5
#define CJ_OBJECT  6

/* ======================================================================
 * SIMD WHITESPACE SKIPPING
 *
 * Whitespace characters in JSON: space(0x20), tab(0x09), CR(0x0D), LF(0x0A)
 * ====================================================================== */

static const char *cj_skip_ws_scalar(const char *p, const char *end) {
    while (p < end) {
        unsigned char c = (unsigned char)*p;
        if (c != 0x20u && c != 0x09u && c != 0x0Du && c != 0x0Au) break;
        ++p;
    }
    return p;
}

#if defined(TINYGLTF_JSON_SIMD_AVX2)
#include <immintrin.h>

static const char *cj_skip_ws(const char *p, const char *end) {
    while (p + 32 <= end) {
        __m256i chunk = _mm256_loadu_si256((const __m256i *)(const void *)p);
        __m256i sp  = _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8(' '));
        __m256i tab = _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8('\t'));
        __m256i cr  = _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8('\r'));
        __m256i lf  = _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8('\n'));
        __m256i ws  = _mm256_or_si256(_mm256_or_si256(sp, tab),
                                      _mm256_or_si256(cr, lf));
        unsigned int mask = (unsigned int)_mm256_movemask_epi8(ws);
        if (mask != 0xFFFFFFFFu) {
#if defined(__GNUC__) || defined(__clang__)
            return p + (int)__builtin_ctz(~mask);
#else
            unsigned int inv = ~mask, idx = 0;
            while (!(inv & (1u << idx))) ++idx;
            return p + idx;
#endif
        }
        p += 32;
    }
    return cj_skip_ws_scalar(p, end);
}

#elif defined(TINYGLTF_JSON_SIMD_SSE2)
#include <emmintrin.h>

static const char *cj_skip_ws(const char *p, const char *end) {
    while (p + 16 <= end) {
        __m128i chunk = _mm_loadu_si128((const __m128i *)(const void *)p);
        __m128i sp  = _mm_cmpeq_epi8(chunk, _mm_set1_epi8(' '));
        __m128i tab = _mm_cmpeq_epi8(chunk, _mm_set1_epi8('\t'));
        __m128i cr  = _mm_cmpeq_epi8(chunk, _mm_set1_epi8('\r'));
        __m128i lf  = _mm_cmpeq_epi8(chunk, _mm_set1_epi8('\n'));
        __m128i ws  = _mm_or_si128(_mm_or_si128(sp, tab),
                                   _mm_or_si128(cr, lf));
        unsigned int mask = (unsigned int)_mm_movemask_epi8(ws);
        if (mask != 0xFFFFu) {
            unsigned int inv = (~mask) & 0xFFFFu;
#if defined(__GNUC__) || defined(__clang__)
            return p + (int)__builtin_ctz(inv);
#else
            unsigned int idx = 0;
            while (!(inv & (1u << idx))) ++idx;
            return p + idx;
#endif
        }
        p += 16;
    }
    return cj_skip_ws_scalar(p, end);
}

#elif defined(TINYGLTF_JSON_SIMD_NEON)
#include <arm_neon.h>

static const char *cj_skip_ws(const char *p, const char *end) {
    while (p + 16 <= end) {
        uint8x16_t chunk = vld1q_u8((const uint8_t *)p);
        uint8x16_t sp    = vceqq_u8(chunk, vdupq_n_u8(' '));
        uint8x16_t tab   = vceqq_u8(chunk, vdupq_n_u8('\t'));
        uint8x16_t cr    = vceqq_u8(chunk, vdupq_n_u8('\r'));
        uint8x16_t lf    = vceqq_u8(chunk, vdupq_n_u8('\n'));
        uint8x16_t ws    = vorrq_u8(vorrq_u8(sp, tab), vorrq_u8(cr, lf));
        uint64x2_t ws64  = vreinterpretq_u64_u8(ws);
        uint64_t   lo    = vgetq_lane_u64(ws64, 0);
        uint64_t   hi    = vgetq_lane_u64(ws64, 1);
        if (lo != UINT64_C(0xFFFFFFFFFFFFFFFF) ||
            hi != UINT64_C(0xFFFFFFFFFFFFFFFF)) {
            uint8_t tmp[16];
            vst1q_u8(tmp, ws);
            for (int i = 0; i < 16; ++i) {
                if (!tmp[i]) return p + i;
            }
        }
        p += 16;
    }
    return cj_skip_ws_scalar(p, end);
}

#else

static const char *cj_skip_ws(const char *p, const char *end) {
    return cj_skip_ws_scalar(p, end);
}

#endif /* SIMD whitespace */

/* ======================================================================
 * SIMD STRING SCANNING (find '"', '\', or control char)
 * ====================================================================== */

static const char *cj_scan_str_scalar(const char *p, const char *end) {
    while (p < end) {
        unsigned char c = (unsigned char)*p;
        if (c == '"' || c == '\\' || c < 0x20u) break;
        ++p;
    }
    return p;
}

#if defined(TINYGLTF_JSON_SIMD_AVX2)

static const char *cj_scan_str(const char *p, const char *end) {
    while (p + 32 <= end) {
        __m256i chunk  = _mm256_loadu_si256((const __m256i *)(const void *)p);
        __m256i eq_q   = _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8('"'));
        __m256i eq_bs  = _mm256_cmpeq_epi8(chunk, _mm256_set1_epi8('\\'));
        /* Control chars: byte <= 0x1F  <=>  min(byte, 0x1F) == byte */
        __m256i ctrl   = _mm256_cmpeq_epi8(
                             _mm256_min_epu8(chunk, _mm256_set1_epi8(0x1F)),
                             chunk);
        __m256i special = _mm256_or_si256(_mm256_or_si256(eq_q, eq_bs), ctrl);
        unsigned int mask = (unsigned int)_mm256_movemask_epi8(special);
        if (mask) {
#if defined(__GNUC__) || defined(__clang__)
            return p + (int)__builtin_ctz(mask);
#else
            unsigned int idx = 0;
            while (!(mask & (1u << idx))) ++idx;
            return p + idx;
#endif
        }
        p += 32;
    }
    return cj_scan_str_scalar(p, end);
}

#elif defined(TINYGLTF_JSON_SIMD_SSE2)

static const char *cj_scan_str(const char *p, const char *end) {
    while (p + 16 <= end) {
        __m128i chunk   = _mm_loadu_si128((const __m128i *)(const void *)p);
        __m128i eq_q    = _mm_cmpeq_epi8(chunk, _mm_set1_epi8('"'));
        __m128i eq_bs   = _mm_cmpeq_epi8(chunk, _mm_set1_epi8('\\'));
        __m128i ctrl    = _mm_cmpeq_epi8(
                              _mm_min_epu8(chunk, _mm_set1_epi8(0x1F)),
                              chunk);
        __m128i special = _mm_or_si128(_mm_or_si128(eq_q, eq_bs), ctrl);
        unsigned int mask = (unsigned int)_mm_movemask_epi8(special);
        if (mask) {
#if defined(__GNUC__) || defined(__clang__)
            return p + (int)__builtin_ctz(mask);
#else
            unsigned int idx = 0;
            while (!(mask & (1u << idx))) ++idx;
            return p + idx;
#endif
        }
        p += 16;
    }
    return cj_scan_str_scalar(p, end);
}

#elif defined(TINYGLTF_JSON_SIMD_NEON)

static const char *cj_scan_str(const char *p, const char *end) {
    uint8x16_t vquote  = vdupq_n_u8('"');
    uint8x16_t vbslash = vdupq_n_u8('\\');
    uint8x16_t v20     = vdupq_n_u8(0x20u);
    while (p + 16 <= end) {
        uint8x16_t chunk   = vld1q_u8((const uint8_t *)p);
        uint8x16_t eq_q    = vceqq_u8(chunk, vquote);
        uint8x16_t eq_bs   = vceqq_u8(chunk, vbslash);
        uint8x16_t ctrl    = vcltq_u8(chunk, v20);
        uint8x16_t special = vorrq_u8(vorrq_u8(eq_q, eq_bs), ctrl);
        uint64x2_t s64     = vreinterpretq_u64_u8(special);
        if (vgetq_lane_u64(s64, 0) || vgetq_lane_u64(s64, 1)) {
            uint8_t tmp[16];
            vst1q_u8(tmp, special);
            for (int i = 0; i < 16; ++i) {
                if (tmp[i]) return p + i;
            }
        }
        p += 16;
    }
    return cj_scan_str_scalar(p, end);
}

#else

static const char *cj_scan_str(const char *p, const char *end) {
    return cj_scan_str_scalar(p, end);
}

#endif /* SIMD string scan */

/* ======================================================================
 * FAST NUMBER PARSING (C-style)
 * ====================================================================== */

static const char *cj_parse_uint64(const char *p, const char *end,
                                   uint64_t *result) {
    uint64_t v = 0;
    while (p < end && (unsigned)(*p - '0') <= 9u) {
        v = v * 10u + (uint64_t)((unsigned char)*p - '0');
        ++p;
    }
    *result = v;
    return p;
}

/*
 * Parse a JSON number starting at [p, end).
 * Sets *is_int, *ival (integer result), *dval (floating-point result).
 * Returns pointer past the last character consumed, or NULL on error.
 */
static const char *cj_parse_number(const char *p, const char *end,
                                   int *is_int, int64_t *ival, double *dval) {
    const char *start = p;
    int neg = 0;
    if (p < end && *p == '-') { neg = 1; ++p; }
    if (p >= end) return NULL;

    uint64_t int_part = 0;
    int has_frac = 0, has_exp = 0;

    if (*p == '0') {
        ++p;
    } else if ((unsigned)(*p - '1') <= 8u) {
        p = cj_parse_uint64(p, end, &int_part);
    } else {
        return NULL;
    }

    if (p < end && *p == '.') has_frac = 1;
    if (p < end && (*p == 'e' || *p == 'E')) has_exp = 1;

    if (!has_frac && !has_exp) {
        int64_t sv = neg ? -(int64_t)int_part : (int64_t)int_part;
        *is_int = 1;
        *ival   = sv;
        *dval   = (double)sv;
        return p;
    }

    /* Floating-point: use strtod for correctness */
    char *eptr = NULL;
    double d = strtod(start, &eptr);
    if (eptr == start) return NULL;
    *is_int = 0;
    *dval   = d;
    *ival   = (int64_t)d;
    return eptr;
}

/* ======================================================================
 * STRING UNESCAPING (C-style)
 * ====================================================================== */

static int cj_hex4(const char *p) {
    int v = 0;
    for (int i = 0; i < 4; ++i) {
        char c = p[i];
        int d;
        if      (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else return -1;
        v = (v << 4) | d;
    }
    return v;
}

static int cj_encode_utf8(unsigned int cp, char *buf) {
    if (cp <= 0x7Fu) {
        buf[0] = (char)cp;
        return 1;
    } else if (cp <= 0x7FFu) {
        buf[0] = (char)(0xC0u | (cp >> 6));
        buf[1] = (char)(0x80u | (cp & 0x3Fu));
        return 2;
    } else if (cp <= 0xFFFFu) {
        buf[0] = (char)(0xE0u | (cp >> 12));
        buf[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        buf[2] = (char)(0x80u | (cp & 0x3Fu));
        return 3;
    } else if (cp <= 0x10FFFFu) {
        buf[0] = (char)(0xF0u | (cp >> 18));
        buf[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
        buf[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        buf[3] = (char)(0x80u | (cp & 0x3Fu));
        return 4;
    }
    return 0;
}

/*
 * Parse and unescape a JSON string from [p, end) where p is AFTER the
 * opening '"' and end is the INCLUSIVE closing '"'.
 * Caller must free() the returned pointer.
 * Returns NULL on allocation failure.
 */
static char *cj_unescape_string(const char *p, const char *str_end,
                                 size_t *out_len) {
    size_t alloc = (size_t)(str_end - p) + 1;
    char *out    = (char *)malloc(alloc);
    if (!out) return NULL;
    char *dst = out;

    while (p < str_end) {
        p = cj_scan_str(p, str_end);
        if (p >= str_end) break;

        unsigned char c = (unsigned char)*p;
        if (c == '"') {
            break; /* should not happen - caller passes str_end=position of '"' */
        } else if (c == '\\') {
            ++p;
            if (p >= str_end) { free(out); return NULL; }
            unsigned char esc = (unsigned char)*p++;
            switch (esc) {
                case '"':  *dst++ = '"';  break;
                case '\\': *dst++ = '\\'; break;
                case '/':  *dst++ = '/';  break;
                case 'b':  *dst++ = '\b'; break;
                case 'f':  *dst++ = '\f'; break;
                case 'n':  *dst++ = '\n'; break;
                case 'r':  *dst++ = '\r'; break;
                case 't':  *dst++ = '\t'; break;
                case 'u': {
                    if (p + 4 > str_end) { free(out); return NULL; }
                    int cp = cj_hex4(p);
                    if (cp < 0) { free(out); return NULL; }
                    p += 4;
                    if (cp >= 0xD800 && cp <= 0xDBFF &&
                        p + 6 <= str_end && p[0] == '\\' && p[1] == 'u') {
                        int cp2 = cj_hex4(p + 2);
                        if (cp2 >= 0xDC00 && cp2 <= 0xDFFF) {
                            unsigned int full = 0x10000u +
                                (((unsigned int)cp - 0xD800u) << 10) +
                                ((unsigned int)cp2 - 0xDC00u);
                            dst += cj_encode_utf8(full, dst);
                            p += 6;
                            break;
                        }
                    }
                    dst += cj_encode_utf8((unsigned int)cp, dst);
                    break;
                }
                default:
                    *dst++ = (char)esc;
                    break;
            }
        } else if (c < 0x20u) {
            ++p; /* skip invalid control char */
        } else {
            /* Should not be reached since scan_str stops here only for
               special chars - but guard just in case */
            *dst++ = (char)c;
            ++p;
        }
    }

    *dst = '\0';
    *out_len = (size_t)(dst - out);
    return out;
}

/* ======================================================================
 * FORWARD DECLARATIONS
 * ====================================================================== */

/*
 * tinygltf_json is the main JSON value class.
 * Its data layout is C-style (all members public, named with trailing _).
 */
class tinygltf_json;

/*
 * tinygltf_json_member stores one key-value pair in a JSON object.
 * Must be defined AFTER tinygltf_json is complete (contains json by value).
 */
struct tinygltf_json_member;

/* ======================================================================
 * tinygltf_json CLASS DECLARATION
 *
 * All data members are public (C-style struct convention).
 * Methods are declared here and implemented after tinygltf_json_member
 * is fully defined.
 * ====================================================================== */

class tinygltf_json {
public:
    /* ------------------------------------------------------------------
     * nlohmann-compatible value type enum
     * ------------------------------------------------------------------ */
    enum class value_t : uint8_t {
        null             = 0,
        boolean          = 1,
        number_integer   = 2,
        number_unsigned  = 3,
        number_float     = 4,
        string           = 5,
        array            = 6,
        object           = 7,
        /* Aliases from nlohmann/json that tinygltf.h references */
        discarded        = 8,
        binary           = 9
    };

    /* ------------------------------------------------------------------
     * C-style data storage (all public for direct access from C functions)
     * ------------------------------------------------------------------ */

    /* Type tag: one of CJ_NULL, CJ_BOOL, CJ_INT, CJ_REAL, CJ_STRING,
       CJ_ARRAY, CJ_OBJECT */
    int type_;

    /* Primitive values (union for space efficiency) */
    union {
        int64_t i_;   /* CJ_INT  */
        double  d_;   /* CJ_REAL */
        int     b_;   /* CJ_BOOL: 0 or 1 */
    };

    /* String storage */
    char   *str_;      /* CJ_STRING: owned, null-terminated */
    size_t  str_len_;

    /* Array storage: flat array of tinygltf_json objects (owned) */
    tinygltf_json *arr_data_;
    size_t         arr_size_;
    size_t         arr_cap_;

    /* Object storage: flat array of tinygltf_json_member (owned) */
    tinygltf_json_member *obj_data_;
    size_t                obj_size_;
    size_t                obj_cap_;

    /* ------------------------------------------------------------------
     * Iterator type (forward-declared here, defined later)
     * ------------------------------------------------------------------ */
    class iterator;
    using const_iterator = iterator;

    /* ------------------------------------------------------------------
     * Low-level helpers (implementations deferred until member is complete)
     * ------------------------------------------------------------------ */
    void init_null_();
    void destroy_();
    void copy_from_(const tinygltf_json &o);
    tinygltf_json_member *find_member_(const char *key) const;
    int  obj_reserve_();
    int  arr_reserve_();
    void make_object_();
    void make_array_();

    /* ------------------------------------------------------------------
     * Constructors and destructor
     * ------------------------------------------------------------------ */
    tinygltf_json();
    tinygltf_json(std::nullptr_t);
    tinygltf_json(bool b);
    tinygltf_json(int i);
    tinygltf_json(int64_t i);
    tinygltf_json(uint64_t u);
    tinygltf_json(double d);
    tinygltf_json(float f);
    tinygltf_json(const char *s);
    tinygltf_json(const std::string &s);
    tinygltf_json(const tinygltf_json &o);
    tinygltf_json(tinygltf_json &&o) noexcept;
    ~tinygltf_json();

    tinygltf_json &operator=(const tinygltf_json &o);
    tinygltf_json &operator=(tinygltf_json &&o) noexcept;

    /* ------------------------------------------------------------------
     * Type checks (nlohmann-compatible)
     * ------------------------------------------------------------------ */
    value_t type() const;
    bool is_null()            const { return type_ == CJ_NULL; }
    bool is_boolean()         const { return type_ == CJ_BOOL; }
    bool is_number()          const { return type_ == CJ_INT || type_ == CJ_REAL; }
    bool is_number_integer()  const { return type_ == CJ_INT; }
    bool is_number_unsigned() const { return type_ == CJ_INT && i_ >= 0; }
    bool is_number_float()    const { return type_ == CJ_REAL; }
    bool is_string()          const { return type_ == CJ_STRING; }
    bool is_array()           const { return type_ == CJ_ARRAY; }
    bool is_object()          const { return type_ == CJ_OBJECT; }

    /* ------------------------------------------------------------------
     * Value access (template specializations after class)
     * ------------------------------------------------------------------ */
    template<typename T> T get() const;

    /* ------------------------------------------------------------------
     * Container methods
     * ------------------------------------------------------------------ */
    size_t size() const;
    bool   empty() const;

    /* ------------------------------------------------------------------
     * Array operations
     * ------------------------------------------------------------------ */
    void push_back(tinygltf_json &&v);
    void push_back(const tinygltf_json &v);

    /* ------------------------------------------------------------------
     * Object operations
     * ------------------------------------------------------------------ */
    tinygltf_json &operator[](const char *key);
    tinygltf_json &operator[](const std::string &key);

    /* ------------------------------------------------------------------
     * Iterators
     * ------------------------------------------------------------------ */
    iterator begin();
    iterator end();
    iterator begin() const;
    iterator end() const;
    iterator find(const char *key) const;
    iterator find(const char *key);
    void erase(iterator &it);

    /* ------------------------------------------------------------------
     * Static factories
     * ------------------------------------------------------------------ */
    static tinygltf_json object();

    /* ------------------------------------------------------------------
     * Serialization / deserialization
     * ------------------------------------------------------------------ */
    std::string dump(int indent = -1) const;

    static tinygltf_json parse(const char *first, const char *last,
                               std::nullptr_t = nullptr,
                               bool allow_exceptions = true);
};

/* ======================================================================
 * tinygltf_json_member FULL DEFINITION
 * (tinygltf_json must be complete before this)
 * ====================================================================== */

struct tinygltf_json_member {
    char             *key;      /* owned, null-terminated */
    size_t            key_len;
    tinygltf_json     val;      /* value stored inline */

    tinygltf_json_member() : key(NULL), key_len(0), val() {}
    ~tinygltf_json_member() { free(key); key = NULL; }

    tinygltf_json_member(const tinygltf_json_member &o)
        : key(NULL), key_len(o.key_len), val(o.val) {
        if (o.key) {
            key = (char *)malloc(o.key_len + 1);
            if (key) memcpy(key, o.key, o.key_len + 1);
        }
    }

    tinygltf_json_member(tinygltf_json_member &&o) noexcept
        : key(o.key), key_len(o.key_len),
          val(static_cast<tinygltf_json &&>(o.val)) {
        o.key = NULL;
        o.key_len = 0;
    }

    tinygltf_json_member &operator=(const tinygltf_json_member &o) {
        if (this != &o) {
            free(key);
            key = NULL;
            key_len = o.key_len;
            val = o.val;
            if (o.key) {
                key = (char *)malloc(o.key_len + 1);
                if (key) memcpy(key, o.key, o.key_len + 1);
            }
        }
        return *this;
    }

    tinygltf_json_member &operator=(tinygltf_json_member &&o) noexcept {
        if (this != &o) {
            free(key);
            key     = o.key;
            key_len = o.key_len;
            val     = static_cast<tinygltf_json &&>(o.val);
            o.key     = NULL;
            o.key_len = 0;
        }
        return *this;
    }
};

/* ======================================================================
 * tinygltf_json::iterator
 * ====================================================================== */

class tinygltf_json::iterator {
public:
    static const int MODE_ARRAY  = 0;
    static const int MODE_OBJECT = 1;

    int mode_;
    union {
        tinygltf_json        *arr_ptr_;
        tinygltf_json_member *obj_ptr_;
    };

    iterator() : mode_(MODE_ARRAY), arr_ptr_(NULL) {}

    explicit iterator(tinygltf_json *p)
        : mode_(MODE_ARRAY), arr_ptr_(p) {}

    explicit iterator(tinygltf_json_member *p)
        : mode_(MODE_OBJECT), obj_ptr_(p) {}

    /* Pre-increment */
    iterator &operator++() {
        if (mode_ == MODE_ARRAY)  ++arr_ptr_;
        else                      ++obj_ptr_;
        return *this;
    }

    /* Post-increment */
    iterator operator++(int) {
        iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    tinygltf_json &operator*() {
        return (mode_ == MODE_ARRAY) ? *arr_ptr_ : obj_ptr_->val;
    }
    const tinygltf_json &operator*() const {
        return (mode_ == MODE_ARRAY) ? *arr_ptr_ : obj_ptr_->val;
    }
    tinygltf_json *operator->() {
        return (mode_ == MODE_ARRAY) ? arr_ptr_ : &obj_ptr_->val;
    }
    const tinygltf_json *operator->() const {
        return (mode_ == MODE_ARRAY) ? arr_ptr_ : &obj_ptr_->val;
    }

    std::string key() const {
        if (mode_ == MODE_OBJECT && obj_ptr_ && obj_ptr_->key)
            return std::string(obj_ptr_->key, obj_ptr_->key_len);
        return std::string();
    }

    tinygltf_json &value() {
        return operator*();
    }
    const tinygltf_json &value() const {
        return operator*();
    }

    bool operator==(const iterator &o) const {
        if (mode_ != o.mode_) return false;
        return (mode_ == MODE_ARRAY)
            ? (arr_ptr_ == o.arr_ptr_)
            : (obj_ptr_ == o.obj_ptr_);
    }
    bool operator!=(const iterator &o) const { return !(*this == o); }
};

/* ======================================================================
 * tinygltf_json METHOD IMPLEMENTATIONS
 * (Now that tinygltf_json_member is fully defined)
 * ====================================================================== */

inline void tinygltf_json::init_null_() {
    type_     = CJ_NULL;
    i_        = 0;
    str_      = NULL;  str_len_  = 0;
    arr_data_ = NULL;  arr_size_ = 0; arr_cap_ = 0;
    obj_data_ = NULL;  obj_size_ = 0; obj_cap_ = 0;
}

inline void tinygltf_json::destroy_() {
    if (type_ == CJ_STRING) {
        free(str_);
        str_ = NULL;
    } else if (type_ == CJ_ARRAY) {
        for (size_t i = 0; i < arr_size_; ++i)
            arr_data_[i].~tinygltf_json();
        free(arr_data_);
        arr_data_ = NULL;
        arr_size_ = arr_cap_ = 0;
    } else if (type_ == CJ_OBJECT) {
        for (size_t i = 0; i < obj_size_; ++i)
            obj_data_[i].~tinygltf_json_member();
        free(obj_data_);
        obj_data_ = NULL;
        obj_size_ = obj_cap_ = 0;
    }
    type_ = CJ_NULL;
}

inline void tinygltf_json::copy_from_(const tinygltf_json &o) {
    type_     = o.type_;
    i_        = o.i_;
    str_      = NULL; str_len_  = 0;
    arr_data_ = NULL; arr_size_ = 0; arr_cap_ = 0;
    obj_data_ = NULL; obj_size_ = 0; obj_cap_ = 0;

    if (o.type_ == CJ_STRING) {
        if (o.str_) {
            str_len_ = o.str_len_;
            str_ = (char *)malloc(str_len_ + 1);
            if (str_) memcpy(str_, o.str_, str_len_ + 1);
        }
    } else if (o.type_ == CJ_ARRAY) {
        if (o.arr_size_ > 0) {
            arr_data_ = (tinygltf_json *)malloc(
                o.arr_size_ * sizeof(tinygltf_json));
            if (arr_data_) {
                arr_size_ = o.arr_size_;
                arr_cap_  = o.arr_size_;
                for (size_t i = 0; i < arr_size_; ++i)
                    new (&arr_data_[i]) tinygltf_json(o.arr_data_[i]);
            }
        }
    } else if (o.type_ == CJ_OBJECT) {
        if (o.obj_size_ > 0) {
            obj_data_ = (tinygltf_json_member *)malloc(
                o.obj_size_ * sizeof(tinygltf_json_member));
            if (obj_data_) {
                obj_size_ = 0;
                obj_cap_  = o.obj_size_;
                for (size_t i = 0; i < o.obj_size_; ++i) {
                    new (&obj_data_[i]) tinygltf_json_member(o.obj_data_[i]);
                    ++obj_size_;
                }
            }
        }
    }
}

inline tinygltf_json_member *tinygltf_json::find_member_(
    const char *key) const {
    size_t klen = strlen(key);
    for (size_t i = 0; i < obj_size_; ++i) {
        if (obj_data_[i].key_len == klen &&
            memcmp(obj_data_[i].key, key, klen) == 0)
            return &obj_data_[i];
    }
    return NULL;
}

inline int tinygltf_json::obj_reserve_() {
    if (obj_size_ < obj_cap_) return 1;
    size_t new_cap = obj_cap_ ? obj_cap_ * 2 : 8;
    tinygltf_json_member *nd = (tinygltf_json_member *)malloc(
        new_cap * sizeof(tinygltf_json_member));
    if (!nd) return 0;
    for (size_t i = 0; i < obj_size_; ++i) {
        new (&nd[i]) tinygltf_json_member(
            static_cast<tinygltf_json_member &&>(obj_data_[i]));
        obj_data_[i].~tinygltf_json_member();
    }
    free(obj_data_);
    obj_data_ = nd;
    obj_cap_  = new_cap;
    return 1;
}

inline int tinygltf_json::arr_reserve_() {
    if (arr_size_ < arr_cap_) return 1;
    size_t new_cap = arr_cap_ ? arr_cap_ * 2 : 8;
    tinygltf_json *nd = (tinygltf_json *)malloc(
        new_cap * sizeof(tinygltf_json));
    if (!nd) return 0;
    for (size_t i = 0; i < arr_size_; ++i) {
        new (&nd[i]) tinygltf_json(
            static_cast<tinygltf_json &&>(arr_data_[i]));
        arr_data_[i].destroy_();
        arr_data_[i].type_ = CJ_NULL;
    }
    free(arr_data_);
    arr_data_ = nd;
    arr_cap_  = new_cap;
    return 1;
}

inline void tinygltf_json::make_object_() {
    destroy_();
    type_ = CJ_OBJECT;
}

inline void tinygltf_json::make_array_() {
    destroy_();
    type_ = CJ_ARRAY;
}

/* Constructors */
inline tinygltf_json::tinygltf_json()           { init_null_(); }
inline tinygltf_json::tinygltf_json(std::nullptr_t) { init_null_(); }
inline tinygltf_json::tinygltf_json(bool b)     { init_null_(); type_ = CJ_BOOL; b_ = b ? 1 : 0; }
inline tinygltf_json::tinygltf_json(int i)      { init_null_(); type_ = CJ_INT;  i_ = (int64_t)i; }
inline tinygltf_json::tinygltf_json(int64_t i)  { init_null_(); type_ = CJ_INT;  i_ = i; }
inline tinygltf_json::tinygltf_json(uint64_t u) { init_null_(); type_ = CJ_INT;  i_ = (int64_t)u; }
inline tinygltf_json::tinygltf_json(double d)   { init_null_(); type_ = CJ_REAL; d_ = d; }
inline tinygltf_json::tinygltf_json(float f)    { init_null_(); type_ = CJ_REAL; d_ = (double)f; }

inline tinygltf_json::tinygltf_json(const char *s) {
    init_null_();
    if (s) {
        type_    = CJ_STRING;
        str_len_ = strlen(s);
        str_     = (char *)malloc(str_len_ + 1);
        if (str_) memcpy(str_, s, str_len_ + 1);
    }
}

inline tinygltf_json::tinygltf_json(const std::string &s) {
    init_null_();
    type_    = CJ_STRING;
    str_len_ = s.size();
    str_     = (char *)malloc(str_len_ + 1);
    if (str_) memcpy(str_, s.c_str(), str_len_ + 1);
}

inline tinygltf_json::tinygltf_json(const tinygltf_json &o) {
    init_null_();
    copy_from_(o);
}

inline tinygltf_json::tinygltf_json(tinygltf_json &&o) noexcept {
    type_     = o.type_;
    i_        = o.i_;
    str_      = o.str_;
    str_len_  = o.str_len_;
    arr_data_ = o.arr_data_;  arr_size_ = o.arr_size_;  arr_cap_ = o.arr_cap_;
    obj_data_ = o.obj_data_;  obj_size_ = o.obj_size_;  obj_cap_ = o.obj_cap_;
    o.type_     = CJ_NULL;
    o.str_      = NULL;
    o.arr_data_ = NULL;  o.arr_size_ = 0;  o.arr_cap_ = 0;
    o.obj_data_ = NULL;  o.obj_size_ = 0;  o.obj_cap_ = 0;
}

inline tinygltf_json::~tinygltf_json() { destroy_(); }

inline tinygltf_json &tinygltf_json::operator=(const tinygltf_json &o) {
    if (this != &o) { destroy_(); copy_from_(o); }
    return *this;
}

inline tinygltf_json &tinygltf_json::operator=(tinygltf_json &&o) noexcept {
    if (this != &o) {
        destroy_();
        type_     = o.type_;
        i_        = o.i_;
        str_      = o.str_;
        str_len_  = o.str_len_;
        arr_data_ = o.arr_data_;  arr_size_ = o.arr_size_;  arr_cap_ = o.arr_cap_;
        obj_data_ = o.obj_data_;  obj_size_ = o.obj_size_;  obj_cap_ = o.obj_cap_;
        o.type_     = CJ_NULL;
        o.str_      = NULL;
        o.arr_data_ = NULL;  o.arr_size_ = 0;  o.arr_cap_ = 0;
        o.obj_data_ = NULL;  o.obj_size_ = 0;  o.obj_cap_ = 0;
    }
    return *this;
}

inline tinygltf_json::value_t tinygltf_json::type() const {
    switch (type_) {
        case CJ_NULL:   return value_t::null;
        case CJ_BOOL:   return value_t::boolean;
        case CJ_INT:    return i_ >= 0 ? value_t::number_unsigned
                                       : value_t::number_integer;
        case CJ_REAL:   return value_t::number_float;
        case CJ_STRING: return value_t::string;
        case CJ_ARRAY:  return value_t::array;
        case CJ_OBJECT: return value_t::object;
        default:        return value_t::null;
    }
}

inline size_t tinygltf_json::size() const {
    if (type_ == CJ_ARRAY)  return arr_size_;
    if (type_ == CJ_OBJECT) return obj_size_;
    return 0;
}

inline bool tinygltf_json::empty() const {
    if (type_ == CJ_ARRAY)  return arr_size_ == 0;
    if (type_ == CJ_OBJECT) return obj_size_ == 0;
    return true;
}

inline void tinygltf_json::push_back(tinygltf_json &&v) {
    if (type_ != CJ_ARRAY) make_array_();
    if (!arr_reserve_()) return;
    new (&arr_data_[arr_size_]) tinygltf_json(
        static_cast<tinygltf_json &&>(v));
    ++arr_size_;
}

inline void tinygltf_json::push_back(const tinygltf_json &v) {
    push_back(tinygltf_json(v));
}

inline tinygltf_json &tinygltf_json::operator[](const char *key) {
    if (type_ != CJ_OBJECT) make_object_();
    tinygltf_json_member *m = find_member_(key);
    if (m) return m->val;
    if (!obj_reserve_()) {
        static tinygltf_json null_fallback;
        return null_fallback;
    }
    tinygltf_json_member *nm = &obj_data_[obj_size_];
    new (nm) tinygltf_json_member();
    size_t klen = strlen(key);
    nm->key = (char *)malloc(klen + 1);
    if (nm->key) {
        memcpy(nm->key, key, klen + 1);
        nm->key_len = klen;
    }
    ++obj_size_;
    return nm->val;
}

inline tinygltf_json &tinygltf_json::operator[](const std::string &key) {
    return operator[](key.c_str());
}

inline tinygltf_json::iterator tinygltf_json::begin() {
    if (type_ == CJ_ARRAY)  return iterator(arr_data_);
    if (type_ == CJ_OBJECT) return iterator(obj_data_);
    return iterator((tinygltf_json *)NULL);
}
inline tinygltf_json::iterator tinygltf_json::end() {
    if (type_ == CJ_ARRAY)  return iterator(arr_data_ + arr_size_);
    if (type_ == CJ_OBJECT) return iterator(obj_data_ + obj_size_);
    return iterator((tinygltf_json *)NULL);
}
inline tinygltf_json::iterator tinygltf_json::begin() const {
    tinygltf_json *self = const_cast<tinygltf_json *>(this);
    return self->begin();
}
inline tinygltf_json::iterator tinygltf_json::end() const {
    tinygltf_json *self = const_cast<tinygltf_json *>(this);
    return self->end();
}

inline tinygltf_json::iterator tinygltf_json::find(const char *key) {
    if (type_ == CJ_OBJECT) {
        tinygltf_json_member *m = find_member_(key);
        if (m) return iterator(m);
        return iterator(obj_data_ + obj_size_);
    }
    return iterator((tinygltf_json *)NULL);
}
inline tinygltf_json::iterator tinygltf_json::find(const char *key) const {
    return const_cast<tinygltf_json *>(this)->find(key);
}

inline void tinygltf_json::erase(tinygltf_json::iterator &it) {
    if (type_ != CJ_OBJECT || it.mode_ != iterator::MODE_OBJECT) return;
    ptrdiff_t idx = it.obj_ptr_ - obj_data_;
    if (idx < 0 || (size_t)idx >= obj_size_) return;
    obj_data_[idx].~tinygltf_json_member();
    for (size_t i = (size_t)idx; i + 1 < obj_size_; ++i) {
        new (&obj_data_[i]) tinygltf_json_member(
            static_cast<tinygltf_json_member &&>(obj_data_[i + 1]));
        obj_data_[i + 1].~tinygltf_json_member();
    }
    --obj_size_;
    it = end();
}

inline tinygltf_json tinygltf_json::object() {
    tinygltf_json j;
    j.make_object_();
    return j;
}

/* ======================================================================
 * get<T>() specializations
 * ====================================================================== */

template<> inline double tinygltf_json::get<double>() const {
    if (type_ == CJ_REAL) return d_;
    if (type_ == CJ_INT)  return (double)i_;
    return 0.0;
}

template<> inline int tinygltf_json::get<int>() const {
    if (type_ == CJ_INT)  return (int)i_;
    if (type_ == CJ_REAL) return (int)d_;
    return 0;
}

template<> inline int64_t tinygltf_json::get<int64_t>() const {
    if (type_ == CJ_INT)  return i_;
    if (type_ == CJ_REAL) return (int64_t)d_;
    return 0;
}

template<> inline uint64_t tinygltf_json::get<uint64_t>() const {
    if (type_ == CJ_INT)  return (uint64_t)i_;
    if (type_ == CJ_REAL) return (uint64_t)d_;
    return 0;
}

template<> inline bool tinygltf_json::get<bool>() const {
    if (type_ == CJ_BOOL) return b_ != 0;
    return false;
}

template<> inline std::string tinygltf_json::get<std::string>() const {
    if (type_ == CJ_STRING && str_)
        return std::string(str_, str_len_);
    return std::string();
}

/* ======================================================================
 * PARSER (C-style recursive descent)
 * ====================================================================== */

struct cj_parse_ctx {
    const char *cur;
    const char *end;
    int         err;
    char        errmsg[256];
};

static void cj_ctx_error(cj_parse_ctx *ctx, const char *msg) {
    if (!ctx->err) {
        ctx->err = 1;
        strncpy(ctx->errmsg, msg, sizeof(ctx->errmsg) - 1);
        ctx->errmsg[sizeof(ctx->errmsg) - 1] = '\0';
    }
}

/* Forward declaration */
static void cj_parse_value(cj_parse_ctx *ctx, tinygltf_json *out);

/*
 * Parse a JSON string from the current position.
 * cur must point to the opening '"'.
 * On success, advances cur past the closing '"' and sets *out_str (owned).
 */
static void cj_parse_string_to(cj_parse_ctx *ctx, char **out_str,
                                size_t *out_len) {
    assert(ctx->cur < ctx->end && *ctx->cur == '"');
    ++ctx->cur; /* skip opening '"' */

    const char *p = ctx->cur;

    /* Fast path: find closing '"' without escapes */
    while (p < ctx->end) {
        p = cj_scan_str(p, ctx->end);
        if (p >= ctx->end) {
            cj_ctx_error(ctx, "unterminated string");
            *out_str = NULL; *out_len = 0;
            return;
        }
        if (*p == '"') {
            /* No escapes: copy directly */
            size_t len = (size_t)(p - ctx->cur);
            char *s = (char *)malloc(len + 1);
            if (!s) { cj_ctx_error(ctx, "out of memory"); *out_str = NULL; *out_len = 0; return; }
            memcpy(s, ctx->cur, len);
            s[len] = '\0';
            *out_str = s;
            *out_len = len;
            ctx->cur = p + 1;
            return;
        }
        if (*p == '\\') {
            /* Has escapes: find true end, then unescape */
            const char *scan = p;
            while (scan < ctx->end) {
                scan = cj_scan_str(scan, ctx->end);
                if (scan >= ctx->end) { cj_ctx_error(ctx, "unterminated string"); *out_str = NULL; *out_len = 0; return; }
                if (*scan == '"') break;
                if (*scan == '\\') {
                    ++scan;
                    if (scan >= ctx->end) { cj_ctx_error(ctx, "truncated escape"); *out_str = NULL; *out_len = 0; return; }
                    if (*scan == 'u') scan += 5;
                    else              ++scan;
                }
            }
            if (ctx->err) { *out_str = NULL; *out_len = 0; return; }
            *out_str = cj_unescape_string(ctx->cur, scan, out_len);
            if (!*out_str) { cj_ctx_error(ctx, "string unescape failed"); }
            ctx->cur = scan + 1;
            return;
        }
        /* Control char (< 0x20) - skip */
        ++p;
    }
    cj_ctx_error(ctx, "unterminated string");
    *out_str = NULL; *out_len = 0;
}

static void cj_parse_array(cj_parse_ctx *ctx, tinygltf_json *out) {
    assert(*ctx->cur == '[');
    ++ctx->cur;
    out->make_array_();

    ctx->cur = cj_skip_ws(ctx->cur, ctx->end);
    if (ctx->cur < ctx->end && *ctx->cur == ']') {
        ++ctx->cur;
        return;
    }

    for (;;) {
        if (ctx->err || ctx->cur >= ctx->end) break;
        ctx->cur = cj_skip_ws(ctx->cur, ctx->end);
        if (ctx->cur >= ctx->end) { cj_ctx_error(ctx, "EOF in array"); break; }
        if (*ctx->cur == ']')     { ++ctx->cur; break; }

        tinygltf_json elem;
        cj_parse_value(ctx, &elem);
        if (ctx->err) break;
        out->push_back(static_cast<tinygltf_json &&>(elem));

        ctx->cur = cj_skip_ws(ctx->cur, ctx->end);
        if (ctx->cur >= ctx->end) { cj_ctx_error(ctx, "EOF after array element"); break; }
        if      (*ctx->cur == ',') ++ctx->cur;
        else if (*ctx->cur == ']') { ++ctx->cur; break; }
        else { cj_ctx_error(ctx, "expected ',' or ']'"); break; }
    }
}

static void cj_parse_object(cj_parse_ctx *ctx, tinygltf_json *out) {
    assert(*ctx->cur == '{');
    ++ctx->cur;
    out->make_object_();

    ctx->cur = cj_skip_ws(ctx->cur, ctx->end);
    if (ctx->cur < ctx->end && *ctx->cur == '}') { ++ctx->cur; return; }

    for (;;) {
        if (ctx->err || ctx->cur >= ctx->end) break;
        ctx->cur = cj_skip_ws(ctx->cur, ctx->end);
        if (ctx->cur >= ctx->end) { cj_ctx_error(ctx, "EOF in object"); break; }
        if (*ctx->cur == '}')     { ++ctx->cur; break; }

        /* Parse key */
        if (*ctx->cur != '"') { cj_ctx_error(ctx, "expected key string"); break; }
        char  *key_str = NULL;
        size_t key_len = 0;
        cj_parse_string_to(ctx, &key_str, &key_len);
        if (ctx->err || !key_str) { free(key_str); break; }

        /* Parse colon */
        ctx->cur = cj_skip_ws(ctx->cur, ctx->end);
        if (ctx->cur >= ctx->end || *ctx->cur != ':') {
            free(key_str);
            cj_ctx_error(ctx, "expected ':'");
            break;
        }
        ++ctx->cur;

        /* Allocate member slot */
        if (!out->obj_reserve_()) { free(key_str); cj_ctx_error(ctx, "OOM"); break; }
        tinygltf_json_member *m = &out->obj_data_[out->obj_size_];
        new (m) tinygltf_json_member();
        m->key     = key_str;
        m->key_len = key_len;
        ++out->obj_size_;

        /* Parse value directly into m->val */
        cj_parse_value(ctx, &m->val);
        if (ctx->err) break;

        ctx->cur = cj_skip_ws(ctx->cur, ctx->end);
        if (ctx->cur >= ctx->end) { cj_ctx_error(ctx, "EOF after value"); break; }
        if      (*ctx->cur == ',') ++ctx->cur;
        else if (*ctx->cur == '}') { ++ctx->cur; break; }
        else { cj_ctx_error(ctx, "expected ',' or '}'"); break; }
    }
}

static void cj_parse_value(cj_parse_ctx *ctx, tinygltf_json *out) {
    ctx->cur = cj_skip_ws(ctx->cur, ctx->end);
    if (ctx->cur >= ctx->end) {
        cj_ctx_error(ctx, "unexpected EOF");
        out->destroy_();
        out->init_null_();
        return;
    }

    char c = *ctx->cur;

    if (c == '"') {
        char  *s   = NULL;
        size_t len = 0;
        cj_parse_string_to(ctx, &s, &len);
        if (ctx->err || !s) { free(s); out->destroy_(); out->init_null_(); return; }
        out->destroy_();
        out->init_null_();
        out->type_    = CJ_STRING;
        out->str_     = s;
        out->str_len_ = len;
    } else if (c == '{') {
        cj_parse_object(ctx, out);
    } else if (c == '[') {
        cj_parse_array(ctx, out);
    } else if (c == 't') {
        if (ctx->end - ctx->cur >= 4 && memcmp(ctx->cur, "true", 4) == 0) {
            ctx->cur += 4;
            *out = tinygltf_json(true);
        } else { cj_ctx_error(ctx, "invalid literal 'true'"); }
    } else if (c == 'f') {
        if (ctx->end - ctx->cur >= 5 && memcmp(ctx->cur, "false", 5) == 0) {
            ctx->cur += 5;
            *out = tinygltf_json(false);
        } else { cj_ctx_error(ctx, "invalid literal 'false'"); }
    } else if (c == 'n') {
        if (ctx->end - ctx->cur >= 4 && memcmp(ctx->cur, "null", 4) == 0) {
            ctx->cur += 4;
            out->destroy_();
            out->init_null_();
        } else { cj_ctx_error(ctx, "invalid literal 'null'"); }
    } else if (c == '-' || (c >= '0' && c <= '9')) {
        int     is_int = 0;
        int64_t ival   = 0;
        double  dval   = 0.0;
        const char *next = cj_parse_number(ctx->cur, ctx->end,
                                            &is_int, &ival, &dval);
        if (!next) { cj_ctx_error(ctx, "invalid number"); return; }
        ctx->cur = next;
        if (is_int) {
            out->destroy_();
            out->init_null_();
            out->type_ = CJ_INT;
            out->i_    = ival;
        } else {
            out->destroy_();
            out->init_null_();
            out->type_ = CJ_REAL;
            out->d_    = dval;
        }
    } else {
        char errbuf[64];
        snprintf(errbuf, sizeof(errbuf),
                 "unexpected character '%c' (0x%02X)",
                 (unsigned char)c >= 0x20u ? c : '?',
                 (unsigned char)c);
        cj_ctx_error(ctx, errbuf);
        out->destroy_();
        out->init_null_();
    }
}

/* ======================================================================
 * SERIALIZATION (C-style string builder)
 * ====================================================================== */

struct cj_strbuf {
    char  *data;
    size_t len;
    size_t cap;
};

static int cj_strbuf_init(cj_strbuf *sb, size_t initial) {
    sb->data = (char *)malloc(initial);
    sb->len  = 0;
    sb->cap  = initial;
    return sb->data ? 1 : 0;
}

static void cj_strbuf_free_data(cj_strbuf *sb) {
    free(sb->data);
    sb->data = NULL;
    sb->len = sb->cap = 0;
}

static int cj_strbuf_grow(cj_strbuf *sb, size_t extra) {
    size_t needed = sb->len + extra;
    if (needed <= sb->cap) return 1;
    size_t new_cap = sb->cap * 2;
    if (new_cap < needed) new_cap = needed + 256;
    char *nd = (char *)realloc(sb->data, new_cap);
    if (!nd) return 0;
    sb->data = nd;
    sb->cap  = new_cap;
    return 1;
}

static int cj_sb_appendn(cj_strbuf *sb, const char *s, size_t n) {
    if (!cj_strbuf_grow(sb, n)) return 0;
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    return 1;
}

static int cj_sb_appendc(cj_strbuf *sb, char c) {
    return cj_sb_appendn(sb, &c, 1);
}

static int cj_sb_appends(cj_strbuf *sb, const char *s) {
    return cj_sb_appendn(sb, s, strlen(s));
}

static int cj_append_str_escaped(cj_strbuf *sb, const char *s, size_t len) {
    if (!cj_sb_appendc(sb, '"')) return 0;
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '"':  if (!cj_sb_appendn(sb, "\\\"", 2)) return 0; break;
            case '\\': if (!cj_sb_appendn(sb, "\\\\", 2)) return 0; break;
            case '\b': if (!cj_sb_appendn(sb, "\\b",  2)) return 0; break;
            case '\f': if (!cj_sb_appendn(sb, "\\f",  2)) return 0; break;
            case '\n': if (!cj_sb_appendn(sb, "\\n",  2)) return 0; break;
            case '\r': if (!cj_sb_appendn(sb, "\\r",  2)) return 0; break;
            case '\t': if (!cj_sb_appendn(sb, "\\t",  2)) return 0; break;
            default:
                if (c < 0x20u) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04X", c);
                    if (!cj_sb_appends(sb, buf)) return 0;
                } else {
                    if (!cj_sb_appendc(sb, (char)c)) return 0;
                }
                break;
        }
    }
    return cj_sb_appendc(sb, '"');
}

static int cj_indent_line(cj_strbuf *sb, int indent, int depth) {
    if (indent <= 0) return 1;
    if (!cj_sb_appendc(sb, '\n')) return 0;
    for (int i = 0; i < indent * depth; ++i)
        if (!cj_sb_appendc(sb, ' ')) return 0;
    return 1;
}

static int cj_serialize(cj_strbuf *sb, const tinygltf_json *v,
                        int indent, int depth) {
    switch (v->type_) {
        case CJ_NULL:
            return cj_sb_appends(sb, "null");
        case CJ_BOOL:
            return cj_sb_appends(sb, v->b_ ? "true" : "false");
        case CJ_INT: {
            char buf[32];
            snprintf(buf, sizeof(buf), "%" PRId64, v->i_);
            return cj_sb_appends(sb, buf);
        }
        case CJ_REAL: {
            char buf[64];
            double d = v->d_;
            /* Round-trip safe formatting */
            snprintf(buf, sizeof(buf), "%.17g", d);
            /* Ensure there's a decimal point for floats */
            if (!strchr(buf, '.') && !strchr(buf, 'e') && !strchr(buf, 'E')
                && !strchr(buf, 'n') && !strchr(buf, 'N')) {
                size_t bl = strlen(buf);
                if (bl + 2 < sizeof(buf)) {
                    buf[bl]   = '.';
                    buf[bl+1] = '0';
                    buf[bl+2] = '\0';
                }
            }
            return cj_sb_appends(sb, buf);
        }
        case CJ_STRING:
            return cj_append_str_escaped(sb,
                       v->str_ ? v->str_ : "", v->str_len_);
        case CJ_ARRAY: {
            if (!cj_sb_appendc(sb, '[')) return 0;
            for (size_t i = 0; i < v->arr_size_; ++i) {
                if (indent > 0 && !cj_indent_line(sb, indent, depth + 1)) return 0;
                if (!cj_serialize(sb, &v->arr_data_[i], indent, depth+1)) return 0;
                if (i + 1 < v->arr_size_ && !cj_sb_appendc(sb, ',')) return 0;
            }
            if (indent > 0 && v->arr_size_ > 0)
                if (!cj_indent_line(sb, indent, depth)) return 0;
            return cj_sb_appendc(sb, ']');
        }
        case CJ_OBJECT: {
            if (!cj_sb_appendc(sb, '{')) return 0;
            for (size_t i = 0; i < v->obj_size_; ++i) {
                if (indent > 0 && !cj_indent_line(sb, indent, depth + 1)) return 0;
                const tinygltf_json_member *m = &v->obj_data_[i];
                const char *key = m->key ? m->key : "";
                if (!cj_append_str_escaped(sb, key, m->key_len)) return 0;
                if (!cj_sb_appendc(sb, ':')) return 0;
                if (indent > 0 && !cj_sb_appendc(sb, ' ')) return 0;
                if (!cj_serialize(sb, &m->val, indent, depth + 1)) return 0;
                if (i + 1 < v->obj_size_ && !cj_sb_appendc(sb, ',')) return 0;
            }
            if (indent > 0 && v->obj_size_ > 0)
                if (!cj_indent_line(sb, indent, depth)) return 0;
            return cj_sb_appendc(sb, '}');
        }
        default:
            return cj_sb_appends(sb, "null");
    }
}

/* ======================================================================
 * tinygltf_json::dump() and ::parse() IMPLEMENTATIONS
 * ====================================================================== */

inline std::string tinygltf_json::dump(int indent) const {
    cj_strbuf sb;
    if (!cj_strbuf_init(&sb, 4096)) return std::string();
    cj_serialize(&sb, this, indent, 0);
    std::string result(sb.data, sb.len);
    cj_strbuf_free_data(&sb);
    return result;
}

inline tinygltf_json tinygltf_json::parse(const char *first, const char *last,
                                           std::nullptr_t,
                                           bool allow_exceptions) {
    cj_parse_ctx ctx;
    ctx.cur      = first;
    ctx.end      = last;
    ctx.err      = 0;
    ctx.errmsg[0] = '\0';

    tinygltf_json result;
    cj_parse_value(&ctx, &result);

    if (ctx.err) {
        if (allow_exceptions) {
            throw std::invalid_argument(
                std::string("tinygltf_json::parse error: ") + ctx.errmsg);
        }
        return tinygltf_json(); /* null on error */
    }
    return result;
}

/* ======================================================================
 * TINYGLTF DETAIL NAMESPACE COMPATIBILITY
 *
 * These declarations make the custom JSON backend available as
 * tinygltf::detail types/functions when TINYGLTF_USE_CUSTOM_JSON is set.
 * ====================================================================== */

namespace tinygltf {
namespace detail {

using json                      = tinygltf_json;
using json_iterator             = tinygltf_json::iterator;
using json_const_iterator       = tinygltf_json::iterator;
using json_const_array_iterator = tinygltf_json::iterator;
using JsonDocument              = tinygltf_json;

inline void JsonParse(JsonDocument &doc, const char *str, size_t length,
                      bool throwExc = false) {
    doc = tinygltf_json::parse(str, str + length, nullptr, throwExc);
}

/* --- Type accessors --- */

inline bool GetInt(const json &o, int &val) {
    if (o.is_number_integer() || o.is_number_unsigned()) {
        val = o.get<int>();
        return true;
    }
    return false;
}

inline bool GetDouble(const json &o, double &val) {
    if (o.is_number_float()) {
        val = o.get<double>();
        return true;
    }
    return false;
}

inline bool GetNumber(const json &o, double &val) {
    if (o.is_number()) {
        val = o.get<double>();
        return true;
    }
    return false;
}

inline bool GetString(const json &o, std::string &val) {
    if (o.is_string()) {
        val = o.get<std::string>();
        return true;
    }
    return false;
}

inline bool IsArray(const json &o)  { return o.is_array(); }
inline bool IsObject(const json &o) { return o.is_object(); }
inline bool IsEmpty(const json &o)  { return o.empty(); }

inline json_const_array_iterator ArrayBegin(const json &o) {
    return o.begin();
}
inline json_const_array_iterator ArrayEnd(const json &o) {
    return o.end();
}

inline json_const_iterator ObjectBegin(const json &o) { return o.begin(); }
inline json_const_iterator ObjectEnd(const json &o)   { return o.end(); }
inline json_iterator       ObjectBegin(json &o)        { return o.begin(); }
inline json_iterator       ObjectEnd(json &o)          { return o.end(); }

inline std::string GetKey(const json_const_iterator &it) { return it.key(); }
inline std::string GetKey(json_iterator &it)              { return it.key(); }

inline const json &GetValue(const json_const_iterator &it) { return *it; }
inline json       &GetValue(json_iterator &it)              { return *it; }

inline bool FindMember(const json &o, const char *member,
                        json_const_iterator &it) {
    it = o.find(member);
    return it != o.end();
}
inline bool FindMember(json &o, const char *member, json_iterator &it) {
    it = o.find(member);
    return it != o.end();
}

inline void Erase(json &o, json_iterator &it) { o.erase(it); }

inline std::string JsonToString(const json &o, int spacing = -1) {
    return o.dump(spacing);
}

/* --- Serialization helpers --- */

inline json JsonFromString(const char *s) { return json(s); }

inline void JsonAssign(json &dest, const json &src) { dest = src; }

inline void JsonAddMember(json &o, const char *key, json &&value) {
    o[key] = static_cast<json &&>(value);
}

inline void JsonPushBack(json &o, json &&value) {
    o.push_back(static_cast<json &&>(value));
}

inline bool JsonIsNull(const json &o) { return o.is_null(); }

inline void JsonSetObject(json &o) { o = json::object(); }

inline void JsonReserveArray(json &o, size_t /*s*/) {
    o.make_array_();
}

/* Stub allocator for RapidJSON-compatibility (not used by custom backend) */
struct CJ_NoAllocator {};
inline CJ_NoAllocator &GetAllocator() {
    static CJ_NoAllocator alloc;
    return alloc;
}

} /* namespace detail */
} /* namespace tinygltf */

#endif /* TINYGLTF_JSON_H_ */
