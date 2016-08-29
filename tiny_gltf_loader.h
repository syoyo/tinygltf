//
// Tiny glTF loader.
//
// Copyright (c) 2015-2016, Syoyo Fujita and many contributors.
// All rights reserved.
// (Licensed under 2-clause BSD liecense)
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//
// Version:
//  - v0.9.4 Support parsing `shader`, `program` and `tecnique` thanks to
//  @lukesanantonio
//  - v0.9.3 Support binary glTF
//  - v0.9.2 Support parsing `texture`
//  - v0.9.1 Support loading glTF asset from memory
//  - v0.9.0 Initial
//
// Tiny glTF loader is using following libraries:
//
//  - picojson: C++ JSON library.
//  - base64: base64 decode/encode library.
//  - stb_image: Image loading library.
//
#ifndef TINY_GLTF_LOADER_H_
#define TINY_GLTF_LOADER_H_

#include <map>
#include <string>
#include <vector>

namespace tinygltf {

#define TINYGLTF_MODE_POINTS (0)
#define TINYGLTF_MODE_LINE (1)
#define TINYGLTF_MODE_LINE_LOOP (2)
#define TINYGLTF_MODE_TRIANGLES (4)
#define TINYGLTF_MODE_TRIANGLE_STRIP (5)
#define TINYGLTF_MODE_TRIANGLE_FAN (6)

#define TINYGLTF_COMPONENT_TYPE_BYTE (5120)
#define TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE (5121)
#define TINYGLTF_COMPONENT_TYPE_SHORT (5122)
#define TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT (5123)
#define TINYGLTF_COMPONENT_TYPE_INT (5124)
#define TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT (5125)
#define TINYGLTF_COMPONENT_TYPE_FLOAT (5126)
#define TINYGLTF_COMPONENT_TYPE_DOUBLE (5127)

#define TINYGLTF_TEXTURE_FILTER_NEAREST (9728)
#define TINYGLTF_TEXTURE_FILTER_LINEAR (9729)
#define TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST (9984)
#define TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST (9985)
#define TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR (9986)
#define TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR (9987)

#define TINYGLTF_TEXTURE_WRAP_RPEAT (10497)
#define TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE (33071)
#define TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT (33648)

// Redeclarations of the above for technique.parameters.
#define TINYGLTF_PARAMETER_TYPE_BYTE (5120)
#define TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE (5121)
#define TINYGLTF_PARAMETER_TYPE_SHORT (5122)
#define TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT (5123)
#define TINYGLTF_PARAMETER_TYPE_INT (5124)
#define TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT (5125)
#define TINYGLTF_PARAMETER_TYPE_FLOAT (5126)

#define TINYGLTF_PARAMETER_TYPE_FLOAT_VEC2 (35664)
#define TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 (35665)
#define TINYGLTF_PARAMETER_TYPE_FLOAT_VEC4 (35666)

#define TINYGLTF_PARAMETER_TYPE_INT_VEC2 (35667)
#define TINYGLTF_PARAMETER_TYPE_INT_VEC3 (35668)
#define TINYGLTF_PARAMETER_TYPE_INT_VEC4 (35669)

#define TINYGLTF_PARAMETER_TYPE_BOOL (35670)
#define TINYGLTF_PARAMETER_TYPE_BOOL_VEC2 (35671)
#define TINYGLTF_PARAMETER_TYPE_BOOL_VEC3 (35672)
#define TINYGLTF_PARAMETER_TYPE_BOOL_VEC4 (35673)

#define TINYGLTF_PARAMETER_TYPE_FLOAT_MAT2 (35674)
#define TINYGLTF_PARAMETER_TYPE_FLOAT_MAT3 (35675)
#define TINYGLTF_PARAMETER_TYPE_FLOAT_MAT4 (35676)

#define TINYGLTF_PARAMETER_TYPE_SAMPLER_2D (35678)

// End parameter types

#define TINYGLTF_TYPE_VEC2 (2)
#define TINYGLTF_TYPE_VEC3 (3)
#define TINYGLTF_TYPE_VEC4 (4)
#define TINYGLTF_TYPE_MAT2 (32 + 2)
#define TINYGLTF_TYPE_MAT3 (32 + 3)
#define TINYGLTF_TYPE_MAT4 (32 + 4)
#define TINYGLTF_TYPE_SCALAR (64 + 1)
#define TINYGLTF_TYPE_VECTOR (64 + 4)
#define TINYGLTF_TYPE_MATRIX (64 + 16)

#define TINYGLTF_IMAGE_FORMAT_JPEG (0)
#define TINYGLTF_IMAGE_FORMAT_PNG (1)
#define TINYGLTF_IMAGE_FORMAT_BMP (2)
#define TINYGLTF_IMAGE_FORMAT_GIF (3)

#define TINYGLTF_TEXTURE_FORMAT_ALPHA (6406)
#define TINYGLTF_TEXTURE_FORMAT_RGB (6407)
#define TINYGLTF_TEXTURE_FORMAT_RGBA (6408)
#define TINYGLTF_TEXTURE_FORMAT_LUMINANCE (6409)
#define TINYGLTF_TEXTURE_FORMAT_LUMINANCE_ALPHA (6410)

#define TINYGLTF_TEXTURE_TARGET_TEXTURE2D (3553)
#define TINYGLTF_TEXTURE_TYPE_UNSIGNED_BYTE (5121)

#define TINYGLTF_TARGET_ARRAY_BUFFER (34962)
#define TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER (34963)

#define TINYGLTF_SHADER_TYPE_VERTEX_SHADER (35633)
#define TINYGLTF_SHADER_TYPE_FRAGMENT_SHADER (35632)

typedef struct {
  std::string string_value;
  std::vector<double> number_array;
} Parameter;

typedef std::map<std::string, Parameter> ParameterMap;

typedef struct {
  std::string sampler;
  std::string target_id;
  std::string target_path;
} AnimationChannel;

typedef struct {
  std::string input;
  std::string interpolation;
  std::string output;
} AnimationSampler;

typedef struct {
  std::string name;
  std::vector<AnimationChannel> channels;
  std::map<std::string, AnimationSampler> samplers;
  ParameterMap parameters;
} Animation;

typedef struct {
  std::string name;
  int minFilter;
  int magFilter;
  int wrapS;
  int wrapT;
  int wrapR;  // TinyGLTF extension
  int pad0;
} Sampler;

typedef struct {
  std::string name;
  int width;
  int height;
  int component;
  int pad0;
  std::vector<unsigned char> image;

  std::string bufferView;  // KHR_binary_glTF extenstion.
  std::string mimeType;    // KHR_binary_glTF extenstion.
} Image;

typedef struct {
  int format;
  int internalFormat;
  std::string sampler;  // Required
  std::string source;   // Required
  int target;
  int type;
  std::string name;
} Texture;

typedef struct {
  std::string name;
  std::string technique;
  ParameterMap values;
} Material;

typedef struct {
  std::string name;
  std::string buffer;  // Required
  size_t byteOffset;   // Required
  size_t byteLength;   // default: 0
  int target;
  int pad0;
} BufferView;

typedef struct {
  std::string bufferView;
  std::string name;
  size_t byteOffset;
  size_t byteStride;
  int componentType;  // One of TINYGLTF_COMPONENT_TYPE_***
  int pad0;
  size_t count;
  int type;  // One of TINYGLTF_TYPE_***
  int pad1;
  std::vector<double> minValues;  // Optional
  std::vector<double> maxValues;  // Optional
} Accessor;

class Camera {
 public:
  Camera() {}
  ~Camera() {}

  std::string name;
  bool isOrthographic;  // false = perspective.

  // Some common properties.
  float aspectRatio;
  float yFov;
  float zFar;
  float zNear;
};

typedef struct {
  std::map<std::string, std::string> attributes;  // A dictionary object of
                                                  // strings, where each string
                                                  // is the ID of the accessor
                                                  // containing an attribute.
  std::string material;  // The ID of the material to apply to this primitive
                         // when rendering.
  std::string indices;   // The ID of the accessor that contains the indices.
  int mode;              // one of TINYGLTF_MODE_***
  int pad0;
} Primitive;

typedef struct {
  std::string name;
  std::vector<Primitive> primitives;
} Mesh;

class Node {
 public:
  Node() {}
  ~Node() {}

  std::string camera;  // camera object referenced by this node.

  std::string name;
  std::vector<std::string> children;
  std::vector<double> rotation;     // length must be 0 or 4
  std::vector<double> scale;        // length must be 0 or 3
  std::vector<double> translation;  // length must be 0 or 3
  std::vector<double> matrix;       // length must be 0 or 16
  std::vector<std::string> meshes;
};

typedef struct {
  std::string name;
  std::vector<unsigned char> data;
} Buffer;

typedef struct {
  std::string name;
  int type;
  int pad0;
  std::vector<unsigned char> source;
} Shader;

typedef struct {
  std::string name;
  std::string vertexShader;
  std::string fragmentShader;
  std::vector<std::string> attributes;
} Program;

typedef struct {
  int count;
  int pad0;
  std::string node;
  std::string semantic;
  int type;
  int pad1;
  Parameter value;
} TechniqueParameter;

typedef struct {
  std::string name;
  std::string program;
  std::map<std::string, TechniqueParameter> parameters;
  std::map<std::string, std::string> attributes;
  std::map<std::string, std::string> uniforms;
} Technique;

typedef struct {
  std::string generator;
  std::string version;
  std::string profile_api;
  std::string profile_version;
  bool premultipliedAlpha;
  char pad[7];
} Asset;

class Scene {
 public:
  Scene() {}
  ~Scene() {}

  std::map<std::string, Accessor> accessors;
  std::map<std::string, Animation> animations;
  std::map<std::string, Buffer> buffers;
  std::map<std::string, BufferView> bufferViews;
  std::map<std::string, Material> materials;
  std::map<std::string, Mesh> meshes;
  std::map<std::string, Node> nodes;
  std::map<std::string, Texture> textures;
  std::map<std::string, Image> images;
  std::map<std::string, Shader> shaders;
  std::map<std::string, Program> programs;
  std::map<std::string, Technique> techniques;
  std::map<std::string, Sampler> samplers;
  std::map<std::string, std::vector<std::string> > scenes;  // list of nodes

  std::string defaultScene;

  Asset asset;
};

enum SectionCheck
{
  NO_REQUIRE = 0x00,
  REQUIRE_SCENE = 0x01,
  REQUIRE_SCENES = 0x02,
  REQUIRE_NODES = 0x04,
  REQUIRE_ACCESSORS = 0x08,
  REQUIRE_BUFFERS = 0x10,
  REQUIRE_BUFFER_VIEWS = 0x20,
  REQUIRE_ALL = 0x3f
};

class TinyGLTFLoader {
 public:
  TinyGLTFLoader() : bin_data_(NULL), bin_size_(0), is_binary_(false) {
    pad[0] = pad[1] = pad[2] = pad[3] = pad[4] = pad[5] = pad[6] = 0;
  }
  ~TinyGLTFLoader() {}

  /// Loads glTF ASCII asset from a file.
  /// Returns false and set error string to `err` if there's an error.
  bool LoadASCIIFromFile(Scene *scene, std::string *err,
                         const std::string &filename,
                         unsigned int check_sections = REQUIRE_ALL);

  /// Loads glTF ASCII asset from string(memory).
  /// `length` = strlen(str);
  /// Returns false and set error string to `err` if there's an error.
  bool LoadASCIIFromString(Scene *scene, std::string *err, const char *str,
                           const unsigned int length,
                           const std::string &base_dir,
                           unsigned int check_sections = REQUIRE_ALL);

  /// Loads glTF binary asset from a file.
  /// Returns false and set error string to `err` if there's an error.
  bool LoadBinaryFromFile(Scene *scene, std::string *err,
                          const std::string &filename,
                          unsigned int check_sections = REQUIRE_ALL);

  /// Loads glTF binary asset from memory.
  /// `length` = strlen(str);
  /// Returns false and set error string to `err` if there's an error.
  bool LoadBinaryFromMemory(Scene *scene, std::string *err,
                            const unsigned char *bytes,
                            const unsigned int length,
                            const std::string &base_dir = "",
                            unsigned int check_sections = REQUIRE_ALL);

 private:
  /// Loads glTF asset from string(memory).
  /// `length` = strlen(str);
  /// Returns false and set error string to `err` if there's an error.
  bool LoadFromString(Scene *scene, std::string *err, const char *str,
                      const unsigned int length, const std::string &base_dir,
                      unsigned int check_sections);

  const unsigned char *bin_data_;
  size_t bin_size_;
  bool is_binary_;
  char pad[7];
};

}  // namespace tinygltf

#ifdef TINYGLTF_LOADER_IMPLEMENTATION
#include <algorithm>
#include <cassert>
#include <fstream>
#include <sstream>

#ifdef TINYGLTF_APPLY_CLANG_WEVERYTHING
// Disable some warnings for external files.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#pragma clang diagnostic ignored "-Wpadded"
#endif

#include "./picojson.h"
#include "./stb_image.h"
#ifdef TINYGLTF_APPLY_CLANG_WEVERYTHING
#pragma clang diagnostic pop
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <wordexp.h>
#endif

#if defined(__sparcv9)
// Big endian
#else
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || MINIZ_X86_OR_X64_CPU
#define TINYGLTF_LITTLE_ENDIAN 1
#endif
#endif

namespace tinygltf {

static void swap4(unsigned int *val) {
#ifdef TINYGLTF_LITTLE_ENDIAN
  (void)val;
#else
  unsigned int tmp = *val;
  unsigned char *dst = reinterpret_cast<unsigned char *>(val);
  unsigned char *src = reinterpret_cast<unsigned char *>(&tmp);

  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
#endif
}

static bool FileExists(const std::string &abs_filename) {
  bool ret;
#ifdef _WIN32
  FILE *fp;
  errno_t err = fopen_s(&fp, abs_filename.c_str(), "rb");
  if (err != 0) {
    return false;
  }
#else
  FILE *fp = fopen(abs_filename.c_str(), "rb");
#endif
  if (fp) {
    ret = true;
    fclose(fp);
  } else {
    ret = false;
  }

  return ret;
}

static std::string ExpandFilePath(const std::string &filepath) {
#ifdef _WIN32
  DWORD len = ExpandEnvironmentStringsA(filepath.c_str(), NULL, 0);
  char *str = new char[len];
  ExpandEnvironmentStringsA(filepath.c_str(), str, len);

  std::string s(str);

  delete[] str;

  return s;
#else

#if defined(TARGET_OS_IPHONE) || defined(TARGET_IPHONE_SIMULATOR)
  // no expansion
  std::string s = filepath;
#else
  std::string s;
  wordexp_t p;

  if (filepath.empty()) {
    return "";
  }

  // char** w;
  int ret = wordexp(filepath.c_str(), &p, 0);
  if (ret) {
    // err
    s = filepath;
    return s;
  }

  // Use first element only.
  if (p.we_wordv) {
    s = std::string(p.we_wordv[0]);
    wordfree(&p);
  } else {
    s = filepath;
  }

#endif

  return s;
#endif
}

static std::string JoinPath(const std::string &path0,
                            const std::string &path1) {
  if (path0.empty()) {
    return path1;
  } else {
    // check '/'
    char lastChar = *path0.rbegin();
    if (lastChar != '/') {
      return path0 + std::string("/") + path1;
    } else {
      return path0 + path1;
    }
  }
}

static std::string FindFile(const std::vector<std::string> &paths,
                            const std::string &filepath) {
  for (size_t i = 0; i < paths.size(); i++) {
    std::string absPath = ExpandFilePath(JoinPath(paths[i], filepath));
    if (FileExists(absPath)) {
      return absPath;
    }
  }

  return std::string();
}

// std::string GetFilePathExtension(const std::string& FileName)
//{
//    if(FileName.find_last_of(".") != std::string::npos)
//        return FileName.substr(FileName.find_last_of(".")+1);
//    return "";
//}

static std::string GetBaseDir(const std::string &filepath) {
  if (filepath.find_last_of("/\\") != std::string::npos)
    return filepath.substr(0, filepath.find_last_of("/\\"));
  return "";
}

// std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const &s);

/*
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

#ifdef TINYGLTF_APPLY_CLANG_WEVERYTHING
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconversion"
#endif
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_decode(std::string const &encoded_string) {
  int in_len = static_cast<int>(encoded_string.size());
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && (encoded_string[in_] != '=') &&
         is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_];
    in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] =
            static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

      char_array_3[0] =
          (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] =
          ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++) ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++) char_array_4[j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4[j] =
          static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] =
        ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}
#ifdef TINYGLTF_APPLY_CLANG_WEVERYTHING
#pragma clang diagnostic pop
#endif

static bool LoadExternalFile(std::vector<unsigned char> *out, std::string *err,
                             const std::string &filename,
                             const std::string &basedir, size_t reqBytes,
                             bool checkSize) {
  out->clear();

  std::vector<std::string> paths;
  paths.push_back(basedir);
  paths.push_back(".");

  std::string filepath = FindFile(paths, filename);
  if (filepath.empty()) {
    if (err) {
      (*err) += "File not found : " + filename + "\n";
    }
    return false;
  }

  std::ifstream f(filepath.c_str(), std::ifstream::binary);
  if (!f) {
    if (err) {
      (*err) += "File open error : " + filepath + "\n";
    }
    return false;
  }

  f.seekg(0, f.end);
  size_t sz = static_cast<size_t>(f.tellg());
  std::vector<unsigned char> buf(sz);

  f.seekg(0, f.beg);
  f.read(reinterpret_cast<char *>(&buf.at(0)),
         static_cast<std::streamsize>(sz));
  f.close();

  if (checkSize) {
    if (reqBytes == sz) {
      out->swap(buf);
      return true;
    } else {
      std::stringstream ss;
      ss << "File size mismatch : " << filepath << ", requestedBytes "
         << reqBytes << ", but got " << sz << std::endl;
      if (err) {
        (*err) += ss.str();
      }
      return false;
    }
  }

  out->swap(buf);
  return true;
}

static bool LoadImageData(Image *image, std::string *err, int req_width,
                          int req_height, const unsigned char *bytes,
                          int size) {
  int w, h, comp;
  unsigned char *data = stbi_load_from_memory(bytes, size, &w, &h, &comp, 0);
  if (!data) {
    if (err) {
      (*err) += "Unknown image format.\n";
    }
    return false;
  }

  if (w < 1 || h < 1) {
    if (err) {
      (*err) += "Unknown image format.\n";
    }
    return false;
  }

  if (req_width > 0) {
    if (req_width != w) {
      if (err) {
        (*err) += "Image width mismatch.\n";
      }
      return false;
    }
  }

  if (req_height > 0) {
    if (req_height != h) {
      if (err) {
        (*err) += "Image height mismatch.\n";
      }
      return false;
    }
  }

  image->width = w;
  image->height = h;
  image->component = comp;
  image->image.resize(static_cast<size_t>(w * h * comp));
  std::copy(data, data + w * h * comp, image->image.begin());

  return true;
}

static bool IsDataURI(const std::string &in) {
  std::string header = "data:application/octet-stream;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:image/png;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:image/jpeg;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:text/plain;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  return false;
}

static bool DecodeDataURI(std::vector<unsigned char> *out,
                          const std::string &in, size_t reqBytes,
                          bool checkSize) {
  std::string header = "data:application/octet-stream;base64,";
  std::string data;
  if (in.find(header) == 0) {
    data = base64_decode(in.substr(header.size()));  // cut mime string.
  }

  if (data.empty()) {
    header = "data:image/jpeg;base64,";
    if (in.find(header) == 0) {
      data = base64_decode(in.substr(header.size()));  // cut mime string.
    }
  }

  if (data.empty()) {
    header = "data:image/png;base64,";
    if (in.find(header) == 0) {
      data = base64_decode(in.substr(header.size()));  // cut mime string.
    }
  }

  if (data.empty()) {
    header = "data:text/plain;base64,";
    if (in.find(header) == 0) {
      data = base64_decode(in.substr(header.size()));
    }
  }

  if (data.empty()) {
    return false;
  }

  if (checkSize) {
    if (data.size() != reqBytes) {
      return false;
    }
    out->resize(reqBytes);
  } else {
    out->resize(data.size());
  }
  std::copy(data.begin(), data.end(), out->begin());
  return true;
}

static bool ParseBooleanProperty(bool *ret, std::string *err,
                                 const picojson::object &o,
                                 const std::string &property, bool required) {
  picojson::object::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing.\n";
      }
    }
    return false;
  }

  if (!it->second.is<bool>()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a bool type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = it->second.get<bool>();
  }

  return true;
}

static bool ParseNumberProperty(double *ret, std::string *err,
                                const picojson::object &o,
                                const std::string &property, bool required) {
  picojson::object::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing.\n";
      }
    }
    return false;
  }

  if (!it->second.is<double>()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a number type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = it->second.get<double>();
  }

  return true;
}

static bool ParseNumberArrayProperty(std::vector<double> *ret, std::string *err,
                                     const picojson::object &o,
                                     const std::string &property,
                                     bool required) {
  picojson::object::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing.\n";
      }
    }
    return false;
  }

  if (!it->second.is<picojson::array>()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not an array.\n";
      }
    }
    return false;
  }

  ret->clear();
  const picojson::array &arr = it->second.get<picojson::array>();
  for (size_t i = 0; i < arr.size(); i++) {
    if (!arr[i].is<double>()) {
      if (required) {
        if (err) {
          (*err) += "'" + property + "' property is not a number.\n";
        }
      }
      return false;
    }
    ret->push_back(arr[i].get<double>());
  }

  return true;
}

static bool ParseStringProperty(std::string *ret, std::string *err,
                                const picojson::object &o,
                                const std::string &property, bool required) {
  picojson::object::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing.\n";
      }
    }
    return false;
  }

  if (!it->second.is<std::string>()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a string type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = it->second.get<std::string>();
  }

  return true;
}

static bool ParseStringArrayProperty(std::vector<std::string> *ret,
                                     std::string *err,
                                     const picojson::object &o,
                                     const std::string &property,
                                     bool required) {
  picojson::object::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing.\n";
      }
    }
    return false;
  }

  if (!it->second.is<picojson::array>()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not an array.\n";
      }
    }
    return false;
  }

  ret->clear();
  const picojson::array &arr = it->second.get<picojson::array>();
  for (size_t i = 0; i < arr.size(); i++) {
    if (!arr[i].is<std::string>()) {
      if (required) {
        if (err) {
          (*err) += "'" + property + "' property is not a string.\n";
        }
      }
      return false;
    }
    ret->push_back(arr[i].get<std::string>());
  }

  return true;
}

static bool ParseStringMapProperty(std::map<std::string, std::string> *ret,
                                   std::string *err, const picojson::object &o,
                                   const std::string &property, bool required) {
  picojson::object::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing.\n";
      }
    }
    return false;
  }

  // Make sure we are dealing with an object / dictionary.
  if (!it->second.is<picojson::object>()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not an object.\n";
      }
    }
    return false;
  }

  ret->clear();
  const picojson::object &dict = it->second.get<picojson::object>();

  picojson::object::const_iterator dictIt(dict.begin());
  picojson::object::const_iterator dictItEnd(dict.end());

  for (; dictIt != dictItEnd; ++dictIt) {
    // Check that the value is a string.
    if (!dictIt->second.is<std::string>()) {
      if (required) {
        if (err) {
          (*err) += "'" + property + "' value is not a string.\n";
        }
      }
      return false;
    }

    // Insert into the list.
    (*ret)[dictIt->first] = dictIt->second.get<std::string>();
  }
  return true;
}

static bool ParseKHRBinaryExtension(const picojson::object &o, std::string *err,
                                    std::string *buffer_view,
                                    std::string *mime_type, int *image_width,
                                    int *image_height) {
  picojson::object j = o;

  if (j.find("extensions") == j.end()) {
    if (err) {
      (*err) += "`extensions' property is missing.\n";
    }
    return false;
  }

  if (!(j["extensions"].is<picojson::object>())) {
    if (err) {
      (*err) += "Invalid `extensions' property.\n";
    }
    return false;
  }

  picojson::object ext = j["extensions"].get<picojson::object>();

  if (ext.find("KHR_binary_glTF") == ext.end()) {
    if (err) {
      (*err) +=
          "`KHR_binary_glTF' property is missing in extension property.\n";
    }
    return false;
  }

  if (!(ext["KHR_binary_glTF"].is<picojson::object>())) {
    if (err) {
      (*err) += "Invalid `KHR_binary_glTF' property.\n";
    }
    return false;
  }

  picojson::object k = ext["KHR_binary_glTF"].get<picojson::object>();

  if (!ParseStringProperty(buffer_view, err, k, "bufferView", true)) {
    return false;
  }

  if (mime_type) {
    ParseStringProperty(mime_type, err, k, "mimeType", false);
  }

  if (image_width) {
    double width = 0.0;
    if (ParseNumberProperty(&width, err, k, "width", false)) {
      (*image_width) = static_cast<int>(width);
    }
  }

  if (image_height) {
    double height = 0.0;
    if (ParseNumberProperty(&height, err, k, "height", false)) {
      (*image_height) = static_cast<int>(height);
    }
  }

  return true;
}

static bool ParseAsset(Asset *asset, std::string *err,
                       const picojson::object &o) {
  ParseStringProperty(&asset->generator, err, o, "generator", false);
  ParseBooleanProperty(&asset->premultipliedAlpha, err, o, "premultipliedAlpha",
                       false);

  ParseStringProperty(&asset->version, err, o, "version", false);

  picojson::object::const_iterator profile = o.find("profile");
  if (profile != o.end()) {
    const picojson::value &v = profile->second;
    if (v.contains("api") & v.get("api").is<std::string>()) {
      asset->profile_api = v.get("api").get<std::string>();
    }
    if (v.contains("version") & v.get("version").is<std::string>()) {
      asset->profile_version = v.get("version").get<std::string>();
    }
  }

  return true;
}

static bool ParseImage(Image *image, std::string *err,
                       const picojson::object &o, const std::string &basedir,
                       bool is_binary, const unsigned char *bin_data,
                       size_t bin_size) {
  std::string uri;
  if (!ParseStringProperty(&uri, err, o, "uri", true)) {
    return false;
  }

  ParseStringProperty(&image->name, err, o, "name", false);

  std::vector<unsigned char> img;

  if (is_binary) {
    // Still binary glTF accepts external dataURI. First try external resources.
    bool loaded = false;
    if (IsDataURI(uri)) {
      loaded = DecodeDataURI(&img, uri, 0, false);
    } else {
      // Assume external .bin file.
      loaded = LoadExternalFile(&img, err, uri, basedir, 0, false);
    }

    if (!loaded) {
      // load data from (embedded) binary data

      if ((bin_size == 0) || (bin_data == NULL)) {
        if (err) {
          (*err) += "Invalid binary data.\n";
        }
        return false;
      }

      // There should be "extensions" property.
      // "extensions":{"KHR_binary_glTF":{"bufferView": "id", ...

      std::string buffer_view;
      std::string mime_type;
      int image_width;
      int image_height;
      bool ret = ParseKHRBinaryExtension(o, err, &buffer_view, &mime_type,
                                         &image_width, &image_height);
      if (!ret) {
        return false;
      }

      if (uri.compare("data:,") == 0) {
        // ok
      } else {
        if (err) {
          (*err) += "Invalid URI for binary data.\n";
        }
        return false;
      }

      // Just only save some information here. Loading actual image data from
      // bufferView is done in other place.
      image->bufferView = buffer_view;
      image->mimeType = mime_type;
      image->width = image_width;
      image->height = image_height;

      return true;
    }
  } else {
    if (IsDataURI(uri)) {
      if (!DecodeDataURI(&img, uri, 0, false)) {
        if (err) {
          (*err) += "Failed to decode 'uri' for image parameter.\n";
        }
        return false;
      }
    } else {
      // Assume external file
      if (!LoadExternalFile(&img, err, uri, basedir, 0, false)) {
        if (err) {
          (*err) += "Failed to load external 'uri'. for image parameter\n";
        }
        return false;
      }
      if (img.empty()) {
        if (err) {
          (*err) += "Image is empty.\n";
        }
        return false;
      }
    }
  }

  return LoadImageData(image, err, 0, 0, &img.at(0),
                       static_cast<int>(img.size()));
}

static bool ParseTexture(Texture *texture, std::string *err,
                         const picojson::object &o,
                         const std::string &basedir) {
  (void)basedir;

  if (!ParseStringProperty(&texture->sampler, err, o, "sampler", true)) {
    return false;
  }

  if (!ParseStringProperty(&texture->source, err, o, "source", true)) {
    return false;
  }

  ParseStringProperty(&texture->name, err, o, "name", false);

  double format = TINYGLTF_TEXTURE_FORMAT_RGBA;
  ParseNumberProperty(&format, err, o, "format", false);

  double internalFormat = TINYGLTF_TEXTURE_FORMAT_RGBA;
  ParseNumberProperty(&internalFormat, err, o, "internalFormat", false);

  double target = TINYGLTF_TEXTURE_TARGET_TEXTURE2D;
  ParseNumberProperty(&target, err, o, "target", false);

  double type = TINYGLTF_TEXTURE_TYPE_UNSIGNED_BYTE;
  ParseNumberProperty(&type, err, o, "type", false);

  texture->format = static_cast<int>(format);
  texture->internalFormat = static_cast<int>(internalFormat);
  texture->target = static_cast<int>(target);
  texture->type = static_cast<int>(type);

  return true;
}

static bool ParseBuffer(Buffer *buffer, std::string *err,
                        const picojson::object &o, const std::string &basedir,
                        bool is_binary = false,
                        const unsigned char *bin_data = NULL,
                        size_t bin_size = 0) {
  double byteLength;
  if (!ParseNumberProperty(&byteLength, err, o, "byteLength", true)) {
    return false;
  }

  std::string uri;
  if (!ParseStringProperty(&uri, err, o, "uri", true)) {
    return false;
  }

  picojson::object::const_iterator type = o.find("type");
  if (type != o.end()) {
    if (type->second.is<std::string>()) {
      const std::string &ty = (type->second).get<std::string>();
      if (ty.compare("arraybuffer") == 0) {
        // buffer.type = "arraybuffer";
      }
    }
  }

  size_t bytes = static_cast<size_t>(byteLength);
  if (is_binary) {
    // Still binary glTF accepts external dataURI. First try external resources.
    bool loaded = false;
    if (IsDataURI(uri)) {
      loaded = DecodeDataURI(&buffer->data, uri, bytes, true);
    } else {
      // Assume external .bin file.
      loaded = LoadExternalFile(&buffer->data, err, uri, basedir, bytes, true);
    }

    if (!loaded) {
      // load data from (embedded) binary data

      if ((bin_size == 0) || (bin_data == NULL)) {
        if (err) {
          (*err) += "Invalid binary data.\n";
        }
        return false;
      }

      if (byteLength > bin_size) {
        if (err) {
          std::stringstream ss;
          ss << "Invalid `byteLength'. Must be equal or less than binary size: "
                "`byteLength' = "
             << byteLength << ", binary size = " << bin_size << std::endl;
          (*err) += ss.str();
        }
        return false;
      }

      if (uri.compare("data:,") == 0) {
        // @todo { check uri }
        buffer->data.resize(static_cast<size_t>(byteLength));
        memcpy(&(buffer->data.at(0)), bin_data,
               static_cast<size_t>(byteLength));

      } else {
        if (err) {
          (*err) += "Invalid URI for binary data.\n";
        }
        return false;
      }
    }

  } else {
    if (IsDataURI(uri)) {
      if (!DecodeDataURI(&buffer->data, uri, bytes, true)) {
        if (err) {
          (*err) += "Failed to decode 'uri'.\n";
        }
        return false;
      }
    } else {
      // Assume external .bin file.
      if (!LoadExternalFile(&buffer->data, err, uri, basedir, bytes, true)) {
        return false;
      }
    }
  }

  ParseStringProperty(&buffer->name, err, o, "name", false);

  return true;
}

static bool ParseBufferView(BufferView *bufferView, std::string *err,
                            const picojson::object &o) {
  std::string buffer;
  if (!ParseStringProperty(&buffer, err, o, "buffer", true)) {
    return false;
  }

  double byteOffset;
  if (!ParseNumberProperty(&byteOffset, err, o, "byteOffset", true)) {
    return false;
  }

  double byteLength = 0.0;
  ParseNumberProperty(&byteLength, err, o, "byteLength", false);

  double target = 0.0;
  ParseNumberProperty(&target, err, o, "target", false);
  int targetValue = static_cast<int>(target);
  if ((targetValue == TINYGLTF_TARGET_ARRAY_BUFFER) ||
      (targetValue == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)) {
    // OK
  } else {
    targetValue = 0;
  }
  bufferView->target = targetValue;

  ParseStringProperty(&bufferView->name, err, o, "name", false);

  bufferView->buffer = buffer;
  bufferView->byteOffset = static_cast<size_t>(byteOffset);
  bufferView->byteLength = static_cast<size_t>(byteLength);

  return true;
}

static bool ParseAccessor(Accessor *accessor, std::string *err,
                          const picojson::object &o) {
  std::string bufferView;
  if (!ParseStringProperty(&bufferView, err, o, "bufferView", true)) {
    return false;
  }

  double byteOffset;
  if (!ParseNumberProperty(&byteOffset, err, o, "byteOffset", true)) {
    return false;
  }

  double componentType;
  if (!ParseNumberProperty(&componentType, err, o, "componentType", true)) {
    return false;
  }

  double count = 0.0;
  if (!ParseNumberProperty(&count, err, o, "count", true)) {
    return false;
  }

  std::string type;
  if (!ParseStringProperty(&type, err, o, "type", true)) {
    return false;
  }

  if (type.compare("SCALAR") == 0) {
    accessor->type = TINYGLTF_TYPE_SCALAR;
  } else if (type.compare("VEC2") == 0) {
    accessor->type = TINYGLTF_TYPE_VEC2;
  } else if (type.compare("VEC3") == 0) {
    accessor->type = TINYGLTF_TYPE_VEC3;
  } else if (type.compare("VEC4") == 0) {
    accessor->type = TINYGLTF_TYPE_VEC4;
  } else if (type.compare("MAT2") == 0) {
    accessor->type = TINYGLTF_TYPE_MAT2;
  } else if (type.compare("MAT3") == 0) {
    accessor->type = TINYGLTF_TYPE_MAT3;
  } else if (type.compare("MAT4") == 0) {
    accessor->type = TINYGLTF_TYPE_MAT4;
  } else {
    std::stringstream ss;
    ss << "Unsupported `type` for accessor object. Got \"" << type << "\"\n";
    if (err) {
      (*err) += ss.str();
    }
    return false;
  }

  double byteStride = 0.0;
  ParseNumberProperty(&byteStride, err, o, "byteStride", false);

  ParseStringProperty(&accessor->name, err, o, "name", false);

  accessor->minValues.clear();
  accessor->maxValues.clear();
  ParseNumberArrayProperty(&accessor->minValues, err, o, "min", false);
  ParseNumberArrayProperty(&accessor->maxValues, err, o, "max", false);

  accessor->count = static_cast<size_t>(count);
  accessor->bufferView = bufferView;
  accessor->byteOffset = static_cast<size_t>(byteOffset);
  accessor->byteStride = static_cast<size_t>(byteStride);

  {
    int comp = static_cast<int>(componentType);
    if (comp >= TINYGLTF_COMPONENT_TYPE_BYTE &&
        comp <= TINYGLTF_COMPONENT_TYPE_DOUBLE) {
      // OK
      accessor->componentType = comp;
    } else {
      std::stringstream ss;
      ss << "Invalid `componentType` in accessor. Got " << comp << "\n";
      if (err) {
        (*err) += ss.str();
      }
      return false;
    }
  }

  return true;
}

static bool ParsePrimitive(Primitive *primitive, std::string *err,
                           const picojson::object &o) {
  if (!ParseStringProperty(&primitive->material, err, o, "material", true)) {
    return false;
  }

  double mode = static_cast<double>(TINYGLTF_MODE_TRIANGLES);
  ParseNumberProperty(&mode, err, o, "mode", false);

  int primMode = static_cast<int>(mode);
  if (primMode != TINYGLTF_MODE_TRIANGLES) {
    if (err) {
      (*err) +=
          "Currently TinyGLTFLoader doesn not support primitive mode other \n"
          "than TRIANGLES.\n";
    }
    return false;
  }
  primitive->mode = primMode;

  primitive->indices = "";
  ParseStringProperty(&primitive->indices, err, o, "indices", false);

  if (!ParseStringMapProperty(&primitive->attributes, err, o, "attributes",
                              true)) {
    return false;
  }

  return true;
}

static bool ParseMesh(Mesh *mesh, std::string *err, const picojson::object &o) {
  ParseStringProperty(&mesh->name, err, o, "name", false);

  mesh->primitives.clear();
  picojson::object::const_iterator primObject = o.find("primitives");
  if ((primObject != o.end()) && (primObject->second).is<picojson::array>()) {
    const picojson::array &primArray =
        (primObject->second).get<picojson::array>();
    for (size_t i = 0; i < primArray.size(); i++) {
      Primitive primitive;
      if (ParsePrimitive(&primitive, err,
                         primArray[i].get<picojson::object>())) {
        // Only add the primitive if the parsing succeeds.
        mesh->primitives.push_back(primitive);
      }
    }
  }

  return true;
}

static bool ParseNode(Node *node, std::string *err, const picojson::object &o) {
  ParseStringProperty(&node->name, err, o, "name", false);

  ParseNumberArrayProperty(&node->rotation, err, o, "rotation", false);
  ParseNumberArrayProperty(&node->scale, err, o, "scale", false);
  ParseNumberArrayProperty(&node->translation, err, o, "translation", false);
  ParseNumberArrayProperty(&node->matrix, err, o, "matrix", false);
  ParseStringArrayProperty(&node->meshes, err, o, "meshes", false);

  node->children.clear();
  picojson::object::const_iterator childrenObject = o.find("children");
  if ((childrenObject != o.end()) &&
      (childrenObject->second).is<picojson::array>()) {
    const picojson::array &childrenArray =
        (childrenObject->second).get<picojson::array>();
    for (size_t i = 0; i < childrenArray.size(); i++) {
      if (!childrenArray[i].is<std::string>()) {
        if (err) {
          (*err) += "Invalid `children` array.\n";
        }
        return false;
      }
      const std::string &childrenNode = childrenArray[i].get<std::string>();
      node->children.push_back(childrenNode);
    }
  }

  return true;
}

static bool ParseParameterProperty(Parameter *param, std::string *err,
                                   const picojson::object &o,
                                   const std::string &prop, bool required) {
  double num_val;

  // A parameter value can either be a string or an array of either a boolean or
  // a number. Booleans of any kind aren't supported here. Granted, it
  // complicates the Parameter structure and breaks it semantically in the sense
  // that the client probably works off the assumption that if the string is
  // empty the vector is used, etc. Would a tagged union work?
  if (ParseStringProperty(&param->string_value, err, o, prop, false)) {
    // Found string property.
    return true;
  } else if (ParseNumberArrayProperty(&param->number_array, err, o, prop,
                                      false)) {
    // Found a number array.
    return true;
  } else if (ParseNumberProperty(&num_val, err, o, prop, false)) {
    param->number_array.push_back(num_val);
    return true;
  } else {
    if (required) {
      if (err) {
        (*err) += "parameter must be a string or number / number array.\n";
      }
    }
    return false;
  }
}

static bool ParseMaterial(Material *material, std::string *err,
                          const picojson::object &o) {
  ParseStringProperty(&material->name, err, o, "name", false);
  ParseStringProperty(&material->technique, err, o, "technique", false);

  material->values.clear();
  picojson::object::const_iterator valuesIt = o.find("values");

  if ((valuesIt != o.end()) && (valuesIt->second).is<picojson::object>()) {
    const picojson::object &values_object =
        (valuesIt->second).get<picojson::object>();

    picojson::object::const_iterator it(values_object.begin());
    picojson::object::const_iterator itEnd(values_object.end());

    for (; it != itEnd; it++) {
      Parameter param;
      if (ParseParameterProperty(&param, err, values_object, it->first,
                                 false)) {
        material->values[it->first] = param;
      }
    }
  }

  return true;
}

static bool ParseShader(Shader *shader, std::string *err,
                        const picojson::object &o, const std::string &basedir,
                        bool is_binary = false,
                        const unsigned char *bin_data = NULL,
                        size_t bin_size = 0) {
  std::string uri;
  if (!ParseStringProperty(&uri, err, o, "uri", true)) {
    return false;
  }

  if (is_binary) {
    // Still binary glTF accepts external dataURI. First try external resources.
    bool loaded = false;
    if (IsDataURI(uri)) {
      loaded = DecodeDataURI(&shader->source, uri, 0, false);
    } else {
      // Assume external .bin file.
      loaded = LoadExternalFile(&shader->source, err, uri, basedir, 0, false);
    }

    if (!loaded) {
      // load data from (embedded) binary data

      if ((bin_size == 0) || (bin_data == NULL)) {
        if (err) {
          (*err) += "Invalid binary data.\n";
        }
        return false;
      }

      // There should be "extensions" property.
      // "extensions":{"KHR_binary_glTF":{"bufferView": "id", ...

      std::string buffer_view;
      std::string mime_type;
      int image_width;
      int image_height;
      bool ret = ParseKHRBinaryExtension(o, err, &buffer_view, &mime_type,
                                         &image_width, &image_height);
      if (!ret) {
        return false;
      }

      if (uri.compare("data:,") == 0) {
        // ok
      } else {
        if (err) {
          (*err) += "Invalid URI for binary data.\n";
        }
        return false;
      }
    }
  } else {
    // Load shader source from data uri
    // TODO: Support ascii or utf-8 data uris.
    if (IsDataURI(uri)) {
      if (!DecodeDataURI(&shader->source, uri, 0, false)) {
        if (err) {
          (*err) += "Failed to decode 'uri' for shader parameter.\n";
        }
        return false;
      }
    } else {
      // Assume external file
      if (!LoadExternalFile(&shader->source, err, uri, basedir, 0, false)) {
        if (err) {
          (*err) += "Failed to load external 'uri' for shader parameter.\n";
        }
        return false;
      }
      if (shader->source.empty()) {
        if (err) {
          (*err) += "shader is empty.\n";  // This may be OK?
        }
        return false;
      }
    }
  }

  double type;
  if (!ParseNumberProperty(&type, err, o, "type", true)) {
    return false;
  }

  shader->type = static_cast<int>(type);

  return true;
}

static bool ParseProgram(Program *program, std::string *err,
                         const picojson::object &o) {
  ParseStringProperty(&program->name, err, o, "name", false);

  if (!ParseStringProperty(&program->vertexShader, err, o, "vertexShader",
                           true)) {
    return false;
  }
  if (!ParseStringProperty(&program->fragmentShader, err, o, "fragmentShader",
                           true)) {
    return false;
  }

  // I suppose the list of attributes isn't needed, but a technique doesn't
  // really make sense without it.
  ParseStringArrayProperty(&program->attributes, err, o, "attributes", false);

  return true;
}

static bool ParseTechniqueParameter(TechniqueParameter *param, std::string *err,
                                    const picojson::object &o) {
  double count = 1;
  ParseNumberProperty(&count, err, o, "count", false);

  double type;
  if (!ParseNumberProperty(&type, err, o, "type", true)) {
    return false;
  }

  ParseStringProperty(&param->node, err, o, "node", false);
  ParseStringProperty(&param->semantic, err, o, "semantic", false);

  ParseParameterProperty(&param->value, err, o, "value", false);

  param->count = static_cast<int>(count);
  param->type = static_cast<int>(type);

  return true;
}

static bool ParseTechnique(Technique *technique, std::string *err,
                           const picojson::object &o) {
  ParseStringProperty(&technique->name, err, o, "name", false);

  if (!ParseStringProperty(&technique->program, err, o, "program", true)) {
    return false;
  }

  ParseStringMapProperty(&technique->attributes, err, o, "attributes", false);
  ParseStringMapProperty(&technique->uniforms, err, o, "uniforms", false);

  technique->parameters.clear();
  picojson::object::const_iterator paramsIt = o.find("parameters");

  // Verify parameters is an object
  if ((paramsIt != o.end()) && (paramsIt->second).is<picojson::object>()) {
    // For each parameter in params_object.
    const picojson::object &params_object =
        (paramsIt->second).get<picojson::object>();

    picojson::object::const_iterator it(params_object.begin());
    picojson::object::const_iterator itEnd(params_object.end());

    for (; it != itEnd; it++) {
      TechniqueParameter param;

      // Skip non-objects
      if (!it->second.is<picojson::object>()) continue;

      // Parse the technique parameter
      const picojson::object &param_obj = it->second.get<picojson::object>();
      if (ParseTechniqueParameter(&param, err, param_obj)) {
        // Add if successful
        technique->parameters[it->first] = param;
      }
    }
  }

  return true;
}

static bool ParseAnimationChannel(AnimationChannel *channel, std::string *err,
                                  const picojson::object &o) {
  if (!ParseStringProperty(&channel->sampler, err, o, "sampler", true)) {
    if (err) {
      (*err) += "`sampler` field is missing in animation channels\n";
    }
    return false;
  }

  picojson::object::const_iterator targetIt = o.find("target");
  if ((targetIt != o.end()) && (targetIt->second).is<picojson::object>()) {
    const picojson::object &target_object =
        (targetIt->second).get<picojson::object>();

    if (!ParseStringProperty(&channel->target_id, err, target_object, "id",
                             true)) {
      if (err) {
        (*err) += "`id` field is missing in animation.channels.target\n";
      }
      return false;
    }

    if (!ParseStringProperty(&channel->target_path, err, target_object, "path",
                             true)) {
      if (err) {
        (*err) += "`path` field is missing in animation.channels.target\n";
      }
      return false;
    }
  }

  return true;
}

static bool ParseAnimation(Animation *animation, std::string *err,
                           const picojson::object &o) {
  {
    picojson::object::const_iterator channelsIt = o.find("channels");
    if ((channelsIt != o.end()) && (channelsIt->second).is<picojson::array>()) {
      const picojson::array &channelArray =
          (channelsIt->second).get<picojson::array>();
      for (size_t i = 0; i < channelArray.size(); i++) {
        AnimationChannel channel;
        if (ParseAnimationChannel(&channel, err,
                                  channelArray[i].get<picojson::object>())) {
          // Only add the channel if the parsing succeeds.
          animation->channels.push_back(channel);
        }
      }
    }
  }

  {
    picojson::object::const_iterator samplerIt = o.find("samplers");
    if ((samplerIt != o.end()) && (samplerIt->second).is<picojson::object>()) {
      const picojson::object &sampler_object =
          (samplerIt->second).get<picojson::object>();

      picojson::object::const_iterator it = sampler_object.begin();
      picojson::object::const_iterator itEnd = sampler_object.end();

      for (; it != itEnd; it++) {
        // Skip non-objects
        if (!it->second.is<picojson::object>()) continue;

        const picojson::object &s = it->second.get<picojson::object>();

        AnimationSampler sampler;
        if (!ParseStringProperty(&sampler.input, err, s, "input", true)) {
          if (err) {
            (*err) += "`input` field is missing in animation.sampler\n";
          }
          return false;
        }
        if (!ParseStringProperty(&sampler.interpolation, err, s,
                                 "interpolation", true)) {
          if (err) {
            (*err) += "`interpolation` field is missing in animation.sampler\n";
          }
          return false;
        }
        if (!ParseStringProperty(&sampler.output, err, s, "output", true)) {
          if (err) {
            (*err) += "`output` field is missing in animation.sampler\n";
          }
          return false;
        }

        animation->samplers[it->first] = sampler;
      }
    }
  }

  picojson::object::const_iterator parametersIt = o.find("parameters");
  if ((parametersIt != o.end()) &&
      (parametersIt->second).is<picojson::object>()) {
    const picojson::object &parameters_object =
        (parametersIt->second).get<picojson::object>();

    picojson::object::const_iterator it(parameters_object.begin());
    picojson::object::const_iterator itEnd(parameters_object.end());

    for (; it != itEnd; it++) {
      Parameter param;
      if (ParseParameterProperty(&param, err, parameters_object, it->first,
                                 false)) {
        animation->parameters[it->first] = param;
      }
    }
  }
  ParseStringProperty(&animation->name, err, o, "name", false);

  return true;
}

static bool ParseSampler(Sampler *sampler, std::string *err,
                         const picojson::object &o) {
  ParseStringProperty(&sampler->name, err, o, "name", false);

  double minFilter =
      static_cast<double>(TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR);
  double magFilter = static_cast<double>(TINYGLTF_TEXTURE_FILTER_LINEAR);
  double wrapS = static_cast<double>(TINYGLTF_TEXTURE_WRAP_RPEAT);
  double wrapT = static_cast<double>(TINYGLTF_TEXTURE_WRAP_RPEAT);
  ParseNumberProperty(&minFilter, err, o, "minFilter", false);
  ParseNumberProperty(&magFilter, err, o, "magFilter", false);
  ParseNumberProperty(&wrapS, err, o, "wrapS", false);
  ParseNumberProperty(&wrapT, err, o, "wrapT", false);

  sampler->minFilter = static_cast<int>(minFilter);
  sampler->magFilter = static_cast<int>(magFilter);
  sampler->wrapS = static_cast<int>(wrapS);
  sampler->wrapT = static_cast<int>(wrapT);

  return true;
}

bool TinyGLTFLoader::LoadFromString(Scene *scene, std::string *err,
                                    const char *str, unsigned int length,
                                    const std::string &base_dir,
                                    unsigned int check_sections) {
  picojson::value v;
  std::string perr = picojson::parse(v, str, str + length);

  if (!perr.empty()) {
    if (err) {
      (*err) = perr;
    }
    return false;
  }

  if (v.contains("scene") && v.get("scene").is<std::string>()) {
    // OK
  } else if (check_sections & REQUIRE_SCENE) {
    if (err) {
      (*err) += "\"scene\" object not found in .gltf\n";
    }
    return false;
  }

  if (v.contains("scenes") && v.get("scenes").is<picojson::object>()) {
    // OK
  } else if (check_sections & REQUIRE_SCENES) {
    if (err) {
      (*err) += "\"scenes\" object not found in .gltf\n";
    }
    return false;
  }

  if (v.contains("nodes") && v.get("nodes").is<picojson::object>()) {
    // OK
  } else if (check_sections & REQUIRE_NODES) {
    if (err) {
      (*err) += "\"nodes\" object not found in .gltf\n";
    }
    return false;
  }

  if (v.contains("accessors") && v.get("accessors").is<picojson::object>()) {
    // OK
  } else if (check_sections & REQUIRE_ACCESSORS) {
    if (err) {
      (*err) += "\"accessors\" object not found in .gltf\n";
    }
    return false;
  }

  if (v.contains("buffers") && v.get("buffers").is<picojson::object>()) {
    // OK
  } else if (check_sections & REQUIRE_BUFFERS) {
    if (err) {
      (*err) += "\"buffers\" object not found in .gltf\n";
    }
    return false;
  }

  if (v.contains("bufferViews") &&
      v.get("bufferViews").is<picojson::object>()) {
    // OK
  } else if (check_sections & REQUIRE_BUFFER_VIEWS) {
    if (err) {
      (*err) += "\"bufferViews\" object not found in .gltf\n";
    }
    return false;
  }

  scene->buffers.clear();
  scene->bufferViews.clear();
  scene->accessors.clear();
  scene->meshes.clear();
  scene->nodes.clear();
  scene->defaultScene = "";

  // 0. Parse Asset
  if (v.contains("asset") && v.get("asset").is<picojson::object>()) {
    const picojson::object &root = v.get("asset").get<picojson::object>();

    ParseAsset(&scene->asset, err, root);
  }

  // 1. Parse Buffer
  if (v.contains("buffers") && v.get("buffers").is<picojson::object>()) {
    const picojson::object &root = v.get("buffers").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; it++) {
      Buffer buffer;
      if (!ParseBuffer(&buffer, err, (it->second).get<picojson::object>(),
                       base_dir, is_binary_, bin_data_, bin_size_)) {
        return false;
      }

      scene->buffers[it->first] = buffer;
    }
  }

  // 2. Parse BufferView
  if (v.contains("bufferViews") &&
      v.get("bufferViews").is<picojson::object>()) {
    const picojson::object &root = v.get("bufferViews").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; it++) {
      BufferView bufferView;
      if (!ParseBufferView(&bufferView, err,
                           (it->second).get<picojson::object>())) {
        return false;
      }

      scene->bufferViews[it->first] = bufferView;
    }
  }

  // 3. Parse Accessor
  if (v.contains("accessors") && v.get("accessors").is<picojson::object>()) {
    const picojson::object &root = v.get("accessors").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; it++) {
      Accessor accessor;
      if (!ParseAccessor(&accessor, err,
                         (it->second).get<picojson::object>())) {
        return false;
      }

      scene->accessors[it->first] = accessor;
    }
  }

  // 4. Parse Mesh
  if (v.contains("meshes") && v.get("meshes").is<picojson::object>()) {
    const picojson::object &root = v.get("meshes").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; it++) {
      Mesh mesh;
      if (!ParseMesh(&mesh, err, (it->second).get<picojson::object>())) {
        return false;
      }

      scene->meshes[it->first] = mesh;
    }
  }

  // 5. Parse Node
  if (v.contains("nodes") && v.get("nodes").is<picojson::object>()) {
    const picojson::object &root = v.get("nodes").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; it++) {
      Node node;
      if (!ParseNode(&node, err, (it->second).get<picojson::object>())) {
        return false;
      }

      scene->nodes[it->first] = node;
    }
  }

  // 6. Parse scenes.
  if (v.contains("scenes") && v.get("scenes").is<picojson::object>()) {
    const picojson::object &root = v.get("scenes").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; it++) {
      if (!((it->second).is<picojson::object>())) {
        if (err) {
          (*err) += "`scenes' does not contain an object.";
        }
        return false;
      }
      const picojson::object &o = (it->second).get<picojson::object>();
      std::vector<std::string> nodes;
      if (!ParseStringArrayProperty(&nodes, err, o, "nodes", false)) {
        return false;
      }

      scene->scenes[it->first] = nodes;
    }
  }

  // 7. Parse default scenes.
  if (v.contains("scene") && v.get("scene").is<std::string>()) {
    const std::string defaultScene = v.get("scene").get<std::string>();

    scene->defaultScene = defaultScene;
  }

  // 8. Parse Material
  if (v.contains("materials") && v.get("materials").is<picojson::object>()) {
    const picojson::object &root = v.get("materials").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; it++) {
      Material material;
      if (!ParseMaterial(&material, err,
                         (it->second).get<picojson::object>())) {
        return false;
      }

      scene->materials[it->first] = material;
    }
  }

  // 9. Parse Image
  if (v.contains("images") && v.get("images").is<picojson::object>()) {
    const picojson::object &root = v.get("images").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; it++) {
      Image image;
      if (!ParseImage(&image, err, (it->second).get<picojson::object>(),
                      base_dir, is_binary_, bin_data_, bin_size_)) {
        return false;
      }

      if (!image.bufferView.empty()) {
        // Load image from the buffer view.
        if (scene->bufferViews.find(image.bufferView) ==
            scene->bufferViews.end()) {
          if (err) {
            std::stringstream ss;
            ss << "bufferView \"" << image.bufferView
               << "\" not found in the scene." << std::endl;
            (*err) += ss.str();
          }
          return false;
        }

        const BufferView &bufferView = scene->bufferViews[image.bufferView];
        const Buffer &buffer = scene->buffers[bufferView.buffer];

        bool ret = LoadImageData(&image, err, image.width, image.height,
                                 &buffer.data[bufferView.byteOffset],
                                 static_cast<int>(bufferView.byteLength));
        if (!ret) {
          return false;
        }
      }

      scene->images[it->first] = image;
    }
  }

  // 10. Parse Texture
  if (v.contains("textures") && v.get("textures").is<picojson::object>()) {
    const picojson::object &root = v.get("textures").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; it++) {
      Texture texture;
      if (!ParseTexture(&texture, err, (it->second).get<picojson::object>(),
                        base_dir)) {
        return false;
      }

      scene->textures[it->first] = texture;
    }
  }

  // 11. Parse Shader
  if (v.contains("shaders") && v.get("shaders").is<picojson::object>()) {
    const picojson::object &root = v.get("shaders").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; ++it) {
      Shader shader;
      if (!ParseShader(&shader, err, (it->second).get<picojson::object>(),
                       base_dir, is_binary_, bin_data_, bin_size_)) {
        return false;
      }

      scene->shaders[it->first] = shader;
    }
  }

  // 12. Parse Program
  if (v.contains("programs") && v.get("programs").is<picojson::object>()) {
    const picojson::object &root = v.get("programs").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; ++it) {
      Program program;
      if (!ParseProgram(&program, err, (it->second).get<picojson::object>())) {
        return false;
      }

      scene->programs[it->first] = program;
    }
  }

  // 13. Parse Technique
  if (v.contains("techniques") && v.get("techniques").is<picojson::object>()) {
    const picojson::object &root = v.get("techniques").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; ++it) {
      Technique technique;
      if (!ParseTechnique(&technique, err,
                          (it->second).get<picojson::object>())) {
        return false;
      }

      scene->techniques[it->first] = technique;
    }
  }

  // 14. Parse Animation
  if (v.contains("animations") && v.get("animations").is<picojson::object>()) {
    const picojson::object &root = v.get("animations").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; ++it) {
      Animation animation;
      if (!ParseAnimation(&animation, err,
                          (it->second).get<picojson::object>())) {
        return false;
      }

      scene->animations[it->first] = animation;
    }
  }

  // 15. Parse Sampler
  if (v.contains("samplers") && v.get("samplers").is<picojson::object>()) {
    const picojson::object &root = v.get("samplers").get<picojson::object>();

    picojson::object::const_iterator it(root.begin());
    picojson::object::const_iterator itEnd(root.end());
    for (; it != itEnd; ++it) {
      Sampler sampler;
      if (!ParseSampler(&sampler, err, (it->second).get<picojson::object>())) {
        return false;
      }

      scene->samplers[it->first] = sampler;
    }
  }
  return true;
}

bool TinyGLTFLoader::LoadASCIIFromString(Scene *scene, std::string *err,
                                         const char *str, unsigned int length,
                                         const std::string &base_dir,
                                         unsigned int check_sections) {
  is_binary_ = false;
  bin_data_ = NULL;
  bin_size_ = 0;

  return LoadFromString(scene, err, str, length, base_dir, check_sections);
}

bool TinyGLTFLoader::LoadASCIIFromFile(Scene *scene, std::string *err,
                                       const std::string &filename,
                                       unsigned int check_sections) {
  std::stringstream ss;

  std::ifstream f(filename.c_str());
  if (!f) {
    ss << "Failed to open file: " << filename << std::endl;
    if (err) {
      (*err) = ss.str();
    }
    return false;
  }

  f.seekg(0, f.end);
  size_t sz = static_cast<size_t>(f.tellg());
  std::vector<char> buf(sz);

  if (sz == 0) {
    if (err) {
      (*err) = "Empty file.";
    }
    return false;
  }

  f.seekg(0, f.beg);
  f.read(&buf.at(0), static_cast<std::streamsize>(sz));
  f.close();

  std::string basedir = GetBaseDir(filename);

  bool ret = LoadASCIIFromString(scene, err, &buf.at(0),
                                 static_cast<unsigned int>(buf.size()), basedir,
                                 check_sections);


  return ret;
}

bool TinyGLTFLoader::LoadBinaryFromMemory(Scene *scene, std::string *err,
                                          const unsigned char *bytes,
                                          unsigned int size,
                                          const std::string &base_dir,
                                          unsigned int check_sections) {
  if (size < 20) {
    if (err) {
      (*err) = "Too short data size for glTF Binary.";
    }
    return false;
  }

  if (bytes[0] == 'g' && bytes[1] == 'l' && bytes[2] == 'T' &&
      bytes[3] == 'F') {
    // ok
  } else {
    if (err) {
      (*err) = "Invalid magic.";
    }
    return false;
  }

  unsigned int version;       // 4 bytes
  unsigned int length;        // 4 bytes
  unsigned int scene_length;  // 4 bytes
  unsigned int scene_format;  // 4 bytes;

  // @todo { Endian swap for big endian machine. }
  memcpy(&version, bytes + 4, 4);
  swap4(&version);
  memcpy(&length, bytes + 8, 4);
  swap4(&length);
  memcpy(&scene_length, bytes + 12, 4);
  swap4(&scene_length);
  memcpy(&scene_format, bytes + 16, 4);
  swap4(&scene_format);

  if ((20 + scene_length >= size) || (scene_length < 1) ||
      (scene_format != 0)) {  // 0 = JSON format.
    if (err) {
      (*err) = "Invalid glTF binary.";
    }
    return false;
  }

  // Extract JSON string.
  std::string jsonString(reinterpret_cast<const char *>(&bytes[20]),
                         scene_length);

  is_binary_ = true;
  bin_data_ = bytes + 20 + scene_length;
  bin_size_ =
      length - (20 + scene_length);  // extract header + JSON scene data.

  bool ret =
      LoadFromString(scene, err, reinterpret_cast<const char *>(&bytes[20]),
                     scene_length, base_dir, check_sections);
  if (!ret) {
    return ret;
  }

  return true;
}

bool TinyGLTFLoader::LoadBinaryFromFile(Scene *scene, std::string *err,
                                        const std::string &filename,
                                        unsigned int check_sections) {
  std::stringstream ss;

  std::ifstream f(filename.c_str(), std::ios::binary);
  if (!f) {
    ss << "Failed to open file: " << filename << std::endl;
    if (err) {
      (*err) = ss.str();
    }
    return false;
  }

  f.seekg(0, f.end);
  size_t sz = static_cast<size_t>(f.tellg());
  std::vector<char> buf(sz);

  f.seekg(0, f.beg);
  f.read(&buf.at(0), static_cast<std::streamsize>(sz));
  f.close();

  std::string basedir = GetBaseDir(filename);

  bool ret = LoadBinaryFromMemory(
      scene, err, reinterpret_cast<unsigned char *>(&buf.at(0)),
      static_cast<unsigned int>(buf.size()), basedir, check_sections);

  return ret;
}

}  // namespace tinygltf

#endif  // TINYGLTF_LOADER_IMPLEMENTATION

#endif  // TINY_GLTF_LOADER_H_
