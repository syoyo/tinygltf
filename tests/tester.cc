#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
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

  bool ret = ctx.LoadASCIIFromString(&model, &err, "bora", strlen("bora"), /* basedir*/ "");

  REQUIRE(false == ret);

}

