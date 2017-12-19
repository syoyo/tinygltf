#include "gltf-loader.h"

#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
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
bool LoadGLTF(const std::string &filename, float scale, std::vector<Mesh<float> > *meshes, std::vector<Material> *materials, std::vector<Texture> *textures) 
{

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
  return false;
}

} // namespace example
