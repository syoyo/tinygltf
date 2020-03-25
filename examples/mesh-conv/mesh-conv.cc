#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#if !defined(__ANDROID__) && !defined(_WIN32)
#include <wordexp.h>
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "../../json.hpp"
#include "../common/clipp.h"

using json = nlohmann::json;

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef _WIN32
#include "../../tiny_gltf.h"
#else
#include "tiny_gltf.h"
#endif

#include "mesh-util.hh"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#endif

namespace {

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

#if 0
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
};

struct MeshPrim {
  std::string name;
  int32_t id{-1};

  int mode{TINYGLTF_MODE_TRIANGLES};

  VertexAttrib position;                  // vec3
  VertexAttrib normal;                    // vec3
  VertexAttrib tangent;                   // vec4
  VertexAttrib color;                     // vec3 or vec4
  std::map<int, VertexAttrib> texcoords;  // <slot, attrib>  vec2
  std::map<int, VertexAttrib> weights;    // <slot, attrib>
  std::map<int, VertexAttrib>
      joints;  // <slot, attrib> store data as float type

  int indices_type{-1}; // storage type(componentType) of `indices`.
  std::vector<uint32_t> indices; // vertex indices
};
#endif

static std::string GetFilePathExtension(const std::string &FileName) {
  if (FileName.find_last_of(".") != std::string::npos)
    return FileName.substr(FileName.find_last_of(".") + 1);
  return "";
}

static size_t ComponentTypeByteSize(int type) {
  switch (type) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
    case TINYGLTF_COMPONENT_TYPE_BYTE:
      return sizeof(char);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
    case TINYGLTF_COMPONENT_TYPE_SHORT:
      return sizeof(short);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
    case TINYGLTF_COMPONENT_TYPE_INT:
      return sizeof(int);
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
      return sizeof(float);
    case TINYGLTF_COMPONENT_TYPE_DOUBLE:
      return sizeof(double);
    default:
      return 0;
  }
}

std::vector<uint8_t> LoadBin(const std::string &filename) {
  std::vector<uint8_t> data;

  std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

  if (is.is_open()) {
    size_t size = size_t(is.tellg());
    is.seekg(0, std::ios::beg);
    if (size < 4) {
      std::cerr << "File size is zero or too short: " << size << "\n";
      return data;
    }

    data.resize(size);
    is.read(reinterpret_cast<char *>(data.data()), std::streamsize(size));
  }

  return data;
}

// TODO(syoyo): Use C++17 like filesystem library

bool FileExists(const std::string &abs_filename) {
  bool ret;
#ifdef _WIN32
  // TODO(syoyo): Support utf8 filepath
  FILE *fp = nullptr;
  errno_t err = fopen_s(&fp, abs_filename.c_str(), "rb");
  if (err != 0) {
    return false;
  }
#else
  FILE *fp = fopen(abs_filename.c_str(), "rb");
#endif
  if (fp) {
    ret = true;
    fclose(fp);
  } else {
    ret = false;
  }

  return ret;
}

static std::string JoinPath(const std::string &path0,
                            const std::string &path1) {
  if (path0.empty()) {
    return path1;
  } else {
    // check '/'
    char lastChar = *path0.rbegin();
    if (lastChar != '/') {
      return path0 + std::string("/") + path1;
    } else {
      return path0 + path1;
    }
  }
}

std::string ExpandFilePath(const std::string &filepath) {
#ifdef _WIN32
  DWORD len = ExpandEnvironmentStringsA(filepath.c_str(), NULL, 0);
  char *str = new char[len];
  ExpandEnvironmentStringsA(filepath.c_str(), str, len);

  std::string s(str);

  delete[] str;

  return s;
#else

#if defined(TARGET_OS_IPHONE) || defined(TARGET_IPHONE_SIMULATOR) || \
    defined(__ANDROID__) || defined(__EMSCRIPTEN__)
  // no expansion
  std::string s = filepath;
#else
  std::string s;
  wordexp_t p;

  if (filepath.empty()) {
    return "";
  }

  // Quote the string to keep any spaces in filepath intact.
  std::string quoted_path = "\"" + filepath + "\"";
  // char** w;
  int ret = wordexp(quoted_path.c_str(), &p, 0);
  if (ret) {
    // err
    s = filepath;
    return s;
  }

  // Use first element only.
  if (p.we_wordv) {
    s = std::string(p.we_wordv[0]);
    wordfree(&p);
  } else {
    s = filepath;
  }

#endif

  return s;
#endif
}

static std::string FindFile(const std::vector<std::string> &paths,
                            const std::string &filepath) {
  for (size_t i = 0; i < paths.size(); i++) {
    std::string absPath = ExpandFilePath(JoinPath(paths[i], filepath));
    if (FileExists(absPath)) {
      return absPath;
    }
  }

  return std::string();
}

static std::string GetBaseDir(const std::string &filepath) {
  if (filepath.find_last_of("/\\") != std::string::npos)
    return filepath.substr(0, filepath.find_last_of("/\\"));
  return "";
}

static int GetSlotId(const std::string &name) {
  if (name.rfind("TEXCOORD_", 0) == 0) {
    int id = 0;
    sscanf(name.c_str(), "TEXCOORD_%d", &id);
    return id;
  } else if (name.rfind("WEIGHTS_", 0) == 0) {
    int id = 0;
    sscanf(name.c_str(), "WEIGHTS_%d", &id);
    return id;
  } else if (name.rfind("JOINTS_", 0) == 0) {
    int id = 0;
    sscanf(name.c_str(), "JOINTS_%d", &id);
    return id;
  }

  return -1;
}

static bool IsAttributeSupported(const std::string &name) {
  constexpr int max_slots = 8;

  if (name.compare("POSITION") == 0) {
    return true;
  }

  if (name.compare("NORMAL") == 0) {
    return true;
  }

  if (name.compare("TANGENT") == 0) {
    return true;
  }

  for (int i = 0; i < max_slots; i++) {
    std::string n = "TEXCOORD_" + std::to_string(i);
    if (n.compare(name) == 0) {
      return true;
    }
  }

  for (int i = 0; i < max_slots; i++) {
    std::string n = "WEIGHTS_" + std::to_string(i);
    if (n.compare(name) == 0) {
      return true;
    }
  }

  for (int i = 0; i < max_slots; i++) {
    std::string n = "JOINTS_" + std::to_string(i);
    if (n.compare(name) == 0) {
      return true;
    }
  }

  return false;
}

static float Unpack(const unsigned char *ptr, int type) {
  if (type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
    unsigned char data = *ptr;
    return float(data);
  } else if (type == TINYGLTF_COMPONENT_TYPE_BYTE) {
    char data = static_cast<char>(*ptr);
    return float(data);
  } else if (type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    uint16_t data = *reinterpret_cast<const uint16_t *>(ptr);
    return float(data);
  } else if (type == TINYGLTF_COMPONENT_TYPE_SHORT) {
    int16_t data = *reinterpret_cast<const int16_t *>(ptr);
    return float(data);
  } else if (type == TINYGLTF_COMPONENT_TYPE_FLOAT) {
    float data = *reinterpret_cast<const float *>(ptr);
    return data;
  } else {
    std::cerr << "???: Unsupported type: " << PrintComponentType(type) << "\n";
    return 0.0f;
  }
}

static uint32_t UnpackIndex(const unsigned char *ptr, int type) {
  if (type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
    unsigned char data = *ptr;
    return uint32_t(data);
  } else if (type == TINYGLTF_COMPONENT_TYPE_BYTE) {
    char data = static_cast<char>(*ptr);
    return uint32_t(data);
  } else if (type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    uint16_t data = *reinterpret_cast<const uint16_t *>(ptr);
    return uint32_t(data);
  } else if (type == TINYGLTF_COMPONENT_TYPE_SHORT) {
    int16_t data = *reinterpret_cast<const int16_t *>(ptr);
    return uint32_t(data);
  } else if (type == TINYGLTF_COMPONENT_TYPE_INT) {
    // TODO(syoyo): Check overflow(2G+ index)
    int32_t data = *reinterpret_cast<const int32_t *>(ptr);
    return uint32_t(data);
  } else if (type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
    uint32_t data = *reinterpret_cast<const uint32_t *>(ptr);
    return uint32_t(data);
  } else if (type == TINYGLTF_COMPONENT_TYPE_SHORT) {
    uint32_t data = *reinterpret_cast<const uint32_t *>(ptr);
    return data;
  } else {
    std::cerr << "???: Unsupported type: " << PrintComponentType(type) << "\n";
    return static_cast<uint32_t>(-1);
  }
}

static bool DumpMesh(const tinygltf::Model &model, const tinygltf::Mesh &mesh,
                     bool verbose, example::MeshPrim *out) {
  out->prims.clear();

  for (size_t i = 0; i < mesh.primitives.size(); i++) {
    const tinygltf::Primitive &primitive = mesh.primitives[i];

    if (primitive.indices < 0) {
      std::cerr << "Primitive indices must be provided\n";
      return false;
    }

    example::PrimSet out_prim;

    // indices.
    {
      const tinygltf::Accessor &indexAccessor =
          model.accessors[size_t(primitive.indices)];

      size_t num_elements = indexAccessor.count;
      std::cout << "index.elements = " << num_elements << "\n";

      size_t byte_stride = ComponentTypeByteSize(indexAccessor.componentType);

      const tinygltf::BufferView &indexBufferView =
          model.bufferViews[size_t(indexAccessor.bufferView)];

      // should be 34963(ELEMENT_ARRAY_BUFFER)
      std::cout << "index.target = " << PrintTarget(indexBufferView.target)
                << "\n";
      if (indexBufferView.target != TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER) {
        std::cerr << "indexBufferView.target must be ELEMENT_ARRAY_BUFFER\n";
        return false;
      }

      const tinygltf::Buffer &indexBuffer =
          model.buffers[size_t(indexBufferView.buffer)];

      std::vector<uint32_t> indices;

      for (size_t k = 0; k < num_elements; k++) {
        // TODO(syoyo): out-of-bounds check.
        const unsigned char *ptr = indexBuffer.data.data() +
                                   indexBufferView.byteOffset +
                                   (k * byte_stride) + indexAccessor.byteOffset;

        uint32_t idx = UnpackIndex(ptr, indexAccessor.componentType);

        if (verbose) {
          std::cout << "vertex_index[" << k << "] = " << idx << "\n";
        }

        indices.push_back(idx);
      }

      out_prim.indices = indices;
      out_prim.indices_type = indexAccessor.componentType;
    }

    // attributes
    {
      std::map<std::string, int>::const_iterator it(
          primitive.attributes.begin());
      std::map<std::string, int>::const_iterator itEnd(
          primitive.attributes.end());

      for (; it != itEnd; it++) {
        // it->first would be "POSITION", "NORMAL", "TEXCOORD_0", ...
        if (!IsAttributeSupported(it->first)) {
          std::cout << "Unsupported attribute: " << it->first << "\n";
          continue;
        }

        if (size_t(it->second) >= model.accessors.size()) {
          std::cerr << "Invalid accessor id: " << it->second << "\n";
          return false;
        }

        const tinygltf::Accessor &accessor =
            model.accessors[size_t(it->second)];

        size_t elem_size = 1;
        if (accessor.type == TINYGLTF_TYPE_SCALAR) {
          elem_size = 1;
        } else if (accessor.type == TINYGLTF_TYPE_VEC2) {
          elem_size = 2;
        } else if (accessor.type == TINYGLTF_TYPE_VEC3) {
          elem_size = 3;
        } else if (accessor.type == TINYGLTF_TYPE_VEC4) {
          elem_size = 4;
        } else {
          std::cerr << "Invalid or unsupported accessor type: "
                    << PrintType(accessor.type) << "\n";
          return false;
        }

        std::cout << PrintComponentType(accessor.componentType) << "\n";

        size_t byte_stride = ComponentTypeByteSize(accessor.componentType);

        std::cout << "attribute: " << it->first << "\n";
        // if (gGLProgramState.attribs[it->first] >= 0) {
        // Compute byteStride from Accessor + BufferView combination.
        int byteStride =
            accessor.ByteStride(model.bufferViews[size_t(accessor.bufferView)]);
        assert(byteStride != -1);

        std::cout << "byteOffset: " << accessor.byteOffset << "\n";

        const tinygltf::BufferView &bufferView =
            model.bufferViews[size_t(accessor.bufferView)];
        const tinygltf::Buffer &buffer =
            model.buffers[size_t(bufferView.buffer)];

        size_t num_elems = accessor.count * elem_size;

        example::VertexAttrib attrib;
        for (size_t k = 0; k < num_elems; k++) {
          // TODO(syoyo): out-of-bounds check.
          const unsigned char *ptr = buffer.data.data() +
                                     bufferView.byteOffset + (k * byte_stride) +
                                     accessor.byteOffset;
          float value = Unpack(ptr, accessor.componentType);
          if (verbose) {
            std::cout << "[" << k << "] value = " << value << "\n";
          }
          attrib.data.push_back(value);
        }
        attrib.component_type = accessor.componentType;
        attrib.data_type = accessor.type;
        attrib.name = it->first;

        if (attrib.name.compare("POSITION") == 0) {
          out_prim.position = attrib;
        } else if (attrib.name.compare("NORMAL") == 0) {
          out_prim.normal = attrib;
        } else if (attrib.name.compare("TANGENT") == 0) {
          out_prim.tangent = attrib;
        } else if (attrib.name.rfind("TEXCOORD_", 0) == 0) {
          int id = GetSlotId(attrib.name);
          std::cout << "texcoord[" << id << "]\n";
          out_prim.texcoords[id] = attrib;
        } else if (attrib.name.rfind("JOINTS_", 0) == 0) {
          int id = GetSlotId(attrib.name);
          std::cout << "joints[" << id << "]\n";
          out_prim.joints[id] = attrib;
        } else if (attrib.name.rfind("WEIGHTS_", 0) == 0) {
          int id = GetSlotId(attrib.name);
          std::cout << "weights[" << id << "]\n";
          out_prim.weights[id] = attrib;
        } else {
          std::cerr << "???: attrib.name = " << attrib.name << "\n";
          return false;
        }
      }
    }

    const tinygltf::Accessor &indexAccessor =
        model.accessors[size_t(primitive.indices)];
    (void)indexAccessor;
    PrintMode(primitive.mode);
    if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
      std::cerr << "Supported Primitive mode is TRIANGLES only at the moment\n";
      return false;
    }

    out_prim.mode = primitive.mode;

    out->prims.push_back(out_prim);
  }

  out->name = mesh.name;

  return true;
}

static bool ExtractMesh(const std::string &asset_path, tinygltf::Model &model,
                        bool verbose, std::vector<example::MeshPrim> *outs) {
  // Get .bin data
  {
    if (model.buffers.size() != 1) {
      std::cerr << "Single element of `buffers` is supported at the moment.\n";
      return false;
    }

    const tinygltf::Buffer &buffer = model.buffers[0];
    if (buffer.uri.empty()) {
      std::cerr << "buffer.uri must be a filepath.\n";
      return false;
    }

    if (buffer.data.size() < 4) {
      std::cerr << "Invalid buffer.byteLength.\n";
      return false;
    }

    std::vector<std::string> search_paths;
    search_paths.push_back(asset_path);

    std::string abs_filepath = FindFile(search_paths, buffer.uri);
    std::vector<uint8_t> bin = LoadBin(abs_filepath);
    if (bin.size() != buffer.data.size()) {
      std::cerr << "Byte size mismatch. Failed to load file: " << buffer.uri
                << "\n";
      std::cerr << "  Searched absolute file path: " << abs_filepath << "\n";
      std::cerr << "  .bin size = " << bin.size()
                << ", size in 'buffer.uri' = " << buffer.data.size() << "\n";
      return false;
    }
  }

  for (const auto &mesh : model.meshes) {
    std::cout << "mesh.name: " << mesh.name << "\n";

    example::MeshPrim output;
    bool ret = DumpMesh(model, mesh, verbose, &output);
    if (!ret) {
      return false;
    }

    outs->push_back(output);
  }

  return true;
}

}  // namespace

int main(int argc, char **argv) {
  std::string op;
  std::string input_filename;
  std::string output_filename = "output.gltf";
  int uvset = 0;
  bool verbose = false;
  bool export_skinweight = true;
  bool no_flip_texcoord_y = false;

  auto cli =
      (clipp::required("-i", "--input") &
           clipp::value("input filename", input_filename),
       clipp::option("-o", "--outout") &
           clipp::value("Output filename(obj2fltf)", output_filename),
       clipp::option("-v", "--verbose").set(verbose).doc("Verbose output"),
       clipp::option("--export_skinweight") &
           clipp::value("Export skin weights(gltf2obj). default true.",
                        export_skinweight),
       clipp::option("--uvset").set(uvset).doc("UV set(TEXCOORD_N) to use"),
       clipp::option("--op") &
           clipp::value("operation mode(`gltf2obj`, `obj2gltf`", op),
       clipp::option("--no-flip-texcoord-y")
           .set(no_flip_texcoord_y)
           .doc("Do not flip texcoord Y"));

  if (!clipp::parse(argc, argv, cli)) {
    std::cout << clipp::make_man_page(cli, argv[0]);
    return EXIT_FAILURE;
  }

  if (op == "gltf2obj") {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    std::string ext = GetFilePathExtension(input_filename);

    {
      bool ret = false;
      if (ext.compare("glb") == 0) {
        // assume binary glTF.
        ret = loader.LoadBinaryFromFile(&model, &err, &warn,
                                        input_filename.c_str());
      } else {
        // assume ascii glTF.
        ret = loader.LoadASCIIFromFile(&model, &err, &warn,
                                       input_filename.c_str());
      }

      if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
      }

      if (!err.empty()) {
        printf("ERR: %s\n", err.c_str());
      }
      if (!ret) {
        printf("Failed to load .glTF : %s\n", argv[1]);
        exit(-1);
      }
    }

#if 0
    json j;
    {
      std::ifstream i(input_filename);
      i >> j;
    }
    std::cout << "j = " << j << "\n";

    json j_patch = R"([
      { "op": "add", "path": "/buffers/-", "value": {
              "name": "plane/data",
              "byteLength": 480,
              "uri": "plane1.bin"
          } }
    ])"_json;

    // a JSON value
    json j_original = R"({
      "baz": ["one", "two", "three"],
      "foo": "bar"
  })"_json;

    //json j_patch = R"([
    //  { "op": "remove", "path": "/buffers" }
    //])"_json;

    std::cout << "patch = " << j_patch.dump(2) << "\n";

  json j_ret = j.patch(j_patch);
  std::cout << "patched = " << j_ret.dump(2) << "\n";
#endif

    std::string basedir = GetBaseDir(input_filename);
    std::vector<example::MeshPrim> meshes;
    bool ret = ExtractMesh(basedir, model, verbose, &meshes);

    size_t n = 0;
    for (const auto &mesh : meshes) {
      // Assume no duplicated name in .glTF data
      std::string basename;
      if (mesh.name.empty()) {
        basename = "untitled-" + std::to_string(n);
      } else {
        basename = mesh.name;
      }


      for (size_t primid = 0; primid < mesh.prims.size(); primid++) {
        example::ObjExportOption options;
        options.primid = int(primid);
        options.export_skinweights = export_skinweight;
        options.uvset = uvset;
        options.flip_texcoord_y = !no_flip_texcoord_y;

        bool ok = example::SaveAsObjMesh(basename, mesh, options);
        if (!ok) {
          std::cout << "Failed to export mesh[" << mesh.name << "].primitives["
                    << primid << "]\n";
          // may ok;
        }
      }
      n++;
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
  } else if (op == "obj2gltf") {
    // Require facevarying layout?
    // facevarying representation is required if a vertex can have multiple
    // normal/uv value. drawback of facevarying is mesh data increases. false =
    // try to keep shared vertex representation as much as possible. true =
    // reorder vertex data and re-assign vertex indices for facevarying data
    // layout.
    bool facevarying = false;

    example::MeshPrim mesh;
    bool ok = example::LoadObjMesh(input_filename, facevarying, &mesh);
    if (!ok) {
      return EXIT_FAILURE;
    }

    if (verbose) {
      PrintMeshPrim(mesh);
    }

    ok = example::SaveAsGLTFMesh(output_filename, mesh);
    if (!ok) {
      std::cerr << "Failed to save mesh as glTF\n";
      return EXIT_FAILURE;
    }
    std::cout << "Write glTF: " << output_filename << "\n";
    return EXIT_SUCCESS;
  } else {
    std::cerr << "Unknown operation: " << op << "\n";
    return EXIT_FAILURE;
  }
}
