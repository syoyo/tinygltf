#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fstream>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++11-long-long"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wc++11-extensions"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcGeom/All.h>

#include "../../picojson.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

// @todo { texcoords, normals }
typedef struct
{
  std::vector<float> vertices;
  std::vector<unsigned int> faces;
} Mesh;

// ----------------------------------------------------------------
// writer module
// @todo { move writer code to tiny_gltf_writer.h }

// http://www.adp-gmbh.ch/cpp/common/base64.html
static const char *base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::string base64_encode(unsigned char const* bytes_to_encode,
                          size_t in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = static_cast<unsigned char>(
          ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
      char_array_4[2] = static_cast<unsigned char>(
          ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4); i++) ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = static_cast<unsigned char>(
        ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
    char_array_4[2] = static_cast<unsigned char>(
        ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];

    while ((i++ < 3)) ret += '=';
  }

  return ret;
}

//static bool EncodeFloatArray(picojson::array* arr, const std::vector<float>& values) {
//  for (size_t i = 0; i < values.size(); i++) {
//    arr->push_back(picojson::value(values[i]));
//  }
//
//  return true;
//}
//

// ---------------------------------------------------------------------------

static const char* g_sep = ":";

static void VisitProperties(std::stringstream& ss, Alembic::AbcGeom::ICompoundProperty parent, const std::string& indent);

template <class PROP>
static void VisitSimpleProperty(std::stringstream& ss, PROP i_prop,
                                const std::string& indent) {
  std::string ptype = "ScalarProperty ";
  if (i_prop.isArray()) {
    ptype = "ArrayProperty ";
  }

  std::string mdstring = "interpretation=";
  mdstring += i_prop.getMetaData().get("interpretation");

  std::stringstream dtype;
  dtype << "datatype=";
  dtype << i_prop.getDataType();

  mdstring += g_sep;

  mdstring += dtype.str();

  ss << indent << "  " << ptype << "name=" << i_prop.getName() << g_sep
     << mdstring << g_sep << "numsamps=" << i_prop.getNumSamples() << std::endl;
}

static void VisitCompoundProperty(std::stringstream& ss,
                                  Alembic::Abc::ICompoundProperty i_prop,
                                  const std::string& indent) {
  std::string io_indent = indent + "  ";

  std::string interp = "schema=";
  interp += i_prop.getMetaData().get("schema");

  ss << io_indent << "CompoundProperty "
     << "name=" << i_prop.getName() << g_sep << interp << std::endl;

  VisitProperties(ss, i_prop, io_indent);
}

void VisitProperties(std::stringstream& ss,
                     Alembic::AbcGeom::ICompoundProperty parent,
                     const std::string& indent) {
  for (size_t i = 0; i < parent.getNumProperties(); i++) {
    const Alembic::Abc::PropertyHeader& header = parent.getPropertyHeader(i);

    if (header.isCompound()) {
      VisitCompoundProperty(
          ss, Alembic::Abc::ICompoundProperty(parent, header.getName()),
          indent);
    } else if (header.isScalar()) {
      VisitSimpleProperty(
          ss, Alembic::Abc::IScalarProperty(parent, header.getName()), indent);

    } else {
      assert(header.isArray());
      VisitSimpleProperty(
          ss, Alembic::Abc::IArrayProperty(parent, header.getName()), indent);
    }
  }
}

static bool
BuildFaceSet(
    std::vector<unsigned int>& faces,
    Alembic::Abc::P3fArraySamplePtr   iP,
    Alembic::Abc::Int32ArraySamplePtr iIndices,
    Alembic::Abc::Int32ArraySamplePtr iCounts)
{

    faces.clear();

    // Get the number of each thing.
    size_t numFaces = iCounts->size();
    size_t numIndices = iIndices->size();
    size_t numPoints = iP->size();
    if ( numFaces < 1 ||
         numIndices < 1 ||
         numPoints < 1 )
    {
        // Invalid.
        std::cerr << "Mesh update quitting because bad arrays"
                  << ", numFaces = " << numFaces
                  << ", numIndices = " << numIndices
                  << ", numPoints = " << numPoints
                  << std::endl;
        return false;
    }

    // Make triangles.
    size_t faceIndexBegin = 0;
    size_t faceIndexEnd = 0;
    for ( size_t face = 0; face < numFaces; ++face )
    {
        faceIndexBegin = faceIndexEnd;
        size_t count = static_cast<size_t>((*iCounts)[face]);
        faceIndexEnd = faceIndexBegin + count;

        // Check this face is valid
        if ( faceIndexEnd > numIndices ||
             faceIndexEnd < faceIndexBegin )
        {
            std::cerr << "Mesh update quitting on face: "
                      << face
                      << " because of wonky numbers"
                      << ", faceIndexBegin = " << faceIndexBegin
                      << ", faceIndexEnd = " << faceIndexEnd
                      << ", numIndices = " << numIndices
                      << ", count = " << count
                      << std::endl;

            // Just get out, make no more triangles.
            break;
        }

        // Checking indices are valid.
        bool goodFace = true;
        for ( size_t fidx = faceIndexBegin;
              fidx < faceIndexEnd; ++fidx )
        {
            if ( static_cast<size_t>(( (*iIndices)[fidx] )) >= numPoints )
            {
                std::cout << "Mesh update quitting on face: "
                          << face
                          << " because of bad indices"
                          << ", indexIndex = " << fidx
                          << ", vertexIndex = " << (*iIndices)[fidx]
                          << ", numPoints = " << numPoints
                          << std::endl;
                goodFace = false;
                break;
            }
        }

        // Make triangles to fill this face.
        if ( goodFace && count > 2 )
        {
            faces.push_back(static_cast<unsigned int>((*iIndices)[faceIndexBegin+0]));
            faces.push_back(static_cast<unsigned int>((*iIndices)[faceIndexBegin+1]));
            faces.push_back(static_cast<unsigned int>((*iIndices)[faceIndexBegin+2]));

            for ( size_t c = 3; c < count; ++c )
            {
                faces.push_back(static_cast<unsigned int>((*iIndices)[faceIndexBegin+0]));
                faces.push_back(static_cast<unsigned int>((*iIndices)[faceIndexBegin+c-1]));
                faces.push_back(static_cast<unsigned int>((*iIndices)[faceIndexBegin+c]));
            }
        }
    }

    return true;

}


// Traverse Alembic object tree and extract mesh object.
// Currently we only extract first found polygon mesh.
static void VisitObjectAndExtractMesh(Mesh* mesh, std::stringstream& ss, bool& foundMesh, const Alembic::AbcGeom::IObject& obj, const std::string& indent) {
  if (foundMesh) {
      return;
  }

  std::string path = obj.getFullName();

  if (path.compare("/") != 0) {
    ss << "Object: path = " << path << std::endl;
  }

  Alembic::AbcGeom::ICompoundProperty props = obj.getProperties();
  VisitProperties(ss, props, indent);

  for (size_t i = 0; i < obj.getNumChildren(); i++) {
    const Alembic::AbcGeom::ObjectHeader& header = obj.getChildHeader(i);

    if (Alembic::AbcGeom::IPolyMesh::matches(header)) {
      // Polygon
      Alembic::AbcGeom::IPolyMesh pmesh(obj, header.getName());

      Alembic::AbcGeom::ISampleSelector samplesel(
          0.0, Alembic::AbcGeom::ISampleSelector::kNearIndex);
      Alembic::AbcGeom::IPolyMeshSchema::Sample psample;
      Alembic::AbcGeom::IPolyMeshSchema& ps = pmesh.getSchema();

      std::cout << "  # of samples = " << ps.getNumSamples() << std::endl;

      if (ps.getNumSamples() > 0) {
        ps.get(psample, samplesel);
        Alembic::Abc::P3fArraySamplePtr P = psample.getPositions();
        std::cout << "  # of positions   = " << P->size() << std::endl;
        std::cout << "  # of face counts = " << psample.getFaceCounts()->size()
                  << std::endl;


        std::vector<unsigned int> faces; // temp
        bool ret = BuildFaceSet(faces, P, psample.getFaceIndices(), psample.getFaceCounts());
        assert(ret);
        (void)(ret);

        mesh->vertices.resize(3 * P->size());
        memcpy(mesh->vertices.data(), P->get(), sizeof(float) * 3 * P->size());
        mesh->faces = faces;

        foundMesh = true;
        return;
 
      } else {
        std::cout << "Warning: # of samples = 0" << std::endl;
      }
    }

    VisitObjectAndExtractMesh(mesh, ss, foundMesh, Alembic::AbcGeom::IObject(obj, obj.getChildHeader(i).getName()), indent);
  }
}

static bool SaveGLTF(const std::string& output_filename,
              const Mesh& mesh) {
  picojson::object root;

  {
    picojson::object asset;
    asset["generator"] = picojson::value("abc2gltf");
    asset["premultipliedAlpha"] = picojson::value(true);
    asset["version"] = picojson::value(static_cast<double>(1));
    picojson::object profile;
    profile["api"] = picojson::value("WebGL");
    profile["version"] = picojson::value("1.0.2");
    asset["profile"] = picojson::value(profile);
    root["assets"] = picojson::value(asset);
  }

  picojson::object buffers;
  {
    {
      std::string vertices_b64data = base64_encode(reinterpret_cast<unsigned char const*>(mesh.vertices.data()), mesh.vertices.size() * sizeof(float));
      picojson::object buf;

      buf["type"] = picojson::value("arraybuffer");
      buf["uri"] = picojson::value(
          std::string("data:application/octet-stream;base64,") + vertices_b64data);
      buf["byteLength"] =
          picojson::value(static_cast<double>(mesh.vertices.size() * sizeof(float)));
      
      buffers["vertices"] = picojson::value(buf); 
    }
    {
      std::string faces_b64data = base64_encode(reinterpret_cast<unsigned char const*>(mesh.faces.data()), mesh.faces.size() * sizeof(unsigned int));
      picojson::object buf;

      buf["type"] = picojson::value("arraybuffer");
      buf["uri"] = picojson::value(
          std::string("data:application/octet-stream;base64,") + faces_b64data);
      buf["byteLength"] =
          picojson::value(static_cast<double>(mesh.vertices.size() * sizeof(unsigned int)));
      
      buffers["indices"] = picojson::value(buf); 
    }
  }
  root["buffers"] = picojson::value(buffers);

  // @todo {}
  picojson::object shaders;
  picojson::object programs;
  picojson::object techniques;
  picojson::object materials;
  picojson::object skins;
  root["shaders"] = picojson::value(shaders);
  root["programs"] = picojson::value(programs);
  root["techniques"] = picojson::value(techniques);
  root["materials"] = picojson::value(materials);
  root["skins"] = picojson::value(skins);

  std::ofstream ifs(output_filename.c_str());
  if (ifs.bad()) {
    std::cerr << "Failed to open " << output_filename << std::endl;
    return false;
  }

  picojson::value v = picojson::value(root);

  std::string s = v.serialize(/* pretty */true);
  ifs.write(s.data(), static_cast<ssize_t>(s.size()));
  ifs.close();

  return true;
}


int main(int argc, char** argv) {
  std::string abc_filename;
  std::string gltf_filename;

  if (argc < 3) {
    std::cerr << "Usage: gltf2abc input.abc output.gltf" << std::endl;
    return EXIT_FAILURE;
  }

  abc_filename = std::string(argv[1]);
  gltf_filename = std::string(argv[2]);

  Alembic::AbcCoreFactory::IFactory factory;
  Alembic::AbcGeom::IArchive archive = factory.getArchive(abc_filename);

  Alembic::AbcGeom::IObject root = archive.getTop();

  std::cout << "# of children " << root.getNumChildren() << std::endl;

  std::stringstream ss;
  Mesh mesh;
  bool foundMesh = false;
  VisitObjectAndExtractMesh(&mesh, ss, foundMesh, root, /* indent */"  ");

  std::cout << ss.str() << std::endl;

  if (foundMesh) {
    bool ret = SaveGLTF(gltf_filename, mesh);
    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
  } else {
    std::cout << "No polygon mesh found in Alembic file" << std::endl;
  }

  return EXIT_SUCCESS;
}
