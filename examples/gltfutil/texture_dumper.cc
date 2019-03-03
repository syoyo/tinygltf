#include <algorithm>
#include <iostream>

#include "stb_image_write.h"
#include "texture_dumper.h"

#include <tiny_gltf.h>

using namespace gltfutil;
using namespace tinygltf;
using std::cout;

texture_dumper::texture_dumper(const Model& input)
    : model(input), configured_format(texture_output_format::png) {
  cout << "Texture dumper\n";
}

void texture_dumper::dump_to_folder(const std::string& path) {
  cout << "dumping to folder " << path << '\n';
  cout << "model file has " << model.textures.size() << " textures.\n";
  size_t index = 0;

  for (const auto& texture : model.textures) {
    index++;
    const auto& image = model.images[texture.source];
    cout << "image name is: \"" << image.name << "\"\n";
    cout << "image size is: " << image.width << 'x' << image.height << '\n';
    cout << "pixel channel count :" << image.component << '\n';
    cout << "pixel bit depth :" << image.bits << '\n';
    std::string name = image.name.empty() ? std::to_string(index) : image.name;

    // TODO stb_image_write doesn't support any 16bit wirtes;
    unsigned char* bytes_to_write =
        const_cast<unsigned char*>(image.image.data());
    unsigned char* tmp_buffer = nullptr;
    if (image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
      unsigned short max = 0xFFFF;
      tmp_buffer = new unsigned char[image.width * image.height *
                                     image.component * sizeof(unsigned char)];

      unsigned short* buffer_as_shorts = (unsigned short*)bytes_to_write;
      for (int i = 0; i < image.component * image.width * image.height; ++i) {
        tmp_buffer[i] =
            (unsigned char)(0xFF * (float(buffer_as_shorts[i]) / float(max)));
      }
      bytes_to_write =
          tmp_buffer;  // swap the pointer, but keep tmp_buffer around as we
                       // need to delete that memory later
    }

    switch (configured_format) {
      case texture_output_format::png:
        name = path + "/" + name + ".png";
        std::cout << "Image will be written to " << name << '\n';
        stbi_write_png(name.c_str(), image.width, image.height, image.component,
                       bytes_to_write, 0);
        break;
      case texture_output_format::bmp:
        std::cout << "Image will be written to " << name << '\n';
        name = path + "/" + name + ".bmp";
        stbi_write_bmp(name.c_str(), image.width, image.height, image.component,
                       bytes_to_write);
        break;
      case texture_output_format::tga:
        std::cout << "Image will be written to " << name << '\n';
        name = path + "/" + name + ".tga";
        stbi_write_tga(name.c_str(), image.width, image.height, image.component,
                       bytes_to_write);
        break;
    }

    delete[] tmp_buffer;
  }
}

void texture_dumper::set_output_format(texture_output_format format) {
  configured_format = format;
}

texture_dumper::texture_output_format texture_dumper::get_fromat_from_string(
    const std::string& str) {
  std::string type = str;
  std::transform(str.begin(), str.end(), type.begin(), ::tolower);

  if (type == "png") return texture_output_format::png;
  if (type == "bmp") return texture_output_format::bmp;
  if (type == "tga") return texture_output_format::tga;

  return texture_output_format::not_specified;
}
