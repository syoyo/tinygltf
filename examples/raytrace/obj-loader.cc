#include "obj-loader.h"
#include "nanort.h"  // for float3

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wcast-align"
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wvariadic-macros"
#pragma clang diagnostic ignored "-Wc++11-extensions"
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#if __has_warning("-Wdouble-promotion")
#pragma clang diagnostic ignored "-Wdouble-promotion"
#endif
#if __has_warning("-Wcomma")
#pragma clang diagnostic ignored "-Wcomma"
#endif
#if __has_warning("-Wcast-qual")
#pragma clang diagnostic ignored "-Wcast-qual"
#endif
#endif

#include "stb_image.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <iostream>

#ifdef NANOSG_USE_CXX11
#include <unordered_map>
#else
#include <map>
#endif

#define USE_TEX_CACHE 1

namespace example {

typedef nanort::real3<float> float3;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#endif

// TODO(LTE): Remove global static definition.
#ifdef NANOSG_USE_CXX11
static std::unordered_map<std::string, int> hashed_tex;
#else
static std::map<std::string, int> hashed_tex;
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

inline void CalcNormal(float3 &N, float3 v0, float3 v1, float3 v2) {
  float3 v10 = v1 - v0;
  float3 v20 = v2 - v0;

  N = vcross(v20, v10);
  N = vnormalize(N);
}

static std::string GetBaseDir(const std::string &filepath) {
  if (filepath.find_last_of("/\\") != std::string::npos)
    return filepath.substr(0, filepath.find_last_of("/\\"));
  return "";
}

static int LoadTexture(const std::string &filename,
                       std::vector<Texture> *textures) {
  int idx;

  if (filename.empty()) return -1;

  std::cout << "  Loading texture : " << filename << std::endl;
  Texture texture;

  // tigra: find in cache. get index
  if (USE_TEX_CACHE) {
    if (hashed_tex.find(filename) != hashed_tex.end()) {
      puts("from cache");
      return hashed_tex[filename];
    }
  }

  int w, h, n;
  unsigned char *data = stbi_load(filename.c_str(), &w, &h, &n, 0);
  if (data) {
    texture.width = w;
    texture.height = h;
    texture.components = n;

    size_t n_elem = size_t(w * h * n);
    texture.image = new unsigned char[n_elem];
    for (size_t i = 0; i < n_elem; i++) {
      texture.image[i] = data[i];
    }

    free(data);

    textures->push_back(texture);

    idx = int(textures->size()) - 1;

    // tigra: store index to cache
    if (USE_TEX_CACHE) {
      hashed_tex[filename] = idx;
    }

    return idx;
  }

  std::cout << "  Failed to load : " << filename << std::endl;
  return -1;
}

static void ComputeBoundingBoxOfMesh(float bmin[3], float bmax[3],
                                     const example::Mesh<float> &mesh) {
  bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
  bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

  for (size_t i = 0; i < mesh.vertices.size() / 3; i++) {
    bmin[0] = std::min(bmin[0], mesh.vertices[3 * i + 0]);
    bmin[1] = std::min(bmin[1], mesh.vertices[3 * i + 1]);
    bmin[2] = std::min(bmin[1], mesh.vertices[3 * i + 2]);

    bmax[0] = std::max(bmax[0], mesh.vertices[3 * i + 0]);
    bmax[1] = std::max(bmax[1], mesh.vertices[3 * i + 1]);
    bmax[2] = std::max(bmax[2], mesh.vertices[3 * i + 2]);
  }
}

bool LoadObj(const std::string &filename, float scale,
             std::vector<Mesh<float> > *meshes,
             std::vector<Material> *out_materials,
             std::vector<Texture> *out_textures) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;

  std::string basedir = GetBaseDir(filename) + "/";
  const char *basepath = (basedir.compare("/") == 0) ? NULL : basedir.c_str();

  // auto t_start = std::chrono::system_clock::now();

  bool ret =
      tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str(),
                       basepath, /* triangulate */ true);

  // auto t_end = std::chrono::system_clock::now();
  // std::chrono::duration<double, std::milli> ms = t_end - t_start;

  if (!err.empty()) {
    std::cerr << err << std::endl;
  }

  if (!ret) {
    return false;
  }

  // std::cout << "[LoadOBJ] Parse time : " << ms.count() << " [msecs]"
  //          << std::endl;

  std::cout << "[LoadOBJ] # of shapes in .obj : " << shapes.size() << std::endl;
  std::cout << "[LoadOBJ] # of materials in .obj : " << materials.size()
            << std::endl;

  {
    size_t total_num_vertices = 0;
    size_t total_num_faces = 0;

    total_num_vertices = attrib.vertices.size() / 3;
    std::cout << "  vertices : " << attrib.vertices.size() / 3 << std::endl;

    for (size_t i = 0; i < shapes.size(); i++) {
      std::cout << "  shape[" << i << "].name : " << shapes[i].name
                << std::endl;
      std::cout << "  shape[" << i
                << "].indices : " << shapes[i].mesh.indices.size() << std::endl;
      assert((shapes[i].mesh.indices.size() % 3) == 0);

      total_num_faces += shapes[i].mesh.indices.size() / 3;

      // tigra: empty name convert to _id
      if (shapes[i].name.length() == 0) {
#ifdef NANOSG_USE_CXX11
        shapes[i].name = "_" + std::to_string(i);
#else
        std::stringstream ss;
        ss << i;
        shapes[i].name = "_" + ss.str();
#endif
        std::cout << "  EMPTY shape[" << i << "].name, new : " << shapes[i].name
                  << std::endl;
      }
    }
    std::cout << "[LoadOBJ] # of faces: " << total_num_faces << std::endl;
    std::cout << "[LoadOBJ] # of vertices: " << total_num_vertices << std::endl;
  }

  // TODO(LTE): Implement tangents and binormals

  for (size_t i = 0; i < shapes.size(); i++) {
    Mesh<float> mesh(/* stride */ sizeof(float) * 3);

    mesh.name = shapes[i].name;

    const size_t num_faces = shapes[i].mesh.indices.size() / 3;
    mesh.faces.resize(num_faces * 3);
    mesh.material_ids.resize(num_faces);
    mesh.facevarying_normals.resize(num_faces * 3 * 3);
    mesh.facevarying_uvs.resize(num_faces * 3 * 2);
    mesh.vertices.resize(num_faces * 3 * 3);

    for (size_t f = 0; f < shapes[i].mesh.indices.size() / 3; f++) {
      // reorder vertices. may create duplicated vertices.
      size_t f0 = size_t(shapes[i].mesh.indices[3 * f + 0].vertex_index);
      size_t f1 = size_t(shapes[i].mesh.indices[3 * f + 1].vertex_index);
      size_t f2 = size_t(shapes[i].mesh.indices[3 * f + 2].vertex_index);

      mesh.vertices[9 * f + 0] = scale * attrib.vertices[3 * f0 + 0];
      mesh.vertices[9 * f + 1] = scale * attrib.vertices[3 * f0 + 1];
      mesh.vertices[9 * f + 2] = scale * attrib.vertices[3 * f0 + 2];

      mesh.vertices[9 * f + 3] = scale * attrib.vertices[3 * f1 + 0];
      mesh.vertices[9 * f + 4] = scale * attrib.vertices[3 * f1 + 1];
      mesh.vertices[9 * f + 5] = scale * attrib.vertices[3 * f1 + 2];

      mesh.vertices[9 * f + 6] = scale * attrib.vertices[3 * f2 + 0];
      mesh.vertices[9 * f + 7] = scale * attrib.vertices[3 * f2 + 1];
      mesh.vertices[9 * f + 8] = scale * attrib.vertices[3 * f2 + 2];

      mesh.faces[3 * f + 0] = static_cast<unsigned int>(3 * f + 0);
      mesh.faces[3 * f + 1] = static_cast<unsigned int>(3 * f + 1);
      mesh.faces[3 * f + 2] = static_cast<unsigned int>(3 * f + 2);

      mesh.material_ids[f] =
          static_cast<unsigned int>(shapes[i].mesh.material_ids[f]);
    }

    if (attrib.normals.size() > 0) {
      for (size_t f = 0; f < shapes[i].mesh.indices.size() / 3; f++) {
        size_t f0, f1, f2;

        f0 = size_t(shapes[i].mesh.indices[3 * f + 0].normal_index);
        f1 = size_t(shapes[i].mesh.indices[3 * f + 1].normal_index);
        f2 = size_t(shapes[i].mesh.indices[3 * f + 2].normal_index);

        if (f0 > 0 && f1 > 0 && f2 > 0) {
          float n0[3], n1[3], n2[3];

          n0[0] = attrib.normals[3 * f0 + 0];
          n0[1] = attrib.normals[3 * f0 + 1];
          n0[2] = attrib.normals[3 * f0 + 2];

          n1[0] = attrib.normals[3 * f1 + 0];
          n1[1] = attrib.normals[3 * f1 + 1];
          n1[2] = attrib.normals[3 * f1 + 2];

          n2[0] = attrib.normals[3 * f2 + 0];
          n2[1] = attrib.normals[3 * f2 + 1];
          n2[2] = attrib.normals[3 * f2 + 2];

          mesh.facevarying_normals[3 * (3 * f + 0) + 0] = n0[0];
          mesh.facevarying_normals[3 * (3 * f + 0) + 1] = n0[1];
          mesh.facevarying_normals[3 * (3 * f + 0) + 2] = n0[2];

          mesh.facevarying_normals[3 * (3 * f + 1) + 0] = n1[0];
          mesh.facevarying_normals[3 * (3 * f + 1) + 1] = n1[1];
          mesh.facevarying_normals[3 * (3 * f + 1) + 2] = n1[2];

          mesh.facevarying_normals[3 * (3 * f + 2) + 0] = n2[0];
          mesh.facevarying_normals[3 * (3 * f + 2) + 1] = n2[1];
          mesh.facevarying_normals[3 * (3 * f + 2) + 2] = n2[2];
        } else {  // face contains invalid normal index. calc geometric normal.
          f0 = size_t(shapes[i].mesh.indices[3 * f + 0].vertex_index);
          f1 = size_t(shapes[i].mesh.indices[3 * f + 1].vertex_index);
          f2 = size_t(shapes[i].mesh.indices[3 * f + 2].vertex_index);

          float3 v0, v1, v2;

          v0[0] = attrib.vertices[3 * f0 + 0];
          v0[1] = attrib.vertices[3 * f0 + 1];
          v0[2] = attrib.vertices[3 * f0 + 2];

          v1[0] = attrib.vertices[3 * f1 + 0];
          v1[1] = attrib.vertices[3 * f1 + 1];
          v1[2] = attrib.vertices[3 * f1 + 2];

          v2[0] = attrib.vertices[3 * f2 + 0];
          v2[1] = attrib.vertices[3 * f2 + 1];
          v2[2] = attrib.vertices[3 * f2 + 2];

          float3 N;
          CalcNormal(N, v0, v1, v2);

          mesh.facevarying_normals[3 * (3 * f + 0) + 0] = N[0];
          mesh.facevarying_normals[3 * (3 * f + 0) + 1] = N[1];
          mesh.facevarying_normals[3 * (3 * f + 0) + 2] = N[2];

          mesh.facevarying_normals[3 * (3 * f + 1) + 0] = N[0];
          mesh.facevarying_normals[3 * (3 * f + 1) + 1] = N[1];
          mesh.facevarying_normals[3 * (3 * f + 1) + 2] = N[2];

          mesh.facevarying_normals[3 * (3 * f + 2) + 0] = N[0];
          mesh.facevarying_normals[3 * (3 * f + 2) + 1] = N[1];
          mesh.facevarying_normals[3 * (3 * f + 2) + 2] = N[2];
        }
      }
    } else {
      // calc geometric normal
      for (size_t f = 0; f < shapes[i].mesh.indices.size() / 3; f++) {
        size_t f0, f1, f2;

        f0 = size_t(shapes[i].mesh.indices[3 * f + 0].vertex_index);
        f1 = size_t(shapes[i].mesh.indices[3 * f + 1].vertex_index);
        f2 = size_t(shapes[i].mesh.indices[3 * f + 2].vertex_index);

        float3 v0, v1, v2;

        v0[0] = attrib.vertices[3 * f0 + 0];
        v0[1] = attrib.vertices[3 * f0 + 1];
        v0[2] = attrib.vertices[3 * f0 + 2];

        v1[0] = attrib.vertices[3 * f1 + 0];
        v1[1] = attrib.vertices[3 * f1 + 1];
        v1[2] = attrib.vertices[3 * f1 + 2];

        v2[0] = attrib.vertices[3 * f2 + 0];
        v2[1] = attrib.vertices[3 * f2 + 1];
        v2[2] = attrib.vertices[3 * f2 + 2];

        float3 N;
        CalcNormal(N, v0, v1, v2);

        mesh.facevarying_normals[3 * (3 * f + 0) + 0] = N[0];
        mesh.facevarying_normals[3 * (3 * f + 0) + 1] = N[1];
        mesh.facevarying_normals[3 * (3 * f + 0) + 2] = N[2];

        mesh.facevarying_normals[3 * (3 * f + 1) + 0] = N[0];
        mesh.facevarying_normals[3 * (3 * f + 1) + 1] = N[1];
        mesh.facevarying_normals[3 * (3 * f + 1) + 2] = N[2];

        mesh.facevarying_normals[3 * (3 * f + 2) + 0] = N[0];
        mesh.facevarying_normals[3 * (3 * f + 2) + 1] = N[1];
        mesh.facevarying_normals[3 * (3 * f + 2) + 2] = N[2];
      }
    }

    if (attrib.texcoords.size() > 0) {
      for (size_t f = 0; f < shapes[i].mesh.indices.size() / 3; f++) {
        size_t f0, f1, f2;

        f0 = size_t(shapes[i].mesh.indices[3 * f + 0].texcoord_index);
        f1 = size_t(shapes[i].mesh.indices[3 * f + 1].texcoord_index);
        f2 = size_t(shapes[i].mesh.indices[3 * f + 2].texcoord_index);

        if (f0 > 0 && f1 > 0 && f2 > 0) {
          float3 n0, n1, n2;

          n0[0] = attrib.texcoords[2 * f0 + 0];
          n0[1] = attrib.texcoords[2 * f0 + 1];

          n1[0] = attrib.texcoords[2 * f1 + 0];
          n1[1] = attrib.texcoords[2 * f1 + 1];

          n2[0] = attrib.texcoords[2 * f2 + 0];
          n2[1] = attrib.texcoords[2 * f2 + 1];

          mesh.facevarying_uvs[2 * (3 * f + 0) + 0] = n0[0];
          mesh.facevarying_uvs[2 * (3 * f + 0) + 1] = n0[1];

          mesh.facevarying_uvs[2 * (3 * f + 1) + 0] = n1[0];
          mesh.facevarying_uvs[2 * (3 * f + 1) + 1] = n1[1];

          mesh.facevarying_uvs[2 * (3 * f + 2) + 0] = n2[0];
          mesh.facevarying_uvs[2 * (3 * f + 2) + 1] = n2[1];
        }
      }
    }

    // Compute pivot translation and add offset to the vertices.
    float bmin[3], bmax[3];
    ComputeBoundingBoxOfMesh(bmin, bmax, mesh);

    float bcenter[3];
    bcenter[0] = 0.5f * (bmax[0] - bmin[0]) + bmin[0];
    bcenter[1] = 0.5f * (bmax[1] - bmin[1]) + bmin[1];
    bcenter[2] = 0.5f * (bmax[2] - bmin[2]) + bmin[2];

    for (size_t v = 0; v < mesh.vertices.size() / 3; v++) {
      mesh.vertices[3 * v + 0] -= bcenter[0];
      mesh.vertices[3 * v + 1] -= bcenter[1];
      mesh.vertices[3 * v + 2] -= bcenter[2];
    }

    mesh.pivot_xform[0][0] = 1.0f;
    mesh.pivot_xform[0][1] = 0.0f;
    mesh.pivot_xform[0][2] = 0.0f;
    mesh.pivot_xform[0][3] = 0.0f;

    mesh.pivot_xform[1][0] = 0.0f;
    mesh.pivot_xform[1][1] = 1.0f;
    mesh.pivot_xform[1][2] = 0.0f;
    mesh.pivot_xform[1][3] = 0.0f;

    mesh.pivot_xform[2][0] = 0.0f;
    mesh.pivot_xform[2][1] = 0.0f;
    mesh.pivot_xform[2][2] = 1.0f;
    mesh.pivot_xform[2][3] = 0.0f;

    mesh.pivot_xform[3][0] = bcenter[0];
    mesh.pivot_xform[3][1] = bcenter[1];
    mesh.pivot_xform[3][2] = bcenter[2];
    mesh.pivot_xform[3][3] = 1.0f;

    meshes->push_back(mesh);
  }

  // material_t -> Material and Texture
  out_materials->resize(materials.size());
  out_textures->resize(0);
  for (size_t i = 0; i < materials.size(); i++) {
    (*out_materials)[i].diffuse[0] = materials[i].diffuse[0];
    (*out_materials)[i].diffuse[1] = materials[i].diffuse[1];
    (*out_materials)[i].diffuse[2] = materials[i].diffuse[2];
    (*out_materials)[i].specular[0] = materials[i].specular[0];
    (*out_materials)[i].specular[1] = materials[i].specular[1];
    (*out_materials)[i].specular[2] = materials[i].specular[2];

    (*out_materials)[i].id = int(i);

    // map_Kd
    (*out_materials)[i].diffuse_texid =
        LoadTexture(materials[i].diffuse_texname, out_textures);
    // map_Ks
    (*out_materials)[i].specular_texid =
        LoadTexture(materials[i].specular_texname, out_textures);
  }

  return true;
}

}  // namespace example
