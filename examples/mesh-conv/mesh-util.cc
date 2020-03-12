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
  int prim_id = 0;
  std::vector<uint8_t> buf;

  // single primitive per mesh
  tinygltf::Primitive primitive;

  const PrimSet &prim = mesh.prims[prim_id];

  // vertex index
  {
    size_t s, e;
    if (!SerializeVertexIndicesToBuffer(prim.indices, prim.indices_type, &buf,
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
    accessor.name = mesh.name + "#" + std::to_string(prim_id) + "/indices";
    accessor.bufferView = bufferViews->size() - 1;
    accessor.minValues.resize(1);
    accessor.minValues[0] = prim.indices_min;
    accessor.maxValues.resize(1);
    accessor.maxValues[0] = prim.indices_max;
    accessor.count = prim.indices.size();
    accessor.componentType = prim.indices_type;
    accessor.type = TINYGLTF_TYPE_SCALAR;
    accessors->push_back(accessor);

    primitive.indices = accessors->size() - 1;
  }

  // position
  {
    size_t s, e;
    if (!SerializeVertexAttribToBuffer(prim.position, &buf, &s, &e)) {
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
        prim.position, bufferViews->size() - 1, /* offset */ 0);
    accessor.name = mesh.name + "#" + std::to_string(prim_id) + "/POSITION";
    accessors->push_back(accessor);

    primitive.attributes["POSITION"] = accessors->size() - 1;
  }

  if (prim.normal.data.size() > 0) {
    size_t s, e;
    if (!SerializeVertexAttribToBuffer(prim.normal, &buf, &s, &e)) {
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
        prim.normal, bufferViews->size() - 1, /* offset */ 0);
    accessor.name = mesh.name + "#" + std::to_string(prim_id) + "/NORMAL";
    accessors->push_back(accessor);

    primitive.attributes["NORMAL"] = accessors->size() - 1;
  }

  if (prim.tangent.data.size() > 0) {
    size_t s, e;
    if (!SerializeVertexAttribToBuffer(prim.tangent, &buf, &s, &e)) {
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
        prim.tangent, bufferViews->size() - 1, /* offset */ 0);
    accessor.name = mesh.name + "#" + std::to_string(prim_id) + "/TANGENT";
    accessors->push_back(accessor);

    primitive.attributes["TANGENT"] = accessors->size() - 1;
  }

  if (prim.texcoords.size() > 0) {
    for (const auto &item : prim.texcoords) {
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
      accessor.name = mesh.name + "#" + std::to_string(prim_id) + "/TEXCOORD_" + std::to_string(item.first);
      accessors->push_back(accessor);

      std::string target = "TEXCOORD_" + std::to_string(item.first);
      primitive.attributes[target] = accessors->size() - 1;
    }
  }

  if (prim.joints.size() > 0) {
    for (const auto &item : prim.joints) {
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
          item.second, bufferViews->size() - 1, /* offset */ 0);
      accessor.name = mesh.name + "#" + std::to_string(prim_id) + "/JOINTS_" + std::to_string(item.first);
      accessors->push_back(accessor);

      std::string target = "JOINTS_" + std::to_string(item.first);
      primitive.attributes[target] = accessors->size() - 1;
    }
  }

  if (prim.weights.size() > 0) {
    for (const auto &item : prim.weights) {
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
          item.second, bufferViews->size() - 1, /* offset */ 0);
      accessor.name = mesh.name + "#" + std::to_string(prim_id) + "/WEIGHTS_" + std::to_string(item.first);
      accessors->push_back(accessor);

      std::string target = "WEIGHTS_" + std::to_string(item.first);
      primitive.attributes[target] = accessors->size() - 1;
    }
  }

  primitive.mode = prim.mode;

  gltfmesh->primitives.push_back(primitive);

  buffer->data = buf;
  buffer->name = "bufffer0";  // temporary

  return true;
}

bool HasValidSkinWeights(const PrimSet &prim)
{
  if ((prim.weights.size() > 0) && (prim.weights.size() == prim.joints.size())) {

    if (prim.weights.size() != prim.joints.size()) {
      std::cerr << "# of JOINTS(" << prim.joints.size() << ") and WEIGHTS(" << prim.weights.size() << ") differs\n";
      return false;
    }

    size_t num_slots = prim.weights.size();

    // Assume weight slots are tightly packed.
    for (size_t slot = 0; slot < num_slots; slot++) {
      if (!prim.weights.count(slot)) {
        std::cerr << "WEIGHTS_" << slot << " not found.\n";
        return false;;
      }

      if (!prim.joints.count(slot)) {
        std::cerr << "JOINTS_" << slot << " not found.\n";
        return false;;
      }
    }

    return true;
  }

  return false;
}

}  // namespace

bool SaveAsObjMesh(const std::string &basename, const MeshPrim &mesh, const ObjExportOption &options) {
  if (options.primid >= mesh.prims.size()) {
    std::cerr << "mesh( " << mesh.name << ") does not contain " << options.primid << "th primitive. mesh.primitives.length = " << mesh.prims.size() << "\n";
    return false;
  }

  const PrimSet &prim = mesh.prims[options.primid];

  if (prim.texcoords.count(options.uvset)) {
    std::cerr << "Exporting uvset " << options.uvset << " requested, but mesh( " << mesh.name << ") does not contain TEXCOORD_" << options.uvset << "\n";
    std::cerr << "UV coord will not be exported to .obj\n";
  }

  std::string obj_filename = basename  + "_" + std::to_string(options.primid) + ".obj";
  std::ofstream ofs(obj_filename);
  if (!ofs) {
    std::cerr << "Failed to open .obj to write: " << obj_filename << "\n";
    return false;
  }


  bool has_vn = false;
  bool has_vt = false;

  has_vn = prim.normal.data.size() == prim.position.data.size();
  has_vt = prim.texcoords.count(options.uvset) &&
           (prim.texcoords.at(options.uvset).data.size() > 0);

  // v
  for (size_t i = 0; i < prim.position.data.size() / 3; i++) {
    ofs << "v " << prim.position.data[3 * i + 0] << " "
        << prim.position.data[3 * i + 1] << " " << prim.position.data[3 * i + 2]
        << "\n";
  }

  // vn
  for (size_t i = 0; i < prim.normal.data.size() / 3; i++) {
    ofs << "vn " << prim.normal.data[3 * i + 0] << " "
        << prim.normal.data[3 * i + 1] << " " << prim.normal.data[3 * i + 2]
        << "\n";
  }


  if (has_vt) {
    assert((prim.texcoords.at(options.uvset).data.size() / 2) == (prim.position.data.size() / 3));

    // vt
    for (size_t i = 0; i < prim.texcoords.at(options.uvset).data.size() / 2; i++) {
      float y = prim.texcoords.at(options.uvset).data[2 * i + 1];
      if (options.flip_texcoord_y) {
        y = 1.0f - y;
      }
      ofs << "vt " << prim.texcoords.at(options.uvset).data[2 * i + 0] << " " << y << "\n";
    }
  }

  if (options.export_skinweights && HasValidSkinWeights(prim)) {

    // WEIGHTS_ and JOINTS_ slots are tightly packed.

    size_t num_slots = prim.weights.size();

    for (size_t v = 0; v < prim.position.data.size() / 3; v++) { // vec3

      std::vector<float> weights(num_slots * 4, 0.0f);
      std::vector<float> joints(num_slots * 4, 0.0f);

      for (size_t slot = 0; slot < num_slots; slot++) {
        for (size_t k = 0; k < 4; k++) {
          weights[slot * 4 + k] = prim.weights.at(slot).data[4 * v + k];
          joints[slot * 4 + k] = prim.joints.at(slot).data[4 * v + k];
        }
      }

      // vertex index start with 0.
      ofs << "vw " << v << " " ;
      for (size_t i = 0; i < weights.size(); i++) {
        ofs << int(joints[i]) << " " << weights[i] << " ";
      }
      ofs << "\n";
    }

  }

  // v, vn, vt has same index
  for (size_t i = 0; i < prim.indices.size() / 3; i++) {
    // .obj's index start with 1.
    int f0 = int(prim.indices[3 * i + 0]) + 1;
    int f1 = int(prim.indices[3 * i + 1]) + 1;
    int f2 = int(prim.indices[3 * i + 2]) + 1;

    ofs << "f " << make_triple(f0, has_vn, has_vt) << " "
        << make_triple(f1, has_vn, has_vt) << " "
        << make_triple(f2, has_vn, has_vt) << "\n";
  }


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

  PrimSet prim;

  // reorder texcoords and normals so that it has same indexing to vertices.
  if (facevarying) {
    prim.position.data.clear();
    prim.normal.data.clear();
    prim.tangent.data.clear();
    prim.texcoords[0] = VertexAttrib();

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

        prim.position.data.push_back(v[0][0]);
        prim.position.data.push_back(v[0][1]);
        prim.position.data.push_back(v[0][2]);

        prim.position.data.push_back(v[1][0]);
        prim.position.data.push_back(v[1][1]);
        prim.position.data.push_back(v[1][2]);

        prim.position.data.push_back(v[2][0]);
        prim.position.data.push_back(v[2][1]);
        prim.position.data.push_back(v[2][2]);

        prim.normal.data.push_back(n[0][0]);
        prim.normal.data.push_back(n[0][1]);
        prim.normal.data.push_back(n[0][2]);

        prim.normal.data.push_back(n[1][0]);
        prim.normal.data.push_back(n[1][1]);
        prim.normal.data.push_back(n[1][2]);

        prim.normal.data.push_back(n[2][0]);
        prim.normal.data.push_back(n[2][1]);
        prim.normal.data.push_back(n[2][2]);

        prim.texcoords[0].data.push_back(tc[0][0]);
        prim.texcoords[0].data.push_back(tc[0][1]);

        prim.texcoords[0].data.push_back(tc[1][0]);
        prim.texcoords[0].data.push_back(tc[1][1]);

        prim.texcoords[0].data.push_back(tc[2][0]);
        prim.texcoords[0].data.push_back(tc[2][1]);

        size_t idx = prim.indices.size();
        prim.indices.push_back(int(idx) + 0);
        prim.indices.push_back(int(idx) + 1);
        prim.indices.push_back(int(idx) + 2);
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

      std::cout << "Max # of weights = " << maxn << "\n";
      int num_slots = 0;
      if (maxn > 0) {
        num_slots = maxn / 4;
      }
      std::cout << "Max # of slots = " << num_slots << "\n";

      for (size_t t = 0; t < size_t(num_slots); t++) {
        VertexAttrib weights, joints;

        size_t num_faceverts = prim.indices.size();

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

        prim.weights[t] = weights;
        prim.joints[t] = joints;
      }
    }

  } else {
    // position/texcoord/normal can be represented in shared vertex manner

    prim.position.data.clear();
    for (size_t v = 0; v < attrib.vertices.size(); v++) {
      prim.position.data.push_back(attrib.vertices[v]);
    }

    prim.normal.data.clear();
    for (size_t v = 0; v < attrib.normals.size(); v++) {
      prim.normal.data.push_back(attrib.normals[v]);
    }

    prim.texcoords[0] = VertexAttrib();
    for (size_t v = 0; v < attrib.texcoords.size(); v++) {
      prim.texcoords[0].data.push_back(attrib.texcoords[v]);
    }

    prim.indices_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
    prim.indices.clear();

    size_t face_index_offset = 0;
    for (size_t s = 0; s < shapes.size(); s++) {
      const tinyobj::shape_t &shape = shapes[s];

      for (size_t f = 0; f < shape.mesh.indices.size(); f++) {
        prim.indices.push_back(uint32_t(face_index_offset) +
                                uint32_t(shape.mesh.indices[f].vertex_index));
      }

      face_index_offset = prim.indices.size();
    }

    // weights/joints
    if (attrib.skin_weights.size() > 0) {
      // Find max # of slots.
      size_t maxn = 0;
      for (size_t i = 0; i < attrib.skin_weights.size(); i++) {
        maxn = std::max(attrib.skin_weights[i].weightValues.size(), maxn);
      }

      std::cout << "Max # of weights = " << maxn << "\n";
      int num_slots = 0;
      if (maxn > 0) {
        num_slots = maxn / 4;
      }
      std::cout << "# of slots = " << num_slots << "\n";

      for (size_t s = 0; s < size_t(num_slots); s++) {
        VertexAttrib weights, joints;

        // Fill with zeros
        weights.data.resize(4 * (prim.position.data.size() / 3), 0.0f);
        joints.data.resize(4 * (prim.position.data.size() / 3), 0.0f);

        for (size_t v = 0; v < attrib.skin_weights.size(); v++) {
          const tinyobj::skin_weight_t &sw = attrib.skin_weights[v];

          assert(sw.vertex_id < (prim.position.data.size() / 3));

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
        prim.weights[s] = weights;

        joints.data_type = TINYGLTF_TYPE_VEC4;
        joints.component_type =
            TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;  // storage format

        prim.joints[s] = joints;
      }
    }
  }

  // postprocessing. e.g. find min/max
  {
    {
      uint32_t minv = 0.0;
      uint32_t maxv = 0.0;
      for (size_t i = 0; i < prim.indices.size(); i++) {
        minv = std::min(minv, uint32_t(prim.indices[i]));
        maxv = std::max(maxv, uint32_t(prim.indices[i]));
      }

      prim.indices_min = int(minv);
      prim.indices_max = int(maxv);

      prim.indices_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
    }

    {
      float bmin[3];
      float bmax[3];
      bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
      bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

      for (size_t i = 0; i < prim.position.data.size() / 3; i++) {
        for (size_t k = 0; k < 3; k++) {
          bmin[k] = std::min(bmin[k], prim.position.data[3 * i + k]);
          bmax[k] = std::max(bmax[k], prim.position.data[3 * i + k]);
        }
      }

      prim.position.minValues.resize(3);
      prim.position.minValues[0] = bmin[0];
      prim.position.minValues[1] = bmin[1];
      prim.position.minValues[2] = bmin[2];

      prim.position.maxValues.resize(3);
      prim.position.maxValues[0] = bmax[0];
      prim.position.maxValues[1] = bmax[1];
      prim.position.maxValues[2] = bmax[2];

      prim.position.data_type = TINYGLTF_TYPE_VEC3;
      prim.position.component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
    }

    {
      float bmin[3];
      float bmax[3];
      bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
      bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

      for (size_t i = 0; i < prim.normal.data.size() / 3; i++) {
        for (size_t k = 0; k < 3; k++) {
          bmin[k] = std::min(bmin[k], prim.normal.data[3 * i + k]);
          bmax[k] = std::max(bmax[k], prim.normal.data[3 * i + k]);
        }
      }

      prim.normal.minValues.resize(3);
      prim.normal.minValues[0] = bmin[0];
      prim.normal.minValues[1] = bmin[1];
      prim.normal.minValues[2] = bmin[2];

      prim.normal.maxValues.resize(3);
      prim.normal.maxValues[0] = bmax[0];
      prim.normal.maxValues[1] = bmax[1];
      prim.normal.maxValues[2] = bmax[2];

      prim.normal.data_type = TINYGLTF_TYPE_VEC3;
      prim.normal.component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
    }

    {
      float bmin[4];
      float bmax[4];
      bmin[0] = bmin[1] = bmin[2] = bmin[3] = std::numeric_limits<float>::max();
      bmax[0] = bmax[1] = bmax[2] = bmax[3] =
          -std::numeric_limits<float>::max();

      size_t n = 3;

      for (size_t i = 0; i < prim.tangent.data.size() / n; i++) {
        for (size_t k = 0; k < n; k++) {
          bmin[k] = std::min(bmin[k], prim.tangent.data[n * i + k]);
          bmax[k] = std::max(bmax[k], prim.tangent.data[n * i + k]);
        }
      }

      prim.tangent.minValues.resize(n);
      prim.tangent.maxValues.resize(n);
      for (size_t k = 0; k < n; k++) {
        prim.tangent.minValues[k] = bmin[k];
        prim.tangent.maxValues[k] = bmax[k];
      }

      prim.tangent.data_type =
          (n == 3) ? TINYGLTF_TYPE_VEC3 : TINYGLTF_TYPE_VEC4;
      prim.tangent.component_type = TINYGLTF_COMPONENT_TYPE_FLOAT;
    }

    // texcoord
    for (auto &item : prim.texcoords) {
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
    for (auto &item : prim.joints) {

      std::cout << "joint -- " << item.first << "\n";

      float bmin[4];
      float bmax[4];
      bmin[0] = bmin[1] = bmin[2] = bmin[3] = float(std::numeric_limits<uint16_t>::max());
      bmax[0] = bmax[1] = bmax[2] = bmax[3] =
          float(-std::numeric_limits<uint16_t>::max());

      size_t n = 4;

      for (size_t i = 0; i < item.second.data.size() / n; i++) {
        for (size_t k = 0; k < n; k++) {
          bmin[k] = std::min(bmin[k], item.second.data[n * i + k]);
          bmax[k] = std::max(bmax[k], item.second.data[n * i + k]);
        }
      }

      // TODO(syoyo): check if the value is within ushort max
      item.second.minValues.resize(n);
      item.second.maxValues.resize(n);
      for (size_t k = 0; k < n; k++) {
        item.second.minValues[k] = bmin[k];
        item.second.maxValues[k] = bmax[k];
      }

      item.second.data_type = TINYGLTF_TYPE_VEC4;
      item.second.component_type =
          TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;  // storage format
    }

    // weights
    for (auto &item : prim.weights) {

      float bmin[4];
      float bmax[4];
      bmin[0] = bmin[1] = bmin[2] = bmin[3] = std::numeric_limits<float>::max();
      // do not allow negative weight
      bmax[0] = bmax[1] = bmax[2] = bmax[3] = 0.0f;

      size_t n = 4;

      for (size_t i = 0; i < item.second.data.size() / n; i++) {
        for (size_t k = 0; k < 4; k++) {
          bmin[k] = std::min(bmin[k], item.second.data[n * i + k]);
          bmax[k] = std::max(bmax[k], item.second.data[n * i + k]);
        }
      }

      item.second.minValues.resize(n);
      item.second.maxValues.resize(n);
      for (size_t k = 0; k < n; k++) {
        item.second.minValues[k] = bmin[k];
        item.second.maxValues[k] = bmax[k];
      }

      item.second.data_type = TINYGLTF_TYPE_VEC4;
      item.second.component_type =
          TINYGLTF_COMPONENT_TYPE_FLOAT;  // storage format
    }
  }

  prim.mode = TINYGLTF_MODE_TRIANGLES;

  mesh->prims.clear();
  mesh->prims.push_back(prim);

  // Use filename as mesh's name
  mesh->name = GetBaseFilename(filename);


  return true;
}

void PrintMeshPrim(const MeshPrim &mesh) {
  for (size_t p = 0; p < mesh.prims.size(); p++) {
    const PrimSet &prim = mesh.prims[p];
    std::cout << "--- primitive[" << p << "] ---\n";

    std::cout << "indices.component_type : "
              << PrintComponentType(prim.indices_type) << "\n";
    std::cout << "# of indices : " << prim.indices.size() << "\n";
    std::cout << "  indices.min = " << prim.indices_min
              << ", max = " << prim.indices_max << "\n";
    for (size_t i = 0; i < prim.indices.size(); i++) {
      std::cout << "  index[" << i << "] = " << prim.indices[i] << "\n";
    }

    std::cout << "position.type : " << PrintType(prim.position.data_type) << "\n";
    std::cout << "position.component_type : "
              << PrintComponentType(prim.position.component_type) << "\n";
    std::cout << "# of positions : " << prim.position.data.size() / 3 << "\n";
    if ((prim.position.minValues.size() == 3) &&
        (prim.position.maxValues.size() == 3)) {
      std::cout << "  position.min = " << prim.position.minValues
                << ", max = " << prim.position.maxValues << "\n";
    }
    for (size_t i = 0; i < prim.position.data.size() / 3; i++) {
      std::cout << "  position[" << i << "] = " << prim.position.data[3 * i + 0]
                << ", " << prim.position.data[3 * i + 1] << ", "
                << prim.position.data[3 * i + 2] << std::endl;
    }

    std::cout << "normal.type : " << PrintType(prim.normal.data_type) << "\n";
    std::cout << "normal.component_type : "
              << PrintComponentType(prim.normal.component_type) << "\n";
    std::cout << "# of normals : " << prim.normal.data.size() / 3 << "\n";
    if ((prim.normal.minValues.size() == 3) &&
        (prim.normal.maxValues.size() == 3)) {
      std::cout << "  normal.min = " << prim.normal.minValues
                << ", max = " << prim.normal.maxValues << "\n";
    }
    for (size_t i = 0; i < prim.normal.data.size() / 3; i++) {
      std::cout << "  normal[" << i << "] = " << prim.normal.data[3 * i + 0]
                << ", " << prim.normal.data[3 * i + 1] << ", "
                << prim.normal.data[3 * i + 2] << std::endl;
    }

    if (prim.tangent.data.size() > 0) {
      assert((prim.tangent.data_type == TINYGLTF_TYPE_VEC3) ||
             (prim.tangent.data_type == TINYGLTF_TYPE_VEC4));

      size_t n = prim.tangent.data_type == TINYGLTF_TYPE_VEC3 ? 3 : 4;

      std::cout << "tangent.type : " << PrintType(prim.tangent.data_type) << "\n";
      std::cout << "tangent.component_type : "
                << PrintComponentType(prim.tangent.component_type) << "\n";
      std::cout << "# of tangents : " << prim.tangent.data.size() / n << "\n";
      if ((prim.tangent.minValues.size() == 3) &&
          (prim.tangent.maxValues.size() == 3)) {
        std::cout << "  tangent.min = " << prim.tangent.minValues
                  << ", max = " << prim.tangent.maxValues << "\n";
      }
      for (size_t i = 0; i < prim.tangent.data.size() / n; i++) {
        std::cout << "  tangent[" << i << "] = " << prim.tangent.data[n * i + 0]
                  << ", " << prim.tangent.data[n * i + 1] << ", "
                  << prim.tangent.data[n * i + 2];

        if (n == 4) {
          std::cout << ", " << prim.tangent.data[n * i + 3];
        }
        std::cout << std::endl;
      }
    }

    std::cout << "# of texcoord slots : " << prim.texcoords.size() << "\n";
    for (const auto &item : prim.texcoords) {
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

    assert(prim.joints.size() == prim.weights.size());
    std::cout << "# of joints/weights slots : " << prim.joints.size() << "\n";
    for (const auto &item : prim.joints) {
      assert(prim.weights.count(item.first));

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

      const VertexAttrib &attrib = prim.weights.at(item.first);

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
}

}  // namespace example
