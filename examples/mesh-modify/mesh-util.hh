#pragma once

#include <vector>
#include <map>

namespace example {

// TODO(syoyo): Support sparse accessor(sparse vertex attribute).
// TODO(syoyo): Support more data type
struct VertexAttrib {
  std::string name;

  // Value are converted to float type.
  std::vector<float> data;

  // Corresponding info in binary data
  int data_type;       // e.g. TINYGLTF_TYPE_VEC2
  int component_type;  // storage type. e.g.
                       // TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT
  uint64_t buffer_offset{0};
  size_t buffer_length{0};

  // Optional.
  std::vector<double> minValues;
  std::vector<double> maxValues;

};

struct MeshPrim {
  std::string name;
  int32_t id{-1};

  int mode; // e.g. TRIANGLES

  VertexAttrib position;                  // vec3
  VertexAttrib normal;                    // vec3
  VertexAttrib tangent;                   // vec4
  VertexAttrib color;                     // vec3 or vec4
  std::map<int, VertexAttrib> texcoords;  // <slot, attrib>  vec2
  std::map<int, VertexAttrib> weights;    // <slot, attrib>
  std::map<int, VertexAttrib>
      joints;  // <slot, attrib> store data as float type


  // min/max of index value. -1 = undef
  int indices_min{-1};
  int indices_max{-1};
  int indices_type{-1}; // storage type(componentType) of `indices`.
  std::vector<uint32_t> indices; // vertex indices
};

///
/// Save MeshPrim as wavefront .obj
///
bool SaveAsObjMesh(const std::string &filename, const MeshPrim &mesh);

///
/// Save MeshPrim as glTF mesh
///
bool SaveAsGLTFMesh(const std::string &filename, const MeshPrim &mesh);

///
/// Loads .obj and convert to MeshPrim
///
/// @param[in] facevarying Construct mesh with facevarying vertex layout(default false)
///
bool LoadObjMesh(const std::string &filename, bool facevarying, MeshPrim *mesh);

///
/// Print MeshPrim datra
///
void PrintMeshPrim(const MeshPrim &mesh);

} // namespace example
