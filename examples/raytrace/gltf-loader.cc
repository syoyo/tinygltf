#include "gltf-loader.h"

#include <iostream>
#include <memory>  // c++11
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

namespace example {

static std::string GetFilePathExtension(const std::string &FileName) {
  if (FileName.find_last_of(".") != std::string::npos)
    return FileName.substr(FileName.find_last_of(".") + 1);
  return "";
}

template <typename T>
struct arrayAdapter {
  unsigned char *dataPtr;
  const size_t elemCount;
  const size_t stride;
  arrayAdapter(unsigned char *ptr, size_t count, size_t byte_stride = 1)
      : dataPtr(ptr), elemCount(count), stride(byte_stride) {}

  T operator[](size_t pos) {
    if (pos >= elemCount)
      throw std::out_of_range(
          "Tried to access beyond the last element of an array adapter");
    return *(reinterpret_cast<T *>(dataPtr + pos * stride));
  }
};

struct intArrayBase {
  virtual int operator[](size_t) = 0;
  virtual size_t size() const = 0;
};

struct floatArrayBase {
  virtual float operator[](size_t) = 0;
  virtual size_t size() const = 0;
};

template <class T>
struct intArray : public intArrayBase {
  arrayAdapter<T> adapter;

  intArray(const arrayAdapter<T> &a) : adapter(a) {}
  int operator[](size_t position) override {
    return static_cast<int>(adapter[position]);
  }

  size_t size() const override { return adapter.elemCount; }
};

template <class T>
struct floatArray : public floatArrayBase {
  arrayAdapter<T> adapter;

  floatArray(const arrayAdapter<T> &a) : adapter(a) {}
  float operator[](size_t position) override {
    return static_cast<float>(adapter[position]);
  }

  size_t size() const override { return adapter.elemCount; }
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
    loadedMesh.name = gltfMesh.name;
    for (const auto &meshPrimitive : gltfMesh.primitives) {
      // get access to the indices
      std::unique_ptr<intArrayBase> indicesArray = nullptr;

      auto &indicesAccessor = model.accessors[meshPrimitive.indices];
      auto &bufferView = model.bufferViews[indicesAccessor.bufferView];
      auto &buffer = model.buffers[bufferView.buffer];
      unsigned char *dataAddress = buffer.data.data() + bufferView.byteOffset +
                                   indicesAccessor.byteOffset;
      const auto byteStride = indicesAccessor.ByteStride(bufferView);
      const auto count = indicesAccessor.count;

      switch (indicesAccessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
          indicesArray = std::unique_ptr<intArray<char> >(new intArray<char>(
              arrayAdapter<char>(dataAddress, count, byteStride)));
          break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
          indicesArray = std::unique_ptr<intArray<unsigned char> >(
              new intArray<unsigned char>(
                  arrayAdapter<unsigned char>(dataAddress, count, byteStride)));
          break;

        case TINYGLTF_COMPONENT_TYPE_SHORT:
          indicesArray = std::unique_ptr<intArray<short> >(new intArray<short>(
              arrayAdapter<short>(dataAddress, count, byteStride)));
          break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
          indicesArray = std::unique_ptr<intArray<unsigned short> >(
              new intArray<unsigned short>(arrayAdapter<unsigned short>(
                  dataAddress, count, byteStride)));
          break;

        case TINYGLTF_COMPONENT_TYPE_INT:
          indicesArray = std::unique_ptr<intArray<int> >(new intArray<int>(
              arrayAdapter<int>(dataAddress, count, byteStride)));
          break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
          indicesArray = std::unique_ptr<intArray<unsigned int> >(
              new intArray<unsigned int>(
                  arrayAdapter<unsigned int>(dataAddress, count, byteStride)));
          break;
        default:
          break;
      }

      if (indicesArray)
        for (size_t i(0); i < indicesArray->size(); ++i) {
          std::cout << (*indicesArray)[i] << " ";
        }

      std::cout << '\n';

      switch (meshPrimitive.mode) {
        case TINYGLTF_MODE_TRIANGLES:  // this is the simpliest case to handle

        {
          std::cout << "Will load a plain old list of trianges\n";

          for (const auto &attribute : meshPrimitive.attributes) {
            if (attribute.first == "POSITION") {
              std::cout << "found position attribute\n";
              const auto posAccessor = model.accessors[attribute.second];
            }

            if (attribute.first == "NORMAL") {
              std::cout << "found normal attribute\n";
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
  }

  std::cerr << "LoadGLTF() function is not yet implemented!" << std::endl;
  return false;
}
}  // namespace example