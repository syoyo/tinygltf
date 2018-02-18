#ifndef EXAMPLE_OBJ_LOADER_H_
#define EXAMPLE_OBJ_LOADER_H_

#include <vector>
#include <string>

#include "mesh.h"
#include "material.h"

namespace example {

///
/// Loads wavefront .obj mesh
///
bool LoadObj(const std::string &filename, float scale, std::vector<Mesh<float> > *meshes, std::vector<Material> *materials, std::vector<Texture> *textures);

}

#endif // EXAMPLE_OBJ_LOADER_H_
