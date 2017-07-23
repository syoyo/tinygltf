#ifndef EXAMPLE_GLTF_LOADER_H_
#define EXAMPLE_GLTF_LOADER_H_

#include <vector>
#include <string>

#include "mesh.h"
#include "material.h"

namespace example {

///
/// Loads glTF 2.0 mesh
///
bool LoadGLTF(const std::string &filename, float scale, std::vector<Mesh<float> > *meshes, std::vector<Material> *materials, std::vector<Texture> *textures);

}

#endif // EXAMPLE_GLTF_LOADER_H_
