#ifndef TINYGLTF_JSON_C_H_
#define TINYGLTF_JSON_C_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct tg3json__parser {
    const char *cur;
    const char *end;
    const char *error;
    size_t depth_limit;
} tg3json__parser;

typedef struct tg3json__buffer {
    char *data;
    size_t len;
    size_t cap;
} tg3json__buffer;

static void tg3json__init_value(tg3json_value *value) {
    if (!value) return;
    memset(value, 0, sizeof(*value));
    value->type = TG3JSON_NULL;
}

static char *tg3json__strndup_local(const char *src, size_t len) {
    char *dst = (char *)malloc(len + 1);
    if (!dst) return NULL;
    if (len > 0) memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
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
    new_ptr = realloc(*ptr, elem_size * new_cap);
    if (!new_ptr) return 0;
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
    if (!tg3json__reserve_bytes((void **)&buf->data, 1, buf->len + len + 1, &buf->cap)) {
        return 0;
    }
    memcpy(buf->data + buf->len, src, len);
    buf->len += len;
    buf->data[buf->len] = '\0';
    return 1;
}

static int tg3json__buf_putc(tg3json__buffer *buf, char c) {
    if (!tg3json__reserve_bytes((void **)&buf->data, 1, buf->len + 2, &buf->cap)) {
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
    memset(&buf, 0, sizeof(buf));

    if (parser->cur >= parser->end || *parser->cur != '"') {
        tg3json__set_error(parser, parser->cur);
        return 0;
    }

    ++parser->cur;
    start = parser->cur;
    while (parser->cur < parser->end) {
        unsigned char c = (unsigned char)*parser->cur;
        if (c == '"') {
            if (!tg3json__buf_append(&buf, start, (size_t)(parser->cur - start))) goto oom;
            ++parser->cur;
            *out_str = buf.data;
            *out_len = buf.len;
            return 1;
        }
        if (c == '\\') {
            uint32_t codepoint;
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
                    free(buf.data);
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
    free(buf.data);
    return 0;

oom:
    tg3json__set_error(parser, parser->cur);
    free(buf.data);
    return 0;
}

static int tg3json__parse_value(tg3json__parser *parser, size_t depth,
                                tg3json_value *out_value);

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
        if (!tg3json__reserve_bytes((void **)&items, sizeof(*items), count + 1, &cap)) goto oom;
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
    free(items);
    tg3json__set_error(parser, parser->cur);
    return 0;

oom:
    while (count > 0) {
        --count;
        tg3json_value_free(&items[count]);
    }
    free(items);
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
            free(key);
            goto fail;
        }
        ++parser->cur;
        parser->cur = tg3json__skip_ws(parser->cur, parser->end);
        if (!tg3json__parse_value(parser, depth + 1, &value)) {
            free(key);
            goto fail;
        }
        if (!tg3json__reserve_bytes((void **)&items, sizeof(*items), count + 1, &cap)) {
            free(key);
            tg3json_value_free(&value);
            goto oom;
        }
        items[count].key = key;
        items[count].key_len = key_len;
        items[count].value = (tg3json_value *)malloc(sizeof(tg3json_value));
        if (!items[count].value) {
            free(key);
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
        free(items[count].key);
        if (items[count].value) {
            tg3json_value_free(items[count].value);
            free(items[count].value);
        }
    }
    free(items);
    tg3json__set_error(parser, parser->cur);
    return 0;

oom:
    while (count > 0) {
        --count;
        free(items[count].key);
        if (items[count].value) {
            tg3json_value_free(items[count].value);
            free(items[count].value);
        }
    }
    free(items);
    tg3json__set_error(parser, parser->cur);
    return 0;
}

static int tg3json__parse_number(tg3json__parser *parser, tg3json_value *out_value) {
    const char *start = parser->cur;
    const char *p = parser->cur;
    int is_real = 0;
    size_t len;
    char stack_buf[128];
    char *num_buf = stack_buf;

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

    len = (size_t)(p - start);
    if (len + 1 > sizeof(stack_buf)) {
        num_buf = (char *)malloc(len + 1);
        if (!num_buf) goto oom;
    }
    memcpy(num_buf, start, len);
    num_buf[len] = '\0';

    if (!is_real) {
        char *endptr = NULL;
        long long v;
        errno = 0;
        v = strtoll(num_buf, &endptr, 10);
        if (errno == 0 && endptr == num_buf + len) {
            out_value->type = TG3JSON_INT;
            out_value->u.integer = (int64_t)v;
            parser->cur = p;
            if (num_buf != stack_buf) free(num_buf);
            return 1;
        }
        is_real = 1;
    }

    {
        char *endptr = NULL;
        errno = 0;
        out_value->u.real = strtod(num_buf, &endptr);
        if (endptr != num_buf + len) {
            if (num_buf != stack_buf) free(num_buf);
            goto fail;
        }
        out_value->type = TG3JSON_REAL;
        parser->cur = p;
        if (num_buf != stack_buf) free(num_buf);
        return 1;
    }

fail:
    tg3json__set_error(parser, parser->cur);
    return 0;
oom:
    tg3json__set_error(parser, parser->cur);
    return 0;
}

static int tg3json__parse_value(tg3json__parser *parser, size_t depth,
                                tg3json_value *out_value) {
    tg3json__init_value(out_value);
    if (depth > parser->depth_limit) {
        tg3json__set_error(parser, parser->cur);
        return 0;
    }
    parser->cur = tg3json__skip_ws(parser->cur, parser->end);
    if (parser->cur >= parser->end) {
        tg3json__set_error(parser, parser->cur);
        return 0;
    }
    switch (*parser->cur) {
        case 'n':
            if ((size_t)(parser->end - parser->cur) >= 4 && memcmp(parser->cur, "null", 4) == 0) {
                out_value->type = TG3JSON_NULL;
                parser->cur += 4;
                return 1;
            }
            break;
        case 't':
            if ((size_t)(parser->end - parser->cur) >= 4 && memcmp(parser->cur, "true", 4) == 0) {
                out_value->type = TG3JSON_BOOL;
                out_value->u.boolean = 1;
                parser->cur += 4;
                return 1;
            }
            break;
        case 'f':
            if ((size_t)(parser->end - parser->cur) >= 5 && memcmp(parser->cur, "false", 5) == 0) {
                out_value->type = TG3JSON_BOOL;
                out_value->u.boolean = 0;
                parser->cur += 5;
                return 1;
            }
            break;
        case '"':
            out_value->type = TG3JSON_STRING;
            return tg3json__parse_string_raw(parser, &out_value->u.string.ptr,
                                             &out_value->u.string.len);
        case '[':
            return tg3json__parse_array(parser, depth, out_value);
        case '{':
            return tg3json__parse_object(parser, depth, out_value);
        default:
            if (*parser->cur == '-' || (*parser->cur >= '0' && *parser->cur <= '9')) {
                return tg3json__parse_number(parser, out_value);
            }
            break;
    }
    tg3json__set_error(parser, parser->cur);
    return 0;
}

int tg3json_parse_n(const char *data, size_t len, size_t depth_limit,
                    tg3json_value *out_value, const char **out_error_pos) {
    tg3json__parser parser;
    tg3json__init_value(out_value);
    parser.cur = data;
    parser.end = data + len;
    parser.error = NULL;
    parser.depth_limit = depth_limit ? depth_limit : 512;

    if (!tg3json__parse_value(&parser, 0, out_value)) {
        if (out_error_pos) *out_error_pos = parser.error;
        tg3json_value_free(out_value);
        return 0;
    }
    parser.cur = tg3json__skip_ws(parser.cur, parser.end);
    if (parser.cur != parser.end) {
        if (out_error_pos) *out_error_pos = parser.cur;
        tg3json_value_free(out_value);
        return 0;
    }
    if (out_error_pos) *out_error_pos = NULL;
    return 1;
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
            free(value->u.string.ptr);
            break;
        case TG3JSON_ARRAY:
            for (i = 0; i < value->u.array.count; ++i) {
                tg3json_value_free(&value->u.array.items[i]);
            }
            free(value->u.array.items);
            break;
        case TG3JSON_OBJECT:
            for (i = 0; i < value->u.object.count; ++i) {
                free(value->u.object.items[i].key);
                if (value->u.object.items[i].value) {
                    tg3json_value_free(value->u.object.items[i].value);
                    free(value->u.object.items[i].value);
                }
            }
            free(value->u.object.items);
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
    return tg3json_value_init_string_n(value, str, str ? strlen(str) : 0);
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
        if (entry->key_len == key_len && memcmp(entry->key, key, key_len) == 0) {
            return entry->value;
        }
    }
    return NULL;
}

const tg3json_value *tg3json_object_get(const tg3json_value *object,
                                        const char *key) {
    if (!key) return NULL;
    return tg3json_object_get_n(object, key, strlen(key));
}

tg3json_value *tg3json_object_get_mut_n(tg3json_value *object,
                                        const char *key, size_t key_len) {
    size_t i;
    if (!object || object->type != TG3JSON_OBJECT) return NULL;
    for (i = 0; i < object->u.object.count; ++i) {
        tg3json_object_entry *entry = &object->u.object.items[i];
        if (entry->key_len == key_len && memcmp(entry->key, key, key_len) == 0) {
            return entry->value;
        }
    }
    return NULL;
}

tg3json_value *tg3json_object_get_mut(tg3json_value *object, const char *key) {
    if (!key) return NULL;
    return tg3json_object_get_mut_n(object, key, strlen(key));
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
    entry->value = (tg3json_value *)malloc(sizeof(tg3json_value));
    if (!entry->value) {
        free(entry->key);
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
    return tg3json_object_set_take_n(object, key, strlen(key), value);
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
    return tg3json_object_set_copy_n(object, key, strlen(key), value);
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
            snprintf(numbuf, sizeof(numbuf), "%lld", (long long)value->u.integer);
            return tg3json__buf_append(buf, numbuf, strlen(numbuf));
        case TG3JSON_REAL:
            snprintf(numbuf, sizeof(numbuf), "%.17g", value->u.real);
            {
                const char *b = numbuf;
                if (*b == '-') ++b;
                if (*b == 'n' || *b == 'N' || *b == 'i' || *b == 'I') {
                    return tg3json__buf_append(buf, "null", 4);
                }
            }
            if (!strchr(numbuf, '.') && !strchr(numbuf, 'e') && !strchr(numbuf, 'E')) {
                size_t nlen = strlen(numbuf);
                if (nlen + 2 < sizeof(numbuf)) {
                    numbuf[nlen] = '.';
                    numbuf[nlen + 1] = '0';
                    numbuf[nlen + 2] = '\0';
                }
            }
            return tg3json__buf_append(buf, numbuf, strlen(numbuf));
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
                            snprintf(numbuf, sizeof(numbuf), "\\u%04x", (unsigned int)c);
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
    memset(&buf, 0, sizeof(buf));
    if (!value || !tg3json__stringify_value_ex(&buf, value, -1, 0)) {
        free(buf.data);
        if (out_len) *out_len = 0;
        return NULL;
    }
    if (!buf.data) {
        buf.data = (char *)malloc(1);
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
    memset(&buf, 0, sizeof(buf));
    if (!value || !tg3json__stringify_value_ex(&buf, value, indent, 0)) {
        free(buf.data);
        if (out_len) *out_len = 0;
        return NULL;
    }
    if (!buf.data) {
        buf.data = (char *)malloc(1);
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
