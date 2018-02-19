#include "gltf-loader.h"

#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"

namespace example {

static std::string GetFilePathExtension(const std::string &FileName) {
  if (FileName.find_last_of(".") != std::string::npos)
    return FileName.substr(FileName.find_last_of(".") + 1);
  return "";
}

///
/// Loads glTF 2.0 mesh
///
bool LoadGLTF(const std::string &filename, float scale,
              std::vector<Mesh<float> > *meshes,
              std::vector<Material> *materials,
              std::vector<Texture> *textures) {
  // TODO(syoyo): Implement
  // TODO(syoyo): Texture
  // TODO(syoyo): Material

  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string ext = GetFilePathExtension(filename);

  bool ret = false;
  if (ext.compare("glb") == 0) {
    // assume binary glTF.
    ret = loader.LoadBinaryFromFile(&model, &err, filename.c_str());
  } else {
    // assume ascii glTF.
    ret = loader.LoadASCIIFromFile(&model, &err, filename.c_str());
  }

  if (!err.empty()) {
    std::cerr << "glTF parse error: " << err << std::endl;
  }
  if (!ret) {
    std::cerr << "Failed to load glTF: " << filename << std::endl;
    return false;
  }

  std::cerr << "LoadGLTF() function is not yet implemented!" << std::endl;

  std::cout << "loaded glTF file has:\n"
            << model.accessors.size() << " accessors\n"
            << model.animations.size() << " animations\n"
            << model.buffers.size() << " buffers\n"
            << model.bufferViews.size() << " bufferViews\n"
            << model.materials.size() << " materials\n"
            << model.meshes.size() << " meshes\n"
            << model.nodes.size() << " nodes\n"
            << model.textures.size() << " textures\n"
            << model.images.size() << " images\n"
            << model.skins.size() << " skins\n"
            << model.samplers.size() << " samplers\n"
            << model.cameras.size() << " cameras\n"
            << model.scenes.size() << " scenes\n"
            << model.lights.size() << " lights\n";

  for (const auto &gltfMesh : model.meshes) {
    std::cout << "Current mesh has " << gltfMesh.primitives.size()
              << " primitives:\n";
    for (const auto &meshPrimitive : gltfMesh.primitives) {
      switch (meshPrimitive.mode) {
        case TINYGLTF_MODE_TRIANGLES:  // this is the simpliest case to handle
          std::cout << "Will load a plain old list of trianges\n";
          break;

        // Other trigangle based modes
        case TINYGLTF_MODE_TRIANGLE_FAN:
        case TINYGLTF_MODE_TRIANGLE_STRIP:
        default:
          std::cerr << "primitive mode not implemented";
          break;

        // These aren't triangles:
        case TINYGLTF_MODE_POINTS:
        case TINYGLTF_MODE_LINE:
        case TINYGLTF_MODE_LINE_LOOP:
          std::cerr << "primitive is not triangle based, ignoring";
      }
    }
  }

  return false;
}

}  // namespace example
