#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TINYGLTF_JSON_C_IMPLEMENTATION
#include "tinygltf_json_c.h"

static uint64_t dbl_bits(double v) {
  uint64_t bits;
  memcpy(&bits, &v, sizeof(bits));
  return bits;
}

static double dbl_from_bits(uint64_t bits) {
  double v;
  memcpy(&v, &bits, sizeof(v));
  return v;
}

static int check_stringify(const char *json, const char *expected) {
  tg3json_value v;
  const char *err = NULL;
  char *out;
  size_t out_len = 0;
  int ok = 1;
  if (!tg3json_parse(json, json + strlen(json), 128, &v, &err)) {
    fprintf(stderr, "parse failed for %s at %td\n", json, err ? err - json : -1);
    return 0;
  }
  out = tg3json_stringify(&v, &out_len);
  if (!out || strcmp(out, expected) != 0) {
    fprintf(stderr, "stringify(%s) = %s, expected %s\n",
            json, out ? out : "(null)", expected);
    ok = 0;
  }
  if (out) TINYGLTF_JSON_FREE(out);
  tg3json_value_free(&v);
  return ok;
}

static int check_parse_rejects(const char *json) {
  tg3json_value v;
  const char *err = NULL;
  if (tg3json_parse(json, json + strlen(json), 128, &v, &err)) {
    fprintf(stderr, "parse unexpectedly accepted %s\n", json);
    tg3json_value_free(&v);
    return 0;
  }
  return 1;
}

static int check_roundtrip(const char *json) {
  tg3json_value a;
  tg3json_value b;
  const char *err = NULL;
  char *out;
  size_t out_len = 0;
  int ok = 1;
  if (!tg3json_parse(json, json + strlen(json), 128, &a, &err)) return 0;
  out = tg3json_stringify(&a, &out_len);
  if (!out || !tg3json_parse(out, out + out_len, 128, &b, &err)) {
    ok = 0;
  } else if (a.type != TG3JSON_REAL ||
             !((b.type == TG3JSON_REAL && dbl_bits(a.u.real) == dbl_bits(b.u.real)) ||
               (b.type == TG3JSON_INT && dbl_bits(a.u.real) == dbl_bits((double)b.u.integer)))) {
    fprintf(stderr, "roundtrip changed bits: %s -> %s\n", json, out);
    ok = 0;
  }
  if (out) TINYGLTF_JSON_FREE(out);
  tg3json_value_free(&a);
  tg3json_value_free(&b);
  return ok;
}

static int check_parse_bits(const char *json, uint64_t expected_bits) {
  tg3json_value v;
  const char *err = NULL;
  if (!tg3json_parse(json, json + strlen(json), 128, &v, &err)) {
    fprintf(stderr, "parse failed for %s at %td\n", json, err ? err - json : -1);
    return 0;
  }
  if (v.type != TG3JSON_REAL || dbl_bits(v.u.real) != expected_bits) {
    fprintf(stderr, "parse bits mismatch: %s -> 0x%llx, expected 0x%llx\n",
            json, (unsigned long long)(v.type == TG3JSON_REAL ? dbl_bits(v.u.real) : 0),
            (unsigned long long)expected_bits);
    tg3json_value_free(&v);
    return 0;
  }
  tg3json_value_free(&v);
  return 1;
}

static int check_parse_float32(void) {
  static const char json[] = "0.10000000149011612";
  tg3json_parse_options opts;
  tg3json_value v;
  const char *err = NULL;
  double expected = (double)(float)0.10000000149011612;
  memset(&opts, 0, sizeof(opts));
  opts.parse_float32 = 1;
  if (!tg3json_parse_n_opts(json, strlen(json), &opts, &v, &err)) {
    fprintf(stderr, "parse_float32 parse failed\n");
    return 0;
  }
  if (v.type != TG3JSON_REAL || dbl_bits(v.u.real) != dbl_bits(expected)) {
    fprintf(stderr, "parse_float32 did not round through float\n");
    tg3json_value_free(&v);
    return 0;
  }
  tg3json_value_free(&v);
  return 1;
}

static int check_nonfinite_stringifies_to_null(void) {
  tg3json_value v;
  char *out;
  size_t len = 0;
  int ok;
  tg3json_value_init_real(&v, dbl_from_bits(0x7ff0000000000000ULL));
  out = tg3json_stringify(&v, &len);
  ok = out && strcmp(out, "null") == 0;
  if (!ok) fprintf(stderr, "inf stringify = %s\n", out ? out : "(null)");
  if (out) TINYGLTF_JSON_FREE(out);
  tg3json_value_free(&v);
  if (!ok) return 0;

  tg3json_value_init_real(&v, dbl_from_bits(0x7ff8000000000001ULL));
  out = tg3json_stringify(&v, &len);
  ok = out && strcmp(out, "null") == 0;
  if (!ok) fprintf(stderr, "nan stringify = %s\n", out ? out : "(null)");
  if (out) TINYGLTF_JSON_FREE(out);
  tg3json_value_free(&v);
  return ok;
}

int main(void) {
  int ok = 1;
  ok = check_stringify("1.0", "1") && ok;
  ok = check_stringify("-1.0", "-1") && ok;
  ok = check_stringify("0.1", "0.1") && ok;
  ok = check_stringify("0.0001", "0.0001") && ok;
  ok = check_stringify("0.00001", "1e-5") && ok;
  ok = check_stringify("1000000000000000.0", "1000000000000000") && ok;
  ok = check_stringify("10000000000000000.0", "1e16") && ok;
  ok = check_roundtrip("1.2345678901234567") && ok;
  ok = check_roundtrip("2.2250738585072014e-308") && ok;
  ok = check_roundtrip("5e-324") && ok;
  ok = check_roundtrip("-5e-324") && ok;
  ok = check_roundtrip("9007199254740993.0") && ok;
  ok = check_parse_bits("1.23456789012345678901", 0x3ff3c0ca428c59fbULL) && ok;
  ok = check_parse_bits("1.234567890123456789012345678901234567890e-100",
                        0x2b31482fe620c5d2ULL) && ok;
  ok = check_parse_bits("1.7976931348623157e308", 0x7fefffffffffffffULL) && ok;
  ok = check_parse_float32() && ok;
  ok = check_nonfinite_stringifies_to_null() && ok;
  ok = check_parse_rejects("+1") && ok;
  ok = check_parse_rejects("01") && ok;
  ok = check_parse_rejects("1.") && ok;
  ok = check_parse_rejects("1e") && ok;
  ok = check_parse_rejects("1e400") && ok;
  ok = check_parse_rejects("-1e400") && ok;
  ok = check_parse_rejects("1.7976931348623159e308") && ok;
  ok = check_parse_rejects("[1,]") && ok;
  return ok ? 0 : 1;
}
