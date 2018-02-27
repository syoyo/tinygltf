#ifndef EXAMPLE_MATERIAL_H_
#define EXAMPLE_MATERIAL_H_

#include <cstdlib>

#ifdef __clang__
#pragma clang diagnostic push
#if __has_warning("-Wzero-as-null-pointer-constant")
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#endif

namespace example {

struct Material {
  // float ambient[3];
  float diffuse[3];
  float specular[3];
  // float reflection[3];
  // float refraction[3];
  int id;
  int diffuse_texid;
  int specular_texid;
  // int reflection_texid;
  // int transparency_texid;
  // int bump_texid;
  // int normal_texid;  // normal map
  // int alpha_texid;  // alpha map

  Material() {
    // ambient[0] = 0.0;
    // ambient[1] = 0.0;
    // ambient[2] = 0.0;
    diffuse[0] = 0.5;
    diffuse[1] = 0.5;
    diffuse[2] = 0.5;
    specular[0] = 0.5;
    specular[1] = 0.5;
    specular[2] = 0.5;
    // reflection[0] = 0.0;
    // reflection[1] = 0.0;
    // reflection[2] = 0.0;
    // refraction[0] = 0.0;
    // refraction[1] = 0.0;
    // refraction[2] = 0.0;
    id = -1;
    diffuse_texid = -1;
    specular_texid = -1;
    // reflection_texid = -1;
    // transparency_texid = -1;
    // bump_texid = -1;
    // normal_texid = -1;
    // alpha_texid = -1;
  }
};

struct Texture {
  int width;
  int height;
  int components;
  int _pad_;
  unsigned char* image;

  Texture() {
    width = -1;
    height = -1;
    components = -1;
    image = NULL;
  }
};

}  // namespace example


#endif  // EXAMPLE_MATERIAL_H_
