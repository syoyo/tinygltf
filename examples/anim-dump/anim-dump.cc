//
// TODO(syoyo): Print extensions and extras for each glTF object.
//
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

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

#if 0
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
#endif

static std::string PrintWrapMode(int mode) {
  if (mode == TINYGLTF_TEXTURE_WRAP_REPEAT) {
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

static std::string PrintIntArray(const std::vector<int> &arr) {
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

#if 0
static std::string PrintParameterMap(const tinygltf::ParameterMap &pmap) {
  std::stringstream ss;

  ss << pmap.size() << std::endl;
  for (auto &kv : pmap) {
    ss << kv.first << " : " << PrintParameterValue(kv.second) << std::endl;
  }

  return ss.str();
}
#endif

static std::string PrintValue(const std::string &name,
                              const tinygltf::Value &value, const int indent, const bool tag = true) {
  std::stringstream ss;

  if (value.IsObject()) {
    const tinygltf::Value::Object &o = value.Get<tinygltf::Value::Object>();
    tinygltf::Value::Object::const_iterator it(o.begin());
    tinygltf::Value::Object::const_iterator itEnd(o.end());
    for (; it != itEnd; it++) {
      ss << PrintValue(it->first, it->second, indent + 1) << std::endl;
    }
  } else if (value.IsString()) {
    if (tag) {
      ss << Indent(indent) << name << " : " << value.Get<std::string>();
    } else {
      ss << " " << value.Get<std::string>() << " ";
    }
  } else if (value.IsBool()) {
    if (tag) {
      ss << Indent(indent) << name << " : " << value.Get<bool>();
    } else {
      ss << " " << value.Get<bool>() << " ";
    }
  } else if (value.IsNumber()) {
    if (tag) {
      ss << Indent(indent) << name << " : " << value.Get<double>();
    } else {
      ss << " " << value.Get<double>() << " ";
    }
  } else if (value.IsInt()) {
    if (tag) {
      ss << Indent(indent) << name << " : " << value.Get<int>();
    } else {
      ss << " " << value.Get<int>() << " ";
    }
  } else if (value.IsArray()) {
    ss << Indent(indent) << name << " [ ";
    for (size_t i = 0; i < value.Size(); i++) {
      ss << PrintValue("", value.Get(int(i)), indent + 1, /* tag */false);
      if (i != (value.ArrayLen()-1)) {
        ss << ", ";
      }

    }
    ss << Indent(indent) << "] ";
  }

  // @todo { binary }

  return ss.str();
}

static void DumpNode(const tinygltf::Node &node, int indent) {
  std::cout << Indent(indent) << "name        : " << node.name << std::endl;
  std::cout << Indent(indent) << "camera      : " << node.camera << std::endl;
  std::cout << Indent(indent) << "mesh        : " << node.mesh << std::endl;
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
            << "children    : " << PrintIntArray(node.children) << std::endl;
}

static void DumpStringIntMap(const std::map<std::string, int> &m, int indent) {
  std::map<std::string, int>::const_iterator it(m.begin());
  std::map<std::string, int>::const_iterator itEnd(m.end());
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
  DumpStringIntMap(primitive.attributes, indent + 1);

  std::cout << Indent(indent) << "extras :" << std::endl
            << PrintValue("extras", primitive.extras, indent + 1) << std::endl;
}

static void DumpExtensions(const tinygltf::ExtensionMap &extension, const int indent)
{
  // TODO(syoyo): pritty print Value
  for (auto &e : extension) {
    std::cout << Indent(indent) << e.first << std::endl;
    std::cout << PrintValue("extensions", e.second, indent+1) << std::endl;
  }  
}

static float DecodeScalarAnimationValue(const size_t i, const tinygltf::Accessor &accessor, const tinygltf::Model &model)
{
  const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
  const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

  const uint8_t *addr = GetBufferAddress(i, accessor, bufferView, buffer);
  if (addr == nullptr) {
    std::cerr << "Invalid glTF data?" << std::endl;
    return 0.0f;
  }

  float value = 0.0f;

  if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_BYTE) {
    value = tinygltf::DecodeAnimationChannelValue(*(reinterpret_cast<const int8_t*>(addr)));
  } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
    value = tinygltf::DecodeAnimationChannelValue(*(reinterpret_cast<const uint8_t*>(addr)));
  } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT) {
    value = tinygltf::DecodeAnimationChannelValue(*(reinterpret_cast<const int16_t*>(addr)));
  } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    value = tinygltf::DecodeAnimationChannelValue(*(reinterpret_cast<const uint16_t*>(addr)));
  } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
    value = *(reinterpret_cast<const float*>(addr));
  } else {
    std::cerr << "??? Unknown componentType : " << PrintComponentType(accessor.componentType) << std::endl;
  }

  return value;
}

static void ProcessAnimation(const tinygltf::Animation &animation, const tinygltf::Model &model)
{
#if 0
  if (animaton_channel.target_path.compare("translation") == 0) {
  } else if (animaton_channel.target_path.compare("rotation") == 0) {
  } else if (animaton_channel.target_path.compare("scale") == 0) {
  } else if (animaton_channel.target_path.compare("weights") == 0) {
  }
#endif


  for (size_t j = 0; j < animation.samplers.size(); j++) {
    std::cout << "== samplers[" << j << "] ===============" << std::endl;
    const tinygltf::AnimationSampler &sampler = animation.samplers[j];
    std::cout << Indent(1) << "interpolation = " << sampler.interpolation<< std::endl;
    std::cout << Indent(1) << "input = " << sampler.input << std::endl;
    std::cout << Indent(1) << "output = " << sampler.output << std::endl;
    
    // input accessor must have min/max property.
    const tinygltf::Accessor &accessor = model.accessors[sampler.input];
    
    for (size_t i = 0; i < accessor.minValues.size(); i++) {
      std::cout << Indent(1) << "input min[" << i << "] = " << accessor.minValues[i] << std::endl;
    }
    for (size_t i = 0; i < accessor.maxValues.size(); i++) {
      std::cout << Indent(1) << "input max[" << i << "] = " << accessor.maxValues[i] << std::endl;
    }

    std::cout << Indent(1) << "input count = " << accessor.count << std::endl;

    for (size_t i = 0; i < accessor.count; i++) {
      if (accessor.type == TINYGLTF_TYPE_SCALAR) {
        float v = DecodeScalarAnimationValue(i, accessor, model);
        std::cout << Indent(2) << "input value[" << i << "] = " << v << std::endl;
      }
    }


    //const tinygltf::Accessor &accessor = model.accessors[sampler.output];
    //std::cout << Indent(2) << "output        : " << sampler.output
    //          << std::endl;
  }
  
}

static void DumpAnim(const tinygltf::Model &model) {
  std::cout << "=== Dump glTF ===" << std::endl;
  std::cout << "asset.copyright          : " << model.asset.copyright
            << std::endl;
  std::cout << "asset.generator          : " << model.asset.generator
            << std::endl;
  std::cout << "asset.version            : " << model.asset.version
            << std::endl;
  std::cout << "asset.minVersion         : " << model.asset.minVersion
            << std::endl;
  std::cout << std::endl;

  std::cout << "=== Dump scene ===" << std::endl;
  std::cout << "defaultScene: " << model.defaultScene << std::endl;

  {
    std::cout << "animations(items=" << model.animations.size() << ")"
              << std::endl;
    for (size_t i = 0; i < model.animations.size(); i++) {
      const tinygltf::Animation &animation = model.animations[i];
      std::cout << Indent(1) << "name         : " << animation.name
                << std::endl;

      std::cout << Indent(1) << "channels : [ " << std::endl;
      for (size_t j = 0; i < animation.channels.size(); i++) {
        std::cout << Indent(2)
                  << "sampler     : " << animation.channels[j].sampler
                  << std::endl;
        std::cout << Indent(2)
                  << "target.id   : " << animation.channels[j].target_node
                  << std::endl;
        std::cout << Indent(2)
                  << "target.path : " << animation.channels[j].target_path
                  << std::endl;
        std::cout << ((i != (animation.channels.size() - 1)) ? "  , " : "");
      }
      std::cout << "  ]" << std::endl;

      std::cout << Indent(1) << "samplers(items=" << animation.samplers.size()
                << ")" << std::endl;
      for (size_t j = 0; j < animation.samplers.size(); j++) {
        const tinygltf::AnimationSampler &sampler = animation.samplers[j];
        std::cout << Indent(2) << "input         : " << sampler.input
                  << std::endl;
        std::cout << Indent(2) << "interpolation : " << sampler.interpolation
                  << std::endl;
        std::cout << Indent(2) << "output        : " << sampler.output
                  << std::endl;
      }

      ProcessAnimation(animation, model);
    }

  }

}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Needs input.gltf\n");
    exit(1);
  }

  tinygltf::Model model;
  tinygltf::TinyGLTF gltf_ctx;
  std::string err;
  std::string warn; 
  std::string input_filename(argv[1]);
  std::string ext = GetFilePathExtension(input_filename);

  bool ret = false;
  if (ext.compare("glb") == 0) {
    std::cout << "Reading binary glTF" << std::endl;
    // assume binary glTF.
    ret = gltf_ctx.LoadBinaryFromFile(&model, &err, &warn, input_filename.c_str());
  } else {
    std::cout << "Reading ASCII glTF" << std::endl;
    // assume ascii glTF.
    ret = gltf_ctx.LoadASCIIFromFile(&model, &err, &warn, input_filename.c_str());
  }

  if (!warn.empty()) {
    printf("Warn: %s\n", warn.c_str());
  }


  if (!err.empty()) {
    printf("Err: %s\n", err.c_str());
  }

  if (!ret) {
    printf("Failed to parse glTF\n");
    return -1;
  }

  DumpAnim(model);

  return 0;
}
