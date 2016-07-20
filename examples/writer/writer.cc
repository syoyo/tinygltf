#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#define TINYGLTF_LOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "../../tiny_gltf_loader.h"

static std::string GetFilePathExtension(const std::string& filename) {
  if (filename.find_last_of(".") != std::string::npos)
    return filename.substr(filename.find_last_of(".") + 1);
  return "";
}

// ----------------------------------------------------------------
// writer module
// @todo { move writer code to tiny_gltf_writer.h }

static std::string EncodeType(int ty) {
  if (ty == TINYGLTF_TYPE_SCALAR) {
    return "SCALAR";
  } else if (ty == TINYGLTF_TYPE_VECTOR) {
    return "VECTOR";
  } else if (ty == TINYGLTF_TYPE_VEC2) {
    return "VEC2";
  } else if (ty == TINYGLTF_TYPE_VEC3) {
    return "VEC3";
  } else if (ty == TINYGLTF_TYPE_VEC4) {
    return "VEC4";
  } else if (ty == TINYGLTF_TYPE_MATRIX) {
    return "MATRIX";
  } else if (ty == TINYGLTF_TYPE_MAT2) {
    return "MAT2";
  } else if (ty == TINYGLTF_TYPE_MAT3) {
    return "MAT3";
  } else if (ty == TINYGLTF_TYPE_MAT4) {
    return "MAT4";
  }
  return "**UNKNOWN**";
}

// http://www.adp-gmbh.ch/cpp/common/base64.html
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64_encode(unsigned char const* bytes_to_encode,
                          unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] =
          ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] =
          ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4); i++) ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] =
        ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] =
        ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];

    while ((i++ < 3)) ret += '=';
  }

  return ret;
}

bool EncodeBuffers(picojson::object* o,
                   const std::map<std::string, tinygltf::Buffer>& buffers) {
  std::map<std::string, tinygltf::Buffer>::const_iterator it(buffers.begin());
  std::map<std::string, tinygltf::Buffer>::const_iterator itEnd(buffers.end());
  for (; it != itEnd; it++) {
    // @todo { Support external file resource. }
    picojson::object buf;
    std::string b64_data =
        base64_encode(it->second.data.data(), it->second.data.size());
    buf["type"] = picojson::value("arraybuffer");
    buf["uri"] = picojson::value(
        std::string("data:application/octet-stream;base64,") + b64_data);
    buf["byteLength"] =
        picojson::value(static_cast<double>(it->second.data.size()));

    (*o)[it->first] = picojson::value(buf);
  }

  return true;
}

bool EncodeBufferViews(
    picojson::object* o,
    const std::map<std::string, tinygltf::BufferView>& bufferViews) {
  std::map<std::string, tinygltf::BufferView>::const_iterator it(
      bufferViews.begin());
  std::map<std::string, tinygltf::BufferView>::const_iterator itEnd(
      bufferViews.end());

  for (; it != itEnd; it++) {
    picojson::object buf;
    buf["buffer"] = picojson::value(it->second.buffer);
    buf["byteLength"] =
        picojson::value(static_cast<double>(it->second.byteLength));
    buf["byteOffset"] =
        picojson::value(static_cast<double>(it->second.byteOffset));
    buf["target"] = picojson::value(static_cast<double>(it->second.target));

    (*o)[it->first] = picojson::value(buf);
  }

  return true;
}

bool EncodeFloatArray(picojson::array* arr, const std::vector<double>& values) {
  for (size_t i = 0; i < values.size(); i++) {
    arr->push_back(picojson::value(values[i]));
  }

  return true;
}

bool EncodeStringArray(picojson::array* arr,
                       const std::vector<std::string>& values) {
  for (size_t i = 0; i < values.size(); i++) {
    arr->push_back(picojson::value(values[i]));
  }

  return true;
}

bool EncodeNode(picojson::object* o, const tinygltf::Node& node) {
  (*o)["name"] = picojson::value(node.name);
  (*o)["camera"] = picojson::value(node.camera);

  if (!node.rotation.empty()) {
    picojson::array arr;
    EncodeFloatArray(&arr, node.rotation);
    (*o)["rotation"] = picojson::value(arr);
  }
  if (!node.scale.empty()) {
    picojson::array arr;
    EncodeFloatArray(&arr, node.scale);
    (*o)["scale"] = picojson::value(arr);
  }
  if (!node.translation.empty()) {
    picojson::array arr;
    EncodeFloatArray(&arr, node.translation);
    (*o)["translation"] = picojson::value(arr);
  }

  if (!node.matrix.empty()) {
    picojson::array arr;
    EncodeFloatArray(&arr, node.matrix);
    (*o)["matrix"] = picojson::value(arr);
  }

  if (!node.meshes.empty()) {
    picojson::array arr;
    EncodeStringArray(&arr, node.meshes);
    (*o)["meshes"] = picojson::value(arr);
  }

  if (!node.children.empty()) {
    picojson::array arr;
    EncodeStringArray(&arr, node.children);
    (*o)["children"] = picojson::value(arr);
  }

  return true;
}

bool EncodeNodes(picojson::object* o,
                 const std::map<std::string, tinygltf::Node>& nodes) {
  std::map<std::string, tinygltf::Node>::const_iterator it(nodes.begin());
  std::map<std::string, tinygltf::Node>::const_iterator itEnd(nodes.end());

  for (; it != itEnd; it++) {
    picojson::object node;
    EncodeNode(&node, it->second);

    (*o)[it->first] = picojson::value(node);
  }

  return true;
}

bool EncodeScenes(
    picojson::object* o,
    const std::map<std::string, std::vector<std::string> >& scenes) {
  std::map<std::string, std::vector<std::string> >::const_iterator it(
      scenes.begin());
  std::map<std::string, std::vector<std::string> >::const_iterator itEnd(
      scenes.end());

  for (; it != itEnd; it++) {
    picojson::object buf;
    picojson::array arr;
    for (size_t i = 0; i < it->second.size(); i++) {
      arr.push_back(picojson::value(it->second[i]));
    }

    buf["nodes"] = picojson::value(arr);

    (*o)[it->first] = picojson::value(buf);
  }

  return true;
}

bool EncodeAccessors(
    picojson::object* o,
    const std::map<std::string, tinygltf::Accessor>& accessors) {
  std::map<std::string, tinygltf::Accessor>::const_iterator it(
      accessors.begin());
  std::map<std::string, tinygltf::Accessor>::const_iterator itEnd(
      accessors.end());
  for (; it != itEnd; it++) {
    picojson::object buf;
    buf["bufferView"] = picojson::value(it->second.bufferView);
    buf["byteOffset"] =
        picojson::value(static_cast<double>(it->second.byteOffset));
    buf["byteStride"] =
        picojson::value(static_cast<double>(it->second.byteStride));
    buf["componentType"] =
        picojson::value(static_cast<double>(it->second.componentType));
    buf["count"] = picojson::value(static_cast<double>(it->second.count));
    buf["type"] = picojson::value(EncodeType(it->second.type));

    if (!it->second.minValues.empty()) {
      picojson::array arr;
      EncodeFloatArray(&arr, it->second.minValues);
      buf["min"] = picojson::value(arr);
    }
    if (!it->second.maxValues.empty()) {
      picojson::array arr;
      EncodeFloatArray(&arr, it->second.maxValues);
      buf["max"] = picojson::value(arr);
    }

    (*o)[it->first] = picojson::value(buf);
  }

  return true;
}

bool EncodePrimitive(picojson::object* o,
                     const tinygltf::Primitive& primitive) {
  (*o)["material"] = picojson::value(primitive.material);
  (*o)["indices"] = picojson::value(primitive.indices);
  (*o)["mode"] = picojson::value(static_cast<double>(primitive.mode));

  std::map<std::string, std::string>::const_iterator it(
      primitive.attributes.begin());
  std::map<std::string, std::string>::const_iterator itEnd(
      primitive.attributes.end());

  picojson::object attributes;
  for (; it != itEnd; it++) {
    picojson::object buf;
    attributes[it->first] = picojson::value(it->second);
  }

  (*o)["attributes"] = picojson::value(attributes);

  return true;
}

bool EncodeMeshes(picojson::object* o,
                  const std::map<std::string, tinygltf::Mesh>& meshes) {
  std::map<std::string, tinygltf::Mesh>::const_iterator it(meshes.begin());
  std::map<std::string, tinygltf::Mesh>::const_iterator itEnd(meshes.end());
  for (; it != itEnd; it++) {
    picojson::object buf;

    buf["name"] = picojson::value(it->second.name);

    picojson::array arr;
    for (size_t i = 0; i < it->second.primitives.size(); i++) {
      picojson::object primitive;
      EncodePrimitive(&primitive, it->second.primitives[i]);
      arr.push_back(picojson::value(primitive));
    }
    buf["primitives"] = picojson::value(arr);

    (*o)[it->first] = picojson::value(buf);
  }
  return true;
}

bool SaveGLTF(const std::string& output_filename,
              const tinygltf::Scene& scene) {
  picojson::object root;

  {
    picojson::object asset;
    asset["generator"] = picojson::value("tinygltf_writer");
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
    bool ret = EncodeBuffers(&buffers, scene.buffers);
    assert(ret);
    root["buffers"] = picojson::value(buffers);
  }

  {
    picojson::object bufferViews;
    bool ret = EncodeBufferViews(&bufferViews, scene.bufferViews);
    assert(ret);
    root["bufferViews"] = picojson::value(bufferViews);
  }

  {
    picojson::object accessors;
    bool ret = EncodeAccessors(&accessors, scene.accessors);
    assert(ret);
    root["accessors"] = picojson::value(accessors);
  }

  {
    picojson::object meshes;
    bool ret = EncodeMeshes(&meshes, scene.meshes);
    assert(ret);
    root["meshes"] = picojson::value(meshes);
  }

  {
    picojson::object nodes;
    bool ret = EncodeNodes(&nodes, scene.nodes);
    assert(ret);
    root["nodes"] = picojson::value(nodes);
  }

  root["scene"] = picojson::value(scene.defaultScene);
  {
    picojson::object scenes;
    bool ret = EncodeScenes(&scenes, scene.scenes);
    assert(ret);
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

  picojson::value v = picojson::value(root);

  std::ofstream ifs(output_filename.c_str());
  if (ifs.bad()) {
    std::cerr << "Failed to open " << output_filename << std::endl;
    return false;
  }

  std::string s = v.serialize(/* pretty */true);
  ifs.write(s.data(), s.size());
  ifs.close();

  return true;
}

// ----------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc < 3) {
    printf("Needs input.gltf output.gltf\n");
    exit(1);
  }

  tinygltf::Scene scene;
  tinygltf::TinyGLTFLoader loader;
  std::string err;
  std::string input_filename(argv[1]);
  std::string ext = GetFilePathExtension(input_filename);

  bool ret = false;
  if (ext.compare("glb") == 0) {
    // assume binary glTF.
    ret = loader.LoadBinaryFromFile(&scene, &err, input_filename.c_str());
  } else {
    // assume ascii glTF.
    ret = loader.LoadASCIIFromFile(&scene, &err, input_filename.c_str());
  }

  if (!err.empty()) {
    printf("Err: %s\n", err.c_str());
  }

  if (!ret) {
    printf("Failed to parse glTF\n");
    return -1;
  }

  ret = SaveGLTF(argv[2], scene);

  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
