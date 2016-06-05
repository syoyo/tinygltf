#define TINYGLTF_LOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf_loader.h"

#include <fstream>
#include <cstdio>
#include <iostream>

static std::string GetFilePathExtension(const std::string &FileName) {
  if (FileName.find_last_of(".") != std::string::npos)
    return FileName.substr(FileName.find_last_of(".") + 1);
  return "";
}

static std::string PrintMode(int mode) {
  if (mode == TINYGLTF_MODE_POINTS) {
    return "POINTS";
  } else if (mode == TINYGLTF_MODE_LINE) {
    return "LINE";
  } else if (mode == TINYGLTF_MODE_LINE_LOOP) {
    return "LINE_LOOP";
  } else if (mode == TINYGLTF_MODE_TRIANGLES) {
    return "TRIANGLES";
  } else if (mode == TINYGLTF_MODE_TRIANGLE_FAN) {
    return "TRIANGLE_FAN";
  } else if (mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
    return "TRIANGLE_STRIP";
  }
  return "**UNKNOWN**";
}

static std::string PrintType(int ty) {
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

static std::string PrintComponentType(int ty) {
  if (ty == TINYGLTF_COMPONENT_TYPE_BYTE) {
    return "BYTE";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
    return "UNSIGNED_BYTE";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_SHORT) {
    return "SHORT";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    return "UNSIGNED_SHORT";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_INT) {
    return "INT";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
    return "UNSIGNED_INT";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_FLOAT) {
    return "FLOAT";
  } else if (ty == TINYGLTF_COMPONENT_TYPE_DOUBLE) {
    return "DOUBLE";
  }

  return "**UNKNOWN**";
}

static std::string PrintFloatArray(const std::vector<double> &arr) {
  if (arr.size() == 0) {
    return "";
  }

  std::stringstream ss;
  ss << "[ ";
  for (size_t i = 0; i < arr.size(); i++) {
    ss << arr[i] << ((i != arr.size() - 1) ? ", " : "");
  }
  ss << " ]";

  return ss.str();
}

static std::string PrintStringArray(const std::vector<std::string> &arr) {
  if (arr.size() == 0) {
    return "";
  }

  std::stringstream ss;
  ss << "[ ";
  for (size_t i = 0; i < arr.size(); i++) {
    ss << arr[i] << ((i != arr.size() - 1) ? ", " : "");
  }
  ss << " ]";

  return ss.str();
}

static std::string Indent(int indent) {
  std::string s;
  for (int i = 0; i < indent; i++) {
    s += "  ";
  }

  return s;
}

static void DumpNode(const tinygltf::Node &node, int indent) {
  std::cout << Indent(indent) << "name        : " << node.name << std::endl;
  std::cout << Indent(indent) << "camera      : " << node.camera << std::endl;
  if (!node.rotation.empty()) {
    std::cout << Indent(indent)
              << "rotation    : " << PrintFloatArray(node.rotation)
              << std::endl;
  }
  if (!node.scale.empty()) {
    std::cout << Indent(indent)
              << "scale       : " << PrintFloatArray(node.scale) << std::endl;
  }
  if (!node.translation.empty()) {
    std::cout << Indent(indent)
              << "translation : " << PrintFloatArray(node.translation)
              << std::endl;
  }

  if (!node.matrix.empty()) {
    std::cout << Indent(indent)
              << "matrix      : " << PrintFloatArray(node.matrix) << std::endl;
  }

  std::cout << Indent(indent)
            << "meshes      : " << PrintStringArray(node.meshes) << std::endl;

  std::cout << Indent(indent)
            << "children    : " << PrintStringArray(node.children) << std::endl;
}

static void DumpPrimitive(const tinygltf::Primitive &primitive, int indent) {
  std::cout << Indent(indent) << "material : " << primitive.material
            << std::endl;
  std::cout << Indent(indent) << "indices : " << primitive.indices
            << std::endl;
  std::cout << Indent(indent) << "mode     : " << PrintMode(primitive.mode)
            << "(" << primitive.mode << ")" << std::endl;
  std::cout << Indent(indent)
            << "attributes(items=" << primitive.attributes.size() << ")"
            << std::endl;
  std::map<std::string, std::string>::const_iterator it(
      primitive.attributes.begin());
  std::map<std::string, std::string>::const_iterator itEnd(
      primitive.attributes.end());
  for (; it != itEnd; it++) {
    std::cout << Indent(indent + 1) << it->first << ": " << it->second
              << std::endl;
  }
}

static void Dump(const tinygltf::Scene &scene) {
  std::cout << "=== Dump glTF ===" << std::endl;
  std::cout << "asset.generator          : " << scene.asset.generator
            << std::endl;
  std::cout << "asset.premultipliedAlpha : " << scene.asset.premultipliedAlpha
            << std::endl;
  std::cout << "asset.version            : " << scene.asset.version
            << std::endl;
  std::cout << "asset.profile.api        : " << scene.asset.profile_api
            << std::endl;
  std::cout << "asset.profile.version    : " << scene.asset.profile_version
            << std::endl;
  std::cout << std::endl;
  std::cout << "=== Dump scene ===" << std::endl;
  std::cout << "defaultScene: " << scene.defaultScene << std::endl;

  {
    std::map<std::string, std::vector<std::string> >::const_iterator it(
        scene.scenes.begin());
    std::map<std::string, std::vector<std::string> >::const_iterator itEnd(
        scene.scenes.end());
    std::cout << "scenes(items=" << scene.scenes.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name  : " << it->first << std::endl;
      std::cout << Indent(2) << "nodes : [ ";
      for (size_t i = 0; i < it->second.size(); i++) {
        std::cout << it->second[i]
                  << ((i != (it->second.size() - 1)) ? ", " : "");
      }
      std::cout << " ] " << std::endl;
    }
  }

  {
    std::map<std::string, tinygltf::Mesh>::const_iterator it(scene.meshes.begin());
    std::map<std::string, tinygltf::Mesh>::const_iterator itEnd(scene.meshes.end());
    std::cout << "meshes(item=" << scene.meshes.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name     : " << it->second.name << std::endl;
      std::cout << Indent(1)
                << "primitives(items=" << it->second.primitives.size()
                << "): " << std::endl;

      for (size_t i = 0; i < it->second.primitives.size(); i++) {
        DumpPrimitive(it->second.primitives[i], 2);
      }
    }
  }

  {
    std::map<std::string, tinygltf::Accessor>::const_iterator it(scene.accessors.begin());
    std::map<std::string, tinygltf::Accessor>::const_iterator itEnd(
        scene.accessors.end());
    std::cout << "accessos(items=" << scene.accessors.size() << ")"
              << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;
      std::cout << Indent(2) << "bufferView   : " << it->second.bufferView
                << std::endl;
      std::cout << Indent(2) << "byteOffset   : " << it->second.byteOffset
                << std::endl;
      std::cout << Indent(2) << "byteStride   : " << it->second.byteStride
                << std::endl;
      std::cout << Indent(2) << "componentType: "
                << PrintComponentType(it->second.componentType) << "("
                << it->second.componentType << ")" << std::endl;
      std::cout << Indent(2) << "count        : " << it->second.count
                << std::endl;
      std::cout << Indent(2) << "type         : " << PrintType(it->second.type)
                << std::endl;
      if (!it->second.minValues.empty()) {
        std::cout << Indent(2) << "min          : [";
        for (size_t i = 0; i < it->second.minValues.size(); i++) {
          std::cout << it->second.minValues[i]
                    << ((i != it->second.minValues.size() - 1) ? ", " : "");
        }
        std::cout << "]" << std::endl;
      }
      if (!it->second.maxValues.empty()) {
        std::cout << Indent(2) << "max          : [";
        for (size_t i = 0; i < it->second.maxValues.size(); i++) {
          std::cout << it->second.maxValues[i]
                    << ((i != it->second.maxValues.size() - 1) ? ", " : "");
        }
        std::cout << "]" << std::endl;
      }
    }
  }

  {
    std::map<std::string, tinygltf::BufferView>::const_iterator it(
        scene.bufferViews.begin());
    std::map<std::string, tinygltf::BufferView>::const_iterator itEnd(
        scene.bufferViews.end());
    std::cout << "bufferViews(items=" << scene.bufferViews.size() << ")"
              << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;
      std::cout << Indent(2) << "buffer       : " << it->second.buffer
                << std::endl;
      std::cout << Indent(2) << "byteLength   : " << it->second.byteLength
                << std::endl;
      std::cout << Indent(2) << "byteOffset   : " << it->second.byteOffset
                << std::endl;
    }
  }

  {
    std::map<std::string, tinygltf::Buffer>::const_iterator it(scene.buffers.begin());
    std::map<std::string, tinygltf::Buffer>::const_iterator itEnd(scene.buffers.end());
    std::cout << "buffers(items=" << scene.buffers.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;
      std::cout << Indent(2) << "byteLength   : " << it->second.data.size()
                << std::endl;
    }
  }

  {
    std::map<std::string, tinygltf::Material>::const_iterator it(scene.materials.begin());
    std::map<std::string, tinygltf::Material>::const_iterator itEnd(
        scene.materials.end());
    std::cout << "materials(items=" << scene.materials.size() << ")"
              << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;
      std::cout << Indent(1) << "technique    : " << it->second.technique
                << std::endl;
      std::cout << Indent(1) << "values(items=" << it->second.values.size()
                << std::endl;

      tinygltf::ParameterMap::const_iterator p(it->second.values.begin());
      tinygltf::ParameterMap::const_iterator pEnd(it->second.values.end());
      for (; p != pEnd; p++) {
        if (!p->second.number_array.empty()) {
          std::cout << Indent(3) << p->first
                    << PrintFloatArray(p->second.number_array) << std::endl;
        }
        if (!p->second.string_value.empty()) {
          std::cout << Indent(3) << p->first << " : " << p->second.string_value
                    << std::endl;
        }
      }
    }
  }

  {
    std::map<std::string, tinygltf::Node>::const_iterator it(scene.nodes.begin());
    std::map<std::string, tinygltf::Node>::const_iterator itEnd(scene.nodes.end());
    std::cout << "nodes(items=" << scene.nodes.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;

      DumpNode(it->second, 2);
    }
  }

  {
    std::map<std::string, tinygltf::Image>::const_iterator it(scene.images.begin());
    std::map<std::string, tinygltf::Image>::const_iterator itEnd(scene.images.end());
    std::cout << "images(items=" << scene.images.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;

      std::cout << Indent(2) << "width     : " << it->second.width << std::endl;
      std::cout << Indent(2) << "height    : " << it->second.height
                << std::endl;
      std::cout << Indent(2) << "component : " << it->second.component
                << std::endl;
      std::cout << Indent(2) << "name      : " << it->second.name << std::endl;
    }
  }

  {
    std::map<std::string, tinygltf::Texture>::const_iterator it(scene.textures.begin());
    std::map<std::string, tinygltf::Texture>::const_iterator itEnd(scene.textures.end());
    std::cout << "textures(items=" << scene.textures.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name           : " << it->first << std::endl;
      std::cout << Indent(1) << "format         : " << it->second.format
                << std::endl;
      std::cout << Indent(1) << "internalFormat : " << it->second.internalFormat
                << std::endl;
      std::cout << Indent(1) << "sampler        : " << it->second.sampler
                << std::endl;
      std::cout << Indent(1) << "source         : " << it->second.source
                << std::endl;
      std::cout << Indent(1) << "target         : " << it->second.target
                << std::endl;
      std::cout << Indent(1) << "type           : " << it->second.type
                << std::endl;
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Needs input.gltf\n");
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

  Dump(scene);

  return 0;
}
