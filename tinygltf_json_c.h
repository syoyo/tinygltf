#ifndef TINYGLTF_JSON_C_H_
#define TINYGLTF_JSON_C_H_

/*
 * Floating-point conversion attribution:
 *
 * - JSON number parsing in this C11 port is based on fast_float.
 *   Upstream: https://github.com/fastfloat/fast_float
 *   Copyright (c) 2021 The fast_float authors.
 *   License: Apache License 2.0, MIT License, or Boost Software License 1.0.
 *
 * - JSON number serialization in this C11 port is based on Dragonbox-style
 *   shortest round-trippable binary floating-point to decimal conversion.
 *   Upstream: https://github.com/jk-jeon/dragonbox
 *   Copyright 2020-2024 Junekey Jeon.
 *   License: Apache License 2.0 with LLVM Exceptions, or Boost Software
 *   License 1.0.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tg3json_type {
    TG3JSON_NULL = 0,
    TG3JSON_BOOL = 1,
    TG3JSON_INT = 2,
    TG3JSON_REAL = 3,
    TG3JSON_STRING = 4,
    TG3JSON_ARRAY = 5,
    TG3JSON_OBJECT = 6
} tg3json_type;

typedef struct tg3json_value tg3json_value;

typedef struct tg3json_string {
    char *ptr;
    size_t len;
} tg3json_string;

typedef struct tg3json_array {
    tg3json_value *items;
    size_t count;
} tg3json_array;

typedef struct tg3json_object_entry {
    char *key;
    size_t key_len;
    tg3json_value *value;
} tg3json_object_entry;

typedef struct tg3json_object {
    tg3json_object_entry *items;
    size_t count;
} tg3json_object;

struct tg3json_value {
    tg3json_type type;
    union {
        int boolean;
        int64_t integer;
        double real;
        tg3json_string string;
        tg3json_array array;
        tg3json_object object;
    } u;
};

int tg3json_parse_n(const char *data, size_t len, size_t depth_limit,
                    tg3json_value *out_value, const char **out_error_pos);
int tg3json_parse(const char *begin, const char *end, size_t depth_limit,
                  tg3json_value *out_value, const char **out_error_pos);
typedef struct tg3json_parse_options {
    size_t depth_limit;       /* 0 = default */
    size_t memory_budget;     /* 0 = unlimited */
    size_t max_single_alloc;  /* 0 = unlimited */
    size_t max_string_length; /* 0 = unlimited */
    int parse_float32;        /* 1 = round JSON reals to float */
} tg3json_parse_options;
int tg3json_parse_n_opts(const char *data, size_t len,
                         const tg3json_parse_options *options,
                         tg3json_value *out_value,
                         const char **out_error_pos);
void tg3json_value_free(tg3json_value *value);
void tg3json_value_init_null(tg3json_value *value);
void tg3json_value_init_bool(tg3json_value *value, int boolean_value);
void tg3json_value_init_int(tg3json_value *value, int64_t integer_value);
void tg3json_value_init_real(tg3json_value *value, double real_value);
int tg3json_value_init_string_n(tg3json_value *value, const char *str, size_t len);
int tg3json_value_init_string(tg3json_value *value, const char *str);
void tg3json_value_init_array(tg3json_value *value);
void tg3json_value_init_object(tg3json_value *value);
int tg3json_value_copy(tg3json_value *dst, const tg3json_value *src);

const tg3json_value *tg3json_object_get(const tg3json_value *object,
                                        const char *key);
const tg3json_value *tg3json_object_get_n(const tg3json_value *object,
                                          const char *key, size_t key_len);
tg3json_value *tg3json_object_get_mut(tg3json_value *object, const char *key);
tg3json_value *tg3json_object_get_mut_n(tg3json_value *object,
                                        const char *key, size_t key_len);
const tg3json_object_entry *tg3json_object_at(const tg3json_value *object,
                                              size_t index);
size_t tg3json_object_size(const tg3json_value *object);
int tg3json_object_set_take_n(tg3json_value *object, const char *key, size_t key_len,
                              tg3json_value *value);
int tg3json_object_set_take(tg3json_value *object, const char *key,
                            tg3json_value *value);
int tg3json_object_set_copy_n(tg3json_value *object, const char *key, size_t key_len,
                              const tg3json_value *value);
int tg3json_object_set_copy(tg3json_value *object, const char *key,
                            const tg3json_value *value);
const tg3json_value *tg3json_array_get(const tg3json_value *array, size_t index);
size_t tg3json_array_size(const tg3json_value *array);
int tg3json_array_append_take(tg3json_value *array, tg3json_value *value);
int tg3json_array_append_copy(tg3json_value *array, const tg3json_value *value);
char *tg3json_stringify(const tg3json_value *value, size_t *out_len);
char *tg3json_stringify_pretty(const tg3json_value *value, int indent, size_t *out_len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef TINYGLTF_JSON_C_IMPLEMENTATION

#ifndef TINYGLTF_JSON_NO_STDLIB
  #include <stdlib.h>
  #include <string.h>
  #include <stdio.h>
  #include <float.h>
#if defined(_MSC_VER) || (defined(LDBL_MANT_DIG) && LDBL_MANT_DIG <= 53)
#define TG3JSON_USE_STDLIB_FPCONV 1
#endif
#endif
#ifndef TG3JSON_USE_STDLIB_FPCONV
#define TG3JSON_USE_STDLIB_FPCONV 0
#endif

#ifdef TINYGLTF_JSON_NO_STDLIB
static void *tg3json__memcpy_fallback(void *dst, const void *src, size_t sz) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (sz--) *d++ = *s++;
    return dst;
}

static void *tg3json__memset_fallback(void *dst, int val, size_t sz) {
    unsigned char *d = (unsigned char *)dst;
    while (sz--) *d++ = (unsigned char)val;
    return dst;
}

static int tg3json__memcmp_fallback(const void *a, const void *b, size_t sz) {
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    while (sz--) {
        if (*pa != *pb) return (int)*pa - (int)*pb;
        ++pa;
        ++pb;
    }
    return 0;
}

static size_t tg3json__strlen_fallback(const char *str) {
    const char *p = str;
    while (*p) ++p;
    return (size_t)(p - str);
}

#endif

#ifndef TINYGLTF_JSON_MALLOC
#ifdef TINYGLTF_JSON_NO_STDLIB
#define TINYGLTF_JSON_MALLOC(sz) NULL
#else
#define TINYGLTF_JSON_MALLOC(sz) malloc(sz)
#endif
#endif
#ifndef TINYGLTF_JSON_REALLOC
#ifdef TINYGLTF_JSON_NO_STDLIB
#define TINYGLTF_JSON_REALLOC(ptr, sz) NULL
#else
#define TINYGLTF_JSON_REALLOC(ptr, sz) realloc(ptr, sz)
#endif
#endif
#ifndef TINYGLTF_JSON_FREE
#ifdef TINYGLTF_JSON_NO_STDLIB
#define TINYGLTF_JSON_FREE(ptr) ((void)(ptr))
#else
#define TINYGLTF_JSON_FREE(ptr) free(ptr)
#endif
#endif

#ifndef TINYGLTF_JSON_MEMCPY
#ifdef TINYGLTF_JSON_NO_STDLIB
#define TINYGLTF_JSON_MEMCPY(dst, src, sz) tg3json__memcpy_fallback((dst), (src), (sz))
#else
#define TINYGLTF_JSON_MEMCPY(dst, src, sz) memcpy(dst, src, sz)
#endif
#endif
#ifndef TINYGLTF_JSON_MEMSET
#ifdef TINYGLTF_JSON_NO_STDLIB
#define TINYGLTF_JSON_MEMSET(dst, val, sz) tg3json__memset_fallback((dst), (val), (sz))
#else
#define TINYGLTF_JSON_MEMSET(dst, val, sz) memset(dst, val, sz)
#endif
#endif
#ifndef TINYGLTF_JSON_MEMCMP
#ifdef TINYGLTF_JSON_NO_STDLIB
#define TINYGLTF_JSON_MEMCMP(a, b, sz) tg3json__memcmp_fallback((a), (b), (sz))
#else
#define TINYGLTF_JSON_MEMCMP(a, b, sz) memcmp(a, b, sz)
#endif
#endif
#ifndef TINYGLTF_JSON_STRLEN
#ifdef TINYGLTF_JSON_NO_STDLIB
#define TINYGLTF_JSON_STRLEN(str) tg3json__strlen_fallback((str))
#else
#define TINYGLTF_JSON_STRLEN(str) strlen(str)
#endif
#endif

static size_t tg3json__itoa(int64_t value, char *buf) {
    char *p = buf;
    uint64_t val;
    char tmp[24];
    int i = 0;
    if (value < 0) {
        *p++ = '-';
        val = (value == -9223372036854775807LL - 1) ? 9223372036854775808ULL : (uint64_t)(-value);
    } else {
        val = (uint64_t)value;
    }
    if (val == 0) {
        *p++ = '0';
        *p = '\0';
        return (size_t)(p - buf);
    }
    while (val > 0) {
        tmp[i++] = (char)('0' + (val % 10));
        val /= 10;
    }
    while (i > 0) {
        *p++ = tmp[--i];
    }
    *p = '\0';
    return (size_t)(p - buf);
}

#if !TG3JSON_USE_STDLIB_FPCONV
static size_t tg3json__utoa(uint64_t value, char *buf) {
    char tmp[32];
    size_t n = 0;
    size_t i = 0;
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    while (value > 0) {
        tmp[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (n > 0) buf[i++] = tmp[--n];
    buf[i] = '\0';
    return i;
}
#endif  /* !TG3JSON_USE_STDLIB_FPCONV */

static uint64_t tg3json__double_bits(double v) {
    uint64_t bits = 0;
    TINYGLTF_JSON_MEMCPY(&bits, &v, sizeof(bits));
    return bits;
}

static int tg3json__is_nan_bits(uint64_t bits) {
    return ((bits & 0x7ff0000000000000ULL) == 0x7ff0000000000000ULL) &&
           ((bits & 0x000fffffffffffffULL) != 0);
}

static int tg3json__is_inf_bits(uint64_t bits) {
    return (bits & 0x7fffffffffffffffULL) == 0x7ff0000000000000ULL;
}

#if !TG3JSON_USE_STDLIB_FPCONV
static double tg3json__double_from_bits(uint64_t bits) {
    double v = 0.0;
    TINYGLTF_JSON_MEMCPY(&v, &bits, sizeof(v));
    return v;
}

static long double tg3json__pow10_ld(int exp10) {
    long double v = 1.0L;
    if (exp10 < 0) {
        exp10 = -exp10;
        while (exp10 >= 16) {
            v *= 1.0e-16L;
            exp10 -= 16;
        }
        while (exp10-- > 0) v *= 0.1L;
    } else {
        while (exp10 >= 16) {
            v *= 1.0e16L;
            exp10 -= 16;
        }
        while (exp10-- > 0) v *= 10.0L;
    }
    return v;
}
#endif

static int tg3json__parse_f64_c(const char *start, const char *end, double *out) {
#if TG3JSON_USE_STDLIB_FPCONV
    size_t len = (size_t)(end - start);
    char stack_buf[128];
    char *buf = stack_buf;
    char *parse_end = NULL;
    double v;
    if (len + 1 > sizeof(stack_buf)) {
        buf = (char *)TINYGLTF_JSON_MALLOC(len + 1);
        if (!buf) return 0;
    }
    if (len > 0) TINYGLTF_JSON_MEMCPY(buf, start, len);
    buf[len] = '\0';
    v = strtod(buf, &parse_end);
    if (parse_end != buf + len || tg3json__is_inf_bits(tg3json__double_bits(v))) {
        if (buf != stack_buf) TINYGLTF_JSON_FREE(buf);
        return 0;
    }
    if (buf != stack_buf) TINYGLTF_JSON_FREE(buf);
    *out = v;
    return 1;
#else
    const char *p = start;
    int neg = 0;
    int saw_digit = 0;
    int nonzero_seen = 0;
    int exp10 = 0;
    uint64_t sig = 0;
    int sig_digits = 0;
    long double extra = 0.0L;
    int extra_digits = 0;
    int exp_sign = 1;
    int exp_lit = 0;
    long double v;

    if (p < end && *p == '-') {
        neg = 1;
        ++p;
    }
    if (p >= end) return 0;
    if (*p == '0') {
        saw_digit = 1;
        ++p;
    } else if (*p >= '1' && *p <= '9') {
        do {
            int d = *p - '0';
            saw_digit = 1;
            nonzero_seen = 1;
            if (sig_digits < 19) {
                sig = sig * 10u + (uint64_t)d;
                ++sig_digits;
            } else if (extra_digits < 64) {
                extra = extra * 10.0L + (long double)d;
                ++extra_digits;
            } else {
                ++exp10;
            }
            ++p;
        } while (p < end && *p >= '0' && *p <= '9');
    } else {
        return 0;
    }
    if (p < end && *p == '.') {
        ++p;
        if (p >= end || *p < '0' || *p > '9') return 0;
        do {
            int d = *p - '0';
            saw_digit = 1;
            if (d != 0 || nonzero_seen) {
                nonzero_seen = nonzero_seen || (d != 0);
                if (sig_digits < 19) {
                    sig = sig * 10u + (uint64_t)d;
                    ++sig_digits;
                    --exp10;
                } else if (extra_digits < 64) {
                    extra = extra * 10.0L + (long double)d;
                    ++extra_digits;
                    --exp10;
                } else {
                    --exp10;
                }
            } else {
                --exp10;
            }
            ++p;
        } while (p < end && *p >= '0' && *p <= '9');
    }
    if (!saw_digit) return 0;
    if (p < end && (*p == 'e' || *p == 'E')) {
        ++p;
        if (p < end && (*p == '+' || *p == '-')) {
            exp_sign = (*p == '-') ? -1 : 1;
            ++p;
        }
        if (p >= end || *p < '0' || *p > '9') return 0;
        while (p < end && *p >= '0' && *p <= '9') {
            if (exp_lit < 10000) exp_lit = exp_lit * 10 + (*p - '0');
            ++p;
        }
        exp10 += exp_sign * exp_lit;
    }
    if (p != end) return 0;
    if (!nonzero_seen || sig_digits == 0) {
        *out = neg ? tg3json__double_from_bits(0x8000000000000000ULL) : 0.0;
        return 1;
    }
    if (exp10 > 309) return 0;
    if (exp10 < -4000) {
        *out = neg ? tg3json__double_from_bits(0x8000000000000000ULL) : 0.0;
        return 1;
    }
    v = (long double)sig;
    if (extra_digits > 0) {
        v = v * tg3json__pow10_ld(extra_digits) + extra;
    }
    if (exp10 != 0) v *= tg3json__pow10_ld(exp10);
    if (neg) v = -v;
    *out = (double)v;
    if (tg3json__is_inf_bits(tg3json__double_bits(*out))) return 0;
    return 1;
#endif
}

#if !TG3JSON_USE_STDLIB_FPCONV
static char *tg3json__write_exp(char *p, int exp10) {
    char tmp[16];
    size_t n;
    *p++ = 'e';
    if (exp10 < 0) {
        *p++ = '-';
        exp10 = -exp10;
    }
    n = tg3json__utoa((uint64_t)exp10, tmp);
    TINYGLTF_JSON_MEMCPY(p, tmp, n);
    return p + n;
}

static char *tg3json__format_decimal_digits(char *out, const char *digits,
                                            int ndigits, int dec_exp,
                                            int negative) {
    char *p = out;
    int i;
    int output_exp = dec_exp + ndigits - 1;
    if (negative) *p++ = '-';
    if (output_exp < -4 || output_exp >= 16) {
        *p++ = digits[0];
        if (ndigits > 1) {
            *p++ = '.';
            for (i = 1; i < ndigits; ++i) *p++ = digits[i];
        }
        return tg3json__write_exp(p, output_exp);
    }
    if (dec_exp >= 0) {
        for (i = 0; i < ndigits; ++i) *p++ = digits[i];
        for (i = 0; i < dec_exp; ++i) *p++ = '0';
        return p;
    }
    if (dec_exp + ndigits > 0) {
        int int_digits = dec_exp + ndigits;
        for (i = 0; i < int_digits; ++i) *p++ = digits[i];
        *p++ = '.';
        for (; i < ndigits; ++i) *p++ = digits[i];
        return p;
    }
    *p++ = '0';
    *p++ = '.';
    for (i = 0; i < -(dec_exp + ndigits); ++i) *p++ = '0';
    for (i = 0; i < ndigits; ++i) *p++ = digits[i];
    return p;
}
#endif  /* !TG3JSON_USE_STDLIB_FPCONV */

static int tg3json__same_f64(double a, double b) {
    return tg3json__double_bits(a) == tg3json__double_bits(b);
}

#if TG3JSON_USE_STDLIB_FPCONV
static void tg3json__normalize_exponent(char *s) {
    char *e = s;
    char *dst;
    int neg = 0;
    while (*e && *e != 'e' && *e != 'E') ++e;
    if (!*e) return;
    *e++ = 'e';
    dst = e;
    if (*e == '+' || *e == '-') {
        neg = (*e == '-');
        ++e;
    }
    while (*e == '0') ++e;
    if (neg) *dst++ = '-';
    if (!*e) {
        *dst++ = '0';
    } else {
        while (*e) *dst++ = *e++;
    }
    *dst = '\0';
}

static void tg3json__expand_short_plain_decimal(char *s) {
    char *p = s;
    char *e;
    char digits[32];
    char out[96];
    int negative = 0;
    int ndigits = 0;
    int int_digits = 0;
    int exp10 = 0;
    int new_point;
    int i;
    char *q = out;

    if (*p == '-') {
        negative = 1;
        ++p;
    }
    e = p;
    while (*e && *e != 'e') ++e;
    if (!*e) return;
    for (; p < e; ++p) {
        if (*p == '.') {
            int_digits = ndigits;
        } else {
            if (ndigits < (int)(sizeof(digits) - 1)) digits[ndigits++] = *p;
        }
    }
    if (int_digits == 0) int_digits = ndigits;
    ++e;
    if (*e == '-') {
        int sign = -1;
        ++e;
        while (*e >= '0' && *e <= '9') exp10 = exp10 * 10 + (*e++ - '0');
        exp10 *= sign;
    } else {
        while (*e >= '0' && *e <= '9') exp10 = exp10 * 10 + (*e++ - '0');
    }
    if (exp10 < -4 || exp10 >= 16) return;

    new_point = int_digits + exp10;
    if (negative) *q++ = '-';
    if (new_point <= 0) {
        *q++ = '0';
        *q++ = '.';
        for (i = 0; i < -new_point; ++i) *q++ = '0';
        for (i = 0; i < ndigits; ++i) *q++ = digits[i];
    } else if (new_point >= ndigits) {
        for (i = 0; i < ndigits; ++i) *q++ = digits[i];
        for (i = ndigits; i < new_point; ++i) *q++ = '0';
    } else {
        for (i = 0; i < new_point; ++i) *q++ = digits[i];
        *q++ = '.';
        for (; i < ndigits; ++i) *q++ = digits[i];
    }
    *q = '\0';
    if (q > out) {
        char *dot = out;
        while (*dot && *dot != '.') ++dot;
        if (*dot == '.') {
            char *end = q - 1;
            while (end > dot && *end == '0') *end-- = '\0';
            if (end == dot) *end = '\0';
        }
    }
    TINYGLTF_JSON_MEMCPY(s, out, (size_t)(TINYGLTF_JSON_STRLEN(out) + 1));
}
#endif

static char *tg3json__dtoa_c(double value, char *buf) {
    uint64_t bits = tg3json__double_bits(value);
    int negative = (int)(bits >> 63);
    uint64_t abits = bits & 0x7fffffffffffffffULL;
#if !TG3JSON_USE_STDLIB_FPCONV
    long double x;
    int dec_e = 0;
    char digits[24];
    int ndigits = 17;
    int i;
    char best[80];
    size_t best_len;
#endif

    if (tg3json__is_nan_bits(bits)) {
        TINYGLTF_JSON_MEMCPY(buf, "nan", 3);
        return buf + 3;
    }
    if (tg3json__is_inf_bits(bits)) {
        if (negative) {
            TINYGLTF_JSON_MEMCPY(buf, "-inf", 4);
            return buf + 4;
        }
        TINYGLTF_JSON_MEMCPY(buf, "inf", 3);
        return buf + 3;
    }
    if (abits == 0) {
        *buf++ = '0';
        return buf;
    }
    if (bits == 0x3ff0000000000000ULL) {
        *buf++ = '1';
        return buf;
    }
    if (bits == 0xbff0000000000000ULL) {
        *buf++ = '-';
        *buf++ = '1';
        return buf;
    }

#if TG3JSON_USE_STDLIB_FPCONV
    {
        int precision;
        char candidate[96];
        char shortest[96];
        shortest[0] = '\0';
        for (precision = 1; precision <= 17; ++precision) {
            char fmt[8];
            double parsed = 0.0;
            int n;
            n = snprintf(fmt, sizeof(fmt), "%%.%dg", precision);
            if (n <= 0 || n >= (int)sizeof(fmt)) continue;
            n = snprintf(candidate, sizeof(candidate), fmt, value);
            if (n <= 0 || n >= (int)sizeof(candidate)) continue;
            tg3json__normalize_exponent(candidate);
            tg3json__expand_short_plain_decimal(candidate);
            if (tg3json__parse_f64_c(candidate, candidate + TINYGLTF_JSON_STRLEN(candidate), &parsed) &&
                tg3json__same_f64(parsed, value)) {
                TINYGLTF_JSON_MEMCPY(shortest, candidate, TINYGLTF_JSON_STRLEN(candidate) + 1);
                break;
            }
        }
        if (!shortest[0]) {
            int n = snprintf(shortest, sizeof(shortest), "%.17g", value);
            if (n <= 0 || n >= (int)sizeof(shortest)) return buf;
            tg3json__normalize_exponent(shortest);
            tg3json__expand_short_plain_decimal(shortest);
        }
        TINYGLTF_JSON_MEMCPY(buf, shortest, TINYGLTF_JSON_STRLEN(shortest));
        return buf + TINYGLTF_JSON_STRLEN(shortest);
    }
#else

    x = negative ? -(long double)value : (long double)value;
    while (x >= 1.0e16L) {
        x *= 1.0e-16L;
        dec_e += 16;
    }
    while (x >= 10.0L) {
        x *= 0.1L;
        ++dec_e;
    }
    while (x < 1.0L) {
        x *= 10.0L;
        --dec_e;
    }
    for (i = 0; i < 18; ++i) {
        int d = (int)x;
        if (d < 0) d = 0;
        if (d > 9) d = 9;
        digits[i] = (char)('0' + d);
        x = (x - (long double)d) * 10.0L;
    }
    if (digits[17] >= '5') {
        int carry = 1;
        for (i = 16; i >= 0 && carry; --i) {
            if (digits[i] == '9') {
                digits[i] = '0';
            } else {
                digits[i]++;
                carry = 0;
            }
        }
        if (carry) {
            digits[0] = '1';
            for (i = 1; i < 17; ++i) digits[i] = '0';
            ++dec_e;
        }
    }
    while (ndigits > 1 && digits[ndigits - 1] == '0') --ndigits;

    {
        char *end = tg3json__format_decimal_digits(best, digits, ndigits,
                                                   dec_e - ndigits + 1,
                                                   negative);
        *end = '\0';
        best_len = (size_t)(end - best);
    }
    for (i = ndigits - 1; i >= 1; --i) {
        char candidate[80];
        char *end;
        double parsed = 0.0;
        end = tg3json__format_decimal_digits(candidate, digits, i,
                                             dec_e - i + 1, negative);
        *end = '\0';
        if (tg3json__parse_f64_c(candidate, end, &parsed) &&
            tg3json__same_f64(parsed, value)) {
            TINYGLTF_JSON_MEMCPY(best, candidate, (size_t)(end - candidate) + 1);
            best_len = (size_t)(end - candidate);
        } else {
            break;
        }
    }
    TINYGLTF_JSON_MEMCPY(buf, best, best_len);
    return buf + best_len;
#endif
}

typedef struct tg3json__parser {
    const char *cur;
    const char *end;
    const char *error;
    size_t depth_limit;
    size_t memory_budget;
    size_t max_single_alloc;
    size_t max_string_length;
    size_t allocated;
    int parse_float32;
} tg3json__parser;

typedef struct tg3json__buffer {
    tg3json__parser *parser;
    char *data;
    size_t len;
    size_t cap;
} tg3json__buffer;

static void tg3json__init_value(tg3json_value *value) {
    if (!value) return;
    TINYGLTF_JSON_MEMSET(value, 0, sizeof(*value));
    value->type = TG3JSON_NULL;
}

static char *tg3json__strndup_local(const char *src, size_t len) {
    char *dst = (char *)TINYGLTF_JSON_MALLOC(len + 1);
    if (!dst) return NULL;
    if (len > 0) TINYGLTF_JSON_MEMCPY(dst, src, len);
    dst[len] = '\0';
    return dst;
}

static void *tg3json__parser_alloc(tg3json__parser *parser, size_t size) {
    void *ptr;
    if (!parser) return TINYGLTF_JSON_MALLOC(size);
    if (parser->max_single_alloc && size > parser->max_single_alloc) return NULL;
    if (parser->memory_budget &&
        (size > parser->memory_budget || parser->allocated > parser->memory_budget - size)) {
        return NULL;
    }
    ptr = TINYGLTF_JSON_MALLOC(size);
    if (!ptr) return NULL;
    parser->allocated += size;
    return ptr;
}

static int tg3json__reserve_bytes(void **ptr, size_t elem_size,
                                  size_t needed, size_t *capacity) {
    void *new_ptr;
    size_t new_cap;

    if (needed <= *capacity) return 1;
    new_cap = (*capacity > 0) ? *capacity : 8;
    while (new_cap < needed) {
        if (new_cap > ((size_t)-1) / 2) {
            new_cap = needed;
            break;
        }
        new_cap *= 2;
    }

    if (elem_size != 0 && new_cap > ((size_t)-1) / elem_size) return 0;
    new_ptr = TINYGLTF_JSON_REALLOC(*ptr, elem_size * new_cap);
    if (!new_ptr) return 0;
    *ptr = new_ptr;
    *capacity = new_cap;
    return 1;
}

static int tg3json__reserve_bytes_parser(tg3json__parser *parser, void **ptr,
                                         size_t elem_size, size_t needed,
                                         size_t *capacity) {
    void *new_ptr;
    size_t new_cap;
    size_t old_bytes;
    size_t new_bytes;

    if (needed <= *capacity) return 1;
    new_cap = (*capacity > 0) ? *capacity : 8;
    while (new_cap < needed) {
        if (new_cap > ((size_t)-1) / 2) {
            new_cap = needed;
            break;
        }
        new_cap *= 2;
    }

    if (elem_size != 0 && new_cap > ((size_t)-1) / elem_size) return 0;
    old_bytes = elem_size * (*capacity);
    new_bytes = elem_size * new_cap;
    if (parser) {
        size_t delta = (new_bytes > old_bytes) ? (new_bytes - old_bytes) : 0;
        if (parser->max_single_alloc && new_bytes > parser->max_single_alloc) return 0;
        if (parser->memory_budget && delta > 0 &&
            (delta > parser->memory_budget ||
             parser->allocated > parser->memory_budget - delta)) {
            return 0;
        }
    }
    new_ptr = TINYGLTF_JSON_REALLOC(*ptr, new_bytes);
    if (!new_ptr) return 0;
    if (parser && new_bytes > old_bytes) parser->allocated += new_bytes - old_bytes;
    *ptr = new_ptr;
    *capacity = new_cap;
    return 1;
}

static const char *tg3json__skip_ws(const char *p, const char *end) {
    while (p < end) {
        unsigned char c = (unsigned char)*p;
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t') break;
        ++p;
    }
    return p;
}

static void tg3json__set_error(tg3json__parser *parser, const char *pos) {
    if (!parser->error) parser->error = pos;
}

static int tg3json__buf_append(tg3json__buffer *buf, const char *src, size_t len) {
    if (len == 0) return 1;
    if (!tg3json__reserve_bytes_parser(buf->parser, (void **)&buf->data, 1,
                                       buf->len + len + 1, &buf->cap)) {
        return 0;
    }
    TINYGLTF_JSON_MEMCPY(buf->data + buf->len, src, len);
    buf->len += len;
    buf->data[buf->len] = '\0';
    return 1;
}

static int tg3json__buf_putc(tg3json__buffer *buf, char c) {
    if (!tg3json__reserve_bytes_parser(buf->parser, (void **)&buf->data, 1,
                                       buf->len + 2, &buf->cap)) {
        return 0;
    }
    buf->data[buf->len++] = c;
    buf->data[buf->len] = '\0';
    return 1;
}

static int tg3json__hex4(const char *p, uint32_t *out) {
    uint32_t value = 0;
    size_t i;
    for (i = 0; i < 4; ++i) {
        unsigned char c = (unsigned char)p[i];
        value <<= 4;
        if (c >= '0' && c <= '9') value |= (uint32_t)(c - '0');
        else if (c >= 'a' && c <= 'f') value |= (uint32_t)(10 + c - 'a');
        else if (c >= 'A' && c <= 'F') value |= (uint32_t)(10 + c - 'A');
        else return 0;
    }
    *out = value;
    return 1;
}

static int tg3json__append_utf8(tg3json__buffer *buf, uint32_t cp) {
    char tmp[4];
    size_t len = 0;
    if (cp <= 0x7Fu) {
        tmp[len++] = (char)cp;
    } else if (cp <= 0x7FFu) {
        tmp[len++] = (char)(0xC0u | ((cp >> 6) & 0x1Fu));
        tmp[len++] = (char)(0x80u | (cp & 0x3Fu));
    } else if (cp <= 0xFFFFu) {
        tmp[len++] = (char)(0xE0u | ((cp >> 12) & 0x0Fu));
        tmp[len++] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        tmp[len++] = (char)(0x80u | (cp & 0x3Fu));
    } else if (cp <= 0x10FFFFu) {
        tmp[len++] = (char)(0xF0u | ((cp >> 18) & 0x07u));
        tmp[len++] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
        tmp[len++] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        tmp[len++] = (char)(0x80u | (cp & 0x3Fu));
    } else {
        return 0;
    }
    return tg3json__buf_append(buf, tmp, len);
}

static int tg3json__parse_string_raw(tg3json__parser *parser,
                                     char **out_str, size_t *out_len) {
    tg3json__buffer buf;
    const char *start;
    TINYGLTF_JSON_MEMSET(&buf, 0, sizeof(buf));
    buf.parser = parser;

    if (parser->cur >= parser->end || *parser->cur != '"') {
        tg3json__set_error(parser, parser->cur);
        return 0;
    }

    ++parser->cur;
    start = parser->cur;
    while (parser->cur < parser->end) {
        unsigned char c = (unsigned char)*parser->cur;
        if (c == '"') {
            size_t final_len = buf.len + (size_t)(parser->cur - start);
            if (parser->max_string_length && final_len > parser->max_string_length) goto oom;
            if (!tg3json__buf_append(&buf, start, (size_t)(parser->cur - start))) goto oom;
            ++parser->cur;
            *out_str = buf.data;
            *out_len = buf.len;
            return 1;
        }
        if (c == '\\') {
            uint32_t codepoint;
            size_t pending_len = (size_t)(parser->cur - start);
            if (parser->max_string_length && buf.len + pending_len > parser->max_string_length) goto oom;
            if (!tg3json__buf_append(&buf, start, (size_t)(parser->cur - start))) goto oom;
            ++parser->cur;
            if (parser->cur >= parser->end) break;
            switch (*parser->cur) {
                case '"': if (!tg3json__buf_putc(&buf, '"')) goto oom; break;
                case '\\': if (!tg3json__buf_putc(&buf, '\\')) goto oom; break;
                case '/': if (!tg3json__buf_putc(&buf, '/')) goto oom; break;
                case 'b': if (!tg3json__buf_putc(&buf, '\b')) goto oom; break;
                case 'f': if (!tg3json__buf_putc(&buf, '\f')) goto oom; break;
                case 'n': if (!tg3json__buf_putc(&buf, '\n')) goto oom; break;
                case 'r': if (!tg3json__buf_putc(&buf, '\r')) goto oom; break;
                case 't': if (!tg3json__buf_putc(&buf, '\t')) goto oom; break;
                case 'u': {
                    if ((size_t)(parser->end - parser->cur) < 5) break;
                    if (!tg3json__hex4(parser->cur + 1, &codepoint)) break;
                    parser->cur += 4;
                    if (codepoint >= 0xD800u && codepoint <= 0xDBFFu) {
                        uint32_t low;
                        if ((size_t)(parser->end - parser->cur) < 7 || parser->cur[1] != '\\' || parser->cur[2] != 'u') break;
                        if (!tg3json__hex4(parser->cur + 3, &low)) break;
                        if (low < 0xDC00u || low > 0xDFFFu) break;
                        codepoint = 0x10000u + (((codepoint - 0xD800u) << 10) | (low - 0xDC00u));
                        parser->cur += 6;
                    } else if (codepoint >= 0xDC00u && codepoint <= 0xDFFFu) {
                        break;
                    }
                    if (!tg3json__append_utf8(&buf, codepoint)) goto oom;
                    break;
                }
                default:
                    tg3json__set_error(parser, parser->cur);
                    TINYGLTF_JSON_FREE(buf.data);
                    return 0;
            }
            ++parser->cur;
            start = parser->cur;
            continue;
        }
        if (c < 0x20u) break;
        ++parser->cur;
    }

    tg3json__set_error(parser, parser->cur);
    TINYGLTF_JSON_FREE(buf.data);
    return 0;

oom:
    tg3json__set_error(parser, parser->cur);
    TINYGLTF_JSON_FREE(buf.data);
    return 0;
}

static int tg3json__parse_value(tg3json__parser *parser, size_t depth,
                                tg3json_value *out_value);

static int tg3json__parse_int64_span(const char *start, const char *end,
                                     int64_t *out) {
    const char *p = start;
    uint64_t value = 0;
    uint64_t limit = (uint64_t)INT64_MAX;
    int neg = 0;
    if (p < end && *p == '-') {
        neg = 1;
        limit += 1u;
        ++p;
    }
    if (p >= end) return 0;
    while (p < end) {
        unsigned digit = (unsigned)(*p - '0');
        if (digit > 9u) return 0;
        if (value > (limit - digit) / 10u) return 0;
        value = value * 10u + (uint64_t)digit;
        ++p;
    }
    if (neg) {
        if (value == ((uint64_t)INT64_MAX + 1u)) {
            *out = INT64_MIN;
        } else {
            *out = -(int64_t)value;
        }
    } else {
        *out = (int64_t)value;
    }
    return 1;
}

static int tg3json__parse_array(tg3json__parser *parser, size_t depth,
                                tg3json_value *out_value) {
    tg3json_value *items = NULL;
    size_t count = 0;
    size_t cap = 0;

    ++parser->cur;
    parser->cur = tg3json__skip_ws(parser->cur, parser->end);
    if (parser->cur < parser->end && *parser->cur == ']') {
        ++parser->cur;
        out_value->type = TG3JSON_ARRAY;
        out_value->u.array.items = NULL;
        out_value->u.array.count = 0;
        return 1;
    }

    while (parser->cur < parser->end) {
        tg3json_value value;
        tg3json__init_value(&value);
        if (!tg3json__reserve_bytes_parser(parser, (void **)&items,
                                           sizeof(*items), count + 1, &cap)) goto oom;
        if (!tg3json__parse_value(parser, depth + 1, &value)) goto fail;
        items[count++] = value;
        parser->cur = tg3json__skip_ws(parser->cur, parser->end);
        if (parser->cur >= parser->end) break;
        if (*parser->cur == ',') {
            ++parser->cur;
            parser->cur = tg3json__skip_ws(parser->cur, parser->end);
            continue;
        }
        if (*parser->cur == ']') {
            ++parser->cur;
            out_value->type = TG3JSON_ARRAY;
            out_value->u.array.items = items;
            out_value->u.array.count = count;
            return 1;
        }
        break;
    }

fail:
    while (count > 0) {
        --count;
        tg3json_value_free(&items[count]);
    }
    TINYGLTF_JSON_FREE(items);
    tg3json__set_error(parser, parser->cur);
    return 0;

oom:
    while (count > 0) {
        --count;
        tg3json_value_free(&items[count]);
    }
    TINYGLTF_JSON_FREE(items);
    tg3json__set_error(parser, parser->cur);
    return 0;
}

static int tg3json__parse_object(tg3json__parser *parser, size_t depth,
                                 tg3json_value *out_value) {
    tg3json_object_entry *items = NULL;
    size_t count = 0;
    size_t cap = 0;

    ++parser->cur;
    parser->cur = tg3json__skip_ws(parser->cur, parser->end);
    if (parser->cur < parser->end && *parser->cur == '}') {
        ++parser->cur;
        out_value->type = TG3JSON_OBJECT;
        out_value->u.object.items = NULL;
        out_value->u.object.count = 0;
        return 1;
    }

    while (parser->cur < parser->end) {
        char *key = NULL;
        size_t key_len = 0;
        tg3json_value value;
        tg3json__init_value(&value);
        if (!tg3json__parse_string_raw(parser, &key, &key_len)) goto fail;
        parser->cur = tg3json__skip_ws(parser->cur, parser->end);
        if (parser->cur >= parser->end || *parser->cur != ':') {
            TINYGLTF_JSON_FREE(key);
            goto fail;
        }
        ++parser->cur;
        parser->cur = tg3json__skip_ws(parser->cur, parser->end);
        if (!tg3json__parse_value(parser, depth + 1, &value)) {
            TINYGLTF_JSON_FREE(key);
            goto fail;
        }
        if (!tg3json__reserve_bytes_parser(parser, (void **)&items,
                                           sizeof(*items), count + 1, &cap)) {
            TINYGLTF_JSON_FREE(key);
            tg3json_value_free(&value);
            goto oom;
        }
        items[count].key = key;
        items[count].key_len = key_len;
        items[count].value = (tg3json_value *)tg3json__parser_alloc(parser, sizeof(tg3json_value));
        if (!items[count].value) {
            TINYGLTF_JSON_FREE(key);
            tg3json_value_free(&value);
            goto oom;
        }
        *items[count].value = value;
        ++count;
        parser->cur = tg3json__skip_ws(parser->cur, parser->end);
        if (parser->cur >= parser->end) break;
        if (*parser->cur == ',') {
            ++parser->cur;
            parser->cur = tg3json__skip_ws(parser->cur, parser->end);
            continue;
        }
        if (*parser->cur == '}') {
            ++parser->cur;
            out_value->type = TG3JSON_OBJECT;
            out_value->u.object.items = items;
            out_value->u.object.count = count;
            return 1;
        }
        break;
    }

fail:
    while (count > 0) {
        --count;
        TINYGLTF_JSON_FREE(items[count].key);
        if (items[count].value) {
            tg3json_value_free(items[count].value);
            TINYGLTF_JSON_FREE(items[count].value);
        }
    }
    TINYGLTF_JSON_FREE(items);
    tg3json__set_error(parser, parser->cur);
    return 0;

oom:
    while (count > 0) {
        --count;
        TINYGLTF_JSON_FREE(items[count].key);
        if (items[count].value) {
            tg3json_value_free(items[count].value);
            TINYGLTF_JSON_FREE(items[count].value);
        }
    }
    TINYGLTF_JSON_FREE(items);
    tg3json__set_error(parser, parser->cur);
    return 0;
}

static int tg3json__parse_number(tg3json__parser *parser, tg3json_value *out_value) {
    const char *start = parser->cur;
    const char *p = parser->cur;
    int is_real = 0;

    if (*p == '-') ++p;
    if (p >= parser->end) goto fail;
    if (*p == '0') {
        ++p;
    } else if (*p >= '1' && *p <= '9') {
        do { ++p; } while (p < parser->end && *p >= '0' && *p <= '9');
    } else {
        goto fail;
    }
    if (p < parser->end && *p == '.') {
        is_real = 1;
        ++p;
        if (p >= parser->end || *p < '0' || *p > '9') goto fail;
        do { ++p; } while (p < parser->end && *p >= '0' && *p <= '9');
    }
    if (p < parser->end && (*p == 'e' || *p == 'E')) {
        is_real = 1;
        ++p;
        if (p < parser->end && (*p == '+' || *p == '-')) ++p;
        if (p >= parser->end || *p < '0' || *p > '9') goto fail;
        do { ++p; } while (p < parser->end && *p >= '0' && *p <= '9');
    }

    if (!is_real) {
        int64_t v;
        if (tg3json__parse_int64_span(start, p, &v)) {
            out_value->type = TG3JSON_INT;
            out_value->u.integer = v;
            parser->cur = p;
            return 1;
        }
        is_real = 1;
    }

    {
        if (!tg3json__parse_f64_c(start, p, &out_value->u.real)) goto fail;
        if (parser->parse_float32) out_value->u.real = (double)(float)out_value->u.real;
        out_value->type = TG3JSON_REAL;
        parser->cur = p;
        return 1;
    }

fail:
    tg3json__set_error(parser, parser->cur);
    return 0;
}

static int tg3json__parse_value(tg3json__parser *parser, size_t depth,
                                tg3json_value *out_value) {
    if (parser->depth_limit && depth > parser->depth_limit) {
        tg3json__set_error(parser, parser->cur);
        return 0;
    }

    parser->cur = tg3json__skip_ws(parser->cur, parser->end);
    if (parser->cur >= parser->end) {
        tg3json__set_error(parser, parser->cur);
        return 0;
    }

    switch (*parser->cur) {
        case '[':
            return tg3json__parse_array(parser, depth, out_value);
        case '{':
            return tg3json__parse_object(parser, depth, out_value);
        case '"': {
            char *str = NULL;
            size_t len = 0;
            if (!tg3json__parse_string_raw(parser, &str, &len)) return 0;
            out_value->type = TG3JSON_STRING;
            out_value->u.string.ptr = str;
            out_value->u.string.len = len;
            return 1;
        }
        case 'n':
            if ((size_t)(parser->end - parser->cur) >= 4 && TINYGLTF_JSON_MEMCMP(parser->cur, "null", 4) == 0) {
                out_value->type = TG3JSON_NULL;
                parser->cur += 4;
                return 1;
            }
            break;
        case 't':
            if ((size_t)(parser->end - parser->cur) >= 4 && TINYGLTF_JSON_MEMCMP(parser->cur, "true", 4) == 0) {
                out_value->type = TG3JSON_BOOL;
                out_value->u.boolean = 1;
                parser->cur += 4;
                return 1;
            }
            break;
        case 'f':
            if ((size_t)(parser->end - parser->cur) >= 5 && TINYGLTF_JSON_MEMCMP(parser->cur, "false", 5) == 0) {
                out_value->type = TG3JSON_BOOL;
                out_value->u.boolean = 0;
                parser->cur += 5;
                return 1;
            }
            break;
        default:
            return tg3json__parse_number(parser, out_value);
    }

    tg3json__set_error(parser, parser->cur);
    return 0;
}

int tg3json_parse_n_opts(const char *data, size_t len,
                         const tg3json_parse_options *options,
                         tg3json_value *out_value,
                         const char **out_error_pos) {
    tg3json__parser parser;
    int ok;

    if (out_error_pos) *out_error_pos = NULL;
    if (!out_value) return 0;
    tg3json__init_value(out_value);
    if (!data) return 0;

    parser.cur = data;
    parser.end = data + len;
    parser.error = NULL;
    parser.depth_limit = options ? (options->depth_limit ? options->depth_limit : 256) : 256;
    parser.memory_budget = options ? options->memory_budget : 0;
    parser.max_single_alloc = options ? options->max_single_alloc : 0;
    parser.max_string_length = options ? options->max_string_length : 0;
    parser.allocated = 0;
    parser.parse_float32 = options ? options->parse_float32 : 0;

    parser.cur = tg3json__skip_ws(parser.cur, parser.end);
    if (parser.cur >= parser.end) {
        if (out_error_pos) *out_error_pos = data;
        return 0;
    }

    ok = tg3json__parse_value(&parser, 0, out_value);
    if (!ok) {
        if (out_error_pos) *out_error_pos = parser.error ? parser.error : parser.cur;
        tg3json_value_free(out_value);
        return 0;
    }

    parser.cur = tg3json__skip_ws(parser.cur, parser.end);
    if (parser.cur != parser.end) {
        if (out_error_pos) *out_error_pos = parser.cur;
        tg3json_value_free(out_value);
        return 0;
    }

    return 1;
}

int tg3json_parse_n(const char *data, size_t len, size_t depth_limit,
                    tg3json_value *out_value, const char **out_error_pos) {
    tg3json_parse_options options;
    TINYGLTF_JSON_MEMSET(&options, 0, sizeof(options));
    options.depth_limit = depth_limit;
    return tg3json_parse_n_opts(data, len, &options, out_value, out_error_pos);
}

int tg3json_parse(const char *begin, const char *end, size_t depth_limit,
                  tg3json_value *out_value, const char **out_error_pos) {
    if (!begin || !end || end < begin) {
        if (out_error_pos) *out_error_pos = begin;
        return 0;
    }
    return tg3json_parse_n(begin, (size_t)(end - begin), depth_limit,
                           out_value, out_error_pos);
}

void tg3json_value_free(tg3json_value *value) {
    size_t i;
    if (!value) return;
    switch (value->type) {
        case TG3JSON_STRING:
            TINYGLTF_JSON_FREE(value->u.string.ptr);
            break;
        case TG3JSON_ARRAY:
            for (i = 0; i < value->u.array.count; ++i) {
                tg3json_value_free(&value->u.array.items[i]);
            }
            TINYGLTF_JSON_FREE(value->u.array.items);
            break;
        case TG3JSON_OBJECT:
            for (i = 0; i < value->u.object.count; ++i) {
                TINYGLTF_JSON_FREE(value->u.object.items[i].key);
                if (value->u.object.items[i].value) {
                    tg3json_value_free(value->u.object.items[i].value);
                    TINYGLTF_JSON_FREE(value->u.object.items[i].value);
                }
            }
            TINYGLTF_JSON_FREE(value->u.object.items);
            break;
        case TG3JSON_NULL:
        case TG3JSON_BOOL:
        case TG3JSON_INT:
        case TG3JSON_REAL:
            /* Scalar variants own no heap memory. */
            break;
    }
    tg3json__init_value(value);
}

void tg3json_value_init_null(tg3json_value *value) {
    tg3json__init_value(value);
}

void tg3json_value_init_bool(tg3json_value *value, int boolean_value) {
    tg3json__init_value(value);
    value->type = TG3JSON_BOOL;
    value->u.boolean = boolean_value ? 1 : 0;
}

void tg3json_value_init_int(tg3json_value *value, int64_t integer_value) {
    tg3json__init_value(value);
    value->type = TG3JSON_INT;
    value->u.integer = integer_value;
}

void tg3json_value_init_real(tg3json_value *value, double real_value) {
    tg3json__init_value(value);
    value->type = TG3JSON_REAL;
    value->u.real = real_value;
}

int tg3json_value_init_string_n(tg3json_value *value, const char *str, size_t len) {
    tg3json__init_value(value);
    value->type = TG3JSON_STRING;
    value->u.string.ptr = tg3json__strndup_local(str ? str : "", str ? len : 0);
    if (!value->u.string.ptr) {
        tg3json__init_value(value);
        return 0;
    }
    value->u.string.len = str ? len : 0;
    return 1;
}

int tg3json_value_init_string(tg3json_value *value, const char *str) {
    return tg3json_value_init_string_n(value, str, str ? TINYGLTF_JSON_STRLEN(str) : 0);
}

void tg3json_value_init_array(tg3json_value *value) {
    tg3json__init_value(value);
    value->type = TG3JSON_ARRAY;
}

void tg3json_value_init_object(tg3json_value *value) {
    tg3json__init_value(value);
    value->type = TG3JSON_OBJECT;
}

int tg3json_value_copy(tg3json_value *dst, const tg3json_value *src) {
    size_t i;
    tg3json__init_value(dst);
    if (!src) return 1;
    switch (src->type) {
        case TG3JSON_NULL:
            return 1;
        case TG3JSON_BOOL:
            tg3json_value_init_bool(dst, src->u.boolean);
            return 1;
        case TG3JSON_INT:
            tg3json_value_init_int(dst, src->u.integer);
            return 1;
        case TG3JSON_REAL:
            tg3json_value_init_real(dst, src->u.real);
            return 1;
        case TG3JSON_STRING:
            return tg3json_value_init_string_n(dst, src->u.string.ptr, src->u.string.len);
        case TG3JSON_ARRAY:
            tg3json_value_init_array(dst);
            for (i = 0; i < src->u.array.count; ++i) {
                if (!tg3json_array_append_copy(dst, &src->u.array.items[i])) {
                    tg3json_value_free(dst);
                    return 0;
                }
            }
            return 1;
        case TG3JSON_OBJECT:
            tg3json_value_init_object(dst);
            for (i = 0; i < src->u.object.count; ++i) {
                if (!tg3json_object_set_copy_n(dst, src->u.object.items[i].key,
                                               src->u.object.items[i].key_len,
                                               src->u.object.items[i].value)) {
                    tg3json_value_free(dst);
                    return 0;
                }
            }
            return 1;
    }
    return 0; /* unreachable: all enum cases handled above. */
}

const tg3json_value *tg3json_object_get_n(const tg3json_value *object,
                                          const char *key, size_t key_len) {
    size_t i;
    if (!object || object->type != TG3JSON_OBJECT) return NULL;
    for (i = 0; i < object->u.object.count; ++i) {
        const tg3json_object_entry *entry = &object->u.object.items[i];
        if (entry->key_len == key_len && TINYGLTF_JSON_MEMCMP(entry->key, key, key_len) == 0) {
            return entry->value;
        }
    }
    return NULL;
}

const tg3json_value *tg3json_object_get(const tg3json_value *object,
                                        const char *key) {
    if (!key) return NULL;
    return tg3json_object_get_n(object, key, TINYGLTF_JSON_STRLEN(key));
}

tg3json_value *tg3json_object_get_mut_n(tg3json_value *object,
                                        const char *key, size_t key_len) {
    size_t i;
    if (!object || object->type != TG3JSON_OBJECT) return NULL;
    for (i = 0; i < object->u.object.count; ++i) {
        tg3json_object_entry *entry = &object->u.object.items[i];
        if (entry->key_len == key_len && TINYGLTF_JSON_MEMCMP(entry->key, key, key_len) == 0) {
            return entry->value;
        }
    }
    return NULL;
}

tg3json_value *tg3json_object_get_mut(tg3json_value *object, const char *key) {
    if (!key) return NULL;
    return tg3json_object_get_mut_n(object, key, TINYGLTF_JSON_STRLEN(key));
}

const tg3json_object_entry *tg3json_object_at(const tg3json_value *object,
                                              size_t index) {
    if (!object || object->type != TG3JSON_OBJECT || index >= object->u.object.count) {
        return NULL;
    }
    return &object->u.object.items[index];
}

size_t tg3json_object_size(const tg3json_value *object) {
    if (!object || object->type != TG3JSON_OBJECT) return 0;
    return object->u.object.count;
}

int tg3json_object_set_take_n(tg3json_value *object, const char *key, size_t key_len,
                              tg3json_value *value) {
    tg3json_value *existing;
    tg3json_object_entry *entry;
    size_t cap;
    if (!object || object->type != TG3JSON_OBJECT || !value || !key) return 0;
    existing = tg3json_object_get_mut_n(object, key, key_len);
    if (existing) {
        tg3json_value_free(existing);
        *existing = *value;
        tg3json__init_value(value);
        return 1;
    }
    cap = object->u.object.count;
    if (!tg3json__reserve_bytes((void **)&object->u.object.items, sizeof(*object->u.object.items),
                                object->u.object.count + 1, &cap)) {
        return 0;
    }
    entry = &object->u.object.items[object->u.object.count];
    entry->key = tg3json__strndup_local(key, key_len);
    if (!entry->key) return 0;
    entry->key_len = key_len;
    entry->value = (tg3json_value *)TINYGLTF_JSON_MALLOC(sizeof(tg3json_value));
    if (!entry->value) {
        TINYGLTF_JSON_FREE(entry->key);
        entry->key = NULL;
        entry->key_len = 0;
        return 0;
    }
    *entry->value = *value;
    tg3json__init_value(value);
    object->u.object.count += 1;
    return 1;
}

int tg3json_object_set_take(tg3json_value *object, const char *key,
                            tg3json_value *value) {
    if (!key) return 0;
    return tg3json_object_set_take_n(object, key, TINYGLTF_JSON_STRLEN(key), value);
}

int tg3json_object_set_copy_n(tg3json_value *object, const char *key, size_t key_len,
                              const tg3json_value *value) {
    tg3json_value copy;
    tg3json__init_value(&copy);
    if (!tg3json_value_copy(&copy, value)) return 0;
    if (!tg3json_object_set_take_n(object, key, key_len, &copy)) {
        tg3json_value_free(&copy);
        return 0;
    }
    return 1;
}

int tg3json_object_set_copy(tg3json_value *object, const char *key,
                            const tg3json_value *value) {
    if (!key) return 0;
    return tg3json_object_set_copy_n(object, key, TINYGLTF_JSON_STRLEN(key), value);
}

const tg3json_value *tg3json_array_get(const tg3json_value *array, size_t index) {
    if (!array || array->type != TG3JSON_ARRAY || index >= array->u.array.count) {
        return NULL;
    }
    return &array->u.array.items[index];
}

size_t tg3json_array_size(const tg3json_value *array) {
    if (!array || array->type != TG3JSON_ARRAY) return 0;
    return array->u.array.count;
}

int tg3json_array_append_take(tg3json_value *array, tg3json_value *value) {
    size_t cap;
    if (!array || array->type != TG3JSON_ARRAY || !value) return 0;
    cap = array->u.array.count;
    if (!tg3json__reserve_bytes((void **)&array->u.array.items, sizeof(*array->u.array.items),
                                array->u.array.count + 1, &cap)) {
        return 0;
    }
    array->u.array.items[array->u.array.count++] = *value;
    tg3json__init_value(value);
    return 1;
}

int tg3json_array_append_copy(tg3json_value *array, const tg3json_value *value) {
    tg3json_value copy;
    tg3json__init_value(&copy);
    if (!tg3json_value_copy(&copy, value)) return 0;
    if (!tg3json_array_append_take(array, &copy)) {
        tg3json_value_free(&copy);
        return 0;
    }
    return 1;
}

static int tg3json__indent(tg3json__buffer *buf, int indent, int depth) {
    int i;
    if (indent <= 0) return 1;
    if (!tg3json__buf_putc(buf, '\n')) return 0;
    for (i = 0; i < indent * depth; ++i) {
        if (!tg3json__buf_putc(buf, ' ')) return 0;
    }
    return 1;
}

static int tg3json__stringify_value_ex(tg3json__buffer *buf, const tg3json_value *value,
                                       int indent, int depth) {
    size_t i;
    char numbuf[64];
    switch (value->type) {
        case TG3JSON_NULL:
            return tg3json__buf_append(buf, "null", 4);
        case TG3JSON_BOOL:
            return value->u.boolean ? tg3json__buf_append(buf, "true", 4)
                                    : tg3json__buf_append(buf, "false", 5);
        case TG3JSON_INT:
            tg3json__itoa(value->u.integer, numbuf);
            return tg3json__buf_append(buf, numbuf, TINYGLTF_JSON_STRLEN(numbuf));
        case TG3JSON_REAL:
            {
                char *end = tg3json__dtoa_c(value->u.real, numbuf);
                *end = '\0';
                const char *b = numbuf;
                if (*b == '-') ++b;
                if (*b == 'n' || *b == 'N' || *b == 'i' || *b == 'I') {
                    return tg3json__buf_append(buf, "null", 4);
                }
            }
            return tg3json__buf_append(buf, numbuf, TINYGLTF_JSON_STRLEN(numbuf));
        case TG3JSON_STRING:
            if (!tg3json__buf_putc(buf, '"')) return 0;
            for (i = 0; i < value->u.string.len; ++i) {
                unsigned char c = (unsigned char)value->u.string.ptr[i];
                switch (c) {
                    case '"': if (!tg3json__buf_append(buf, "\\\"", 2)) return 0; break;
                    case '\\': if (!tg3json__buf_append(buf, "\\\\", 2)) return 0; break;
                    case '\b': if (!tg3json__buf_append(buf, "\\b", 2)) return 0; break;
                    case '\f': if (!tg3json__buf_append(buf, "\\f", 2)) return 0; break;
                    case '\n': if (!tg3json__buf_append(buf, "\\n", 2)) return 0; break;
                    case '\r': if (!tg3json__buf_append(buf, "\\r", 2)) return 0; break;
                    case '\t': if (!tg3json__buf_append(buf, "\\t", 2)) return 0; break;
                    default:
                        if (c < 0x20u) {
                            numbuf[0] = '\\';
                            numbuf[1] = 'u';
                            numbuf[2] = '0';
                            numbuf[3] = '0';
                            numbuf[4] = "0123456789abcdef"[(c >> 4) & 0xf];
                            numbuf[5] = "0123456789abcdef"[c & 0xf];
                            numbuf[6] = '\0';
                            if (!tg3json__buf_append(buf, numbuf, 6)) return 0;
                        } else {
                            if (!tg3json__buf_putc(buf, (char)c)) return 0;
                        }
                        break;
                }
            }
            return tg3json__buf_putc(buf, '"');
        case TG3JSON_ARRAY:
            if (!tg3json__buf_putc(buf, '[')) return 0;
            for (i = 0; i < value->u.array.count; ++i) {
                if (i > 0 && !tg3json__buf_putc(buf, ',')) return 0;
                if (indent > 0 && !tg3json__indent(buf, indent, depth + 1)) return 0;
                if (!tg3json__stringify_value_ex(buf, &value->u.array.items[i], indent, depth + 1)) return 0;
            }
            if (indent > 0 && value->u.array.count > 0 && !tg3json__indent(buf, indent, depth)) return 0;
            return tg3json__buf_putc(buf, ']');
        case TG3JSON_OBJECT:
            if (!tg3json__buf_putc(buf, '{')) return 0;
            for (i = 0; i < value->u.object.count; ++i) {
                tg3json_value key_value;
                if (i > 0 && !tg3json__buf_putc(buf, ',')) return 0;
                if (indent > 0 && !tg3json__indent(buf, indent, depth + 1)) return 0;
                tg3json__init_value(&key_value);
                key_value.type = TG3JSON_STRING;
                key_value.u.string.ptr = value->u.object.items[i].key;
                key_value.u.string.len = value->u.object.items[i].key_len;
                if (!tg3json__stringify_value_ex(buf, &key_value, indent, depth + 1)) return 0;
                if (!tg3json__buf_putc(buf, ':')) return 0;
                if (indent > 0 && !tg3json__buf_putc(buf, ' ')) return 0;
                if (!tg3json__stringify_value_ex(buf, value->u.object.items[i].value, indent, depth + 1)) return 0;
            }
            if (indent > 0 && value->u.object.count > 0 && !tg3json__indent(buf, indent, depth)) return 0;
            return tg3json__buf_putc(buf, '}');
    }
    return 0; /* unreachable: all enum cases handled above. */
}

char *tg3json_stringify(const tg3json_value *value, size_t *out_len) {
    tg3json__buffer buf;
    TINYGLTF_JSON_MEMSET(&buf, 0, sizeof(buf));
    if (!value || !tg3json__stringify_value_ex(&buf, value, -1, 0)) {
        TINYGLTF_JSON_FREE(buf.data);
        if (out_len) *out_len = 0;
        return NULL;
    }
    if (!buf.data) {
        buf.data = (char *)TINYGLTF_JSON_MALLOC(1);
        if (!buf.data) {
            if (out_len) *out_len = 0;
            return NULL;
        }
        buf.data[0] = '\0';
    }
    if (out_len) *out_len = buf.len;
    return buf.data;
}

char *tg3json_stringify_pretty(const tg3json_value *value, int indent, size_t *out_len) {
    tg3json__buffer buf;
    TINYGLTF_JSON_MEMSET(&buf, 0, sizeof(buf));
    if (!value || !tg3json__stringify_value_ex(&buf, value, indent, 0)) {
        TINYGLTF_JSON_FREE(buf.data);
        if (out_len) *out_len = 0;
        return NULL;
    }
    if (!buf.data) {
        buf.data = (char *)TINYGLTF_JSON_MALLOC(1);
        if (!buf.data) {
            if (out_len) *out_len = 0;
            return NULL;
        }
        buf.data[0] = '\0';
    }
    if (out_len) *out_len = buf.len;
    return buf.data;
}

#endif /* TINYGLTF_JSON_C_IMPLEMENTATION */
#endif /* TINYGLTF_JSON_C_H_ */
