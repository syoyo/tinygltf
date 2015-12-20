#define TINYGLTF_LOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf_loader.h"

#include <fstream>
#include <cstdio>
#include <iostream>

std::string PrintMode(int mode)
{
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


std::string PrintType(int ty)
{
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

std::string PrintComponentType(int ty)
{
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

std::string PrintFloatArray(const std::vector<double>& arr)
{
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

std::string PrintStringArray(const std::vector<std::string>& arr)
{
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


std::string Indent(int indent)
{
  std::string s;
  for (int i = 0; i < indent; i++) {
    s += "  ";
  }

  return s;
}

void DumpNode(const Node& node, int indent)
{
  std::cout << Indent(indent) <<   "name        : " << node.name << std::endl;
  std::cout << Indent(indent) <<   "camera      : " << node.camera << std::endl;
  if (!node.rotation.empty()) {
    std::cout << Indent(indent) << "rotation    : " << PrintFloatArray(node.rotation) << std::endl;
  }
  if (!node.scale.empty()) {
    std::cout << Indent(indent) << "scale       : " << PrintFloatArray(node.scale) << std::endl;
  }
  if (!node.translation.empty()) {
    std::cout << Indent(indent) << "translation : " << PrintFloatArray(node.translation) << std::endl;
  }

  if (!node.matrix.empty()) {
    std::cout << Indent(indent) << "matrix      : " << PrintFloatArray(node.matrix) << std::endl;
  }

  std::cout << Indent(indent) <<   "meshes      : " << PrintStringArray(node.meshes) << std::endl;

  std::cout << Indent(indent) <<   "children    : " << PrintStringArray(node.children) << std::endl;

}

void DumpPrimitive(const Primitive& primitive, int indent)
{
  std::cout << Indent(indent) << "material : " << primitive.material << std::endl;
  std::cout << Indent(indent) << "mode     : " << PrintMode(primitive.mode) << "(" << primitive.mode << ")" << std::endl;
  std::cout << Indent(indent) << "attributes(items=" << primitive.attributes.size() << ")" << std::endl;
  std::map<std::string, std::string>::const_iterator it(primitive.attributes.begin());
  std::map<std::string, std::string>::const_iterator itEnd(primitive.attributes.end());
  for (; it != itEnd; it++) {
    std::cout << Indent(indent + 1) << it->first << ": " << it->second << std::endl;
  }

}

void Dump(const Scene& scene)
{
  std::cout << "=== Dump glTF ===" << std::endl;
  std::cout << "asset.generator          : " << scene.asset.generator << std::endl;
  std::cout << "asset.premultipliedAlpha : " << scene.asset.premultipliedAlpha << std::endl;
  std::cout << "asset.version            : " << scene.asset.version << std::endl;
  std::cout << "asset.profile.api        : " << scene.asset.profile_api << std::endl;
  std::cout << "asset.profile.version    : " << scene.asset.profile_version << std::endl;
  std::cout << std::endl;
  std::cout << "=== Dump scene ===" << std::endl;
  std::cout << "defaultScene: " << scene.defaultScene << std::endl;

  {
    std::map<std::string, std::vector<std::string> >::const_iterator it(scene.scenes.begin());
    std::map<std::string, std::vector<std::string> >::const_iterator itEnd(scene.scenes.end());
    std::cout << "scenes(items=" << scene.scenes.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name  : " << it->first << std::endl;
      std::cout << Indent(2) << "nodes : [ ";
      for (size_t i = 0; i < it->second.size(); i++) {
        std::cout << it->second[i] << ((i != it->second.size()) ? ", " : "");
      }
      std::cout << " ] " << std::endl;
    }
  }

  {
    std::map<std::string, Mesh>::const_iterator it(scene.meshes.begin());
    std::map<std::string, Mesh>::const_iterator itEnd(scene.meshes.end());
    std::cout << "meshes(item=" << scene.meshes.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name     : " << it->second.name << std::endl;
      std::cout << Indent(1) << "primitives(items=" << it->second.primitives.size() << "): " << std::endl;
    
      for (size_t i = 0; i < it->second.primitives.size(); i++) {
        DumpPrimitive(it->second.primitives[i], 2);
      }
    }
  }

  {
    std::map<std::string, Accessor>::const_iterator it(scene.accessors.begin());
    std::map<std::string, Accessor>::const_iterator itEnd(scene.accessors.end());
    std::cout << "accessos(items=" << scene.accessors.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;
      std::cout << Indent(2) << "bufferView   : " << it->second.bufferView << std::endl;
      std::cout << Indent(2) << "byteOffset   : " << it->second.byteOffset << std::endl;
      std::cout << Indent(2) << "byteStride   : " << it->second.byteStride << std::endl;
      std::cout << Indent(2) << "componentType: " << PrintComponentType(it->second.componentType) << "(" << it->second.componentType << ")" << std::endl;
      std::cout << Indent(2) << "count        : " << it->second.count << std::endl;
      std::cout << Indent(2) << "type         : " << PrintType(it->second.type) << std::endl;
      if (!it->second.minValues.empty()) {
        std::cout << Indent(2) << "min          : [";
        for (size_t i = 0; i < it->second.minValues.size(); i++) {
          std::cout << it->second.minValues[i] << ((i != it->second.minValues.size()-1) ? ", " : "");
        }
        std::cout << "]" << std::endl;
      }
      if (!it->second.maxValues.empty()) {
        std::cout << Indent(2) << "max          : [";
        for (size_t i = 0; i < it->second.maxValues.size(); i++) {
          std::cout << it->second.maxValues[i] << ((i != it->second.maxValues.size()-1) ? ", " : "");
        }
        std::cout << "]" << std::endl;
      }
    }
  }

  {
    std::map<std::string, BufferView>::const_iterator it(scene.bufferViews.begin());
    std::map<std::string, BufferView>::const_iterator itEnd(scene.bufferViews.end());
    std::cout << "bufferViews(items=" << scene.bufferViews.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;
      std::cout << Indent(2) << "buffer       : " << it->second.buffer << std::endl;
      std::cout << Indent(2) << "byteLength   : " << it->second.byteLength << std::endl;
      std::cout << Indent(2) << "byteOffset   : " << it->second.byteOffset << std::endl;
    }
  }

  {
    std::map<std::string, Buffer>::const_iterator it(scene.buffers.begin());
    std::map<std::string, Buffer>::const_iterator itEnd(scene.buffers.end());
    std::cout << "buffers(items=" << scene.buffers.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;
      std::cout << Indent(2) << "byteLength   : " << it->second.data.size() << std::endl;
    }
  }

  {
    std::map<std::string, Material>::const_iterator it(scene.materials.begin());
    std::map<std::string, Material>::const_iterator itEnd(scene.materials.end());
    std::cout << "materials(items=" << scene.materials.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;
      std::cout << Indent(1) << "technique    : " << it->second.technique << std::endl;
      std::cout << Indent(1) << "values(items=" << it->second.values.size() << std::endl;

      FloatParameterMap::const_iterator p(it->second.values.begin());
      FloatParameterMap::const_iterator pEnd(it->second.values.end());
      for (; p != pEnd; p++) {
        std::cout << Indent(3) << p->first << PrintFloatArray(p->second) << std::endl;
      }
    }
  }

  {
    std::map<std::string, Node>::const_iterator it(scene.nodes.begin());
    std::map<std::string, Node>::const_iterator itEnd(scene.nodes.end());
    std::cout << "nodes(items=" << scene.nodes.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;

      DumpNode(it->second, 2);
    }
  }
  
  {
    std::map<std::string, Image>::const_iterator it(scene.images.begin());
    std::map<std::string, Image>::const_iterator itEnd(scene.images.end());
    std::cout << "images(items=" << scene.images.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;

      std::cout << Indent(2) << "width     : " << it->second.width << std::endl;
      std::cout << Indent(2) << "height    : " << it->second.height << std::endl;
      std::cout << Indent(2) << "component : " << it->second.component << std::endl;
      std::cout << Indent(2) << "name      : " << it->second.name << std::endl;
    }
  }
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    printf("Needs input.gltf\n");
    exit(1);
  }

  Scene scene; 
  TinyGLTFLoader loader;
  std::string err;
  
  bool ret = loader.LoadFromFile(scene, err, argv[1]);

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
