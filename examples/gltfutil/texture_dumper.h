#pragma once

#include <string>

#include <tiny_gltf.h>

namespace gltfutil {
class texture_dumper {
 public:
  enum class texture_output_format { png, bmp, tga };

 private:
  const tinygltf::Model& model;
  texture_output_format configured_format;

 public:
  texture_dumper(const tinygltf::Model& inputModel);
  void dump_to_folder(const std::string& path = "./");
};
}  // namespace gltfutil
