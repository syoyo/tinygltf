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
  bool use_exr = false; // Use EXR for 16bit image?

 public:
  texture_dumper(const tinygltf::Model& inputModel);
  void dump_to_folder(const std::string& path = "./");
  void set_output_format(texture_output_format format);
  void set_use_exr(const bool value) {
    use_exr = value;
  }

  static texture_output_format get_fromat_from_string(const std::string& str);
};
}  // namespace gltfutil
