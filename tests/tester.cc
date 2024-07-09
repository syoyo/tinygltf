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

static tinygltf::detail::JsonDocument JsonConstruct(const char* str)
{
  tinygltf::detail::JsonDocument doc;
  tinygltf::detail::JsonParse(doc, str, strlen(str));
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
      tinygltf::detail::JsonDocument o;
      double nan = std::numeric_limits<double>::quiet_NaN();
      tinygltf::detail::JsonAddMember(o, "int", tinygltf::detail::json(nan));
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
      tinygltf::detail::JsonDocument o;
      double nan = std::numeric_limits<double>::quiet_NaN();
      tinygltf::detail::JsonAddMember(o, "int", tinygltf::detail::json(nan));
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
    REQUIRE(warn.empty());
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
  // Add default constructed material to model
  m.materials.push_back({});
  // Serialize model to output stream
  std::stringstream os;
  tinygltf::TinyGLTF ctx;
  bool ret = ctx.WriteGltfSceneToStream(&m, os, false, false);
  REQUIRE(true == ret);
  // Parse serialized model
  nlohmann::json j = nlohmann::json::parse(os.str());
  // Serialized materials shall hold an empty object that
  // represents the default constructed material
  REQUIRE(j.find("materials") != j.end());
  REQUIRE(j["materials"].is_array());
  REQUIRE(1 == j["materials"].size());
  CHECK(j["materials"][0].is_object());
  CHECK(j["materials"][0].empty());
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

  ret = ctx.WriteGltfSceneToStream(&model, os, false, false);
  REQUIRE(true == ret);

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
  bool ret = ctx.WriteGltfSceneToStream(const_cast<const tinygltf::Model *>(&m), os, false,
                             false);
  REQUIRE(true == ret);
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
                   const tinygltf::FsCallbacks* fs, const tinygltf::URICallbacks *uri_cb,
                   std::string *out_uri, void *user_pointer) -> bool {
    (void)basepath;
    (void)image;
    (void)fs;
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
                   const tinygltf::FsCallbacks* fs, const tinygltf::URICallbacks *uri_cb,
                   std::string *out_uri, void *user_pointer) -> bool {
    (void)basepath;
    (void)filename;
    (void)image;
    (void)embedImages;
    (void)fs;
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

TEST_CASE("filesize-check", "[issue-416]") {

  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  ctx.SetMaxExternalFileSize(10); // 10 bytes. will fail to load texture image.

  bool ret = ctx.LoadASCIIFromFile(&model, &err, &warn, "../models/Cube/Cube.gltf");
  if (!err.empty()) {
    std::cerr << err << std::endl;
  }

  REQUIRE(false == ret);
}

TEST_CASE("load-issue-416-model", "[issue-416]") {

  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  bool ret = ctx.LoadASCIIFromFile(&model, &err, &warn, "issue-416.gltf");
  if (!warn.empty()) {
    std::cout << "WARN:" << warn << std::endl;
  }
  if (!err.empty()) {
    std::cerr << "ERR:" << err << std::endl;
  }

  // external file load fails, but reading glTF itself is ok.
  REQUIRE(true == ret);
}

TEST_CASE("serialize-empty-node", "[issue-457]") {
  tinygltf::Model m;
  // Add default constructed node to model
  m.nodes.push_back({});
  // Add scene to model
  m.scenes.push_back({});
  // The scene's only node is the empty node
  m.scenes.front().nodes.push_back(0);

  // Serialize model to output stream
  std::stringstream os;
  tinygltf::TinyGLTF ctx;
  bool ret = ctx.WriteGltfSceneToStream(&m, os, false, false);
  REQUIRE(true == ret);

  // Parse serialized model
  nlohmann::json j = nlohmann::json::parse(os.str());

  // Serialized nodes shall hold an empty object that
  // represents the default constructed node
  REQUIRE(j.find("nodes") != j.end());
  REQUIRE(j["nodes"].is_array());
  REQUIRE(1 == j["nodes"].size());
  CHECK(j["nodes"][0].is_object());
  CHECK(j["nodes"][0].empty());

  // We also want to make sure that the serialized scene
  // is referencing the empty node.

  // There shall be a single serialized scene
  auto scenes = j.find("scenes");
  REQUIRE(scenes != j.end());
  REQUIRE(scenes->is_array());
  REQUIRE(1 == scenes->size());
  auto scene = scenes->at(0);
  REQUIRE(scene.is_object());
  // The scene's nodes array shall hold a reference
  // to the single node
  auto nodes = scene.find("nodes");
  REQUIRE(nodes != scene.end());
  REQUIRE(nodes->is_array());
  REQUIRE(1 == nodes->size());
  auto node = nodes->at(0);
  CHECK(node.is_number_integer());
  int idx = -1;
  node.get_to(idx);
  CHECK(0 == idx);
}

TEST_CASE("serialize-light-index", "[issue-458]") {

  // Create the light
  tinygltf::Light light;
  light.type = "point";
  light.intensity = 0.75;
  light.color = std::vector<double>{1.0, 0.8, 0.95};

  // Stream to serialize to
  std::stringstream os;

  {
    tinygltf::Model m;
    tinygltf::Scene scene;
    // Add the light to the model
    m.lights.push_back(light);
    // Create a node that uses the light
    tinygltf::Node node;
    node.light = 0;
    // Add the node to the model
    m.nodes.push_back(node);
    // Add the node to the scene
    scene.nodes.push_back(0);
    // Add the scene to the model
    m.scenes.push_back(scene);
    // Serialize model to output stream
    tinygltf::TinyGLTF ctx;
    bool ret = ctx.WriteGltfSceneToStream(&m, os, false, false);
    REQUIRE(true == ret);
  }

  {
    tinygltf::Model m;
    tinygltf::TinyGLTF ctx;
    // Parse the serialized model
    bool ok = ctx.LoadASCIIFromString(&m, nullptr, nullptr, os.str().c_str(), os.str().size(), "");
    REQUIRE(true == ok);
    // Check if the light was correctly serialized
    REQUIRE(1 == m.lights.size());
    CHECK(m.lights[0] == light);
    // Check that the node properly references the light
    REQUIRE(1 == m.nodes.size());
    CHECK(m.nodes[0].light == 0);
  }
}

TEST_CASE("default-material", "[issue-459]") {
  const std::vector<double> default_emissive_factor{ 0.0, 0.0, 0.0 };
  const std::vector<double> default_base_color_factor{ 1.0, 1.0, 1.0, 1.0 };
  const std::string default_alpha_mode = "OPAQUE";
  const double default_alpha_cutoff = 0.5;
  const bool default_double_sided = false;
  const double default_metallic_factor = 1.0;
  const double default_roughness_factor = 1.0;
  // Check that default constructed material
  // holds actual default GLTF material properties
  tinygltf::Material mat;
  CHECK(mat.alphaMode == default_alpha_mode);
  CHECK(mat.alphaCutoff == default_alpha_cutoff);
  CHECK(mat.doubleSided == default_double_sided);
  CHECK(mat.emissiveFactor == default_emissive_factor);
  CHECK(mat.pbrMetallicRoughness.baseColorFactor == default_base_color_factor);
  CHECK(mat.pbrMetallicRoughness.metallicFactor == default_metallic_factor);
  CHECK(mat.pbrMetallicRoughness.roughnessFactor == default_roughness_factor);
  // None of the textures should be set
  CHECK(mat.normalTexture.index == -1);
  CHECK(mat.occlusionTexture.index == -1);
  CHECK(mat.emissiveTexture.index == -1);
}

TEST_CASE("serialize-empty-scene", "[issue-464]") {
  // Stream to serialize to
  std::stringstream os;

  {
    tinygltf::Model m;
    // Add empty scene to the model
    m.scenes.push_back({});
    // Serialize model to output stream
    tinygltf::TinyGLTF ctx;
    bool ret = ctx.WriteGltfSceneToStream(&m, os, false, false);
    REQUIRE(true == ret);
  }

  {
    tinygltf::Model m;
    tinygltf::TinyGLTF ctx;
    // Parse the serialized model
    bool ok = ctx.LoadASCIIFromString(&m, nullptr, nullptr, os.str().c_str(), os.str().size(), "");
    REQUIRE(true == ok);
    // Make sure the empty scene is there
    REQUIRE(1 == m.scenes.size());
    tinygltf::Scene scene{};
    // Check that the scene is empty
    CHECK(m.scenes[0] == scene);
  }
}

TEST_CASE("zero-sized-bin-chunk-glb", "[issue-440]") {

  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  // Input glb has zero-sized data in bin chunk(8 bytes for BIN chunk, and chunksize == 0)
  // The spec https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#binary-buffer says
  //
  // When the binary buffer is empty or when it is stored by other means, this chunk SHOULD be omitted.
  //
  // 'SHOULD' mean 'RECOMMENDED', so we'll need to allow such zero-sized bin chunk is NOT omitted.
  bool ret = ctx.LoadBinaryFromFile(&model, &err, &warn, "../models/regression/zero-sized-bin-chunk-issue-440.glb");
  if (!warn.empty()) {
    std::cout << "WARN: " << warn << "\n";
  }
  if (!err.empty()) {
    std::cerr << err << std::endl;
  }

  REQUIRE(true == ret);
}

TEST_CASE("serialize-node-emitter", "[KHR_audio]") {
  // Stream to serialize to
  std::stringstream os;

  {
    tinygltf::Model m;
    // Create a default audio emitter
    m.audioEmitters.resize(1);
    // Create a single node
    m.nodes.resize(1);
    // The node references the single emitter
    m.nodes[0].emitter = 0;
    // Create a single scene
    m.scenes.resize(1);
    // Make the scene reference the single node
    m.scenes[0].nodes.push_back(0);

    // Serialize model to output stream
    tinygltf::TinyGLTF ctx;
    bool ret = ctx.WriteGltfSceneToStream(&m, os, false, false);
    REQUIRE(true == ret);
  }

  {
    tinygltf::Model m;
    tinygltf::TinyGLTF ctx;
    // Parse the serialized model
    bool ok = ctx.LoadASCIIFromString(&m, nullptr, nullptr, os.str().c_str(), os.str().size(), "");
    REQUIRE(true == ok);

    // Make sure the single scene is there
    REQUIRE(1 == m.scenes.size());
    // Make sure all three nodes are there
    REQUIRE(1 == m.nodes.size());
    // Make sure the single root node of the scene is there
    REQUIRE(1 == m.scenes[0].nodes.size());
    REQUIRE(0 == m.scenes[0].nodes[0]);
    // Retrieve the scene root node
    const tinygltf::Node& node = m.nodes[m.scenes[0].nodes[0]];
    // Make sure the single root node has both lod nodes
    REQUIRE(0 == node.emitter);
  }
}

TEST_CASE("serialize-lods", "[lods]") {
  // Stream to serialize to
  std::stringstream os;

  {
    tinygltf::Model m;

    m.nodes.resize(4);
    // Add Node 1 and Node 2 as lods to Node 0
    m.nodes[0].lods.push_back(1);
    m.nodes[0].lods.push_back(2);

    // Add Material 1 and Material 2 as lods to Material 0
    m.materials.resize(4);
    m.materials[0].lods.push_back(1);
    m.materials[0].lods.push_back(2);

    tinygltf::Scene scene;
    // Scene uses Node 0 and 3 as root node
    scene.nodes.push_back(0);
    scene.nodes.push_back(3);
    // Add scene to the model
    m.scenes.push_back(scene);

    // Serialize model to output stream
    tinygltf::TinyGLTF ctx;
    bool ret = ctx.WriteGltfSceneToStream(&m, os, false, false);
    REQUIRE(true == ret);
  }

  {
    tinygltf::Model m;
    tinygltf::TinyGLTF ctx;
    // Parse the serialized model
    bool ok = ctx.LoadASCIIFromString(&m, nullptr, nullptr, os.str().c_str(), os.str().size(), "");
    REQUIRE(true == ok);
    // Make sure the model's used extensions hold MSFT_lod
    CHECK(m.extensionsUsed.size() == 1);
    CHECK(m.extensionsUsed[0].compare("MSFT_lod") == 0);
    // MSFT_lod is not a required extension
    CHECK(m.extensionsRequired.size() == 0);

    // Make sure all four materials are there
    REQUIRE(4 == m.materials.size());
    // Make sure the first material has both lod materials
    REQUIRE(2 == m.materials[0].lods.size());
    // Make sure the order is still the same after serialization and deserialization
    CHECK(1 == m.materials[0].lods[0]);
    CHECK(2 == m.materials[0].lods[1]);
    // Make sure the material with lods exposes the MSFT_lod extension
    CHECK(m.materials[0].extensions.size() == 1);
    CHECK(m.materials[0].extensions.count("MSFT_lod") == 1);
    // Make sure the last material has no lod materials
    CHECK(0 == m.materials[3].lods.size());
    // Make sure the material without lods does not exposes the MSFT_lod extension
    CHECK(m.materials[3].extensions.size() == 0);
    CHECK(m.materials[3].extensions.count("MSFT_lod") == 0);

    // Make sure the single scene is there
    REQUIRE(1 == m.scenes.size());
    // Make sure all four nodes are there
    REQUIRE(4 == m.nodes.size());
    // Make sure the two root nodes of the scene are there
    REQUIRE(2 == m.scenes[0].nodes.size());
    REQUIRE(0 == m.scenes[0].nodes[0]);
    REQUIRE(3 == m.scenes[0].nodes[1]);
    // Retrieve the node with lods
    const tinygltf::Node& nodeWithLods = m.nodes[m.scenes[0].nodes[0]];
    // Make sure the node has both lod nodes
    REQUIRE(2 == nodeWithLods.lods.size());
    // Make sure the order is still the same after serialization and deserialization
    CHECK(1 == nodeWithLods.lods[0]);
    CHECK(2 == nodeWithLods.lods[1]);
    // Make sure the node with lods exposes the MSFT_lod extension
    CHECK(nodeWithLods.extensions.size() == 1);
    CHECK(nodeWithLods.extensions.count("MSFT_lod") == 1);
    // Retrieve the node without lods
    const tinygltf::Node& nodeWithoutLods = m.nodes[m.scenes[0].nodes[1]];
    // Make sure the node has no lod nodes
    CHECK(0 == nodeWithoutLods.lods.size());
    // Make sure the node without lods does not exposes the MSFT_lod extension
    CHECK(nodeWithoutLods.extensions.size() == 0);
    CHECK(nodeWithoutLods.extensions.count("MSFT_lod") == 0);
  }
}

TEST_CASE("write-image-issue", "[issue-473]") {
  std::string err;
  std::string warn;
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  bool ok = ctx.LoadASCIIFromFile(&model, &err, &warn, "../models/Cube/Cube.gltf");
  REQUIRE(ok);
  REQUIRE(err.empty());
  REQUIRE(warn.empty());

  REQUIRE(model.images.size() == 2);
  REQUIRE(model.images[0].uri == "Cube_BaseColor.png");
  REQUIRE(model.images[1].uri == "Cube_MetallicRoughness.png");

  REQUIRE_FALSE(model.images[0].image.empty());
  REQUIRE_FALSE(model.images[1].image.empty());

  ok = ctx.WriteGltfSceneToFile(&model, "Cube.gltf");
  REQUIRE(ok);

  for (const auto& image : model.images) {
    std::fstream file(image.uri);
    CHECK(file.good());
  }
}

TEST_CASE("images-as-is", "[issue-487]") {
  std::string err;
  std::string warn;
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  ctx.SetImagesAsIs(true);
  bool ok = ctx.LoadASCIIFromFile(&model, &err, &warn, "../models/Cube/Cube.gltf");
  REQUIRE(ok);
  REQUIRE(err.empty());
  REQUIRE(warn.empty());

  for (const auto& image : model.images) {
    CHECK(image.as_is == true);
    CHECK_FALSE(image.uri.empty());
    CHECK_FALSE(image.image.empty());

#ifndef TINYGLTF_NO_STB_IMAGE
    // Make sure we can decode the images
    int w = -1, h = -1, component = -1;
    unsigned char *data = stbi_load_from_memory(image.image.data(), static_cast<int>(image.image.size()), &w, &h, &component, 0);
    CHECK(data != nullptr);
    CHECK(w == 512);
    CHECK(h == 512);
    CHECK(component >= 3);
    stbi_image_free(data);
#endif
  }

  // Write glTF model to disk, and images as separate files
  {
    ok = ctx.WriteGltfSceneToFile(&model, "Cube_with_image_files.gltf");
    REQUIRE(ok);

    // All the images should have been written to disk with their original data
    for (const auto& image : model.images)  {
      // Make sure the image files exist
      std::fstream file(image.uri);
      CHECK(file.good());
#ifndef TINYGLTF_NO_STB_IMAGE
      // Make sure we can load the images
      int w = -1, h = -1, component = -1;
      unsigned char *data = stbi_load(image.uri.c_str(), &w, &h, &component, 0);
      CHECK(data != nullptr);
      CHECK(w == 512);
      CHECK(h == 512);
      CHECK(component >= 3);
      stbi_image_free(data);
#endif
    }
  }

  // Write glTF model to disk, and embed images as data URIs
  {
    ok = ctx.WriteGltfSceneToFile(&model, "Cube_with_embedded_images.gltf", true, false);
    REQUIRE(ok);

    // Load above model again, and check if the images are loaded properly
    tinygltf::Model embeddedImages;
    ctx.SetImagesAsIs(false);
    bool ok = ctx.LoadASCIIFromFile(&embeddedImages, &err, &warn, "Cube_with_embedded_images.gltf");
    REQUIRE(ok);
    REQUIRE(err.empty());
    REQUIRE(warn.empty());

    for (const auto& image : embeddedImages.images) {
      CHECK(image.as_is == false);
      CHECK_FALSE(image.mimeType.empty());
      CHECK_FALSE(image.image.empty());
      CHECK(image.width == 512);
      CHECK(image.height == 512);
      CHECK(image.component >= 3);
    }
  }

  // Write glTF model to disk, as GLB
  {
    ok = ctx.WriteGltfSceneToFile(&model, "Cube.glb", true, true, true, true);
    REQUIRE(ok);

    // Load above model again, and check if the images are loaded properly
    tinygltf::Model glbModel;
    ctx.SetImagesAsIs(false);
    bool ok = ctx.LoadBinaryFromFile(&glbModel, &err, &warn, "Cube.glb");
    REQUIRE(ok);
    REQUIRE(err.empty());
    REQUIRE(warn.empty());

    for (const auto& image : glbModel.images) {
      CHECK(image.as_is == false);
      CHECK_FALSE(image.mimeType.empty());
      CHECK_FALSE(image.image.empty());
      CHECK(image.width == 512);
      CHECK(image.height == 512);
      CHECK(image.component >= 3);
    }
  }
}

TEST_CASE("inverse-bind-matrices-optional", "[issue-492]") {
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;
  std::string err;
  std::string warn;

  bool ret = ctx.LoadBinaryFromFile(&model, &err, &warn, "issue-492.glb");
  if (!warn.empty()) {
    std::cout << "WARN:" << warn << std::endl;
  }
  if (!err.empty()) {
    std::cerr << "ERR:" << err << std::endl;
  }

  REQUIRE(true == ret);
  REQUIRE(err.empty());
}

bool LoadImageData(tinygltf::Image * /* image */, const int /* image_idx */, std::string * /* err */,
                   std::string * /* warn */, int /* req_width */, int /* req_height */,
                   const unsigned char * /* bytes */, int /* size */, void * /*user_data */) {
    return true;
}

bool WriteImageData(const std::string * /* basepath */, const std::string * /* filename */,
                    const tinygltf::Image *image, bool /* embedImages */,
                    const tinygltf::FsCallbacks * /* fs_cb */, const tinygltf::URICallbacks * /* uri_cb */,
                    std::string * /* out_uri */, void * user_pointer) {
    REQUIRE(user_pointer != nullptr);
    auto counter = static_cast<int*>(user_pointer);
    *counter = *counter + 1;
    return true;
}

TEST_CASE("empty-images-not-written", "[issue-495]") {
  std::string err;
  std::string warn;
  tinygltf::Model model;
  tinygltf::TinyGLTF ctx;

  ctx.SetImageLoader(LoadImageData, nullptr);
  bool ok = ctx.LoadASCIIFromFile(&model, &err, &warn, "../models/Cube/Cube.gltf");
  REQUIRE(ok);
  REQUIRE(err.empty());
  REQUIRE(warn.empty());

  CHECK(model.images.size() == 2);
  for (const auto& image : model.images) {
    // No data loaded or decoded
    CHECK(image.image.empty());
    // The URI is kept
    CHECK_FALSE(image.uri.empty());
    // The URI should not be a data URI
    CHECK(image.uri.find("data:") != 0);
  }

  // Now write the loaded model
  int counter = 0;
  ctx.SetImageWriter(WriteImageData, &counter);
  ok = ctx.WriteGltfSceneToFile(&model, "issue-495-external.gltf");
  CHECK(ok);
  // WriteImageData should be invoked for both images
  CHECK(counter == 2);
}
