#pragma once

#include <string>

#include <tiny_gltf.h>

namespace gltfutil {
class texture_dumper {
 public:
  enum class texture_output_format { png, bmp, tga, not_specified };

 private:
  const tinygltf::Model& model;
  texture_output_format configured_format;

 public:
  texture_dumper(const tinygltf::Model& inputModel);
  void dump_to_folder(const std::string& path = "./");
  void set_output_format(texture_output_format format);

  static texture_output_format get_fromat_from_string(const std::string& str);
};
}  // namespace gltfutil
