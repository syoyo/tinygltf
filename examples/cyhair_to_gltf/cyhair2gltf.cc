#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fstream>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++11-long-long"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wc++11-extensions"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#define PICOJSON_USE_INT64
#include "../../picojson.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "../../tiny_gltf_loader.h" // To import some TINYGLTF_*** macros.

#include "cyhair_loader.h"

// Curves are represented as an array of curve.
// i'th curve has nverts[i] points.
// TODO(syoyo) knots, order to support NURBS curve.
typedef struct
{
  std::vector<float> points;
  std::vector<int> nverts;  // # of vertices per strand(curve).
} Curves;

// ----------------------------------------------------------------
// writer module
// @todo { move writer code to tiny_gltf_writer.h }

// http://www.adp-gmbh.ch/cpp/common/base64.html
static const char *base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::string base64_encode(unsigned char const* bytes_to_encode,
                          size_t in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = static_cast<unsigned char>(
          ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
      char_array_4[2] = static_cast<unsigned char>(
          ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4); i++) ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = static_cast<unsigned char>(
        ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
    char_array_4[2] = static_cast<unsigned char>(
        ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];

    while ((i++ < 3)) ret += '=';
  }

  return ret;
}

static bool SaveCurvesToGLTF(const std::string& output_filename,
              const Curves& curves) {
  picojson::object root;

  {
    picojson::object asset;
    asset["generator"] = picojson::value("abc2gltf");
    asset["premultipliedAlpha"] = picojson::value(true);
    asset["version"] = picojson::value(static_cast<double>(1));
    picojson::object profile;
    profile["api"] = picojson::value("WebGL");
    profile["version"] = picojson::value("1.0.2");
    asset["profile"] = picojson::value(profile);
    root["assets"] = picojson::value(asset);
  }

  {
    picojson::object buffers;
    {
      {
        std::string b64data = base64_encode(reinterpret_cast<unsigned char const*>(curves.points.data()), curves.points.size() * sizeof(float));
        picojson::object buf;

        buf["type"] = picojson::value("arraybuffer");
        buf["uri"] = picojson::value(
            std::string("data:application/octet-stream;base64,") + b64data);
        buf["byteLength"] =
            picojson::value(static_cast<double>(curves.points.size() * sizeof(float)));
        
        buffers["points"] = picojson::value(buf); 
      }

      // Out extension
      {
        std::string b64data = base64_encode(reinterpret_cast<unsigned char const*>(curves.nverts.data()), curves.nverts.size() * sizeof(int));
        picojson::object buf;

        buf["type"] = picojson::value("arraybuffer");
        buf["uri"] = picojson::value(
            std::string("data:application/octet-stream;base64,") + b64data);
        buf["byteLength"] =
            picojson::value(static_cast<double>(curves.nverts.size() * sizeof(int)));
        
        buffers["nverts"] = picojson::value(buf); 
      }

    }
    root["buffers"] = picojson::value(buffers);
  }

  {
    picojson::object buffer_views;
    {
      {
        picojson::object buffer_view_points;
        buffer_view_points["buffer"] = picojson::value(std::string("points"));    
        buffer_view_points["byteLength"] = picojson::value(static_cast<double>(curves.points.size() * sizeof(float)));
        buffer_view_points["byteOffset"] = picojson::value(static_cast<double>(0));
        buffer_view_points["target"] = picojson::value(static_cast<int64_t>(TINYGLTF_TARGET_ARRAY_BUFFER));
        buffer_views["bufferView_points"] = picojson::value(buffer_view_points);
      }

      {
        picojson::object buffer_view_nverts;
        buffer_view_nverts["buffer"] = picojson::value(std::string("nverts"));    
        buffer_view_nverts["byteLength"] = picojson::value(static_cast<double>(curves.nverts.size() * sizeof(int)));
        buffer_view_nverts["byteOffset"] = picojson::value(static_cast<double>(0));
        buffer_view_nverts["target"] = picojson::value(static_cast<int64_t>(TINYGLTF_TARGET_ARRAY_BUFFER));
        buffer_views["bufferView_nverts"] = picojson::value(buffer_view_nverts);
      }

    }

    root["bufferViews"] = picojson::value(buffer_views);
  }

  {
    picojson::object attributes;
  
    attributes["POSITION"] = picojson::value(std::string("accessor_points"));
    attributes["NVERTS"] = picojson::value(std::string("accessor_nverts"));

    // Extra information for curves primtive.
    picojson::object extra;
    extra["ext_mode"] = picojson::value("curves");
    
    picojson::object primitive;
    primitive["attributes"] = picojson::value(attributes);
    //primitive["indices"] = picojson::value("accessor_indices");
    primitive["material"] = picojson::value("material_1");
    primitive["mode"] = picojson::value(static_cast<int64_t>(TINYGLTF_MODE_POINTS)); // Use GL_POINTS for backward compatibility
    primitive["extras"] = picojson::value(extra);


    picojson::array primitive_array;
    primitive_array.push_back(picojson::value(primitive));

    picojson::object m;
    m["primitives"] = picojson::value(primitive_array);

    picojson::object meshes;
    meshes["mesh_1"] = picojson::value(m);

    
    root["meshes"] = picojson::value(meshes);
  }

  {
    picojson::object accessors;

    {
      picojson::object accessor_points;
      accessor_points["bufferView"] = picojson::value(std::string("bufferView_points"));
      accessor_points["byteOffset"] = picojson::value(static_cast<int64_t>(0));
      accessor_points["byteStride"] = picojson::value(static_cast<double>(3 * sizeof(float)));
      accessor_points["componentType"] = picojson::value(static_cast<int64_t>(TINYGLTF_COMPONENT_TYPE_FLOAT));
      accessor_points["count"] = picojson::value(static_cast<int64_t>(curves.points.size()));
      accessor_points["type"] = picojson::value(std::string("VEC3"));
      accessors["accessor_points"] = picojson::value(accessor_points);
    }

    {
      picojson::object accessor_nverts;
      accessor_nverts["bufferView"] = picojson::value(std::string("bufferView_nverts"));
      accessor_nverts["byteOffset"] = picojson::value(static_cast<int64_t>(0));
      accessor_nverts["byteStride"] = picojson::value(static_cast<double>(sizeof(int)));
      accessor_nverts["componentType"] = picojson::value(static_cast<int64_t>(TINYGLTF_COMPONENT_TYPE_INT));
      accessor_nverts["count"] = picojson::value(static_cast<int64_t>(curves.nverts.size()));
      accessor_nverts["type"] = picojson::value(std::string("SCALAR"));
      accessors["accessor_nverts"] = picojson::value(accessor_nverts);
    }

    picojson::object accessor_indices;

    root["accessors"] = picojson::value(accessors);
  }

  {
    // Use Default Material(Do not supply `material.technique`)
    picojson::object default_material;
    picojson::object materials;

    materials["material_1"] = picojson::value(default_material);

    root["materials"] = picojson::value(materials);

  }

  {
    picojson::object nodes;
    picojson::object node;
    picojson::array  meshes;

    meshes.push_back(picojson::value(std::string("mesh_1")));

    node["meshes"] = picojson::value(meshes);

    nodes["node_1"] = picojson::value(node);
    root["nodes"] = picojson::value(nodes);
  }

  {
    picojson::object defaultScene;
    picojson::array nodes;
    
    nodes.push_back(picojson::value(std::string("node_1")));

    defaultScene["nodes"] = picojson::value(nodes);

    root["scene"] = picojson::value("defaultScene");
    picojson::object scenes;
    scenes["defaultScene"] = picojson::value(defaultScene);
    root["scenes"] = picojson::value(scenes);
  }


  // @todo {}
  picojson::object shaders;
  picojson::object programs;
  picojson::object techniques;
  picojson::object materials;
  picojson::object skins;
  root["shaders"] = picojson::value(shaders);
  root["programs"] = picojson::value(programs);
  root["techniques"] = picojson::value(techniques);
  root["materials"] = picojson::value(materials);
  root["skins"] = picojson::value(skins);

  std::ofstream ifs(output_filename.c_str());
  if (ifs.bad()) {
    std::cerr << "Failed to open " << output_filename << std::endl;
    return false;
  }

  picojson::value v = picojson::value(root);

  std::string s = v.serialize(/* pretty */true);
  ifs.write(s.data(), static_cast<ssize_t>(s.size()));
  ifs.close();

  return true;
}


int main(int argc, char** argv) {
  std::string cyhair_filename;
  std::string gltf_filename;

  if (argc < 3) {
    std::cerr << "Usage: cyhair2abc input.hair output.gltf" << std::endl;
    return EXIT_FAILURE;
  }

  cyhair_filename = std::string(argv[1]);
  gltf_filename = std::string(argv[2]);

  example::CyHair cyhair;
  {
    bool ret = cyhair.Load(cyhair_filename.c_str()); 
    if (!ret) {
      std::cerr << "Failed to load " << cyhair_filename << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Convert to curves.
  Curves curves;
  {
    // TODO(syoyo): thickness, colors, etc.
    curves.points = cyhair.points_;

    // NVETS = numSegments + 1
    if (cyhair.segments_.empty()) {
      for (size_t i = 0; i < cyhair.num_strands_; i++) {
        curves.nverts.push_back(cyhair.default_segments_ + 1);
      }
    } else {
      for (size_t i = 0; i < cyhair.segments_.size(); i++) {
        curves.nverts.push_back(cyhair.segments_[i] + 1);
      }
    }
  }

  bool ret = SaveCurvesToGLTF(gltf_filename, curves);
  if (ret) {
    std::cout << "Wrote " << gltf_filename << std::endl;
  } else {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
