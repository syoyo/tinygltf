#include "gltf-loader.h"

#include <iostream>
#include <memory>  // c++11
#include <stdexcept>
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

using std::out_of_range;

namespace example {

static std::string GetFilePathExtension(const std::string &FileName) {
  if (FileName.find_last_of(".") != std::string::npos)
    return FileName.substr(FileName.find_last_of(".") + 1);
  return "";
}

/// Adapts an array of bytes to an array of T. Will advace of byte_stride each
/// elements.
template <typename T>
struct arrayAdapter {
  /// Pointer to the bytes
  const unsigned char *dataPtr;
  /// Number of elements in the array
  const size_t elemCount;
  /// Stride in bytes between two elements
  const size_t stride;
  /// Constructor.

  /// Construct an array adapter.
  /// \param ptr Pointer to the start of the data, with offset applied
  /// \param count Number of elements in the array
  /// \param byte_stride Stride betweens elements in the array
  arrayAdapter(const unsigned char *ptr, size_t count, size_t byte_stride = 1)
      : dataPtr(ptr), elemCount(count), stride(byte_stride) {}

  /// Returns a *copy* of a single element. Can't be used to modify it.
  T operator[](size_t pos) const {
    if (pos >= elemCount)
      throw out_of_range(
          "Tried to access beyond the last element of an array adapter");
    return *(reinterpret_cast<const T *>(dataPtr + pos * stride));
  }
};

/// Interface of any adapted array that returns ingeger data
struct intArrayBase {
  virtual ~intArrayBase() = default;
  virtual int operator[](size_t) const = 0;
  virtual size_t size() const = 0;
};

/// Interface of any adapted array that returns float data
struct floatArrayBase {
  virtual ~floatArrayBase() = default;
  virtual float operator[](size_t) const = 0;
  virtual size_t size() const = 0;
};

/// An array that loads interger types, returns them as int
template <class T>
struct intArray : public intArrayBase {
  arrayAdapter<T> adapter;

  intArray(const arrayAdapter<T> &a) : adapter(a) {}
  int operator[](size_t position) const override {
    return static_cast<int>(adapter[position]);
  }

  size_t size() const override { return adapter.elemCount; }
};

template <class T>
struct floatArray : public floatArrayBase {
  arrayAdapter<T> adapter;

  floatArray(const arrayAdapter<T> &a) : adapter(a) {}
  float operator[](size_t position) const override {
    return static_cast<float>(adapter[position]);
  }

  size_t size() const override { return adapter.elemCount; }
};

#pragma pack(push, 1)

template <typename T>
struct v2 {
  T x, y;
};
/// 3D vector of floats without padding
template <typename T>
struct v3 {
  T x, y, z;
};

/// 4D vector of floats without padding
template <typename T>
struct v4 {
  T x, y, z, w;
};

#pragma pack(pop)

using v2f = v2<float>;
using v3f = v3<float>;
using v4f = v4<float>;
using v2d = v2<double>;
using v3d = v3<double>;
using v4d = v4<double>;

struct v3fArray {
  arrayAdapter<v3f> adapter;
  v3fArray(const arrayAdapter<v3f> &a) : adapter(a) {}

  v3f operator[](size_t position) const { return adapter[position]; }
  size_t size() const { return adapter.elemCount; }
};

struct v4fArray {
  arrayAdapter<v4f> adapter;
  v4fArray(const arrayAdapter<v4f> &a) : adapter(a) {}

  v4f operator[](size_t position) const { return adapter[position]; }
  size_t size() const { return adapter.elemCount; }
};

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

    Mesh<float> loadedMesh(sizeof(float) * 3);

    v3f pMin, pMax;

    loadedMesh.name = gltfMesh.name;
    for (const auto &meshPrimitive : gltfMesh.primitives) {
      // get access to the indices
      std::unique_ptr<intArrayBase> indicesArrayPtr = nullptr;
      {
        auto &indicesAccessor = model.accessors[meshPrimitive.indices];
        auto &bufferView = model.bufferViews[indicesAccessor.bufferView];
        auto &buffer = model.buffers[bufferView.buffer];
        unsigned char *dataAddress = buffer.data.data() +
                                     bufferView.byteOffset +
                                     indicesAccessor.byteOffset;
        const auto byteStride = indicesAccessor.ByteStride(bufferView);
        const auto count = indicesAccessor.count;

        switch (indicesAccessor.componentType) {
          case TINYGLTF_COMPONENT_TYPE_BYTE:
            indicesArrayPtr =
                std::unique_ptr<intArray<char> >(new intArray<char>(
                    arrayAdapter<char>(dataAddress, count, byteStride)));
            break;

          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            indicesArrayPtr = std::unique_ptr<intArray<unsigned char> >(
                new intArray<unsigned char>(arrayAdapter<unsigned char>(
                    dataAddress, count, byteStride)));
            break;

          case TINYGLTF_COMPONENT_TYPE_SHORT:
            indicesArrayPtr =
                std::unique_ptr<intArray<short> >(new intArray<short>(
                    arrayAdapter<short>(dataAddress, count, byteStride)));
            break;

          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            indicesArrayPtr = std::unique_ptr<intArray<unsigned short> >(
                new intArray<unsigned short>(arrayAdapter<unsigned short>(
                    dataAddress, count, byteStride)));
            break;

          case TINYGLTF_COMPONENT_TYPE_INT:
            indicesArrayPtr = std::unique_ptr<intArray<int> >(new intArray<int>(
                arrayAdapter<int>(dataAddress, count, byteStride)));
            break;

          case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            indicesArrayPtr = std::unique_ptr<intArray<unsigned int> >(
                new intArray<unsigned int>(arrayAdapter<unsigned int>(
                    dataAddress, count, byteStride)));
            break;
          default:
            break;
        }
      }
      const auto &indices = *indicesArrayPtr;

      if (indicesArrayPtr)
        for (size_t i(0); i < indicesArrayPtr->size(); ++i) {
          std::cout << indices[i] << " ";
          loadedMesh.faces.push_back(indices[i]);
        }

      std::cout << '\n';

      switch (meshPrimitive.mode) {
        case TINYGLTF_MODE_TRIANGLES:  // this is the simpliest case to handle

        {
          std::cout << "Will load a plain old list of trianges\n";

          for (const auto &attribute : meshPrimitive.attributes) {
            const auto attribAccessor = model.accessors[attribute.second];
            const auto &bufferView =
                model.bufferViews[attribAccessor.bufferView];
            const auto &buffer = model.buffers[bufferView.buffer];
            auto dataPtr = buffer.data.data() + bufferView.byteOffset +
                           attribAccessor.byteOffset;
            const auto byte_stride = attribAccessor.ByteStride(bufferView);
            const auto count = attribAccessor.count;

            if (attribute.first == "POSITION") {
              std::cout << "found position attribute\n";
              pMin.x = attribAccessor.minValues[0];
              pMin.y = attribAccessor.minValues[1];
              pMin.z = attribAccessor.minValues[2];
              pMax.x = attribAccessor.maxValues[0];
              pMax.y = attribAccessor.maxValues[1];
              pMax.z = attribAccessor.maxValues[2];
              switch (attribAccessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                  switch (attribAccessor.type) {
                    case TINYGLTF_TYPE_VEC3: {
                      /// 3D vector of float
                      v3fArray positions(
                          arrayAdapter<v3f>(dataPtr, count, byte_stride));
                      for (size_t i{0}; i < positions.size(); ++i) {
                        const auto v = positions[i];
                        std::cout << '(' << v.x << ", " << v.y << ", " << v.z
                                  << ")\n";

                        loadedMesh.vertices.push_back(v.x * scale);
                        loadedMesh.vertices.push_back(v.y * scale);
                        loadedMesh.vertices.push_back(v.z * scale);
                      }
                    } break;
                    default:
                      // TODO Handle error
                      break;
                  }
                  break;
                default:
                  // TODO handle error
                  break;
              }
            }

            if (attribute.first == "NORMAL") {
              std::cout << "found normal attribute\n";

              switch (attribAccessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                  switch (attribAccessor.type) {
                    case TINYGLTF_TYPE_VEC3: {
                      std::cout << "normal vec3\n";
                      v3fArray normals(
                          arrayAdapter<v3f>(dataPtr, count, byte_stride));
                      for (size_t i{0}; i < normals.size(); ++i) {
                        const auto v = normals[i];
                        std::cout << '(' << v.x << ", " << v.y << ", " << v.z
                                  << ")\n";

                        loadedMesh.facevarying_normals.push_back(v.x);
                        loadedMesh.facevarying_normals.push_back(v.y);
                        loadedMesh.facevarying_normals.push_back(v.z);
                      }
                    } break;
                    case TINYGLTF_TYPE_VEC4:
                      std::cout << "normal vec4";
                      break;
                    default:
                      // TODO handle error
                      break;
                  }
                default:
                  // TODO handle error
                  break;
              }
            }
          }
        }

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

    // TODO compute pivot point

    // bbox :
    v3f bCenter;
    bCenter.x = 0.5f * (pMax.x - pMin.x) + pMin.x;
    bCenter.y = 0.5f * (pMax.y - pMin.y) + pMin.y;
    bCenter.z = 0.5f * (pMax.z - pMin.z) + pMin.z;

    for (size_t v = 0; v < loadedMesh.vertices.size() / 3; v++) {
      loadedMesh.vertices[3 * v + 0] -= bCenter.x;
      loadedMesh.vertices[3 * v + 1] -= bCenter.y;
      loadedMesh.vertices[3 * v + 2] -= bCenter.z;
    }

    loadedMesh.pivot_xform[0][0] = 1.0f;
    loadedMesh.pivot_xform[0][1] = 0.0f;
    loadedMesh.pivot_xform[0][2] = 0.0f;
    loadedMesh.pivot_xform[0][3] = 0.0f;

    loadedMesh.pivot_xform[1][0] = 0.0f;
    loadedMesh.pivot_xform[1][1] = 1.0f;
    loadedMesh.pivot_xform[1][2] = 0.0f;
    loadedMesh.pivot_xform[1][3] = 0.0f;

    loadedMesh.pivot_xform[2][0] = 0.0f;
    loadedMesh.pivot_xform[2][1] = 0.0f;
    loadedMesh.pivot_xform[2][2] = 1.0f;
    loadedMesh.pivot_xform[2][3] = 0.0f;

    loadedMesh.pivot_xform[3][0] = bCenter.x;
    loadedMesh.pivot_xform[3][1] = bCenter.y;
    loadedMesh.pivot_xform[3][2] = bCenter.z;
    loadedMesh.pivot_xform[3][3] = 1.0f;

    for (size_t i{0}; i < loadedMesh.faces.size(); ++i)
      loadedMesh.material_ids.push_back(materials->at(0).id);

    meshes->push_back(loadedMesh);
    ret = true;
  }
  // std::cerr << "LoadGLTF() function is not yet implemented!" << std::endl;
  return ret;
}
}  // namespace example
