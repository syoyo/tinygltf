/*
 * Intensive parser unit tests for tinygltf with the custom JSON backend
 * (tinygltf_json.h).
 *
 * These tests exercise the custom JSON parser and glTF loader with a wide
 * range of edge-case inputs: deeply nested structures, malformed JSON,
 * truncated inputs, unicode escapes, number edge cases, and round-trip
 * serialisation/deserialisation.
 *
 * Build:
 *   c++ -std=c++11 -DTINYGLTF_USE_CUSTOM_JSON -I.. -I. -g -O0 \
 *       -o tester_intensive_customjson tester_intensive_customjson.cc
 */

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#ifndef TINYGLTF_USE_CUSTOM_JSON
#define TINYGLTF_USE_CUSTOM_JSON
#endif
#include "tiny_gltf.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <cmath>
#include <limits>

/* ---------- helpers --------------------------------------------------------*/

/* Convenience: parse JSON string through the custom backend. */
static tinygltf::detail::JsonDocument JsonConstruct(const char *str) {
  tinygltf::detail::JsonDocument doc;
  tinygltf::detail::JsonParse(doc, str, strlen(str));
  return doc;
}

/* Helper: access the i-th element of an array-type json node. */
static tinygltf::detail::json &JsonArrayAt(tinygltf::detail::json &arr,
                                           size_t idx) {
  auto it = arr.begin();
  for (size_t i = 0; i < idx; ++i) ++it;
  return *it;
}

/* Load a glTF JSON string through TinyGLTF and return success. */
static bool LoadGltfFromString(const char *json,
                               tinygltf::Model *model = nullptr,
                               std::string *err = nullptr,
                               std::string *warn = nullptr) {
  tinygltf::Model tmp_model;
  std::string tmp_err, tmp_warn;
  if (!model) model = &tmp_model;
  if (!err) err = &tmp_err;
  if (!warn) warn = &tmp_warn;
  tinygltf::TinyGLTF ctx;
  return ctx.LoadASCIIFromString(model, err, warn, json,
                                 static_cast<unsigned int>(strlen(json)),
                                 /* base_dir */ "");
}

/* ===== Section 1: Custom JSON parser edge-cases =========================== */

TEST_CASE("cj-parse-empty-input", "[customjson][parse]") {
  auto doc = JsonConstruct("");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-parse-whitespace-only", "[customjson][parse]") {
  auto doc = JsonConstruct("   \t\n\r  ");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-parse-null", "[customjson][parse]") {
  auto doc = JsonConstruct("null");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-parse-true", "[customjson][parse]") {
  auto doc = JsonConstruct("true");
  REQUIRE(doc.is_boolean());
  REQUIRE(doc.get<bool>() == true);
}

TEST_CASE("cj-parse-false", "[customjson][parse]") {
  auto doc = JsonConstruct("false");
  REQUIRE(doc.is_boolean());
  REQUIRE(doc.get<bool>() == false);
}

TEST_CASE("cj-parse-integer-zero", "[customjson][parse]") {
  auto doc = JsonConstruct("0");
  REQUIRE(doc.is_number());
  REQUIRE(doc.get<int>() == 0);
}

TEST_CASE("cj-parse-negative-zero", "[customjson][parse]") {
  auto doc = JsonConstruct("-0");
  REQUIRE(doc.is_number());
  // -0 should still compare equal to 0
  REQUIRE(doc.get<double>() == 0.0);
}

TEST_CASE("cj-parse-positive-integer", "[customjson][parse]") {
  auto doc = JsonConstruct("42");
  REQUIRE(doc.is_number());
  REQUIRE(doc.get<int>() == 42);
}

TEST_CASE("cj-parse-negative-integer", "[customjson][parse]") {
  auto doc = JsonConstruct("-999");
  REQUIRE(doc.is_number());
  REQUIRE(doc.get<int>() == -999);
}

TEST_CASE("cj-parse-large-integer", "[customjson][parse]") {
  auto doc = JsonConstruct("2147483647");
  REQUIRE(doc.is_number());
  REQUIRE(doc.get<int>() == 2147483647);
}

TEST_CASE("cj-parse-double", "[customjson][parse]") {
  auto doc = JsonConstruct("3.14159");
  REQUIRE(doc.is_number());
  REQUIRE(doc.get<double>() == Approx(3.14159));
}

TEST_CASE("cj-parse-double-exponent", "[customjson][parse]") {
  auto doc = JsonConstruct("1.5e10");
  REQUIRE(doc.is_number());
  REQUIRE(doc.get<double>() == Approx(1.5e10));
}

TEST_CASE("cj-parse-double-negative-exponent", "[customjson][parse]") {
  auto doc = JsonConstruct("2.5e-3");
  REQUIRE(doc.is_number());
  REQUIRE(doc.get<double>() == Approx(0.0025));
}

TEST_CASE("cj-parse-double-positive-exponent", "[customjson][parse]") {
  auto doc = JsonConstruct("1E+2");
  REQUIRE(doc.is_number());
  REQUIRE(doc.get<double>() == Approx(100.0));
}

TEST_CASE("cj-parse-simple-string", "[customjson][parse]") {
  auto doc = JsonConstruct("\"hello world\"");
  REQUIRE(doc.is_string());
  REQUIRE(doc.get<std::string>() == "hello world");
}

TEST_CASE("cj-parse-empty-string", "[customjson][parse]") {
  auto doc = JsonConstruct("\"\"");
  REQUIRE(doc.is_string());
  REQUIRE(doc.get<std::string>().empty());
}

TEST_CASE("cj-parse-string-escapes", "[customjson][parse]") {
  // Test all standard JSON escape sequences
  auto doc = JsonConstruct("\"a\\nb\\tc\\rd\\\\e\\\"/f\"");
  REQUIRE(doc.is_string());
  std::string expected = "a\nb\tc\rd\\e\"/f";
  REQUIRE(doc.get<std::string>() == expected);
}

TEST_CASE("cj-parse-string-unicode-escape", "[customjson][parse]") {
  // \u0041 == 'A'
  auto doc = JsonConstruct("\"\\u0041\"");
  REQUIRE(doc.is_string());
  REQUIRE(doc.get<std::string>() == "A");
}

TEST_CASE("cj-parse-string-unicode-non-ascii", "[customjson][parse]") {
  // \u00E9 == é (UTF-8: 0xC3 0xA9)
  auto doc = JsonConstruct("\"caf\\u00E9\"");
  REQUIRE(doc.is_string());
  REQUIRE(doc.get<std::string>() == "caf\xC3\xA9");
}

TEST_CASE("cj-parse-string-unicode-cjk", "[customjson][parse]") {
  // \u4E16 == 世 (UTF-8: 0xE4 0xB8 0x96)
  auto doc = JsonConstruct("\"\\u4E16\"");
  REQUIRE(doc.is_string());
  std::string expected;
  expected += '\xE4';
  expected += '\xB8';
  expected += '\x96';
  REQUIRE(doc.get<std::string>() == expected);
}

TEST_CASE("cj-parse-empty-array", "[customjson][parse]") {
  auto doc = JsonConstruct("[]");
  REQUIRE(doc.is_array());
  REQUIRE(doc.size() == 0);
}

TEST_CASE("cj-parse-array-of-ints", "[customjson][parse]") {
  auto doc = JsonConstruct("[1, 2, 3, 4, 5]");
  REQUIRE(doc.is_array());
  REQUIRE(doc.size() == 5);
  for (size_t i = 0; i < 5; i++) {
    REQUIRE(JsonArrayAt(doc, i).get<int>() == static_cast<int>(i + 1));
  }
}

TEST_CASE("cj-parse-nested-arrays", "[customjson][parse]") {
  auto doc = JsonConstruct("[[1, 2], [3, [4, 5]]]");
  REQUIRE(doc.is_array());
  REQUIRE(doc.size() == 2);
  REQUIRE(JsonArrayAt(doc, 0).is_array());
  REQUIRE(JsonArrayAt(doc, 0).size() == 2);
  REQUIRE(JsonArrayAt(doc, 1).is_array());
  REQUIRE(JsonArrayAt(doc, 1).size() == 2);
  REQUIRE(JsonArrayAt(JsonArrayAt(doc, 1), 1).is_array());
  REQUIRE(JsonArrayAt(JsonArrayAt(doc, 1), 1).size() == 2);
}

TEST_CASE("cj-parse-empty-object", "[customjson][parse]") {
  auto doc = JsonConstruct("{}");
  REQUIRE(doc.is_object());
  REQUIRE(doc.size() == 0);
}

TEST_CASE("cj-parse-simple-object", "[customjson][parse]") {
  auto doc = JsonConstruct("{\"a\": 1, \"b\": \"hello\", \"c\": true}");
  REQUIRE(doc.is_object());
  REQUIRE(doc.size() == 3);

  int val = 0;
  REQUIRE(tinygltf::detail::GetInt(doc["a"], val));
  REQUIRE(val == 1);

  std::string s;
  REQUIRE(tinygltf::detail::GetString(doc["b"], s));
  REQUIRE(s == "hello");

  REQUIRE(doc["c"].get<bool>() == true);
}

TEST_CASE("cj-parse-nested-object", "[customjson][parse]") {
  auto doc = JsonConstruct("{\"outer\": {\"inner\": {\"value\": 42}}}");
  REQUIRE(doc.is_object());
  REQUIRE(doc["outer"].is_object());
  REQUIRE(doc["outer"]["inner"].is_object());
  int v = 0;
  REQUIRE(tinygltf::detail::GetInt(doc["outer"]["inner"]["value"], v));
  REQUIRE(v == 42);
}

TEST_CASE("cj-parse-mixed-types-array", "[customjson][parse]") {
  auto doc =
      JsonConstruct("[null, true, false, 42, 3.14, \"text\", [], {}]");
  REQUIRE(doc.is_array());
  REQUIRE(doc.size() == 8);
  REQUIRE(JsonArrayAt(doc, 0).is_null());
  REQUIRE(JsonArrayAt(doc, 1).is_boolean());
  REQUIRE(JsonArrayAt(doc, 2).is_boolean());
  REQUIRE(JsonArrayAt(doc, 3).is_number());
  REQUIRE(JsonArrayAt(doc, 4).is_number());
  REQUIRE(JsonArrayAt(doc, 5).is_string());
  REQUIRE(JsonArrayAt(doc, 6).is_array());
  REQUIRE(JsonArrayAt(doc, 7).is_object());
}

/* ===== Section 2: Deeply nested structures ================================ */

TEST_CASE("cj-deep-nested-arrays", "[customjson][depth]") {
  // Build a deeply nested array: [[[[...]]]]
  const int depth = 200;
  std::string json;
  for (int i = 0; i < depth; i++) json += "[";
  json += "1";
  for (int i = 0; i < depth; i++) json += "]";

  auto doc = JsonConstruct(json.c_str());
  REQUIRE(doc.is_array());

  // Walk down
  tinygltf::detail::json *cur = &doc;
  for (int i = 0; i < depth - 1; i++) {
    REQUIRE(cur->is_array());
    REQUIRE(cur->size() == 1);
    cur = &JsonArrayAt(*cur, 0);
  }
  REQUIRE(cur->is_array());
  REQUIRE(cur->size() == 1);
  REQUIRE(JsonArrayAt(*cur, 0).get<int>() == 1);
}

TEST_CASE("cj-deep-nested-objects", "[customjson][depth]") {
  const int depth = 100;
  std::string json;
  for (int i = 0; i < depth; i++) {
    json += "{\"k\":";
  }
  json += "42";
  for (int i = 0; i < depth; i++) {
    json += "}";
  }

  auto doc = JsonConstruct(json.c_str());
  REQUIRE(doc.is_object());

  tinygltf::detail::json *cur = &doc;
  for (int i = 0; i < depth - 1; i++) {
    REQUIRE(cur->is_object());
    cur = &((*cur)["k"]);
  }
  REQUIRE(cur->is_object());
  int v = 0;
  REQUIRE(tinygltf::detail::GetInt((*cur)["k"], v));
  REQUIRE(v == 42);
}

/* ===== Section 3: Malformed input handling ================================ */

TEST_CASE("cj-malformed-truncated-object", "[customjson][malformed]") {
  auto doc = JsonConstruct("{\"key\":");
  // Should return null on parse error
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-truncated-array", "[customjson][malformed]") {
  auto doc = JsonConstruct("[1, 2,");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-trailing-comma-array", "[customjson][malformed]") {
  auto doc = JsonConstruct("[1, 2, 3,]");
  // Trailing comma is not valid JSON
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-trailing-comma-object", "[customjson][malformed]") {
  auto doc = JsonConstruct("{\"a\": 1,}");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-missing-colon", "[customjson][malformed]") {
  auto doc = JsonConstruct("{\"key\" \"value\"}");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-missing-comma", "[customjson][malformed]") {
  auto doc = JsonConstruct("[1 2 3]");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-double-comma", "[customjson][malformed]") {
  auto doc = JsonConstruct("[1,,2]");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-unquoted-key", "[customjson][malformed]") {
  auto doc = JsonConstruct("{key: 1}");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-single-quoted-string", "[customjson][malformed]") {
  auto doc = JsonConstruct("{'key': 1}");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-unterminated-string", "[customjson][malformed]") {
  auto doc = JsonConstruct("\"hello");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-bad-escape", "[customjson][malformed]") {
  auto doc = JsonConstruct("\"bad\\xescape\"");
  // \\x is not a valid JSON escape
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-truncated-unicode-escape", "[customjson][malformed]") {
  auto doc = JsonConstruct("\"trunc\\u00\"");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-incomplete-true", "[customjson][malformed]") {
  auto doc = JsonConstruct("tru");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-incomplete-false", "[customjson][malformed]") {
  auto doc = JsonConstruct("fals");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-incomplete-null", "[customjson][malformed]") {
  auto doc = JsonConstruct("nul");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-extra-data", "[customjson][malformed]") {
  // Valid JSON followed by extra non-whitespace data
  auto doc = JsonConstruct("42 extra");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-trailing-whitespace-ok", "[customjson][parse]") {
  // Valid JSON followed by whitespace only should be accepted
  auto doc = JsonConstruct("42   \t\n ");
  REQUIRE(doc.is_number());
  REQUIRE(doc.get<int>() == 42);
}

TEST_CASE("cj-malformed-just-brace", "[customjson][malformed]") {
  auto doc = JsonConstruct("{");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-just-bracket", "[customjson][malformed]") {
  auto doc = JsonConstruct("[");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-mismatched-brackets", "[customjson][malformed]") {
  auto doc = JsonConstruct("[}");
  REQUIRE(doc.is_null());
}

TEST_CASE("cj-malformed-mismatched-braces", "[customjson][malformed]") {
  auto doc = JsonConstruct("{]");
  REQUIRE(doc.is_null());
}

/* ===== Section 4: glTF-level loading edge-cases =========================== */

TEST_CASE("cj-gltf-empty-string", "[customjson][gltf]") {
  REQUIRE(false == LoadGltfFromString(""));
}

TEST_CASE("cj-gltf-garbage-input", "[customjson][gltf]") {
  REQUIRE(false == LoadGltfFromString("not json at all"));
}

TEST_CASE("cj-gltf-valid-null", "[customjson][gltf]") {
  REQUIRE(false == LoadGltfFromString("null"));
}

TEST_CASE("cj-gltf-valid-array-root", "[customjson][gltf]") {
  // glTF expects root to be an object, not an array
  REQUIRE(false == LoadGltfFromString("[]"));
}

TEST_CASE("cj-gltf-minimal-valid", "[customjson][gltf]") {
  // Minimal valid glTF 2.0
  const char *json = R"({"asset":{"version":"2.0"}})";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));
  REQUIRE(model.asset.version == "2.0");
}

TEST_CASE("cj-gltf-empty-meshes", "[customjson][gltf]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "meshes": []
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));
  REQUIRE(model.meshes.empty());
}

TEST_CASE("cj-gltf-scene-with-nodes", "[customjson][gltf]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "scenes": [{"nodes": [0]}],
    "nodes": [{"name": "TestNode"}]
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));
  REQUIRE(model.scenes.size() == 1);
  REQUIRE(model.nodes.size() == 1);
  REQUIRE(model.nodes[0].name == "TestNode");
}

TEST_CASE("cj-gltf-node-with-transform", "[customjson][gltf]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "nodes": [{
      "translation": [1.0, 2.0, 3.0],
      "rotation": [0.0, 0.0, 0.0, 1.0],
      "scale": [1.0, 1.0, 1.0]
    }]
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));
  REQUIRE(model.nodes.size() == 1);
  REQUIRE(model.nodes[0].translation.size() == 3);
  REQUIRE(model.nodes[0].translation[0] == Approx(1.0));
  REQUIRE(model.nodes[0].translation[1] == Approx(2.0));
  REQUIRE(model.nodes[0].translation[2] == Approx(3.0));
  REQUIRE(model.nodes[0].rotation.size() == 4);
  REQUIRE(model.nodes[0].scale.size() == 3);
}

TEST_CASE("cj-gltf-material-pbr", "[customjson][gltf]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "materials": [{
      "pbrMetallicRoughness": {
        "baseColorFactor": [1.0, 0.5, 0.25, 1.0],
        "metallicFactor": 0.8,
        "roughnessFactor": 0.2
      }
    }]
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));
  REQUIRE(model.materials.size() == 1);
  auto &pbr = model.materials[0].pbrMetallicRoughness;
  REQUIRE(pbr.baseColorFactor.size() == 4);
  REQUIRE(pbr.baseColorFactor[0] == Approx(1.0));
  REQUIRE(pbr.baseColorFactor[1] == Approx(0.5));
  REQUIRE(pbr.baseColorFactor[2] == Approx(0.25));
  REQUIRE(pbr.metallicFactor == Approx(0.8));
  REQUIRE(pbr.roughnessFactor == Approx(0.2));
}

TEST_CASE("cj-gltf-accessors-and-bufferviews", "[customjson][gltf]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "buffers": [{"uri": "data:application/octet-stream;base64,AAAAAAAAAAA=", "byteLength": 8}],
    "bufferViews": [{"buffer": 0, "byteOffset": 0, "byteLength": 8}],
    "accessors": [{
      "bufferView": 0,
      "componentType": 5126,
      "count": 2,
      "type": "SCALAR",
      "max": [1.0],
      "min": [0.0]
    }]
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));
  REQUIRE(model.buffers.size() == 1);
  REQUIRE(model.bufferViews.size() == 1);
  REQUIRE(model.accessors.size() == 1);
  REQUIRE(model.accessors[0].componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
  REQUIRE(model.accessors[0].count == 2);
  REQUIRE(model.accessors[0].type == TINYGLTF_TYPE_SCALAR);
}

TEST_CASE("cj-gltf-extensions", "[customjson][gltf]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "extensionsUsed": ["KHR_lights_punctual"],
    "extensions": {
      "KHR_lights_punctual": {
        "lights": [{
          "color": [1.0, 1.0, 1.0],
          "intensity": 5.0,
          "type": "point"
        }]
      }
    }
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));
  REQUIRE(model.extensionsUsed.size() == 1);
  REQUIRE(model.extensionsUsed[0] == "KHR_lights_punctual");
  REQUIRE(model.extensions.count("KHR_lights_punctual") == 1);
}

TEST_CASE("cj-gltf-extras", "[customjson][gltf]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "nodes": [{
      "name": "Test",
      "extras": {"custom_field": 123, "nested": {"a": [1,2,3]}}
    }]
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));
  REQUIRE(model.nodes.size() == 1);
  // extras should be preserved as a Value
  REQUIRE(model.nodes[0].extras.IsObject());
}

/* ===== Section 5: Round-trip serialisation ================================ */

TEST_CASE("cj-roundtrip-minimal", "[customjson][roundtrip]") {
  const char *json = R"({"asset":{"version":"2.0"}})";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));

  // Serialise to string
  std::stringstream os;
  tinygltf::TinyGLTF ctx;
  REQUIRE(true == ctx.WriteGltfSceneToStream(&model, os, false, false));

  // Re-parse and verify
  tinygltf::Model model2;
  std::string json2 = os.str();
  REQUIRE(true ==
          ctx.LoadASCIIFromString(&model2, &err, &warn, json2.c_str(),
                                  static_cast<unsigned int>(json2.size()), ""));
  REQUIRE(model2.asset.version == "2.0");
}

TEST_CASE("cj-roundtrip-with-nodes", "[customjson][roundtrip]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "scenes": [{"nodes": [0]}],
    "nodes": [
      {"name": "Root", "children": [1]},
      {"name": "Child", "translation": [1.0, 2.0, 3.0]}
    ]
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));

  std::stringstream os;
  tinygltf::TinyGLTF ctx;
  REQUIRE(true == ctx.WriteGltfSceneToStream(&model, os, false, false));

  tinygltf::Model model2;
  std::string json2 = os.str();
  REQUIRE(true ==
          ctx.LoadASCIIFromString(&model2, &err, &warn, json2.c_str(),
                                  static_cast<unsigned int>(json2.size()), ""));
  REQUIRE(model2.nodes.size() == 2);
  REQUIRE(model2.nodes[0].name == "Root");
  REQUIRE(model2.nodes[1].name == "Child");
  REQUIRE(model2.nodes[1].translation[0] == Approx(1.0));
}

TEST_CASE("cj-roundtrip-material", "[customjson][roundtrip]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "materials": [{
      "name": "TestMat",
      "pbrMetallicRoughness": {
        "baseColorFactor": [0.8, 0.2, 0.1, 1.0],
        "metallicFactor": 0.5,
        "roughnessFactor": 0.7
      },
      "doubleSided": true
    }]
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));

  std::stringstream os;
  tinygltf::TinyGLTF ctx;
  REQUIRE(true == ctx.WriteGltfSceneToStream(&model, os, false, false));

  tinygltf::Model model2;
  std::string json2 = os.str();
  REQUIRE(true ==
          ctx.LoadASCIIFromString(&model2, &err, &warn, json2.c_str(),
                                  static_cast<unsigned int>(json2.size()), ""));
  REQUIRE(model2.materials.size() == 1);
  REQUIRE(model2.materials[0].name == "TestMat");
  REQUIRE(model2.materials[0].doubleSided == true);
  REQUIRE(model2.materials[0].pbrMetallicRoughness.metallicFactor ==
          Approx(0.5));
}

/* ===== Section 6: Binary (GLB) loading ==================================== */

TEST_CASE("cj-glb-invalid-magic", "[customjson][glb]") {
  // GLB file header fields: magic (4), version (4), length (4) = 12 bytes.
  // TinyGLTF::LoadBinaryFromMemory, however, requires at least 20 bytes
  // (12-byte file header + 8-byte first chunk header) before checking magic/version.
  // This buffer is only 12 bytes long (and uses a wrong magic), so the loader
  // will reject it as an undersized/invalid GLB binary.
  unsigned char data[12] = {0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
                            0x00, 0x00, 0x0C, 0x00, 0x00, 0x00};
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err, warn;
  bool ret = ctx.LoadBinaryFromMemory(&model, &err, &warn, data, sizeof(data));
  REQUIRE(false == ret);
}

TEST_CASE("cj-glb-truncated-header", "[customjson][glb]") {
  // Less than 12 bytes
  unsigned char data[8] = {0x67, 0x6C, 0x54, 0x46, 0x02, 0x00, 0x00, 0x00};
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err, warn;
  bool ret = ctx.LoadBinaryFromMemory(&model, &err, &warn, data, sizeof(data));
  REQUIRE(false == ret);
}

TEST_CASE("cj-glb-zero-length", "[customjson][glb]") {
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err, warn;
  bool ret = ctx.LoadBinaryFromMemory(&model, &err, &warn, nullptr, 0);
  REQUIRE(false == ret);
}

/* ===== Section 7: JSON detail helpers ===================================== */

TEST_CASE("cj-detail-GetInt", "[customjson][detail]") {
  auto doc = JsonConstruct("{\"val\": 42}");
  int v = 0;
  REQUIRE(tinygltf::detail::GetInt(doc["val"], v));
  REQUIRE(v == 42);
}

TEST_CASE("cj-detail-GetDouble", "[customjson][detail]") {
  auto doc = JsonConstruct("{\"val\": 3.14}");
  double v = 0;
  REQUIRE(tinygltf::detail::GetDouble(doc["val"], v));
  REQUIRE(v == Approx(3.14));
}

TEST_CASE("cj-detail-GetNumber", "[customjson][detail]") {
  auto doc = JsonConstruct("{\"val\": 99}");
  double v = 0;
  REQUIRE(tinygltf::detail::GetNumber(doc["val"], v));
  REQUIRE(v == Approx(99.0));
}

TEST_CASE("cj-detail-GetString", "[customjson][detail]") {
  auto doc = JsonConstruct("{\"val\": \"hello\"}");
  std::string v;
  REQUIRE(tinygltf::detail::GetString(doc["val"], v));
  REQUIRE(v == "hello");
}

TEST_CASE("cj-detail-IsArray", "[customjson][detail]") {
  auto doc = JsonConstruct("[1, 2]");
  REQUIRE(tinygltf::detail::IsArray(doc));
  auto doc2 = JsonConstruct("42");
  REQUIRE_FALSE(tinygltf::detail::IsArray(doc2));
}

TEST_CASE("cj-detail-IsObject", "[customjson][detail]") {
  auto doc = JsonConstruct("{\"a\": 1}");
  REQUIRE(tinygltf::detail::IsObject(doc));
  auto doc2 = JsonConstruct("[]");
  REQUIRE_FALSE(tinygltf::detail::IsObject(doc2));
}

TEST_CASE("cj-detail-IsEmpty", "[customjson][detail]") {
  auto doc = JsonConstruct("{}");
  REQUIRE(tinygltf::detail::IsEmpty(doc));
  auto doc2 = JsonConstruct("{\"a\": 1}");
  REQUIRE_FALSE(tinygltf::detail::IsEmpty(doc2));
}

TEST_CASE("cj-detail-JsonIsNull", "[customjson][detail]") {
  auto doc = JsonConstruct("null");
  REQUIRE(tinygltf::detail::JsonIsNull(doc));
  auto doc2 = JsonConstruct("42");
  REQUIRE_FALSE(tinygltf::detail::JsonIsNull(doc2));
}

/* ===== Section 8: Serialisation helpers =================================== */

TEST_CASE("cj-detail-JsonFromString", "[customjson][detail]") {
  auto j = tinygltf::detail::JsonFromString("test");
  REQUIRE(j.is_string());
  REQUIRE(j.get<std::string>() == "test");
}

TEST_CASE("cj-detail-JsonSetObject", "[customjson][detail]") {
  tinygltf::detail::json j;
  tinygltf::detail::JsonSetObject(j);
  REQUIRE(j.is_object());
  REQUIRE(j.size() == 0);
}

TEST_CASE("cj-detail-JsonAddMember", "[customjson][detail]") {
  tinygltf::detail::json j;
  tinygltf::detail::JsonSetObject(j);
  auto val = tinygltf::detail::JsonFromString("bar");
  tinygltf::detail::JsonAddMember(j, "foo", std::move(val));
  REQUIRE(j.size() == 1);
  std::string s;
  REQUIRE(tinygltf::detail::GetString(j["foo"], s));
  REQUIRE(s == "bar");
}

TEST_CASE("cj-detail-JsonPushBack", "[customjson][detail]") {
  auto j = JsonConstruct("[]");
  tinygltf::detail::json val;
  val = JsonConstruct("42");
  tinygltf::detail::JsonPushBack(j, std::move(val));
  REQUIRE(j.size() == 1);
  REQUIRE(JsonArrayAt(j, 0).get<int>() == 42);
}

/* ===== Section 9: Stress / large input ==================================== */

TEST_CASE("cj-large-array", "[customjson][stress]") {
  // Array with 1000 integers
  std::string json = "[";
  for (int i = 0; i < 1000; i++) {
    if (i > 0) json += ",";
    json += std::to_string(i);
  }
  json += "]";
  auto doc = JsonConstruct(json.c_str());
  REQUIRE(doc.is_array());
  REQUIRE(doc.size() == 1000);
  REQUIRE(JsonArrayAt(doc, 0).get<int>() == 0);
  REQUIRE(JsonArrayAt(doc, 999).get<int>() == 999);
}

TEST_CASE("cj-large-object", "[customjson][stress]") {
  // Object with 500 keys
  std::string json = "{";
  for (int i = 0; i < 500; i++) {
    if (i > 0) json += ",";
    json += "\"key" + std::to_string(i) + "\":" + std::to_string(i);
  }
  json += "}";
  auto doc = JsonConstruct(json.c_str());
  REQUIRE(doc.is_object());
  REQUIRE(doc.size() == 500);
  int v = -1;
  REQUIRE(tinygltf::detail::GetInt(doc["key0"], v));
  REQUIRE(v == 0);
  REQUIRE(tinygltf::detail::GetInt(doc["key499"], v));
  REQUIRE(v == 499);
}

TEST_CASE("cj-long-string", "[customjson][stress]") {
  // String with 10000 characters
  std::string content(10000, 'x');
  std::string json = "\"" + content + "\"";
  auto doc = JsonConstruct(json.c_str());
  REQUIRE(doc.is_string());
  REQUIRE(doc.get<std::string>().size() == 10000);
}

TEST_CASE("cj-many-escapes", "[customjson][stress]") {
  // String with many escape sequences
  std::string json = "\"";
  for (int i = 0; i < 500; i++) {
    json += "\\n\\t";
  }
  json += "\"";
  auto doc = JsonConstruct(json.c_str());
  REQUIRE(doc.is_string());
  REQUIRE(doc.get<std::string>().size() == 1000);
}

/* ===== Section 10: glTF complex models ==================================== */

TEST_CASE("cj-gltf-multiple-meshes-primitives", "[customjson][gltf]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "meshes": [
      {"primitives": [{"attributes": {"POSITION": 0}, "mode": 4}]},
      {"primitives": [
        {"attributes": {"POSITION": 0}, "mode": 4},
        {"attributes": {"POSITION": 0, "NORMAL": 1}, "mode": 4}
      ]}
    ],
    "accessors": [
      {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3",
       "max": [1,1,1], "min": [0,0,0]},
      {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3",
       "max": [1,1,1], "min": [0,0,0]}
    ],
    "bufferViews": [{"buffer": 0, "byteLength": 36}],
    "buffers": [{"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", "byteLength": 36}]
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));
  REQUIRE(model.meshes.size() == 2);
  REQUIRE(model.meshes[0].primitives.size() == 1);
  REQUIRE(model.meshes[1].primitives.size() == 2);
}

TEST_CASE("cj-gltf-animations", "[customjson][gltf]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "animations": [{
      "channels": [{"sampler": 0, "target": {"node": 0, "path": "translation"}}],
      "samplers": [{"input": 0, "output": 1, "interpolation": "LINEAR"}]
    }],
    "nodes": [{"name": "AnimNode"}],
    "accessors": [
      {"bufferView": 0, "componentType": 5126, "count": 2, "type": "SCALAR",
       "max": [1.0], "min": [0.0]},
      {"bufferView": 0, "componentType": 5126, "count": 2, "type": "VEC3",
       "max": [1,1,1], "min": [0,0,0]}
    ],
    "bufferViews": [{"buffer": 0, "byteLength": 24}],
    "buffers": [{"uri": "data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", "byteLength": 24}]
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));
  REQUIRE(model.animations.size() == 1);
  REQUIRE(model.animations[0].channels.size() == 1);
  REQUIRE(model.animations[0].samplers.size() == 1);
  REQUIRE(model.animations[0].channels[0].target_path == "translation");
}

TEST_CASE("cj-gltf-cameras", "[customjson][gltf]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "cameras": [
      {
        "type": "perspective",
        "perspective": {
          "aspectRatio": 1.5,
          "yfov": 0.66,
          "zfar": 100.0,
          "znear": 0.01
        }
      },
      {
        "type": "orthographic",
        "orthographic": {
          "xmag": 10.0,
          "ymag": 10.0,
          "zfar": 100.0,
          "znear": 0.01
        }
      }
    ]
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));
  REQUIRE(model.cameras.size() == 2);
  REQUIRE(model.cameras[0].type == "perspective");
  REQUIRE(model.cameras[1].type == "orthographic");
}

TEST_CASE("cj-gltf-skins", "[customjson][gltf]") {
  const char *json = R"({
    "asset": {"version": "2.0"},
    "nodes": [
      {"name": "Armature"},
      {"name": "Bone1"},
      {"name": "Bone2"}
    ],
    "skins": [{
      "joints": [1, 2],
      "skeleton": 0
    }]
  })";
  tinygltf::Model model;
  std::string err, warn;
  REQUIRE(true == LoadGltfFromString(json, &model, &err, &warn));
  REQUIRE(model.skins.size() == 1);
  REQUIRE(model.skins[0].joints.size() == 2);
  REQUIRE(model.skins[0].skeleton == 0);
}

/* ===== Section 11: Truncated glTF JSON at various positions =============== */

TEST_CASE("cj-gltf-truncated-at-key", "[customjson][truncated]") {
  REQUIRE(false == LoadGltfFromString("{\"asset\""));
}

TEST_CASE("cj-gltf-truncated-at-colon", "[customjson][truncated]") {
  REQUIRE(false == LoadGltfFromString("{\"asset\":"));
}

TEST_CASE("cj-gltf-truncated-mid-value", "[customjson][truncated]") {
  REQUIRE(false == LoadGltfFromString("{\"asset\":{\"ver"));
}

TEST_CASE("cj-gltf-truncated-after-comma", "[customjson][truncated]") {
  REQUIRE(false ==
          LoadGltfFromString("{\"asset\":{\"version\":\"2.0\"},"));
}

/* ===== Section 12: JSON dump / serialisation ============================== */

TEST_CASE("cj-dump-null", "[customjson][dump]") {
  auto doc = JsonConstruct("null");
  std::string s = doc.dump(-1);
  REQUIRE(s == "null");
}

TEST_CASE("cj-dump-bool", "[customjson][dump]") {
  auto doc = JsonConstruct("true");
  REQUIRE(doc.dump(-1) == "true");
  auto doc2 = JsonConstruct("false");
  REQUIRE(doc2.dump(-1) == "false");
}

TEST_CASE("cj-dump-int", "[customjson][dump]") {
  auto doc = JsonConstruct("42");
  REQUIRE(doc.dump(-1) == "42");
}

TEST_CASE("cj-dump-string", "[customjson][dump]") {
  auto doc = JsonConstruct("\"hello\"");
  std::string s = doc.dump(-1);
  REQUIRE(s == "\"hello\"");
}

TEST_CASE("cj-dump-array", "[customjson][dump]") {
  auto doc = JsonConstruct("[1,2,3]");
  std::string s = doc.dump(-1);
  // Should contain all three values
  REQUIRE(s.find("1") != std::string::npos);
  REQUIRE(s.find("2") != std::string::npos);
  REQUIRE(s.find("3") != std::string::npos);
}

TEST_CASE("cj-dump-object", "[customjson][dump]") {
  auto doc = JsonConstruct("{\"key\":\"val\"}");
  std::string s = doc.dump(-1);
  REQUIRE(s.find("\"key\"") != std::string::npos);
  REQUIRE(s.find("\"val\"") != std::string::npos);
}

TEST_CASE("cj-dump-roundtrip", "[customjson][dump]") {
  const char *input = "{\"a\":[1,2,3],\"b\":{\"c\":true}}";
  auto doc = JsonConstruct(input);
  std::string s = doc.dump(-1);
  // Re-parse the dump output
  auto doc2 = JsonConstruct(s.c_str());
  REQUIRE(doc2.is_object());
  REQUIRE(doc2["a"].is_array());
  REQUIRE(doc2["a"].size() == 3);
  REQUIRE(doc2["b"]["c"].get<bool>() == true);
}

/* ===== Section 13: Fuzz-like systematic truncation ======================== */

TEST_CASE("cj-systematic-truncation", "[customjson][fuzzlike]") {
  // Take a valid JSON string and verify the parser doesn't crash
  // at every truncation point
  const char *valid_json =
      R"({"asset":{"version":"2.0"},"nodes":[{"name":"test","translation":[1.0,2.0,3.0]}]})";
  size_t len = strlen(valid_json);

  for (size_t i = 0; i < len; i++) {
    std::string truncated(valid_json, i);
    tinygltf::detail::JsonDocument doc;
    // Should not crash; may fail gracefully
    tinygltf::detail::JsonParse(doc, truncated.c_str(), truncated.size());
    // No assertion on result - just checking it doesn't crash/hang
  }
}

TEST_CASE("cj-systematic-byte-flip", "[customjson][fuzzlike]") {
  // Take a valid JSON string and flip each byte, verify no crash
  const char *valid_json = R"({"asset":{"version":"2.0"}})";
  size_t len = strlen(valid_json);

  for (size_t i = 0; i < len; i++) {
    std::string mutated(valid_json, len);
    mutated[i] = static_cast<char>(mutated[i] ^ 0xFF);
    tinygltf::detail::JsonDocument doc;
    tinygltf::detail::JsonParse(doc, mutated.c_str(), mutated.size());
    // No assertion - just checking it doesn't crash
  }
}

TEST_CASE("cj-systematic-null-insertion", "[customjson][fuzzlike]") {
  // Insert null bytes at various positions
  const char *valid_json = R"({"a":"b","c":1})";
  size_t len = strlen(valid_json);

  for (size_t i = 0; i <= len; i++) {
    std::string mutated(valid_json, len);
    mutated.insert(i, 1, '\0');
    tinygltf::detail::JsonDocument doc;
    tinygltf::detail::JsonParse(doc, mutated.c_str(), mutated.size());
    // No assertion - just checking it doesn't crash
  }
}
