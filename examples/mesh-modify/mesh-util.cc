#include "mesh-util.hh"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

// ../common/
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// Include defines
#include "tiny_gltf.h"

namespace example {

#if 0
static inline int32_t GetComponentSizeInBytes(uint32_t componentType) {
  if (componentType == TINYGLTF_COMPONENT_TYPE_BYTE) {
    return 1;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
    return 1;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_SHORT) {
    return 2;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    return 2;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_INT) {
    return 4;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
    return 4;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
    return 4;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_DOUBLE) {
    return 8;
  } else {
    // Unknown componenty type
    return -1;
  }
}
#endif

static inline int32_t GetNumComponentsInType(uint32_t ty) {
  if (ty == TINYGLTF_TYPE_SCALAR) {
    return 1;
  } else if (ty == TINYGLTF_TYPE_VEC2) {
    return 2;
  } else if (ty == TINYGLTF_TYPE_VEC3) {
    return 3;
  } else if (ty == TINYGLTF_TYPE_VEC4) {
    return 4;
  } else if (ty == TINYGLTF_TYPE_MAT2) {
    return 4;
  } else if (ty == TINYGLTF_TYPE_MAT3) {
    return 9;
  } else if (ty == TINYGLTF_TYPE_MAT4) {
    return 16;
  } else {
    // Unknown componenty type
    return -1;
  }
}

size_t VertexAttrib::numElements() const {
  size_t n = GetNumComponentsInType(data_type);

  return data.size() / n;
}

// https://stackoverflow.com/questions/8520560/get-a-file-name-from-a-path
std::string GetBaseFilename(const std::string &filepath) {
  return filepath.substr(filepath.find_last_of("/\\") + 1);
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
  os << "(";
  for (size_t i = 0; i < v.size(); i++) {
    os << v[i];
    if (i != (v.size() - 1)) {
      os << ", ";
    }
  }
  os << ")";

  return os;
}

#if 0
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
#endif

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

// TODO(syoyo): Specify CCW(Counter-ClockWise) or CW(ClockWise)
static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
  float v10[3];
  v10[0] = v1[0] - v0[0];
  v10[1] = v1[1] - v0[1];
  v10[2] = v1[2] - v0[2];

  float v20[3];
  v20[0] = v2[0] - v0[0];
  v20[1] = v2[1] - v0[1];
  v20[2] = v2[2] - v0[2];

  N[0] = v20[1] * v10[2] - v20[2] * v10[1];
  N[1] = v20[2] * v10[0] - v20[0] * v10[2];
  N[2] = v20[0] * v10[1] - v20[1] * v10[0];

  float len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
  if (len2 > 0.0f) {
    float len = sqrtf(len2);

    N[0] /= len;
    N[1] /= len;
    N[2] /= len;
  }
}

static std::string make_triple(int i, bool has_vn, bool has_vt) {
  std::stringstream ss;

  if (has_vn && has_vt) {
    ss << i << "/" << i << "/" << i;
  } else if (has_vn) {
    ss << i << "//" << i;
  } else if (has_vt) {
    ss << i << "/" << i;
  } else {
    ss << i;
  }

  return ss.str();
}

namespace {

tinygltf::Accessor ConvertToGLTFAccessor(const VertexAttrib &attrib,
                                         int bufferView, size_t byteOffset) {
  tinygltf::Accessor accessor;

  accessor.bufferView = bufferView;
  accessor.name = attrib.name;
  accessor.byteOffset = byteOffset;
  accessor.componentType = attrib.component_type;
  accessor.count = attrib.numElements();
  accessor.type = attrib.data_type;
  accessor.minValues = attrib.minValues;
  accessor.maxValues = attrib.maxValues;

  return accessor;
}

// data is appended to `buf`
bool SerializeVertexAttribToBuffer(const VertexAttrib &attrib,
                                   std::vector<uint8_t> *buf, size_t *begin_loc,
                                   size_t *end_loc) {
  (*begin_loc) = buf->size();

  if (attrib.component_type == TINYGLTF_COMPONENT_TYPE_BYTE) {
    for (size_t i = 0; i < attrib.data.size(); i++) {
      int8_t d = static_cast<int8_t>(attrib.data[i]);
      buf->push_back(static_cast<uint8_t>(d));
    }
  } else if (attrib.component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
    for (size_t i = 0; i < attrib.data.size(); i++) {
      uint8_t d = static_cast<uint8_t>(attrib.data[i]);
      buf->push_back(d);
    }
  } else if (attrib.component_type == TINYGLTF_COMPONENT_TYPE_SHORT) {
    for (size_t i = 0; i < attrib.data.size(); i++) {
      int16_t d = static_cast<int16_t>(attrib.data[i]);
      uint8_t *ptr = reinterpret_cast<uint8_t *>(&d);
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
    }
  } else if (attrib.component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    for (size_t i = 0; i < attrib.data.size(); i++) {
      uint16_t d = static_cast<uint16_t>(attrib.data[i]);
      uint8_t *ptr = reinterpret_cast<uint8_t *>(&d);
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
    }
  } else if (attrib.component_type == TINYGLTF_COMPONENT_TYPE_INT) {
    for (size_t i = 0; i < attrib.data.size(); i++) {
      int32_t d = static_cast<int32_t>(attrib.data[i]);
      uint8_t *ptr = reinterpret_cast<uint8_t *>(&d);
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
    }
  } else if (attrib.component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
    for (size_t i = 0; i < attrib.data.size(); i++) {
      uint32_t d = static_cast<uint32_t>(attrib.data[i]);
      uint8_t *ptr = reinterpret_cast<uint8_t *>(&d);
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
    }
  } else if (attrib.component_type == TINYGLTF_COMPONENT_TYPE_FLOAT) {
    for (size_t i = 0; i < attrib.data.size(); i++) {
      float d = attrib.data[i];
      uint8_t *ptr = reinterpret_cast<uint8_t *>(&d);
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
    }
  } else {
    std::cerr << "Unsupported component type: "
              << PrintComponentType(attrib.component_type) << "\n";
    return false;
  }

  (*end_loc) = buf->size();

  return true;
}

// data is appended to `buf`
bool SerializeVertexIndicesToBuffer(const std::vector<uint32_t> &indices,
                                    int data_type, std::vector<uint8_t> *buf,
                                    size_t *begin_loc, size_t *end_loc) {
  // TODO(syoyo): Check alignment

  (*begin_loc) = buf->size();

  if (data_type == TINYGLTF_COMPONENT_TYPE_BYTE) {
    for (size_t i = 0; i < indices.size(); i++) {
      int8_t d = static_cast<int8_t>(indices[i]);
      buf->push_back(static_cast<uint8_t>(d));
    }
  } else if (data_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
    for (size_t i = 0; i < indices.size(); i++) {
      uint8_t d = static_cast<uint8_t>(indices[i]);
      buf->push_back(d);
    }
  } else if (data_type == TINYGLTF_COMPONENT_TYPE_SHORT) {
    for (size_t i = 0; i < indices.size(); i++) {
      int16_t d = static_cast<int16_t>(indices[i]);
      uint8_t *ptr = reinterpret_cast<uint8_t *>(&d);
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
    }
  } else if (data_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    for (size_t i = 0; i < indices.size(); i++) {
      uint16_t d = static_cast<uint16_t>(indices[i]);
      uint8_t *ptr = reinterpret_cast<uint8_t *>(&d);
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
    }
  } else if (data_type == TINYGLTF_COMPONENT_TYPE_INT) {
    for (size_t i = 0; i < indices.size(); i++) {
      int32_t d = static_cast<int32_t>(indices[i]);
      uint8_t *ptr = reinterpret_cast<uint8_t *>(&d);
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
    }
  } else if (data_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
    for (size_t i = 0; i < indices.size(); i++) {
      uint32_t d = static_cast<uint32_t>(indices[i]);
      uint8_t *ptr = reinterpret_cast<uint8_t *>(&d);
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
      ptr++;
      buf->push_back(*ptr);
    }
  } else {
    std::cerr << "Unsupported data type: " << PrintType(data_type) << "\n";
    return false;
  }

  (*end_loc) = buf->size();

  return true;
}

bool ConvertToGLTFMesh(const MeshPrim &mesh, int buffer_id,
                       tinygltf::Mesh *gltfmesh,
                       std::vector<tinygltf::Accessor> *accessors,
                       std::vector<tinygltf::BufferView> *bufferViews,
                       tinygltf::Buffer *buffer) {
  std::vector<uint8_t> buf;

  // single primitive per mesh
  tinygltf::Primitive primitive;

  // vertex index
  {
    size_t s, e;
    if (!SerializeVertexIndicesToBuffer(mesh.indices, mesh.indices_type, &buf,
                                        &s, &e)) {
      return false;
    }
    std::cout << "indices: [" << s << ", " << e << "]\n";

    tinygltf::BufferView bufferView;
    bufferView.buffer = buffer_id;
    bufferView.byteLength = e - s;
    bufferView.byteOffset = s;
    bufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
    bufferViews->push_back(bufferView);

    tinygltf::Accessor accessor;
    accessor.bufferView = bufferViews->size() - 1;
    accessor.minValues.resize(1);
    accessor.minValues[0] = mesh.indices_min;
    accessor.maxValues.resize(1);
    accessor.maxValues[0] = mesh.indices_max;
    accessor.count = mesh.indices.size();
    accessor.componentType = mesh.indices_type;
    accessor.type = TINYGLTF_TYPE_SCALAR;
    accessors->push_back(accessor);

    primitive.indices = accessors->size() - 1;
  }

  // position
  {
    size_t s, e;
    if (!SerializeVertexAttribToBuffer(mesh.position, &buf, &s, &e)) {
      return false;
    }
    std::cout << "postion.byteRange: [" << s << ", " << e << "]\n";

    tinygltf::BufferView bufferView;
    bufferView.buffer = buffer_id;
    bufferView.byteLength = e - s;
    bufferView.byteOffset = s;
    bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    bufferViews->push_back(bufferView);

    tinygltf::Accessor accessor = ConvertToGLTFAccessor(
        mesh.position, bufferViews->size() - 1, /* offset */ 0);
    accessors->push_back(accessor);

    primitive.attributes["POSITION"] = accessors->size() - 1;
  }

  if (mesh.normal.data.size() > 0) {
    size_t s, e;
    if (!SerializeVertexAttribToBuffer(mesh.normal, &buf, &s, &e)) {
      return false;
    }
    std::cout << "normal.byteRange: [" << s << ", " << e << "]\n";

    tinygltf::BufferView bufferView;
    bufferView.buffer = buffer_id;
    bufferView.byteLength = e - s;
    bufferView.byteOffset = s;
    bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    bufferViews->push_back(bufferView);

    tinygltf::Accessor accessor = ConvertToGLTFAccessor(
        mesh.normal, bufferViews->size() - 1, /* offset */ 0);
    accessors->push_back(accessor);

    primitive.attributes["NORMAL"] = accessors->size() - 1;
  }

  if (mesh.tangent.data.size() > 0) {
    size_t s, e;
    if (!SerializeVertexAttribToBuffer(mesh.tangent, &buf, &s, &e)) {
      return false;
    }
    std::cout << "tangent.byteRange: [" << s << ", " << e << "]\n";

    tinygltf::BufferView bufferView;
    bufferView.buffer = buffer_id;
    bufferView.byteLength = e - s;
    bufferView.byteOffset = s;
    bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    bufferViews->push_back(bufferView);

    tinygltf::Accessor accessor = ConvertToGLTFAccessor(
        mesh.tangent, bufferViews->size() - 1, /* offset */ 0);
    accessors->push_back(accessor);

    primitive.attributes["TANGENT"] = accessors->size() - 1;
  }

  if (mesh.texcoords.size() > 0) {
    for (const auto &item : mesh.texcoords) {
      size_t s, e;
      if (!SerializeVertexAttribToBuffer(item.second, &buf, &s, &e)) {
        return false;
      }
      std::cout << "tangent.byteRange: [" << s << ", " << e << "]\n";

      tinygltf::BufferView bufferView;
      bufferView.buffer = buffer_id;
      bufferView.byteLength = e - s;
      bufferView.byteOffset = s;
      bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
      bufferViews->push_back(bufferView);

      tinygltf::Accessor accessor = ConvertToGLTFAccessor(
          item.second, bufferViews->size() - 1, /* offset */ 0);
      accessors->push_back(accessor);

      std::string target = "TEXCOORD_" + std::to_string(item.first);
      primitive.attributes[target] = accessors->size() - 1;
    }
  }

  if (mesh.joints.size() > 0) {
    for (const auto &item : mesh.joints) {
      size_t s, e;
      if (!SerializeVertexAttribToBuffer(item.second, &buf, &s, &e)) {
        return false;
      }
      std::cout << "joint[" << item.first << "].byteRange: [" << s << ", " << e
                << "]\n";

      tinygltf::BufferView bufferView;
      bufferView.buffer = buffer_id;
      bufferView.byteLength = e - s;
      bufferView.byteOffset = s;
      bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
      bufferViews->push_back(bufferView);

      tinygltf::Accessor accessor = ConvertToGLTFAccessor(
          mesh.tangent, bufferViews->size() - 1, /* offset */ 0);
      accessors->push_back(accessor);

      std::string target = "JOINTS_" + std::to_string(item.first);
      primitive.attributes[target] = accessors->size() - 1;
    }
  }

  if (mesh.weights.size() > 0) {
    for (const auto &item : mesh.weights) {
      size_t s, e;
      if (!SerializeVertexAttribToBuffer(item.second, &buf, &s, &e)) {
        return false;
      }
      std::cout << "weight[" << item.first << "].byteRange: [" << s << ", " << e
                << "]\n";

      tinygltf::BufferView bufferView;
      bufferView.buffer = buffer_id;
      bufferView.byteLength = e - s;
      bufferView.byteOffset = s;
      bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
      bufferViews->push_back(bufferView);

      tinygltf::Accessor accessor = ConvertToGLTFAccessor(
          mesh.tangent, bufferViews->size() - 1, /* offset */ 0);
      accessors->push_back(accessor);

      std::string target = "WEIGHTS_" + std::to_string(item.first);
      primitive.attributes[target] = accessors->size() - 1;
    }
  }

  primitive.mode = mesh.mode;

  gltfmesh->primitives.push_back(primitive);

  buffer->data = buf;
  buffer->name = "bufffer0";  // temporary

  return true;
}

}  // namespace

bool SaveAsObjMesh(const std::string &filename, const MeshPrim &mesh,
                   bool flip_texcoord_y) {
  std::ofstream ofs(filename);
  if (!ofs) {
    std::cerr << "Failed to open .obj to write: " << filename << "\n";
    return false;
  }

  bool has_vn = false;
  bool has_vt = false;

  has_vn = mesh.normal.data.size() == mesh.position.data.size();
  has_vt = mesh.texcoords.count(0) &&
           (mesh.texcoords.at(0).data.size() > 0);  // TEXCOORD_0

  // v
  for (size_t i = 0; i < mesh.position.data.size() / 3; i++) {
    ofs << "v " << mesh.position.data[3 * i + 0] << " "
        << mesh.position.data[3 * i + 1] << " " << mesh.position.data[3 * i + 2]
        << "\n";
  }

  // vn
  for (size_t i = 0; i < mesh.normal.data.size() / 3; i++) {
    ofs << "vn " << mesh.normal.data[3 * i + 0] << " "
        << mesh.normal.data[3 * i + 1] << " " << mesh.normal.data[3 * i + 2]
        << "\n";
  }

  assert((mesh.texcoords.at(0).data.size() / 2) ==
         (mesh.position.data.size() / 3));

  // vt
  for (size_t i = 0; i < mesh.texcoords.at(0).data.size() / 2; i++) {
    float y = mesh.texcoords.at(0).data[2 * i + 1];
    if (flip_texcoord_y) {
      y = 1.0f - y;
    }
    ofs << "vt " << mesh.texcoords.at(0).data[2 * i + 0] << " " << y << "\n";
  }

  // v, vn, vt has same index
  for (size_t i = 0; i < mesh.indices.size() / 3; i++) {
    // .obj's index start with 1.
    int f0 = int(mesh.indices[3 * i + 0]) + 1;
    int f1 = int(mesh.indices[3 * i + 1]) + 1;
    int f2 = int(mesh.indices[3 * i + 2]) + 1;

    ofs << "f " << make_triple(f0, has_vn, has_vt) << " "
        << make_triple(f1, has_vn, has_vt) << " "
        << make_triple(f2, has_vn, has_vt) << "\n";
  }

  // TODO(syoyo): Write joints/weights

  return true;
}

bool SaveAsGLTFMesh(const std::string &filename, const MeshPrim &mesh) {
  tinygltf::TinyGLTF ctx;
  tinygltf::Model model;

  bool embedImages = false;
  bool embedBuffers = false;
  bool prettyPrint = true;
  bool writeBinary = false;

  // Create dummy node
  tinygltf::Node node;
  node.mesh = 0;
  node.name = "mesh";

  tinygltf::Scene scene;
  scene.nodes.push_back(0);

  model.defaultScene = 0;
  model.scenes.push_back(scene);
  model.nodes.push_back(node);

  int buffer_id = 0;
  tinygltf::Mesh gltfmesh;
  tinygltf::Buffer buffer;
  if (!ConvertToGLTFMesh(mesh, buffer_id, &gltfmesh, &(model.accessors),
                         &(model.bufferViews), &buffer)) {
    std::cerr << "Failed to convert Mesh\n";
    return false;
  }

  std::cout << "mesh.name: " << mesh.name << "\n";
  gltfmesh.name = mesh.name;

  model.meshes.push_back(gltfmesh);

  model.buffers.push_back(buffer);

  // Fill some required fields
  tinygltf::Asset asset;
  asset.version = "2.0";  // required
  asset.generator = "mesh-modify";

  model.asset = asset;

  bool ret = ctx.WriteGltfSceneToFile(&model, filename, embedImages,
                                      embedBuffers, prettyPrint, writeBinary);

  return ret;
}

bool RequireFacevaringLayout(const tinyobj::attrib_t &attrib,
                             const std::vector<tinyobj::shape_t> &shapes) {
  // Check if all normals and texcoords has same index with vertex
  if ((attrib.texcoords.size() / 2) != attrib.vertices.size() / 3) {
    std::cerr << "Texcoords and Vertices length mismatch. Mesh data cannot be "
                 "represented as non-facevarying. texcoords.size = "
              << (attrib.texcoords.size() / 2)
              << ", vertices.size = " << (attrib.vertices.size() / 3) << "\n";
    return true;
  }
  if ((attrib.normals.size() / 3) != attrib.vertices.size() / 3) {
    std::cerr << "Normals and Vertices length mismatch. Mesh data cannot be "
                 "represented as non-facevarying. normals.size = "
              << (attrib.normals.size() / 3)
              << ", vertices.szie = " << (attrib.vertices.size() / 3) << "\n";
    return true;
  }

  // Check indices.
  for (size_t s = 0; s < shapes.size(); s++) {
    const tinyobj::shape_t &shape = shapes[s];

    for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++) {
      tinyobj::index_t idx0 = shape.mesh.indices[3 * f + 0];
      tinyobj::index_t idx1 = shape.mesh.indices[3 * f + 1];
      tinyobj::index_t idx2 = shape.mesh.indices[3 * f + 2];

      // index must be all same
      if ((idx0.vertex_index != idx0.normal_index) ||
          (idx0.vertex_index != idx0.texcoord_index)) {
        return true;
      }

      if ((idx1.vertex_index != idx1.normal_index) ||
          (idx1.vertex_index != idx1.texcoord_index)) {
        return true;
      }

      if ((idx2.vertex_index != idx2.normal_index) ||
          (idx2.vertex_index != idx2.texcoord_index)) {
        return true;
      }
    }
  }

  return false;
}

static void ConstructVertexSkinWeight(
    const std::vector<tinyobj::real_t> &vertices,
    const std::vector<tinyobj::skin_weight_t> &skin_weights,
    std::vector<tinyobj::skin_weight_t> *vertex_skin_weights) {
  size_t num_vertices = vertices.size() / 3;

  vertex_skin_weights->resize(num_vertices);

  for (size_t i = 0; i < skin_weights.size(); i++) {
    const tinyobj::skin_weight_t &skin = skin_weights[i];

    assert(skin.vertex_id >= 0);
    assert(skin.vertex_id < num_vertices);

    (*vertex_skin_weights)[skin.vertex_id] = skin;
  }

  // now you can lookup i'th vertex skin weight by `vertex_skin_weights[i]`
}

bool LoadObjMesh(const std::string &filename, bool facevarying,
                 MeshPrim *mesh) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string warn;
  std::string err;
  bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                              filename.c_str(), /* base_path */ nullptr,
                              /* triangulate */ true);
  if (!warn.empty()) {
    std::cout << "WARN: " << warn << std::endl;
  }

  if (!err.empty()) {
    std::cerr << "ERR: " << err << std::endl;
  }

  if (!ret) {
    std::cerr << "Failed to load wavefront .obj\n";
    return false;
  }

  if (!facevarying) {
    // TODO(syoyo): Allow per-shape non-facevarying layout
    facevarying = RequireFacevaringLayout(attrib, shapes);
  }

  bool has_texcoord = (attrib.texcoords.size() > 0);
  bool has_normal = (attrib.normals.size() > 0);

  float bmin[3];
  float bmax[3];
  bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
  bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

  // reorder texcoords and normals so that it has same indexing to vertices.
  if (facevarying) {
    mesh->position.data.clear();
    mesh->normal.data.clear();
    mesh->tangent.data.clear();
    mesh->texcoords[0] = VertexAttrib();

    // Concat shapes
    for (size_t s = 0; s < shapes.size(); s++) {
      const tinyobj::shape_t &shape = shapes[s];

      for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++) {
        tinyobj::index_t idx0 = shape.mesh.indices[3 * f + 0];
        tinyobj::index_t idx1 = shape.mesh.indices[3 * f + 1];
        tinyobj::index_t idx2 = shape.mesh.indices[3 * f + 2];

        float tc[3][2];
        if (has_texcoord) {
          if ((idx0.texcoord_index < 0) || (idx1.texcoord_index < 0) ||
              (idx2.texcoord_index < 0)) {
            // This face does contain valid texcoord
            tc[0][0] = 0.0f;
            tc[0][1] = 0.0f;
            tc[1][0] = 0.0f;
            tc[1][1] = 0.0f;
            tc[2][0] = 0.0f;
            tc[2][1] = 0.0f;
          } else {
            assert(attrib.texcoords.size() >
                   size_t(2 * idx0.texcoord_index + 1));
            assert(attrib.texcoords.size() >
                   size_t(2 * idx1.texcoord_index + 1));
            assert(attrib.texcoords.size() >
                   size_t(2 * idx2.texcoord_index + 1));

            // Flip Y coord.
            tc[0][0] = attrib.texcoords[2 * idx0.texcoord_index];
            tc[0][1] = 1.0f - attrib.texcoords[2 * idx0.texcoord_index + 1];
            tc[1][0] = attrib.texcoords[2 * idx1.texcoord_index];
            tc[1][1] = 1.0f - attrib.texcoords[2 * idx1.texcoord_index + 1];
            tc[2][0] = attrib.texcoords[2 * idx2.texcoord_index];
            tc[2][1] = 1.0f - attrib.texcoords[2 * idx2.texcoord_index + 1];
          }
        } else {
          tc[0][0] = 0.0f;
          tc[0][1] = 0.0f;
          tc[1][0] = 0.0f;
          tc[1][1] = 0.0f;
          tc[2][0] = 0.0f;
          tc[2][1] = 0.0f;
        }

        float v[3][3];
        for (int k = 0; k < 3; k++) {
          int f0 = idx0.vertex_index;
          int f1 = idx1.vertex_index;
          int f2 = idx2.vertex_index;
          assert(f0 >= 0);
          assert(f1 >= 0);
          assert(f2 >= 0);

          v[0][k] = attrib.vertices[3 * f0 + k];
          v[1][k] = attrib.vertices[3 * f1 + k];
          v[2][k] = attrib.vertices[3 * f2 + k];
          bmin[k] = std::min(v[0][k], bmin[k]);
          bmin[k] = std::min(v[1][k], bmin[k]);
          bmin[k] = std::min(v[2][k], bmin[k]);
          bmax[k] = std::max(v[0][k], bmax[k]);
          bmax[k] = std::max(v[1][k], bmax[k]);
          bmax[k] = std::max(v[2][k], bmax[k]);
        }

        float n[3][3];
        if (has_normal) {
          if ((idx0.normal_index < 0) || (idx1.normal_index < 0) ||
              (idx2.normal_index < 0)) {
            // This face does contain valid normal
            // Calc geometric normal
            CalcNormal(n[0], v[0], v[1], v[2]);
            n[1][0] = n[0][0];
            n[1][1] = n[0][1];
            n[1][2] = n[0][2];

            n[2][0] = n[0][0];
            n[2][1] = n[0][1];
            n[2][2] = n[0][2];

          } else {
            int nf0 = idx0.normal_index;
            int nf1 = idx1.normal_index;
            int nf2 = idx2.normal_index;

            for (int k = 0; k < 3; k++) {
              assert(size_t(3 * nf0 + k) < attrib.normals.size());
              assert(size_t(3 * nf1 + k) < attrib.normals.size());
              assert(size_t(3 * nf2 + k) < attrib.normals.size());
              n[0][k] = attrib.normals[3 * nf0 + k];
              n[1][k] = attrib.normals[3 * nf1 + k];
              n[2][k] = attrib.normals[3 * nf2 + k];
            }
          }
        } else {
          // Calc geometric normal
          CalcNormal(n[0], v[0], v[1], v[2]);
          n[1][0] = n[0][0];
          n[1][1] = n[0][1];
          n[1][2] = n[0][2];

          n[2][0] = n[0][0];
          n[2][1] = n[0][1];
          n[2][2] = n[0][2];
        }

        mesh->position.data.push_back(v[0][0]);
        mesh->position.data.push_back(v[0][1]);
        mesh->position.data.push_back(v[0][2]);

        mesh->position.data.push_back(v[1][0]);
        mesh->position.data.push_back(v[1][1]);
        mesh->position.data.push_back(v[1][2]);

        mesh->position.data.push_back(v[2][0]);
        mesh->position.data.push_back(v[2][1]);
        mesh->position.data.push_back(v[2][2]);

        mesh->normal.data.push_back(n[0][0]);
        mesh->normal.data.push_back(n[0][1]);
        mesh->normal.data.push_back(n[0][2]);

        mesh->normal.data.push_back(n[1][0]);
        mesh->normal.data.push_back(n[1][1]);
        mesh->normal.data.push_back(n[1][2]);

        mesh->normal.data.push_back(n[2][0]);
        mesh->normal.data.push_back(n[2][1]);
        mesh->normal.data.push_back(n[2][2]);

        mesh->texcoords[0].data.push_back(tc[0][0]);
        mesh->texcoords[0].data.push_back(tc[0][1]);

        mesh->texcoords[0].data.push_back(tc[1][0]);
        mesh->texcoords[0].data.push_back(tc[1][1]);

        mesh->texcoords[0].data.push_back(tc[2][0]);
        mesh->texcoords[0].data.push_back(tc[2][1]);

        size_t idx = mesh->indices.size();
        mesh->indices.push_back(int(idx) + 0);
        mesh->indices.push_back(int(idx) + 1);
        mesh->indices.push_back(int(idx) + 2);
      }
    }

    // weights/joints
    if (attrib.skin_weights.size() > 0) {
      // Reorder vertex skin weights
      std::vector<tinyobj::skin_weight_t> vertex_skin_weights;
      ConstructVertexSkinWeight(attrib.vertices, attrib.skin_weights,
                                &vertex_skin_weights);

      // Find max # of slots.
      size_t maxn = 0;
      for (size_t i = 0; i < vertex_skin_weights.size(); i++) {
        maxn = std::max(vertex_skin_weights[i].weightValues.size(), maxn);
      }

      int num_slots = 0;
      if (maxn > 0) {
        num_slots = (((maxn - 1) / 4) + 1) * 4;
      }
      std::cout << "# of slots = " << num_slots << "\n";

      for (size_t t = 0; t < size_t(num_slots); t++) {
        VertexAttrib weights, joints;

        size_t num_faceverts = mesh->indices.size();

        // facevarying weights/joints Fill with zeros
        weights.data.resize(4 * num_faceverts, 0.0f);
        joints.data.resize(4 * num_faceverts, 0.0f);

        for (size_t s = 0; s < shapes.size(); s++) {
          const tinyobj::shape_t &shape = shapes[s];

          for (size_t f = 0; f < shape.mesh.indices.size(); f++) {
            tinyobj::index_t idx = shape.mesh.indices[f];

            size_t vid = idx.vertex_index;

            assert(vid < vertex_skin_weights.size());
            const tinyobj::skin_weight_t &sw = vertex_skin_weights[vid];

            for (size_t j0 = 0; j0 < 4; j0++) {
              size_t j = t * 4 + j0;
              if (j < sw.weightValues.size()) {
                joints.data[4 * vid + j0] = float(sw.weightValues[j].joint_id);
                weights.data[4 * vid + j0] = float(sw.weightValues[j].weight);
              }
            }
          }
        }

        mesh->weights[t] = weights;
        mesh->joints[t] = joints;
      }
    }

  } else {
    // position/texcoord/normal can be represented in shared vertex manner

    mesh->position.data.clear();
    for (size_t v = 0; v < attrib.vertices.size(); v++) {
      mesh->position.data.push_back(attrib.vertices[v]);
    }

    mesh->normal.data.clear();
    for (size_t v = 0; v < attrib.normals.size(); v++) {
      mesh->normal.data.push_back(attrib.normals[v]);
    }

    mesh->texcoords[0] = VertexAttrib();
    for (size_t v = 0; v < attrib.texcoords.size(); v++) {
      mesh->texcoords[0].data.push_back(attrib.texcoords[v]);
    }

    mesh->indices_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
    mesh->indices.clear();

    size_t face_index_offset = 0;
    for (size_t s = 0; s < shapes.size(); s++) {
      const tinyobj::shape_t &shape = shapes[s];

      for (size_t f = 0; f < shape.mesh.indices.size(); f++) {
        mesh->indices.push_back(uint32_t(face_index_offset) +
                                uint32_t(shape.mesh.indices[f].vertex_index));
      }

      face_index_offset = mesh->indices.size();
    }

    // weights/joints
    if (attrib.skin_weights.size() > 0) {
      // Find max # of slots.
      size_t maxn = 0;
      for (size_t i = 0; i < attrib.skin_weights.size(); i++) {
        maxn = std::max(attrib.skin_weights[i].weightValues.size(), maxn);
      }

      int num_slots = 0;
      if (maxn > 0) {
        num_slots = (((maxn - 1) / 4) + 1) * 4;
      }
      std::cout << "# of slots = " << num_slots << "\n";

      for (size_t s = 0; s < size_t(num_slots); s++) {
        VertexAttrib weights, joints;

        // Fill with zeros
        weights.data.resize(4 * (mesh->position.data.size() / 3), 0.0f);
        joints.data.resize(4 * (mesh->position.data.size() / 3), 0.0f);

        for (size_t v = 0; v < attrib.skin_weights.size(); v++) {
          const tinyobj::skin_weight_t &sw = attrib.skin_weights[v];

          assert(sw.vertex_id < (mesh->position.data.size() / 3));

          size_t dst_vid = sw.vertex_id;

          for (size_t j0 = 0; j0 < 4; j0++) {
            size_t j = s * 4 + j0;
            if (j < sw.weightValues.size()) {
              joints.data[4 * dst_vid + j0] =
                  float(sw.weightValues[j].joint_id);
              weights.data[4 * dst_vid + j0] = float(sw.weightValues[j].weight);
            }
          }
        }

        weights.data_type = TINYGLTF_TYPE_VEC4;
        weights.component_type =
            TINYGLTF_COMPONENT_TYPE_FLOAT;  // storage format
        mesh->weights[s] = weights;

        joints.data_type = TINYGLTF_TYPE_VEC4;
        joints.component_type =
            TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;  // storage format

        mesh->joints[s] = joints;
      }
    }
  }

  // postprocessing. e.g. find min/max
  {
    {
      uint32_t minv = 0.0;
      uint32_t maxv = 0.0;
      for (size_t i = 0; i < mesh->indices.size(); i++) {
        minv = std::min(minv, uint32_t(mesh->indices[i]));
        maxv = std::max(maxv, uint32_t(mesh->indices[i]));
      }

      mesh->indices_min = int(minv);
      mesh->indices_max = int(maxv);

      mesh->indices_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
    }

    {
      float bmin[3];
      float bmax[3];
      bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
      bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

      for (size_t i = 0; i < mesh->position.data.size() / 3; i++) {
        for (size_t k = 0; k < 3; k++) {
          bmin[k] = std::min(bmin[k], mesh->position.data[3 * i + k]);
          bmax[k] = std::max(bmax[k], mesh->position.data[3 * i + k]);
        }
      }

      mesh->position.minValues.resize(3);
      mesh->position.minValues[0] = bmin[0];
      mesh->position.minValues[1] = bmin[1];
      mesh->position.minValues[2] = bmin[2];

      mesh->position.maxValues.resize(3);
      mesh->position.maxValues[0] = bmax[0];
      mesh->position.maxValues[1] = bmax[1];
      mesh->position.maxValues[2] = bmax[2];

      mesh->position.data_type = TINYGLTF_TYPE_VEC3;
      mesh->position.component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
    }

    {
      float bmin[3];
      float bmax[3];
      bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
      bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

      for (size_t i = 0; i < mesh->normal.data.size() / 3; i++) {
        for (size_t k = 0; k < 3; k++) {
          bmin[k] = std::min(bmin[k], mesh->normal.data[3 * i + k]);
          bmax[k] = std::max(bmax[k], mesh->normal.data[3 * i + k]);
        }
      }

      mesh->normal.minValues.resize(3);
      mesh->normal.minValues[0] = bmin[0];
      mesh->normal.minValues[1] = bmin[1];
      mesh->normal.minValues[2] = bmin[2];

      mesh->normal.maxValues.resize(3);
      mesh->normal.maxValues[0] = bmax[0];
      mesh->normal.maxValues[1] = bmax[1];
      mesh->normal.maxValues[2] = bmax[2];

      mesh->normal.data_type = TINYGLTF_TYPE_VEC3;
      mesh->normal.component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
    }

    {
      float bmin[4];
      float bmax[4];
      bmin[0] = bmin[1] = bmin[2] = bmin[3] = std::numeric_limits<float>::max();
      bmax[0] = bmax[1] = bmax[2] = bmin[3] =
          -std::numeric_limits<float>::max();

      size_t n = 3;

      for (size_t i = 0; i < mesh->tangent.data.size() / n; i++) {
        for (size_t k = 0; k < n; k++) {
          bmin[k] = std::min(bmin[k], mesh->tangent.data[n * i + k]);
          bmax[k] = std::max(bmax[k], mesh->tangent.data[n * i + k]);
        }
      }

      mesh->tangent.minValues.resize(n);
      mesh->tangent.maxValues.resize(n);
      for (size_t k = 0; k < n; k++) {
        mesh->tangent.minValues[k] = bmin[k];
        mesh->tangent.maxValues[k] = bmax[k];
      }

      mesh->tangent.data_type =
          (n == 3) ? TINYGLTF_TYPE_VEC3 : TINYGLTF_TYPE_VEC4;
      mesh->tangent.component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
    }

    // texcoord
    for (auto &item : mesh->texcoords) {
      float bmin[2];
      float bmax[2];
      bmin[0] = bmin[1] = std::numeric_limits<float>::max();
      bmax[0] = bmax[1] = -std::numeric_limits<float>::max();

      for (size_t i = 0; i < item.second.data.size() / 2; i++) {
        for (size_t k = 0; k < 2; k++) {
          bmin[k] = std::min(bmin[k], item.second.data[2 * i + k]);
          bmax[k] = std::max(bmax[k], item.second.data[2 * i + k]);
        }
      }

      item.second.minValues.resize(2);
      item.second.maxValues.resize(2);
      for (size_t k = 0; k < 2; k++) {
        item.second.minValues[k] = bmin[k];
        item.second.maxValues[k] = bmax[k];
      }

      item.second.data_type = TINYGLTF_TYPE_VEC2;
      item.second.component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
    }

    // joints
    for (auto &item : mesh->joints) {
      float bmin;
      float bmax;
      bmin = std::numeric_limits<float>::max();
      bmax = -std::numeric_limits<float>::max();

      for (size_t i = 0; i < item.second.data.size(); i++) {
        bmin = std::min(bmin, item.second.data[i]);
        bmax = std::max(bmax, item.second.data[i]);
      }

      item.second.minValues.resize(1);
      item.second.maxValues.resize(1);

      item.second.minValues[0] = bmin;
      item.second.maxValues[0] = bmax;

      item.second.data_type = TINYGLTF_TYPE_VEC4;
      item.second.component_type =
          TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;  // storage format
    }

    // weights
    for (auto &item : mesh->weights) {
      float bmin;
      float bmax;
      bmin = std::numeric_limits<float>::max();
      bmax = -std::numeric_limits<float>::max();

      for (size_t i = 0; i < item.second.data.size(); i++) {
        bmin = std::min(bmin, item.second.data[i]);
        bmax = std::max(bmax, item.second.data[i]);
      }

      item.second.minValues.resize(1);
      item.second.maxValues.resize(1);

      item.second.minValues[0] = bmin;
      item.second.maxValues[0] = bmax;

      item.second.data_type = TINYGLTF_TYPE_VEC4;
      item.second.component_type =
          TINYGLTF_COMPONENT_TYPE_FLOAT;  // storage format
    }
  }

  // Use filename as mesh's name
  mesh->name = GetBaseFilename(filename);

  mesh->mode = TINYGLTF_MODE_TRIANGLES;

  return true;
}

void PrintMeshPrim(const MeshPrim &mesh) {
  std::cout << "indices.component_type : "
            << PrintComponentType(mesh.indices_type) << "\n";
  std::cout << "# of indices : " << mesh.indices.size() << "\n";
  std::cout << "  indices.min = " << mesh.indices_min
            << ", max = " << mesh.indices_max << "\n";
  for (size_t i = 0; i < mesh.indices.size(); i++) {
    std::cout << "  index[" << i << "] = " << mesh.indices[i] << "\n";
  }

  std::cout << "position.type : " << PrintType(mesh.position.data_type) << "\n";
  std::cout << "position.component_type : "
            << PrintComponentType(mesh.position.component_type) << "\n";
  std::cout << "# of positions : " << mesh.position.data.size() / 3 << "\n";
  if ((mesh.position.minValues.size() == 3) &&
      (mesh.position.maxValues.size() == 3)) {
    std::cout << "  position.min = " << mesh.position.minValues
              << ", max = " << mesh.position.maxValues << "\n";
  }
  for (size_t i = 0; i < mesh.position.data.size() / 3; i++) {
    std::cout << "  position[" << i << "] = " << mesh.position.data[3 * i + 0]
              << ", " << mesh.position.data[3 * i + 1] << ", "
              << mesh.position.data[3 * i + 2] << std::endl;
  }

  std::cout << "normal.type : " << PrintType(mesh.normal.data_type) << "\n";
  std::cout << "normal.component_type : "
            << PrintComponentType(mesh.normal.component_type) << "\n";
  std::cout << "# of normals : " << mesh.normal.data.size() / 3 << "\n";
  if ((mesh.normal.minValues.size() == 3) &&
      (mesh.normal.maxValues.size() == 3)) {
    std::cout << "  normal.min = " << mesh.normal.minValues
              << ", max = " << mesh.normal.maxValues << "\n";
  }
  for (size_t i = 0; i < mesh.normal.data.size() / 3; i++) {
    std::cout << "  normal[" << i << "] = " << mesh.normal.data[3 * i + 0]
              << ", " << mesh.normal.data[3 * i + 1] << ", "
              << mesh.normal.data[3 * i + 2] << std::endl;
  }

  if (mesh.tangent.data.size() > 0) {
    assert((mesh.tangent.data_type == TINYGLTF_TYPE_VEC3) ||
           (mesh.tangent.data_type == TINYGLTF_TYPE_VEC4));

    size_t n = mesh.tangent.data_type == TINYGLTF_TYPE_VEC3 ? 3 : 4;

    std::cout << "tangent.type : " << PrintType(mesh.tangent.data_type) << "\n";
    std::cout << "tangent.component_type : "
              << PrintComponentType(mesh.tangent.component_type) << "\n";
    std::cout << "# of tangents : " << mesh.tangent.data.size() / n << "\n";
    if ((mesh.tangent.minValues.size() == 3) &&
        (mesh.tangent.maxValues.size() == 3)) {
      std::cout << "  tangent.min = " << mesh.tangent.minValues
                << ", max = " << mesh.tangent.maxValues << "\n";
    }
    for (size_t i = 0; i < mesh.tangent.data.size() / n; i++) {
      std::cout << "  tangent[" << i << "] = " << mesh.tangent.data[n * i + 0]
                << ", " << mesh.tangent.data[n * i + 1] << ", "
                << mesh.tangent.data[n * i + 2];

      if (n == 4) {
        std::cout << ", " << mesh.tangent.data[n * i + 3];
      }
      std::cout << std::endl;
    }
  }

  std::cout << "# of texcoord slots : " << mesh.texcoords.size() << "\n";
  for (const auto &item : mesh.texcoords) {
    std::cout << "TEXCOORD_" << item.first << "\n";

    assert(item.second.data_type == TINYGLTF_TYPE_VEC2);
    std::cout << "texcoord.type : " << PrintType(item.second.data_type) << "\n";
    std::cout << "texcoord.component_type : "
              << PrintComponentType(item.second.component_type) << "\n";
    std::cout << "# of texcoords : " << item.second.data.size() / 2 << "\n";
    if ((item.second.minValues.size() == 2) &&
        (item.second.maxValues.size() == 2)) {
      std::cout << "  texcood.min = " << item.second.minValues
                << ", max = " << item.second.maxValues << "\n";
    }
    for (size_t i = 0; i < item.second.data.size() / 2; i++) {
      std::cout << "  texcoord[" << i << "] = " << item.second.data[2 * i + 0]
                << ", " << item.second.data[2 * i + 1];
      std::cout << std::endl;
    }
  }

  assert(mesh.joints.size() == mesh.weights.size());
  std::cout << "# of joints/weights slots : " << mesh.joints.size() << "\n";
  for (const auto &item : mesh.joints) {
    assert(mesh.weights.count(item.first));

    assert(item.second.data_type == TINYGLTF_TYPE_VEC4);

    // joint must be uint8 or uint16
    assert(
        (item.second.component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) ||
        (item.second.component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT));

    std::cout << "joint.type : " << PrintType(item.second.data_type) << "\n";
    std::cout << "joint.component_type : "
              << PrintComponentType(item.second.component_type) << "\n";

    std::cout << "JOINTS_" << item.first << "\n";
    for (size_t i = 0; i < item.second.data.size(); i++) {
      std::cout << "  joints[" << i << "] = " << int(item.second.data[i])
                << "\n";
    }

    const VertexAttrib &attrib = mesh.weights.at(item.first);

    // weight must be uint8 or uint16(normalized), or float
    assert((attrib.component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) ||
           (attrib.component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) ||
           (attrib.component_type == TINYGLTF_COMPONENT_TYPE_FLOAT));

    std::cout << "weight.type : " << PrintType(attrib.data_type) << "\n";
    std::cout << "weight.component_type : "
              << PrintComponentType(attrib.component_type) << "\n";
    std::cout << "WEIGHTS_" << item.first << "\n";
    for (size_t i = 0; i < attrib.data.size(); i++) {
      std::cout << "  weights[" << i << "] = " << attrib.data[i] << "\n";
    }
  }
}

}  // namespace example
