#include "mesh-util.hh"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

// ../common/
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace example {

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

bool RequireFacevaringLayout(const tinyobj::attrib_t &attrib,
                             const std::vector<tinyobj::shape_t> &shapes) {
  // Check if all normals and texcoords has same index with vertex
  if ((attrib.texcoords.size() / 3) != attrib.vertices.size() / 3) {
    std::cerr << "Texcoords and Vertices length mismatch. Mesh data cannot be "
                 "represented as non-facevarying\n";
    return true;
  }
  if ((attrib.normals.size() / 2) != attrib.vertices.size() / 3) {
    std::cerr << "Normals and Vertices length mismatch. Mesh data cannot be "
                 "represented as non-facevarying\n";
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

    mesh->indices.clear();

    size_t face_index_offset = 0;
    for (size_t s = 0; s < shapes.size(); s++) {
      const tinyobj::shape_t &shape = shapes[s];

      for (size_t f = 0; f < shape.mesh.indices.size(); f++) {
        mesh->indices.push_back(uint32_t(face_index_offset) + uint32_t(shape.mesh.indices[f].vertex_index));
      }

      face_index_offset = mesh->indices.size();
    }

  }

  return true;
}

}  // namespace example
