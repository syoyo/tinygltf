#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <string>

#include "texture_dumper.h"

namespace gltfutil {

enum class ui_mode { cli, interactive };
enum class cli_action { not_set, help, dump };
enum class FileType { Ascii, Binary, Unknown };

/// Probe inside the file, or check the extension to determine if we have to
/// load a text file, or a binary file
FileType detectType(const std::string& path) {
  // Quickly open the file as binary and chekc if there's the gltf binary magic
  // number
  {
    auto probe = std::ifstream(path, std::ios_base::binary);
    if (!probe) throw std::runtime_error("Could not open " + path);

    std::array<char, 5> buffer;
    for (size_t i{0}; i < 4; ++i) probe >> buffer[i];
    buffer[4] = 0;

    if (std::string("glTF") == std::string(buffer.data())) {
      std::cout
          << "Detected binary file thanks to the magic number at the start!\n";
      return FileType::Binary;
    }
  }

  // If we don't have any better, check the file extension.
  auto extension = path.substr(path.find_last_of('.') + 1);
  std::transform(std::begin(extension), std::end(extension),
                 std::begin(extension),
                 [](char c) { return char(::tolower(int(c))); });
  if (extension == "gltf") return FileType::Ascii;
  if (extension == "glb") return FileType::Binary;

  return FileType::Unknown;
}

struct configuration {
  std::string input_path, output_dir;
  ui_mode mode;
  cli_action action = cli_action::not_set;
  texture_dumper::texture_output_format requested_format =
      texture_dumper::texture_output_format::not_specified;
  bool use_exr = false;

  bool has_output_dir;
  bool is_valid() {
    // TODO impl check
    return true;
  }
};

}  // namespace gltfutil
