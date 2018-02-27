#ifndef EXAMPLE_GLTF_LOADER_H_
#define EXAMPLE_GLTF_LOADER_H_

#include <stdexcept>
#include <string>
#include <vector>

#include "material.h"
#include "mesh.h"

namespace example {

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

  /// Construct an array adapter.
  /// \param ptr Pointer to the start of the data, with offset applied
  /// \param count Number of elements in the array
  /// \param byte_stride Stride betweens elements in the array
  arrayAdapter(const unsigned char *ptr, size_t count, size_t byte_stride)
      : dataPtr(ptr), elemCount(count), stride(byte_stride) {}

  /// Returns a *copy* of a single element. Can't be used to modify it.
  T operator[](size_t pos) const {
    if (pos >= elemCount)
      throw std::out_of_range(
          "Tried to access beyond the last element of an array adapter with "
          "count " +
          std::to_string(elemCount) + " while getting elemnet number " +
          std::to_string(pos));
    return *(reinterpret_cast<const T *>(dataPtr + pos * stride));
  }
};

/// Interface of any adapted array that returns ingeger data
struct intArrayBase {
  virtual ~intArrayBase() = default;
  virtual unsigned int operator[](size_t) const = 0;
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
  unsigned int operator[](size_t position) const override {
    return static_cast<unsigned int>(adapter[position]);
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

struct v2fArray {
  arrayAdapter<v2f> adapter;
  v2fArray(const arrayAdapter<v2f> &a) : adapter(a) {}

  v2f operator[](size_t position) const { return adapter[position]; }
  size_t size() const { return adapter.elemCount; }
};

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

struct v2dArray {
  arrayAdapter<v2d> adapter;
  v2dArray(const arrayAdapter<v2d> &a) : adapter(a) {}

  v2d operator[](size_t position) const { return adapter[position]; }
  size_t size() const { return adapter.elemCount; }
};

struct v3dArray {
  arrayAdapter<v3d> adapter;
  v3dArray(const arrayAdapter<v3d> &a) : adapter(a) {}

  v3d operator[](size_t position) const { return adapter[position]; }
  size_t size() const { return adapter.elemCount; }
};

struct v4dArray {
  arrayAdapter<v4d> adapter;
  v4dArray(const arrayAdapter<v4d> &a) : adapter(a) {}

  v4d operator[](size_t position) const { return adapter[position]; }
  size_t size() const { return adapter.elemCount; }
};

///
/// Loads glTF 2.0 mesh
///
bool LoadGLTF(const std::string &filename, float scale,
              std::vector<Mesh<float> > *meshes,
              std::vector<Material> *materials, std::vector<Texture> *textures);

}  // namespace example

#endif  // EXAMPLE_GLTF_LOADER_H_
