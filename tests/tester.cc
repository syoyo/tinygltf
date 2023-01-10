#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

// Nlohmann json(include ../json.hpp)
#include "json.hpp"

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>

static JsonDocument JsonConstruct(const char* str)
{
  JsonDocument doc;
  JsonParse(doc, str, strlen(str));
  return doc;
}


TEST_CASE("parse-error", "[parse]") {

  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  bool ret = ctx.LoadASCIIFromString(&model, &err, &warn, "bora", static_cast<int>(strlen("bora")), /* basedir*/ "");

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

TEST_CASE("extension-overwrite", "[issue-261]") {

  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  bool ret = ctx.LoadASCIIFromFile(&model, &err, &warn, "../models/Extensions-overwrite-issue261/issue-261.gltf");
  if (!err.empty()) {
    std::cerr << err << std::endl;
  }
  REQUIRE(true == ret);

  REQUIRE(model.extensionsUsed.size() == 3);
  {
    bool has_ext_lights = false;
    has_ext_lights |= (model.extensionsUsed[0].compare("KHR_lights_punctual") == 0);
    has_ext_lights |= (model.extensionsUsed[1].compare("KHR_lights_punctual") == 0);
    has_ext_lights |= (model.extensionsUsed[2].compare("KHR_lights_punctual") == 0);

    REQUIRE(true == has_ext_lights);
  }

  {
    REQUIRE(model.extensions.size() == 2);
    REQUIRE(model.extensions.count("NV_MDL"));
    REQUIRE(model.extensions.count("KHR_lights_punctual"));
  }

  // TODO(syoyo): create temp directory.
  {
    ret = ctx.WriteGltfSceneToFile(&model, "issue-261.gltf", true, true);
    REQUIRE(true == ret);

    tinygltf::Model m;

    // read back serialized glTF
    bool ret = ctx.LoadASCIIFromFile(&m, &err, &warn, "issue-261.gltf");
    if (!err.empty()) {
      std::cerr << err << std::endl;
    }
    REQUIRE(true == ret);

    REQUIRE(m.extensionsUsed.size() == 3);

    REQUIRE(m.extensions.size() == 2);
    REQUIRE(m.extensions.count("NV_MDL"));
    REQUIRE(m.extensions.count("KHR_lights_punctual"));

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
    CHECK(tinygltf::ParseIntegerProperty(&result, &err, JsonConstruct("{\"zero\" : 0}"), "zero",
                                         true));
    REQUIRE(err == "");
    REQUIRE(result == 0);

    CHECK(tinygltf::ParseIntegerProperty(&result, &err, JsonConstruct("{\"int\": -1234}"), "int",
                                         true));
    REQUIRE(err == "");
    REQUIRE(result == -1234);
  }

  SECTION("detects missing properties") {
    std::string err;
    int result = -1;
    CHECK_FALSE(tinygltf::ParseIntegerProperty(&result, &err, JsonConstruct(""), "int", true));
    REQUIRE_THAT(err, Catch::Contains("'int' property is missing"));
    REQUIRE(result == -1);
  }

  SECTION("handled missing but not required properties") {
    std::string err;
    int result = -1;
    CHECK_FALSE(
        tinygltf::ParseIntegerProperty(&result, &err, JsonConstruct(""), "int", false));
    REQUIRE(err == "");
    REQUIRE(result == -1);
  }

  SECTION("invalid integers") {
    std::string err;
    int result = -1;

    CHECK_FALSE(tinygltf::ParseIntegerProperty(&result, &err, JsonConstruct("{\"int\": 0.5}"),
      "int", true));
    REQUIRE_THAT(err, Catch::Contains("not an integer type"));

    // Excessively large values and NaN aren't allowed either.
    err.clear();
    CHECK_FALSE(tinygltf::ParseIntegerProperty(&result, &err, JsonConstruct("{\"int\": 1e300}"),
      "int", true));
    REQUIRE_THAT(err, Catch::Contains("not an integer type"));

    err.clear();
    {
      JsonDocument o;
      double nan = std::numeric_limits<double>::quiet_NaN();
      tinygltf::JsonAddMember(o, "int", json(nan));
      CHECK_FALSE(tinygltf::ParseIntegerProperty(
        &result, &err, o,
        "int", true));
      REQUIRE_THAT(err, Catch::Contains("not an integer type"));
    }
  }
}

TEST_CASE("parse-unsigned", "[bounds-checking]") {
  SECTION("parses valid unsigned integers") {
    // Use string-based parsing here, using the initializer list syntax doesn't
    // parse 0 as unsigned.
    auto zero_obj = JsonConstruct("{\"zero\": 0}");

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

    CHECK_FALSE(tinygltf::ParseUnsignedProperty(&result, &err, JsonConstruct("{\"int\": -1234}"),
                                                "int", true));
    REQUIRE_THAT(err, Catch::Contains("not a positive integer"));

    err.clear();
    CHECK_FALSE(tinygltf::ParseUnsignedProperty(&result, &err, JsonConstruct("{\"int\": 0.5}"),
                                                "int", true));
    REQUIRE_THAT(err, Catch::Contains("not a positive integer"));

    // Excessively large values and NaN aren't allowed either.
    err.clear();
    CHECK_FALSE(tinygltf::ParseUnsignedProperty(&result, &err, JsonConstruct("{\"int\": 1e300}"),
                                                "int", true));
    REQUIRE_THAT(err, Catch::Contains("not a positive integer"));

    err.clear();
    {
      JsonDocument o;
      double nan = std::numeric_limits<double>::quiet_NaN();
      tinygltf::JsonAddMember(o, "int", json(nan));
      CHECK_FALSE(tinygltf::ParseUnsignedProperty(
        &result, &err, o,
        "int", true));
      REQUIRE_THAT(err, Catch::Contains("not a positive integer"));
    }
  }
}

TEST_CASE("parse-integer-array", "[bounds-checking]") {
  SECTION("parses valid integers") {
    std::string err;
    std::vector<int> result;
    CHECK(tinygltf::ParseIntegerArrayProperty(&result, &err,
                                              JsonConstruct("{\"x\": [-1, 2, 3]}"), "x", true));
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
        &result, &err, JsonConstruct("{\"x\": [-1, 1e300, 3]}"), "x", true));
    REQUIRE_THAT(err, Catch::Contains("not an integer type"));
  }
}

TEST_CASE("pbr-khr-texture-transform", "[material]") {
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  // Loading is expected to fail, but not crash.
  bool ret = ctx.LoadASCIIFromFile(
      &model, &err, &warn,
      "../models/Cube-texture-ext/Cube-textransform.gltf");
  REQUIRE(ret == true);

  REQUIRE(model.materials.size() == 2);
  REQUIRE(model.materials[0].emissiveTexture.extensions.count("KHR_texture_transform") == 1);
  REQUIRE(model.materials[0].emissiveTexture.extensions["KHR_texture_transform"].IsObject());

  tinygltf::Value::Object &texform = model.materials[0].emissiveTexture.extensions["KHR_texture_transform"].Get<tinygltf::Value::Object>();

  REQUIRE(texform.count("scale"));

  REQUIRE(texform["scale"].IsArray());

  // Note: It looks json.hpp parse integer JSON number as integer, not floating point.
  // IsNumber return true either value is int or floating point.
  REQUIRE(texform["scale"].Get(0).IsNumber());
  REQUIRE(texform["scale"].Get(1).IsNumber());

  double scale[2];
  scale[0] = texform["scale"].Get(0).GetNumberAsDouble();
  scale[1] = texform["scale"].Get(1).GetNumberAsDouble();

  REQUIRE(scale[0] == Approx(1.0));
  REQUIRE(scale[1] == Approx(-1.0));

}

TEST_CASE("image-uri-spaces", "[issue-236]") {

  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  // Test image file with single spaces.
  {
    tinygltf::Model model;
    bool ret = ctx.LoadASCIIFromFile(
        &model, &err, &warn,
        "../models/CubeImageUriSpaces/CubeImageUriSpaces.gltf");
    if (!warn.empty()) {
      std::cerr << warn << std::endl;
    }
    if (!err.empty()) {
      std::cerr << err << std::endl;
    }

    REQUIRE(true == ret);
    REQUIRE(warn.empty());
    REQUIRE(err.empty());
    REQUIRE(model.images.size() == 1);
    REQUIRE(model.images[0].uri.find(' ') != std::string::npos);
  }

  // Test image file with a beginning space, trailing space, and greater than
  // one consecutive spaces.
  tinygltf::Model model;
  bool ret = ctx.LoadASCIIFromFile(
      &model, &err, &warn,
      "../models/CubeImageUriSpaces/CubeImageUriMultipleSpaces.gltf");
  if (!warn.empty()) {
    std::cerr << warn << std::endl;
  }
  if (!err.empty()) {
    std::cerr << err << std::endl;
  }

  REQUIRE(true == ret);
  REQUIRE(warn.empty());
  REQUIRE(err.empty());
  REQUIRE(model.images.size() == 1);
  REQUIRE(model.images[0].uri.size() > 1);
  REQUIRE(model.images[0].uri[0] == ' ');

  // Test the URI encoding API by saving and re-load the file, without embedding
  // the image.
  // TODO(syoyo): create temp directory.
  {
    // Encoder that only replaces spaces with "%20".
    auto uriencode = [](const std::string &in_uri,
                        const std::string &object_type, std::string *out_uri,
                        void *user_data) -> bool {
      (void)user_data;
      bool imageOrBuffer = object_type == "image" || object_type == "buffer";
      REQUIRE(true == imageOrBuffer);
      *out_uri = {};
      for (char c : in_uri) {
        if (c == ' ')
          *out_uri += "%20";
        else
          *out_uri += c;
      }
      return true;
    };

    // Remove the buffer URI, so a new one is generated based on the gltf
    // filename and then encoded with the above callback.
    model.buffers[0].uri.clear();

    tinygltf::URICallbacks uri_cb{uriencode, tinygltf::URIDecode, nullptr};
    ctx.SetURICallbacks(uri_cb);
    ret = ctx.WriteGltfSceneToFile(&model, " issue-236.gltf", false, false);
    REQUIRE(true == ret);

    // read back serialized glTF
    tinygltf::Model saved;
    bool ret = ctx.LoadASCIIFromFile(&saved, &err, &warn, " issue-236.gltf");
    if (!err.empty()) {
      std::cerr << err << std::endl;
    }
    REQUIRE(true == ret);
    REQUIRE(err.empty());
    REQUIRE(!warn.empty());  // relative image path won't exist in tests/
    REQUIRE(saved.images.size() == model.images.size());

    // The image uri in CubeImageUriMultipleSpaces.gltf is not encoded and
    // should be different after encoding spaces with %20.
    REQUIRE(model.images[0].uri != saved.images[0].uri);

    // Verify the image path remains the same after uri decoding
    std::string image_uri, image_uri_saved;
    (void)tinygltf::URIDecode(model.images[0].uri, &image_uri, nullptr);
    (void)tinygltf::URIDecode(saved.images[0].uri, &image_uri_saved, nullptr);
    REQUIRE(image_uri == image_uri_saved);

    // Verify the buffer's generated and encoded URI
    REQUIRE(saved.buffers.size() == model.buffers.size());
    REQUIRE(saved.buffers[0].uri == "%20issue-236.bin");
  }
}

TEST_CASE("serialize-empty-material", "[issue-294]") {

  tinygltf::Model m;

  tinygltf::Material mat;
  mat.pbrMetallicRoughness.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f}; // default baseColorFactor
  m.materials.push_back(mat);

  std::stringstream os;

  tinygltf::TinyGLTF ctx;
  ctx.WriteGltfSceneToStream(&m, os, false, false);

  // use nlohmann json
  nlohmann::json j = nlohmann::json::parse(os.str());

  REQUIRE(1 == j["materials"].size());
  REQUIRE(j["materials"][0].is_object());

}

TEST_CASE("empty-skeleton-id", "[issue-321]") {

  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  bool ret = ctx.LoadASCIIFromFile(&model, &err, &warn, "../models/regression/unassigned-skeleton.gltf");
  if (!err.empty()) {
    std::cerr << err << std::endl;
  }
  REQUIRE(true == ret);

  REQUIRE(model.skins.size() == 1);
  REQUIRE(model.skins[0].skeleton == -1); // unassigned

  std::stringstream os;

  ctx.WriteGltfSceneToStream(&model, os, false, false);

  // use nlohmann json
  nlohmann::json j = nlohmann::json::parse(os.str());

  // Ensure `skeleton` property is not written to .gltf(was serialized as -1)
  REQUIRE(1 == j["skins"].size());
  REQUIRE(j["skins"][0].is_object());
  REQUIRE(j["skins"][0].count("skeleton") == 0);

}

#ifndef TINYGLTF_NO_FS
TEST_CASE("expandpath-utf-8", "[pr-226]") {

  std::string s1 = "\xe5\xaf\xb9"; // utf-8 string

  std::string ret = tinygltf::ExpandFilePath(s1, /* userdata */nullptr);

  // expected: E5 AF B9
  REQUIRE(3 == ret.size());

  REQUIRE(0xe5 == static_cast<uint8_t>(ret[0]));
  REQUIRE(0xaf == static_cast<uint8_t>(ret[1]));
  REQUIRE(0xb9 == static_cast<uint8_t>(ret[2]));

}
#endif

TEST_CASE("empty-bin-buffer", "[issue-382]") {
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  tinygltf::Model model_empty;
  std::stringstream stream;
  bool ret = ctx.WriteGltfSceneToStream(&model_empty, stream, false, true);
  REQUIRE(ret == true);
  std::string str = stream.str();
  const unsigned char* bytes = (unsigned char*)str.data();
  ret = ctx.LoadBinaryFromMemory(&model, &err, &warn, bytes, str.size());
  if (!err.empty()) {
    std::cerr << err << std::endl;
  }
  REQUIRE(true == ret);

  err.clear();
  warn.clear();

  tinygltf::Model model_empty_buffer;
  model_empty_buffer.buffers.push_back(tinygltf::Buffer());
  stream = std::stringstream();
  ret = ctx.WriteGltfSceneToStream(&model_empty_buffer, stream, false, true);
  REQUIRE(ret == true);
  str = stream.str();
  bytes = (unsigned char*)str.data();
  ret = ctx.LoadBinaryFromMemory(&model, &err, &warn, bytes, str.size());
  if (err.empty()) {
    std::cerr << "there should have been an error reported" << std::endl;
  }
  REQUIRE(false == ret);

  err.clear();
  warn.clear();

  tinygltf::Model model_single_byte_buffer;
  tinygltf::Buffer buffer;
  buffer.data.push_back(0);
  model_single_byte_buffer.buffers.push_back(buffer);
  stream = std::stringstream();
  ret = ctx.WriteGltfSceneToStream(&model_single_byte_buffer, stream, false, true);
  REQUIRE(ret == true);
  str = stream.str();
  {
    std::ofstream ofs("tmp.glb");
    ofs.write(str.data(), str.size());
  }

  bytes = (unsigned char*)str.data();
  ret = ctx.LoadBinaryFromMemory(&model_single_byte_buffer, &err, &warn, bytes, str.size());
  if (!err.empty()) {
    std::cerr << err << std::endl;
  }
  REQUIRE(true == ret);
}

TEST_CASE("serialize-const-image", "[issue-394]") {
  tinygltf::Model m;
  tinygltf::Image i;
  i.width = 1;
  i.height = 1;
  i.component = 4;
  i.bits = 8;
  i.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
  i.image = {255, 255, 255, 255};
  i.uri = "image.png";
  m.images.push_back(i);

  std::stringstream os;

  tinygltf::TinyGLTF ctx;
  ctx.WriteGltfSceneToStream(const_cast<const tinygltf::Model *>(&m), os, false,
                             false);
  REQUIRE(m.images[0].uri == i.uri);

  // use nlohmann json
  nlohmann::json j = nlohmann::json::parse(os.str());

  REQUIRE(1 == j["images"].size());
  REQUIRE(j["images"][0].is_object());
  REQUIRE(j["images"][0]["uri"].get<std::string>() != i.uri);
  REQUIRE(j["images"][0]["uri"].get<std::string>().rfind("data:", 0) == 0);
}

TEST_CASE("serialize-image-callback", "[issue-394]") {
  tinygltf::Model m;
  tinygltf::Image i;
  i.width = 1;
  i.height = 1;
  i.bits = 32;
  i.image = {255, 255, 255, 255};
  i.uri = "foo";
  m.images.push_back(i);

  std::stringstream os;

  auto writer = [](const std::string *basepath, const std::string *filename,
                   const tinygltf::Image *image, bool embedImages,
                   const tinygltf::URICallbacks *uri_cb, std::string *out_uri,
                   void *user_pointer) -> bool {
    (void)basepath;
    (void)image;
    (void)uri_cb;
    REQUIRE(*filename == "foo");
    REQUIRE(embedImages == true);
    REQUIRE(user_pointer == (void *)0xba5e1e55);
    *out_uri = "bar";
    return true;
  };

  tinygltf::TinyGLTF ctx;
  ctx.SetImageWriter(writer, (void *)0xba5e1e55);
  bool result = ctx.WriteGltfSceneToStream(const_cast<const tinygltf::Model *>(&m), os, false,
                             false);

  // use nlohmann json
  nlohmann::json j = nlohmann::json::parse(os.str());

  REQUIRE(true == result);
  REQUIRE(1 == j["images"].size());
  REQUIRE(j["images"][0].is_object());
  REQUIRE(j["images"][0]["uri"].get<std::string>() == "bar");
}

TEST_CASE("serialize-image-failure", "[issue-394]") {
  tinygltf::Model m;
  tinygltf::Image i;
  // Set some data so the ImageWriter callback will be called
  i.image = {255, 255, 255, 255};
  m.images.push_back(i);

  std::stringstream os;

  auto writer = [](const std::string *basepath, const std::string *filename,
                   const tinygltf::Image *image, bool embedImages,
                   const tinygltf::URICallbacks *uri_cb, std::string *out_uri,
                   void *user_pointer) -> bool {
    (void)basepath;
    (void)filename;
    (void)image;
    (void)embedImages;
    (void)uri_cb;
    (void)out_uri;
    (void)user_pointer;
    return false;
  };

  tinygltf::TinyGLTF ctx;
  ctx.SetImageWriter(writer, (void *)0xba5e1e55);
  bool result = ctx.WriteGltfSceneToStream(const_cast<const tinygltf::Model *>(&m), os, false,
                             false);

  REQUIRE(false == result);
  REQUIRE(os.str().size() == 0);
}
