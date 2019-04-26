#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>

TEST_CASE("parse-error", "[parse]") {

  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  bool ret = ctx.LoadASCIIFromString(&model, &err, &warn, "bora", strlen("bora"), /* basedir*/ "");

  REQUIRE(false == ret);

}

TEST_CASE("datauri-in-glb", "[issue-79]") {

  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  bool ret = ctx.LoadBinaryFromFile(&model, &err, &warn, "../models/box01.glb");
  if (!err.empty()) {
    std::cerr << err << std::endl;
  }

  REQUIRE(true == ret);
}

TEST_CASE("extension-with-empty-object", "[issue-97]") {

  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  bool ret = ctx.LoadASCIIFromFile(&model, &err, &warn, "../models/Extensions-issue97/test.gltf");
  if (!err.empty()) {
    std::cerr << err << std::endl;
  }
  REQUIRE(true == ret);

  REQUIRE(model.extensionsUsed.size() == 1);
  REQUIRE(model.extensionsUsed[0].compare("VENDOR_material_some_ext") == 0);

  REQUIRE(model.materials.size() == 1);
  REQUIRE(model.materials[0].extensions.size() == 1);
  REQUIRE(model.materials[0].extensions.count("VENDOR_material_some_ext") == 1);

  // TODO(syoyo): create temp directory.
  {
    ret = ctx.WriteGltfSceneToFile(&model, "issue-97.gltf", true, true);
    REQUIRE(true == ret);

    tinygltf::Model m;

    // read back serialized glTF
    bool ret = ctx.LoadASCIIFromFile(&m, &err, &warn, "issue-97.gltf");
    if (!err.empty()) {
      std::cerr << err << std::endl;
    }
    REQUIRE(true == ret);

    REQUIRE(m.extensionsUsed.size() == 1);
    REQUIRE(m.extensionsUsed[0].compare("VENDOR_material_some_ext") == 0);

    REQUIRE(m.materials.size() == 1);
    REQUIRE(m.materials[0].extensions.size() == 1);
    REQUIRE(m.materials[0].extensions.count("VENDOR_material_some_ext") == 1);
  }

}

TEST_CASE("invalid-primitive-indices", "[bounds-checking]") {
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  // Loading is expected to fail, but not crash.
  bool ret = ctx.LoadASCIIFromFile(
      &model, &err, &warn,
      "../models/BoundsChecking/invalid-primitive-indices.gltf");
  REQUIRE_THAT(err,
               Catch::Contains("primitive indices accessor out of bounds"));
  REQUIRE_FALSE(ret);
}

TEST_CASE("invalid-buffer-view-index", "[bounds-checking]") {
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  // Loading is expected to fail, but not crash.
  bool ret = ctx.LoadASCIIFromFile(
      &model, &err, &warn,
      "../models/BoundsChecking/invalid-buffer-view-index.gltf");
  REQUIRE_THAT(err, Catch::Contains("accessor[0] invalid bufferView"));
  REQUIRE_FALSE(ret);
}

TEST_CASE("invalid-buffer-index", "[bounds-checking]") {
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  // Loading is expected to fail, but not crash.
  bool ret = ctx.LoadASCIIFromFile(
      &model, &err, &warn,
      "../models/BoundsChecking/invalid-buffer-index.gltf");
  REQUIRE_THAT(
      err, Catch::Contains("image[0] buffer \"1\" not found in the scene."));
  REQUIRE_FALSE(ret);
}

TEST_CASE("glb-invalid-length", "[bounds-checking]") {
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  // This glb has a much longer length than the provided data and should fail
  // initial range checks.
  const unsigned char glb_invalid_length[] = "glTF"
      "\x20\x00\x00\x00" "\x6c\x66\x00\x00"     //
  //  |     version     |     length      |
      "\x02\x00\x00\x00" "\x4a\x53\x4f\x4e{}";  //
  //  |  model length   |   model format  |

  bool ret = ctx.LoadBinaryFromMemory(&model, &err, &warn, glb_invalid_length,
                                      sizeof(glb_invalid_length));
  REQUIRE_THAT(err, Catch::Contains("Invalid glTF binary."));
  REQUIRE_FALSE(ret);
}

TEST_CASE("integer-out-of-bounds", "[bounds-checking]") {
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  // Loading is expected to fail, but not crash.
  bool ret = ctx.LoadASCIIFromFile(
      &model, &err, &warn,
      "../models/BoundsChecking/integer-out-of-bounds.gltf");
  REQUIRE_THAT(err, Catch::Contains("not a positive integer"));
  REQUIRE_FALSE(ret);
}

TEST_CASE("parse-integer", "[bounds-checking]") {
  SECTION("parses valid numbers") {
    std::string err;
    int result = 123;
    CHECK(tinygltf::ParseIntegerProperty(&result, &err, {{"zero", 0}}, "zero",
                                         true));
    REQUIRE(err == "");
    REQUIRE(result == 0);

    CHECK(tinygltf::ParseIntegerProperty(&result, &err, {{"int", -1234}}, "int",
                                         true));
    REQUIRE(err == "");
    REQUIRE(result == -1234);
  }

  SECTION("detects missing properties") {
    std::string err;
    int result = -1;
    CHECK_FALSE(tinygltf::ParseIntegerProperty(&result, &err, {}, "int", true));
    REQUIRE_THAT(err, Catch::Contains("'int' property is missing"));
    REQUIRE(result == -1);
  }

  SECTION("handled missing but not required properties") {
    std::string err;
    int result = -1;
    CHECK_FALSE(
        tinygltf::ParseIntegerProperty(&result, &err, {}, "int", false));
    REQUIRE(err == "");
    REQUIRE(result == -1);
  }

  SECTION("invalid integers") {
    std::string err;
    int result = -1;

    CHECK_FALSE(tinygltf::ParseIntegerProperty(&result, &err, {{"int", 0.5}},
                                               "int", true));
    REQUIRE_THAT(err, Catch::Contains("not an integer type"));

    // Excessively large values and NaN aren't allowed either.
    err.clear();
    CHECK_FALSE(tinygltf::ParseIntegerProperty(&result, &err, {{"int", 1e300}},
                                               "int", true));
    REQUIRE_THAT(err, Catch::Contains("not an integer type"));

    err.clear();
    CHECK_FALSE(tinygltf::ParseIntegerProperty(
        &result, &err, {{"int", std::numeric_limits<double>::quiet_NaN()}},
        "int", true));
    REQUIRE_THAT(err, Catch::Contains("not an integer type"));
  }
}

TEST_CASE("parse-unsigned", "[bounds-checking]") {
  SECTION("parses valid unsigned integers") {
    // Use string-based parsing here, using the initializer list syntax doesn't
    // parse 0 as unsigned.
    json zero_obj = json::parse("{\"zero\": 0}");

    std::string err;
    size_t result = 123;
    CHECK(
        tinygltf::ParseUnsignedProperty(&result, &err, zero_obj, "zero", true));
    REQUIRE(err == "");
    REQUIRE(result == 0);
  }

  SECTION("invalid integers") {
    std::string err;
    size_t result = -1;

    CHECK_FALSE(tinygltf::ParseUnsignedProperty(&result, &err, {{"int", -1234}},
                                                "int", true));
    REQUIRE_THAT(err, Catch::Contains("not a positive integer"));

    err.clear();
    CHECK_FALSE(tinygltf::ParseUnsignedProperty(&result, &err, {{"int", 0.5}},
                                                "int", true));
    REQUIRE_THAT(err, Catch::Contains("not a positive integer"));

    // Excessively large values and NaN aren't allowed either.
    err.clear();
    CHECK_FALSE(tinygltf::ParseUnsignedProperty(&result, &err, {{"int", 1e300}},
                                                "int", true));
    REQUIRE_THAT(err, Catch::Contains("not a positive integer"));

    err.clear();
    CHECK_FALSE(tinygltf::ParseUnsignedProperty(
        &result, &err, {{"int", std::numeric_limits<double>::quiet_NaN()}},
        "int", true));
    REQUIRE_THAT(err, Catch::Contains("not a positive integer"));
  }
}

TEST_CASE("parse-integer-array", "[bounds-checking]") {
  SECTION("parses valid integers") {
    std::string err;
    std::vector<int> result;
    CHECK(tinygltf::ParseIntegerArrayProperty(&result, &err,
                                              {{"x", {-1, 2, 3}}}, "x", true));
    REQUIRE(err == "");
    REQUIRE(result.size() == 3);
    REQUIRE(result[0] == -1);
    REQUIRE(result[1] == 2);
    REQUIRE(result[2] == 3);
  }

  SECTION("invalid integers") {
    std::string err;
    std::vector<int> result;
    CHECK_FALSE(tinygltf::ParseIntegerArrayProperty(
        &result, &err, {{"x", {-1, 1e300, 3}}}, "x", true));
    REQUIRE_THAT(err, Catch::Contains("not an integer type"));
  }
}
