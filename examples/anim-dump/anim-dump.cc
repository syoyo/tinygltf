//
// TODO(syoyo): Print extensions and extras for each glTF object.
//
#include "tiny_gltf_util.h"

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

static std::string Indent(const int indent) {
  std::string s;
  for (int i = 0; i < indent; i++) {
    s += "  ";
  }

  return s;
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
        float v;
        if (tinygltf::util::DecodeScalarAnimationValue(i, accessor, model, &v)) {
          std::cout << Indent(2) << "input value[" << i << "] = " << v << std::endl;
        }
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
