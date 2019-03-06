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
                              const tinygltf::Value &value, const int indent,
                              const bool tag = true) {
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
      ss << PrintValue("", value.Get(int(i)), indent + 1, /* tag */ false);
      if (i != (value.ArrayLen() - 1)) {
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

static void DumpExtensions(const tinygltf::ExtensionMap &extension,
                           const int indent) {
  // TODO(syoyo): pritty print Value
  for (auto &e : extension) {
    std::cout << Indent(indent) << e.first << std::endl;
    std::cout << PrintValue("extensions", e.second, indent + 1) << std::endl;
  }
}

static void Dump(const tinygltf::Model &model) {
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
    std::cout << "scenes(items=" << model.scenes.size() << ")" << std::endl;
    for (size_t i = 0; i < model.scenes.size(); i++) {
      std::cout << Indent(1) << "scene[" << i
                << "] name  : " << model.scenes[i].name << std::endl;
      DumpExtensions(model.scenes[i].extensions, 1);
    }
  }

  {
    std::cout << "meshes(item=" << model.meshes.size() << ")" << std::endl;
    for (size_t i = 0; i < model.meshes.size(); i++) {
      std::cout << Indent(1) << "name     : " << model.meshes[i].name
                << std::endl;
      std::cout << Indent(1)
                << "primitives(items=" << model.meshes[i].primitives.size()
                << "): " << std::endl;

      for (size_t k = 0; k < model.meshes[i].primitives.size(); k++) {
        DumpPrimitive(model.meshes[i].primitives[k], 2);
      }
    }
  }

  {
    for (size_t i = 0; i < model.accessors.size(); i++) {
      const tinygltf::Accessor &accessor = model.accessors[i];
      std::cout << Indent(1) << "name         : " << accessor.name << std::endl;
      std::cout << Indent(2) << "bufferView   : " << accessor.bufferView
                << std::endl;
      std::cout << Indent(2) << "byteOffset   : " << accessor.byteOffset
                << std::endl;
      std::cout << Indent(2) << "componentType: "
                << PrintComponentType(accessor.componentType) << "("
                << accessor.componentType << ")" << std::endl;
      std::cout << Indent(2) << "count        : " << accessor.count
                << std::endl;
      std::cout << Indent(2) << "type         : " << PrintType(accessor.type)
                << std::endl;
      if (!accessor.minValues.empty()) {
        std::cout << Indent(2) << "min          : [";
        for (size_t k = 0; k < accessor.minValues.size(); k++) {
          std::cout << accessor.minValues[k]
                    << ((k != accessor.minValues.size() - 1) ? ", " : "");
        }
        std::cout << "]" << std::endl;
      }
      if (!accessor.maxValues.empty()) {
        std::cout << Indent(2) << "max          : [";
        for (size_t k = 0; k < accessor.maxValues.size(); k++) {
          std::cout << accessor.maxValues[k]
                    << ((k != accessor.maxValues.size() - 1) ? ", " : "");
        }
        std::cout << "]" << std::endl;
      }

      if (accessor.sparse.isSparse) {
        std::cout << Indent(2) << "sparse:" << std::endl;
        std::cout << Indent(3) << "count  : " << accessor.sparse.count
                  << std::endl;
        std::cout << Indent(3) << "indices: " << std::endl;
        std::cout << Indent(4)
                  << "bufferView   : " << accessor.sparse.indices.bufferView
                  << std::endl;
        std::cout << Indent(4)
                  << "byteOffset   : " << accessor.sparse.indices.byteOffset
                  << std::endl;
        std::cout << Indent(4) << "componentType: "
                  << PrintComponentType(accessor.sparse.indices.componentType)
                  << "(" << accessor.sparse.indices.componentType << ")"
                  << std::endl;
        std::cout << Indent(3) << "values : " << std::endl;
        std::cout << Indent(4)
                  << "bufferView   : " << accessor.sparse.values.bufferView
                  << std::endl;
        std::cout << Indent(4)
                  << "byteOffset   : " << accessor.sparse.values.byteOffset
                  << std::endl;
      }
    }
  }

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
    }
  }

  {
    std::cout << "bufferViews(items=" << model.bufferViews.size() << ")"
              << std::endl;
    for (size_t i = 0; i < model.bufferViews.size(); i++) {
      const tinygltf::BufferView &bufferView = model.bufferViews[i];
      std::cout << Indent(1) << "name         : " << bufferView.name
                << std::endl;
      std::cout << Indent(2) << "buffer       : " << bufferView.buffer
                << std::endl;
      std::cout << Indent(2) << "byteLength   : " << bufferView.byteLength
                << std::endl;
      std::cout << Indent(2) << "byteOffset   : " << bufferView.byteOffset
                << std::endl;
      std::cout << Indent(2) << "byteStride   : " << bufferView.byteStride
                << std::endl;
      std::cout << Indent(2)
                << "target       : " << PrintTarget(bufferView.target)
                << std::endl;
    }
  }

  {
    std::cout << "buffers(items=" << model.buffers.size() << ")" << std::endl;
    for (size_t i = 0; i < model.buffers.size(); i++) {
      const tinygltf::Buffer &buffer = model.buffers[i];
      std::cout << Indent(1) << "name         : " << buffer.name << std::endl;
      std::cout << Indent(2) << "byteLength   : " << buffer.data.size()
                << std::endl;
    }
  }

  {
    std::cout << "materials(items=" << model.materials.size() << ")"
              << std::endl;
    for (size_t i = 0; i < model.materials.size(); i++) {
      const tinygltf::Material &material = model.materials[i];
      std::cout << Indent(1) << "name         : " << material.name << std::endl;
      std::cout << Indent(1) << "values(items=" << material.values.size() << ")"
                << std::endl;

      tinygltf::ParameterMap::const_iterator p(material.values.begin());
      tinygltf::ParameterMap::const_iterator pEnd(material.values.end());
      for (; p != pEnd; p++) {
        std::cout << Indent(2) << p->first << ": "
                  << PrintParameterValue(p->second) << std::endl;
      }
    }
  }

  {
    std::cout << "nodes(items=" << model.nodes.size() << ")" << std::endl;
    for (size_t i = 0; i < model.nodes.size(); i++) {
      const tinygltf::Node &node = model.nodes[i];
      std::cout << Indent(1) << "name         : " << node.name << std::endl;

      DumpNode(node, 2);
    }
  }

  {
    std::cout << "images(items=" << model.images.size() << ")" << std::endl;
    for (size_t i = 0; i < model.images.size(); i++) {
      const tinygltf::Image &image = model.images[i];
      std::cout << Indent(1) << "name         : " << image.name << std::endl;

      std::cout << Indent(2) << "width     : " << image.width << std::endl;
      std::cout << Indent(2) << "height    : " << image.height << std::endl;
      std::cout << Indent(2) << "component : " << image.component << std::endl;
      DumpExtensions(image.extensions, 1);
    }
  }

  {
    std::cout << "textures(items=" << model.textures.size() << ")" << std::endl;
    for (size_t i = 0; i < model.textures.size(); i++) {
      const tinygltf::Texture &texture = model.textures[i];
      std::cout << Indent(1) << "sampler        : " << texture.sampler
                << std::endl;
      std::cout << Indent(1) << "source         : " << texture.source
                << std::endl;
      DumpExtensions(texture.extensions, 1);
    }
  }

  {
    std::cout << "samplers(items=" << model.samplers.size() << ")" << std::endl;

    for (size_t i = 0; i < model.samplers.size(); i++) {
      const tinygltf::Sampler &sampler = model.samplers[i];
      std::cout << Indent(1) << "name (id)    : " << sampler.name << std::endl;
      std::cout << Indent(2)
                << "minFilter    : " << PrintFilterMode(sampler.minFilter)
                << std::endl;
      std::cout << Indent(2)
                << "magFilter    : " << PrintFilterMode(sampler.magFilter)
                << std::endl;
      std::cout << Indent(2)
                << "wrapS        : " << PrintWrapMode(sampler.wrapS)
                << std::endl;
      std::cout << Indent(2)
                << "wrapT        : " << PrintWrapMode(sampler.wrapT)
                << std::endl;
    }
  }

  {
    std::cout << "cameras(items=" << model.cameras.size() << ")" << std::endl;

    for (size_t i = 0; i < model.cameras.size(); i++) {
      const tinygltf::Camera &camera = model.cameras[i];
      std::cout << Indent(1) << "name (id)    : " << camera.name << std::endl;
      std::cout << Indent(1) << "type         : " << camera.type << std::endl;

      if (camera.type.compare("perspective") == 0) {
        std::cout << Indent(2)
                  << "aspectRatio   : " << camera.perspective.aspectRatio
                  << std::endl;
        std::cout << Indent(2) << "yfov          : " << camera.perspective.yfov
                  << std::endl;
        std::cout << Indent(2) << "zfar          : " << camera.perspective.zfar
                  << std::endl;
        std::cout << Indent(2) << "znear         : " << camera.perspective.znear
                  << std::endl;
      } else if (camera.type.compare("orthographic") == 0) {
        std::cout << Indent(2) << "xmag          : " << camera.orthographic.xmag
                  << std::endl;
        std::cout << Indent(2) << "ymag          : " << camera.orthographic.ymag
                  << std::endl;
        std::cout << Indent(2) << "zfar          : " << camera.orthographic.zfar
                  << std::endl;
        std::cout << Indent(2)
                  << "znear         : " << camera.orthographic.znear
                  << std::endl;
      }
    }
  }

  // toplevel extensions
  {
    std::cout << "extensions(items=" << model.extensions.size() << ")"
              << std::endl;
    DumpExtensions(model.extensions, 1);
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
    ret = gltf_ctx.LoadBinaryFromFile(&model, &err, &warn,
                                      input_filename.c_str());
  } else {
    std::cout << "Reading ASCII glTF" << std::endl;
    // assume ascii glTF.
    ret =
        gltf_ctx.LoadASCIIFromFile(&model, &err, &warn, input_filename.c_str());
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

  Dump(model);

  return 0;
}
