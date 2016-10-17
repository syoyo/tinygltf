#define TINYGLTF_LOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_gltf_loader.h"

#include <cstdio>
#include <fstream>
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

static std::string PrintTarget(int target) {
  if (target == 34962) {
    return "GL_ARRAY_BUFFER";
  } else if (target == 34963) {
    return "GL_ELEMENT_ARRAY_BUFFER";
  } else {
    return "**UNKNOWN**";
  }
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

static std::string PrintShaderType(int ty) {
  if (ty == TINYGLTF_SHADER_TYPE_VERTEX_SHADER) {
    return "VERTEX_SHADER";
  } else if (ty == TINYGLTF_SHADER_TYPE_FRAGMENT_SHADER) {
    return "FRAGMENT_SHADER";
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

static std::string PrintParameterType(int ty) {
  if (ty == TINYGLTF_PARAMETER_TYPE_BYTE) {
    return "BYTE";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE) {
    return "UNSIGNED_BYTE";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_SHORT) {
    return "SHORT";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT) {
    return "UNSIGNED_SHORT";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_INT) {
    return "INT";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT) {
    return "UNSIGNED_INT";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT) {
    return "FLOAT";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC2) {
    return "FLOAT_VEC2";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3) {
    return "FLOAT_VEC3";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC4) {
    return "FLOAT_VEC4";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_INT_VEC2) {
    return "INT_VEC2";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_INT_VEC3) {
    return "INT_VEC3";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_INT_VEC4) {
    return "INT_VEC4";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_BOOL) {
    return "BOOL";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_BOOL_VEC2) {
    return "BOOL_VEC2";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_BOOL_VEC3) {
    return "BOOL_VEC3";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_BOOL_VEC4) {
    return "BOOL_VEC4";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_MAT2) {
    return "FLOAT_MAT2";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_MAT3) {
    return "FLOAT_MAT3";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_FLOAT_MAT4) {
    return "FLOAT_MAT4";
  } else if (ty == TINYGLTF_PARAMETER_TYPE_SAMPLER_2D) {
    return "SAMPLER_2D";
  }

  return "**UNKNOWN**";
}

static std::string PrintWrapMode(int mode) {
  if (mode == TINYGLTF_TEXTURE_WRAP_RPEAT) {
    return "REPEAT";
  } else if (mode == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE) {
    return "CLAMP_TO_EDGE";
  } else if (mode == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) {
    return "MIRRORED_REPEAT";
  }

  return "**UNKNOWN**";
}

static std::string PrintFilterMode(int mode) {
  if (mode == TINYGLTF_TEXTURE_FILTER_NEAREST) {
    return "NEAREST";
  } else if (mode == TINYGLTF_TEXTURE_FILTER_LINEAR) {
    return "LINEAR";
  } else if (mode == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST) {
    return "NEAREST_MIPMAP_NEAREST";
  } else if (mode == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR) {
    return "NEAREST_MIPMAP_LINEAR";
  } else if (mode == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST) {
    return "LINEAR_MIPMAP_NEAREST";
  } else if (mode == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR) {
    return "LINEAR_MIPMAP_LINEAR";
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

static std::string Indent(const int indent) {
  std::string s;
  for (int i = 0; i < indent; i++) {
    s += "  ";
  }

  return s;
}

static std::string PrintParameterValue(const tinygltf::Parameter &param) {
  if (!param.number_array.empty()) {
    return PrintFloatArray(param.number_array);
  } else {
    return param.string_value;
  }
}

static std::string PrintValue(const std::string& name, const tinygltf::Value &value, const int indent) {
  std::stringstream ss;

  if (value.IsObject()) {
    const tinygltf::Value::Object& o = value.Get<tinygltf::Value::Object>();
    tinygltf::Value::Object::const_iterator it(o.begin());
    tinygltf::Value::Object::const_iterator itEnd(o.end());
    for (; it != itEnd; it++) {
      ss << PrintValue(name, it->second, indent + 1);
    }
  } else if (value.IsString()) {
    ss << Indent(indent) << name << " : " << value.Get<std::string>() << std::endl;
  } else if (value.IsBool()) {
    ss << Indent(indent) << name << " : " << value.Get<bool>() << std::endl;
  } else if (value.IsNumber()) {
    ss << Indent(indent) << name << " : " << value.Get<double>() << std::endl;
  } else if (value.IsInt()) {
    ss << Indent(indent) << name << " : " << value.Get<int>() << std::endl;
  }
  // @todo { binary, array }

  return ss.str();
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

static void DumpStringMap(const std::map<std::string, std::string> &map,
                          int indent) {
  std::map<std::string, std::string>::const_iterator it(map.begin());
  std::map<std::string, std::string>::const_iterator itEnd(map.end());
  for (; it != itEnd; it++) {
    std::cout << Indent(indent) << it->first << ": " << it->second << std::endl;
  }
}

static void DumpPrimitive(const tinygltf::Primitive &primitive, int indent) {
  std::cout << Indent(indent) << "material : " << primitive.material
            << std::endl;
  std::cout << Indent(indent) << "indices : " << primitive.indices << std::endl;
  std::cout << Indent(indent) << "mode     : " << PrintMode(primitive.mode)
            << "(" << primitive.mode << ")" << std::endl;
  std::cout << Indent(indent)
            << "attributes(items=" << primitive.attributes.size() << ")"
            << std::endl;
  DumpStringMap(primitive.attributes, indent + 1);

  std::cout << Indent(indent)
            << "extras :" << std::endl
            << PrintValue("extras", primitive.extras, indent+1) << std::endl;
}

static void DumpTechniqueParameter(const tinygltf::TechniqueParameter &param,
                                   int indent) {
  std::cout << Indent(indent) << "count    : " << param.count << std::endl;
  std::cout << Indent(indent) << "node     : " << param.node << std::endl;
  std::cout << Indent(indent) << "semantic : " << param.semantic << std::endl;
  std::cout << Indent(indent) << "type     : " << PrintParameterType(param.type)
            << std::endl;
  std::cout << Indent(indent)
            << "value    : " << PrintParameterValue(param.value) << std::endl;
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
    std::map<std::string, tinygltf::Mesh>::const_iterator it(
        scene.meshes.begin());
    std::map<std::string, tinygltf::Mesh>::const_iterator itEnd(
        scene.meshes.end());
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
    std::map<std::string, tinygltf::Accessor>::const_iterator it(
        scene.accessors.begin());
    std::map<std::string, tinygltf::Accessor>::const_iterator itEnd(
        scene.accessors.end());
    std::cout << "accessors(items=" << scene.accessors.size() << ")"
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
    std::map<std::string, tinygltf::Animation>::const_iterator it(
        scene.animations.begin());
    std::map<std::string, tinygltf::Animation>::const_iterator itEnd(
        scene.animations.end());
    std::cout << "animations(items=" << scene.animations.size() << ")"
              << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;

      std::cout << Indent(1) << "channels : [ " << std::endl;
      for (size_t i = 0; i < it->second.channels.size(); i++) {
        std::cout << Indent(2)
                  << "sampler     : " << it->second.channels[i].sampler
                  << std::endl;
        std::cout << Indent(2)
                  << "target.id   : " << it->second.channels[i].target_id
                  << std::endl;
        std::cout << Indent(2)
                  << "target.path : " << it->second.channels[i].target_path
                  << std::endl;
        std::cout << ((i != (it->second.channels.size() - 1)) ? "  , " : "");
      }
      std::cout << "  ]" << std::endl;

      std::map<std::string, tinygltf::AnimationSampler>::const_iterator
          samplerIt(it->second.samplers.begin());
      std::map<std::string, tinygltf::AnimationSampler>::const_iterator
          samplerItEnd(it->second.samplers.end());
      std::cout << Indent(1) << "samplers(items=" << it->second.samplers.size()
                << ")" << std::endl;
      for (; samplerIt != samplerItEnd; samplerIt++) {
        std::cout << Indent(1) << "name          : " << samplerIt->first
                  << std::endl;
        std::cout << Indent(2) << "input         : " << samplerIt->second.input
                  << std::endl;
        std::cout << Indent(2)
                  << "interpolation : " << samplerIt->second.interpolation
                  << std::endl;
        std::cout << Indent(2) << "output        : " << samplerIt->second.output
                  << std::endl;
      }

      {
        std::cout << Indent(1)
                  << "parameters(items=" << it->second.parameters.size() << ")"
                  << std::endl;
        tinygltf::ParameterMap::const_iterator p(it->second.parameters.begin());
        tinygltf::ParameterMap::const_iterator pEnd(
            it->second.parameters.end());
        for (; p != pEnd; p++) {
          std::cout << Indent(2) << p->first << ": "
                    << PrintParameterValue(p->second) << std::endl;
        }
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
      std::cout << Indent(2)
                << "target       : " << PrintTarget(it->second.target)
                << std::endl;
    }
  }

  {
    std::map<std::string, tinygltf::Buffer>::const_iterator it(
        scene.buffers.begin());
    std::map<std::string, tinygltf::Buffer>::const_iterator itEnd(
        scene.buffers.end());
    std::cout << "buffers(items=" << scene.buffers.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;
      std::cout << Indent(2) << "byteLength   : " << it->second.data.size()
                << std::endl;
    }
  }

  {
    std::map<std::string, tinygltf::Material>::const_iterator it(
        scene.materials.begin());
    std::map<std::string, tinygltf::Material>::const_iterator itEnd(
        scene.materials.end());
    std::cout << "materials(items=" << scene.materials.size() << ")"
              << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;
      std::cout << Indent(1) << "technique    : " << it->second.technique
                << std::endl;
      std::cout << Indent(1) << "values(items=" << it->second.values.size()
                << ")" << std::endl;

      tinygltf::ParameterMap::const_iterator p(it->second.values.begin());
      tinygltf::ParameterMap::const_iterator pEnd(it->second.values.end());
      for (; p != pEnd; p++) {
        std::cout << Indent(2) << p->first << ": "
                  << PrintParameterValue(p->second) << std::endl;
      }
    }
  }

  {
    std::map<std::string, tinygltf::Node>::const_iterator it(
        scene.nodes.begin());
    std::map<std::string, tinygltf::Node>::const_iterator itEnd(
        scene.nodes.end());
    std::cout << "nodes(items=" << scene.nodes.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name         : " << it->first << std::endl;

      DumpNode(it->second, 2);
    }
  }

  {
    std::map<std::string, tinygltf::Image>::const_iterator it(
        scene.images.begin());
    std::map<std::string, tinygltf::Image>::const_iterator itEnd(
        scene.images.end());
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
    std::map<std::string, tinygltf::Texture>::const_iterator it(
        scene.textures.begin());
    std::map<std::string, tinygltf::Texture>::const_iterator itEnd(
        scene.textures.end());
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

  {
    std::map<std::string, tinygltf::Shader>::const_iterator it(
        scene.shaders.begin());
    std::map<std::string, tinygltf::Shader>::const_iterator itEnd(
        scene.shaders.end());

    std::cout << "shaders(items=" << scene.shaders.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name (id)      : " << it->first << std::endl;
      std::cout << Indent(2)
                << "type           : " << PrintShaderType(it->second.type)
                << std::endl;

      std::cout << Indent(2) << "name (json)    : " << it->second.name
                << std::endl;

      // Indent shader source nicely.
      std::string shader_source(Indent(3));
      shader_source.resize(shader_source.size() + it->second.source.size());

      std::vector<unsigned char>::const_iterator sourceIt(
          it->second.source.begin());
      std::vector<unsigned char>::const_iterator sourceItEnd(
          it->second.source.end());

      for (; sourceIt != sourceItEnd; ++sourceIt) {
        shader_source += static_cast<char>(*sourceIt);
        if (*sourceIt == '\n') {
          shader_source += Indent(3);
        }
      }
      std::cout << Indent(2) << "source         :\n"
                << shader_source << std::endl;
    }
  }

  {
    std::map<std::string, tinygltf::Program>::const_iterator it(
        scene.programs.begin());
    std::map<std::string, tinygltf::Program>::const_iterator itEnd(
        scene.programs.end());

    std::cout << "programs(items=" << scene.programs.size() << ")" << std::endl;
    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name           : " << it->first << std::endl;
      std::cout << Indent(2) << "vertexShader   : " << it->second.vertexShader
                << std::endl;
      std::cout << Indent(2) << "fragmentShader : " << it->second.fragmentShader
                << std::endl;
      std::cout << Indent(2) << "attributes     : "
                << PrintStringArray(it->second.attributes) << std::endl;
      std::cout << Indent(2) << "name           : " << it->second.name
                << std::endl;
    }
  }

  {
    std::map<std::string, tinygltf::Technique>::const_iterator it(
        scene.techniques.begin());
    std::map<std::string, tinygltf::Technique>::const_iterator itEnd(
        scene.techniques.end());

    std::cout << "techniques(items=" << scene.techniques.size() << ")"
              << std::endl;

    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name (id)    : " << it->first << std::endl;
      std::cout << Indent(2) << "program      : " << it->second.program
                << std::endl;

      std::cout << Indent(2) << "name (json)  : " << it->second.name
                << std::endl;

      std::cout << Indent(2)
                << "parameters(items=" << it->second.parameters.size() << ")"
                << std::endl;

      std::map<std::string, tinygltf::TechniqueParameter>::const_iterator
          paramIt(it->second.parameters.begin());
      std::map<std::string, tinygltf::TechniqueParameter>::const_iterator
          paramItEnd(it->second.parameters.end());

      for (; paramIt != paramItEnd; ++paramIt) {
        std::cout << Indent(3) << "name     : " << paramIt->first << std::endl;
        DumpTechniqueParameter(paramIt->second, 4);
      }

      std::cout << Indent(2)
                << "attributes(items=" << it->second.attributes.size() << ")"
                << std::endl;

      DumpStringMap(it->second.attributes, 3);

      std::cout << Indent(2) << "uniforms(items=" << it->second.uniforms.size()
                << ")" << std::endl;
      DumpStringMap(it->second.uniforms, 3);
    }
  }

  {
    std::map<std::string, tinygltf::Sampler>::const_iterator it(
        scene.samplers.begin());
    std::map<std::string, tinygltf::Sampler>::const_iterator itEnd(
        scene.samplers.end());

    std::cout << "samplers(items=" << scene.samplers.size() << ")" << std::endl;

    for (; it != itEnd; it++) {
      std::cout << Indent(1) << "name (id)    : " << it->first << std::endl;
      std::cout << Indent(2)
                << "minFilter    : " << PrintFilterMode(it->second.minFilter)
                << std::endl;
      std::cout << Indent(2)
                << "magFilter    : " << PrintFilterMode(it->second.magFilter)
                << std::endl;
      std::cout << Indent(2)
                << "wrapS        : " << PrintWrapMode(it->second.wrapS)
                << std::endl;
      std::cout << Indent(2)
                << "wrapT        : " << PrintWrapMode(it->second.wrapT)
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
