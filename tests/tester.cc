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


