#include <algorithm>
#include <iostream>

#include "stb_image_write.h"
#include "texture_dumper.h"

#include "lodepng.h"  // ../common

#include "tinyexr.h"

#include <tiny_gltf.h>

using namespace gltfutil;
using namespace tinygltf;
using std::cout;

static LodePNGColorType GetLodePNGColorType(int channels) {
  if (channels == 1) {
    return LodePNGColorType::LCT_GREY;
  } else if (channels == 2) {
    return LodePNGColorType::LCT_GREY_ALPHA;
  } else if (channels == 3) {
    return LodePNGColorType::LCT_RGB;
  } else if (channels == 4) {
    return LodePNGColorType::LCT_RGBA;
  } else {
    std::cerr << "??? unsupported channels " << channels << "\n";
    return LodePNGColorType::LCT_RGB;  // FIXME(syoyo): Raise error
  }
}

static void ToBigEndian(std::vector<uint8_t>* image) {

  assert(image->size() % 2 == 0);

  union {
    unsigned int i;
    char c[4];
  } bint = {0x01020304};

  bool is_big_endian = (bint.c[0] == 1);

  if (is_big_endian) {
    return;
  }

  uint16_t *ptr = reinterpret_cast<uint16_t *>(image->data());
  size_t n = image->size() / 2;

  for (size_t i = 0; i < n; i++) {
    ptr[i] = ((0xFF00 & ptr[i]) >> 8) | ((0x00FF & ptr[i]) << 8);
  }
}

static bool Save16bitImageAsEXR(const std::string& filename,
                                const tinygltf::Image& image) {
  assert(image.bits == 16);

  std::vector<float> buf(image.width * image.height * image.component);

  // widen to float image.
  // Store as is(i.e, pixel value range is [0.0, 65535.0])
  const unsigned short* ptr =
      reinterpret_cast<const unsigned short*>(image.image.data());
  for (size_t i = 0; i < image.width * image.height * image.component; i++) {
    buf[i] = float(ptr[i]);
  }

  const char* err = nullptr;
  int ret = SaveEXR(buf.data(), image.width, image.height, image.component,
                    /* save_as_fp16 */ 0, filename.c_str(), &err);

  if (err) {
    std::cerr << "EXR err: " << err << std::endl;
    FreeEXRErrorMessage(err);
    return false;
  }

  return (ret == TINYEXR_SUCCESS);
}

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
    std::string basename =
        image.name.empty() ? std::to_string(index) : image.name;

    unsigned char* bytes_to_write =
        const_cast<unsigned char*>(image.image.data());

    std::string filename;
    switch (configured_format) {
      case texture_output_format::png:
        filename = path + "/" + basename + ".png";

        if (this->use_exr) {
          if (image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            filename = path + "/" + basename + ".exr";
          }
        }

        std::cout << "Image will be written to " << filename << '\n';

        if (image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
          if (this->use_exr) {
            bool ret = Save16bitImageAsEXR(filename, image);
            assert(ret);
          } else {
            // Use lodepng to save 16bit PNG.
            // NOTE(syoyo): `loadpng::encode` requires image data must be stored in big endian.
            std::vector<uint8_t> tmp = image.image; // copy
            ToBigEndian(&tmp);

            unsigned ret = lodepng::encode(
                filename, tmp.data(), image.width, image.height,
                GetLodePNGColorType(image.component), /* bits */ 16);
            assert(ret == 0);  // 0 = no err.
          }
        } else {
          // TODO(syoyo): check status
          stbi_write_png(filename.c_str(), image.width, image.height,
                         image.component, bytes_to_write, 0);
        }
        break;
      case texture_output_format::bmp:
        filename = path + "/" + basename + ".bmp";
        std::cout << "Image will be written to " << filename << '\n';
        stbi_write_bmp(filename.c_str(), image.width, image.height,
                       image.component, bytes_to_write);
        break;
      case texture_output_format::tga:
        filename = path + "/" + basename + ".tga";
        std::cout << "Image will be written to " << filename << '\n';
        stbi_write_tga(filename.c_str(), image.width, image.height,
                       image.component, bytes_to_write);
        break;
    }
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
