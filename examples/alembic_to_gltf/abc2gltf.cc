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

#define PICOJSON_USE_INT64
#include "../../picojson.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "../../tiny_gltf_loader.h" // To import some TINYGLTF_*** macros.

class Node
{
 public:
  Node() {
    xform[0] = 1.0;
    xform[1] = 0.0;
    xform[2] = 0.0;
    xform[3] = 0.0;

    xform[4] = 0.0;
    xform[5] = 1.0;
    xform[6] = 0.0;
    xform[7] = 0.0;

    xform[8] = 0.0;
    xform[9] = 0.0;
    xform[10] = 1.0;
    xform[11] = 0.0;

    xform[12] = 0.0;
    xform[13] = 0.0;
    xform[14] = 0.0;
    xform[15] = 1.0;
  }
  virtual ~Node();

  Node(const Node& rhs) {
    name = rhs.name;
    for (size_t i = 0; i < 16; i++) {
      xform[i] = rhs.xform[i];
    } 

    children = rhs.children;
  }

  Node &operator=(const Node& rhs) {
    name = rhs.name;
    for (size_t i = 0; i < 16; i++) {
      xform[i] = rhs.xform[i];
    } 
 
    children = rhs.children;

    return (*this);
  }

  std::string name;
  double xform[16];

  std::vector<Node *> children;
};

Node::~Node()
{

}

class Mesh : public Node
{
 public:
  Mesh() {}
  virtual ~Mesh();

  Mesh &operator=(const Mesh& rhs) {
    vertices = rhs.vertices;
    normals = rhs.normals;
    facevarying_normals = rhs.facevarying_normals;
    texcoords = rhs.texcoords;
    facevarying_texcoords = rhs.facevarying_texcoords;
    faces = rhs.faces;
 
    return (*this);
  }


  std::vector<float> vertices;

  // Either `normals` or `facevarying_normals` is filled.
  std::vector<float> normals;
  std::vector<float> facevarying_normals;

  // Either `texcoords` or `facevarying_texcoords` is filled.
  std::vector<float> texcoords;
  std::vector<float> facevarying_texcoords;

  std::vector<unsigned int> faces;
};

Mesh::~Mesh()
{

}

// Curves are represented as an array of curve.
// i'th curve has nverts[i] points.
// TODO(syoyo) knots, order to support NURBS curve.
class Curves : public Node
{
 public:
  Curves() {}
  virtual ~Curves();

  Curves &operator=(const Curves& rhs) {
    points = rhs.points;
    nverts = rhs.nverts;
 
    return (*this);
  }

  std::vector<float> points;
  std::vector<int> nverts;  // # of vertices per strand(curve).
};

Curves::~Curves()
{
}

// Points represent particle data.
// TODO(syoyo)
class Points : public Node
{
 public:
  Points() {}
  ~Points();

  Points &operator=(const Points& rhs) {
    points = rhs.points;
    radiuss = rhs.radiuss;
 
    return (*this);
  }

  std::vector<float> points;
  std::vector<float> radiuss;
};

Points::~Points()
{

}

// TODO(Nurbs)

typedef struct
{
  std::map<std::string, Mesh*> mesh_map;
  std::map<std::string, Curves*> curves_map;
  //std::map<std::string, Points> points_map;

  std::map<std::string, Node*> node_map;

} Scene;

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

static void readPolyNormals(
  std::vector<float> *normals,
  std::vector<float> *facevarying_normals,
  Alembic::AbcGeom::IN3fGeomParam normals_param)
{
  normals->clear();
  facevarying_normals->clear();

  if (!normals_param) {
    return;
  }

  if ((normals_param.getScope() != Alembic::AbcGeom::kVertexScope) &&
      (normals_param.getScope() != Alembic::AbcGeom::kVaryingScope) &&
      (normals_param.getScope() != Alembic::AbcGeom::kFacevaryingScope))
  {
    std::cout << "Normal vector has an unsupported scope" << std::endl;
    return;
  }

  if (normals_param.getScope() == Alembic::AbcGeom::kVertexScope) {
    std::cout << "Normal: VertexScope" << std::endl;
  } else if (normals_param.getScope() == Alembic::AbcGeom::kVaryingScope) {
    std::cout << "Normal: VaryingScope" << std::endl;
  } else if (normals_param.getScope() == Alembic::AbcGeom::kFacevaryingScope) {
    std::cout << "Normal: FacevaryingScope" << std::endl;
  }

  // @todo { lerp normal among time sample.}
  Alembic::AbcGeom::IN3fGeomParam::Sample samp;
  Alembic::AbcGeom::ISampleSelector samplesel(
      0.0, Alembic::AbcGeom::ISampleSelector::kNearIndex);
  normals_param.getExpanded(samp, samplesel);

  Alembic::Abc::N3fArraySamplePtr P = samp.getVals();
  size_t sample_size = P->size();

  if (normals_param.getScope() == Alembic::AbcGeom::kFacevaryingScope) {
    for (size_t i = 0; i < sample_size; i++) {
      facevarying_normals->push_back((*P)[i].x); 
      facevarying_normals->push_back((*P)[i].y); 
      facevarying_normals->push_back((*P)[i].z); 
    }
  } else {
    for (size_t i = 0; i < sample_size; i++) {
      normals->push_back((*P)[i].x); 
      normals->push_back((*P)[i].y); 
      normals->push_back((*P)[i].z); 
    }
  }
}

// @todo { Support multiple UVset. }
static void readPolyUVs(
  std::vector<float> *uvs,
  std::vector<float> *facevarying_uvs,
  Alembic::AbcGeom::IV2fGeomParam uvs_param)
{
  uvs->clear();
  facevarying_uvs->clear();

  if (!uvs_param) {
    return;
  }

  if (uvs_param.getNumSamples() > 0) {
      std::string uv_set_name = Alembic::Abc::GetSourceName(uvs_param.getMetaData());
      std::cout << "UVset : " << uv_set_name << std::endl;
  }

  if (uvs_param.isConstant()) {
    std::cout << "UV is constant" << std::endl;
  }

  if (uvs_param.getScope() == Alembic::AbcGeom::kVertexScope) {
    std::cout << "UV: VertexScope" << std::endl;
  } else if (uvs_param.getScope() == Alembic::AbcGeom::kVaryingScope) {
    std::cout << "UV: VaryingScope" << std::endl;
  } else if (uvs_param.getScope() == Alembic::AbcGeom::kFacevaryingScope) {
    std::cout << "UV: FacevaryingScope" << std::endl;
  }

  // @todo { lerp normal among time sample.}
  Alembic::AbcGeom::IV2fGeomParam::Sample samp;
  Alembic::AbcGeom::ISampleSelector samplesel(
      0.0, Alembic::AbcGeom::ISampleSelector::kNearIndex);
  uvs_param.getIndexed(samp, samplesel);

  Alembic::Abc::V2fArraySamplePtr P = samp.getVals();
  size_t sample_size = P->size();

  if (uvs_param.getScope() == Alembic::AbcGeom::kFacevaryingScope) {
    for (size_t i = 0; i < sample_size; i++) {
      facevarying_uvs->push_back((*P)[i].x); 
      facevarying_uvs->push_back((*P)[i].y); 
    }
  } else {
    for (size_t i = 0; i < sample_size; i++) {
      uvs->push_back((*P)[i].x); 
      uvs->push_back((*P)[i].y); 
    }
  }
}


// Traverse Alembic object tree and extract scene data.
static void VisitObjectAndExtractScene(Node* node_out, std::stringstream& ss, const Alembic::AbcGeom::IObject& obj, const std::string& indent) {

  std::string path = obj.getFullName();

  if (path.compare("/") != 0) {
    ss << "Object: path = " << path << std::endl;
  }

  Alembic::AbcGeom::ICompoundProperty props = obj.getProperties();
  VisitProperties(ss, props, indent);

  ss << "# of children = " << obj.getNumChildren() << std::endl;

  for (size_t i = 0; i < obj.getNumChildren(); i++) {
    const Alembic::AbcGeom::ObjectHeader& header = obj.getChildHeader(i);
    ss << " Child: header = " << header.getName() << std::endl;

    Alembic::AbcGeom::ICompoundProperty cprops = obj.getChild(i).getProperties();
    VisitProperties(ss, props, indent);

    Node *node = NULL;

    if (Alembic::AbcGeom::IXform::matches(header)) {
      ss << "    IXform" << std::endl;

      Alembic::AbcGeom::IXform xform( obj, header.getName() );

      if ( xform.getSchema().isConstant() )
      {
          Alembic::AbcGeom::M44d static_matrix = xform.getSchema().getValue().getMatrix();
          ss << "IXform static: " << header.getName() << ", " << static_matrix << std::endl;

          Node *xform_node = new Node();
          xform_node->name = header.getName();
          xform_node->xform[0] = static_matrix[0][0];
          xform_node->xform[1] = static_matrix[0][1];
          xform_node->xform[2] = static_matrix[0][2];
          xform_node->xform[3] = static_matrix[0][3];
          xform_node->xform[4] = static_matrix[1][0];
          xform_node->xform[5] = static_matrix[1][1];
          xform_node->xform[6] = static_matrix[1][2];
          xform_node->xform[7] = static_matrix[1][3];
          xform_node->xform[8] = static_matrix[2][0];
          xform_node->xform[9] = static_matrix[2][1];
          xform_node->xform[10] = static_matrix[2][2];
          xform_node->xform[11] = static_matrix[2][3];
          xform_node->xform[12] = static_matrix[3][0];
          xform_node->xform[13] = static_matrix[3][1];
          xform_node->xform[14] = static_matrix[3][2];
          xform_node->xform[15] = static_matrix[3][3];

          node = xform_node;

      } else {

        Alembic::AbcGeom::ISampleSelector samplesel(
            0.0, Alembic::AbcGeom::ISampleSelector::kNearIndex);
        Alembic::AbcGeom::IXformSchema& ps = xform.getSchema();
        Alembic::AbcGeom::M44d matrix = ps.getValue( samplesel ).getMatrix();
        ss << matrix << std::endl;

      }

      ss << " Child: xform" << std::endl;
    } else if (Alembic::AbcGeom::IPolyMesh::matches(header)) {
      ss << "    IPolyMesh" << std::endl;

      // Polygon
      Alembic::AbcGeom::IPolyMesh pmesh(obj, header.getName());

      Alembic::AbcGeom::ISampleSelector samplesel(
          0.0, Alembic::AbcGeom::ISampleSelector::kNearIndex);
      Alembic::AbcGeom::IPolyMeshSchema::Sample psample;
      Alembic::AbcGeom::IPolyMeshSchema& ps = pmesh.getSchema();

      std::cout << "  # of samples = " << ps.getNumSamples() << std::endl;

      if (ps.getNumSamples() > 0) {
        Mesh *mesh = new Mesh();
        ps.get(psample, samplesel);
        Alembic::Abc::P3fArraySamplePtr P = psample.getPositions();
        std::cout << "  # of positions   = " << P->size() << std::endl;
        std::cout << "  # of face counts = " << psample.getFaceCounts()->size()
                  << std::endl;


        // normals
        Alembic::AbcGeom::IN3fGeomParam normals_param = ps.getNormalsParam();
        std::vector<float> normals;
        std::vector<float> facevarying_normals;
        readPolyNormals(&normals, &facevarying_normals, normals_param);
        std::cout << "  # of normals   = " << (normals.size() / 3) << std::endl;
        std::cout << "  # of facevarying normals   = " << (facevarying_normals.size() / 3) << std::endl;
        mesh->normals = normals;
        

        // UV
        Alembic::AbcGeom::IV2fGeomParam uvs_param = ps.getUVsParam();
        std::vector<float> uvs;
        std::vector<float> facevarying_uvs;
        readPolyUVs(&uvs, &facevarying_uvs,  uvs_param);
        std::cout << "  # of uvs   = " << (uvs.size() / 2) << std::endl;
        std::cout << "  # of facevarying_uvs   = " << (facevarying_uvs.size() / 2) << std::endl;
        mesh->texcoords = uvs;
        mesh->facevarying_texcoords = facevarying_uvs;

        std::vector<unsigned int> faces; // temp
        bool ret = BuildFaceSet(faces, P, psample.getFaceIndices(), psample.getFaceCounts());
        if (!ret) {
          std::cout << "  No faces in polymesh" << std::endl;
        }

        mesh->vertices.resize(3 * P->size());
        memcpy(mesh->vertices.data(), P->get(), sizeof(float) * 3 * P->size());
        mesh->faces = faces;

        node = mesh;

        //assert(scene->mesh_map.find(path) == scene->mesh_map.end());
        //scene->mesh_map[path] = mesh;
        //return;
 
      } else {
        std::cout << "Warning: # of samples = 0" << std::endl;
      }
    } else if (Alembic::AbcGeom::ICurves::matches(header)) {
      ss << "    ICurves" << std::endl;

      // Curves
      Alembic::AbcGeom::ICurves curve(obj, header.getName());

      Alembic::AbcGeom::ISampleSelector samplesel(
          0.0, Alembic::AbcGeom::ISampleSelector::kNearIndex);
      Alembic::AbcGeom::ICurvesSchema::Sample psample;
      Alembic::AbcGeom::ICurvesSchema& ps = curve.getSchema();

      std::cout << "  # of samples = " << ps.getNumSamples() << std::endl;

      if (ps.getNumSamples() > 0) {
        Curves curves;
        ps.get(psample, samplesel);

        const size_t num_curves = psample.getNumCurves();
        std::cout << "  # of curves = " << num_curves << std::endl;

        Alembic::Abc::P3fArraySamplePtr P = psample.getPositions();
        Alembic::Abc::FloatArraySamplePtr knots = psample.getKnots();
        Alembic::Abc::UcharArraySamplePtr orders = psample.getOrders();

        Alembic::Abc::Int32ArraySamplePtr num_vertices = psample.getCurvesNumVertices();

        if (knots) std::cout << "  # of knots= " << knots->size() << std::endl;
        if (orders) std::cout << "  # of orders= " << orders->size() << std::endl;
        std::cout << "  # of nvs= " << num_vertices->size() << std::endl;

        curves.points.resize(3 * P->size());
        memcpy(curves.points.data(), P->get(), sizeof(float) * 3 * P->size());

#if 1
        for (size_t k = 0; k < P->size(); k++) {
          std::cout << "P[" << k << "] " << (*P)[k].x << ", " << (*P)[k].y << ", " << (*P)[k].z << std::endl;
        }

        if (knots) {
        for (size_t k = 0; k < knots->size(); k++) {
          std::cout << "knots[" << k << "] " << (*knots)[k] << std::endl;
        }
        }

        if (orders) { 
        for (size_t k = 0; k < orders->size(); k++) {
          std::cout << "orders[" << k << "] " << (*orders)[k] << std::endl;
        }
        }

        for (size_t k = 0; k < num_vertices->size(); k++) {
          std::cout << "nv[" << k << "] " << (*num_vertices)[k] << std::endl;
        }
#endif
        
        if (num_vertices) {
          curves.nverts.resize(num_vertices->size());
          memcpy(curves.nverts.data(), num_vertices->get(), sizeof(int) * num_vertices->size());
        }

        //assert(scene->curves_map.find(path) == scene->curves_map.end());
        //scene->curves_map[path] = curves;
        //return;
 
      } else {
        std::cout << "Warning: # of samples = 0" << std::endl;
      }

    } else if (Alembic::AbcGeom::IFaceSet::matches(header)) {
      ss << "    IFaceSet" << std::endl;
    } else {
      ss << "    TODO" << std::endl;
    }

    if (node) {
      // Visit child.
      VisitObjectAndExtractScene(node, ss, Alembic::AbcGeom::IObject(obj, obj.getChildHeader(i).getName()), indent);
    }

    node_out->children.push_back(node);
  }
}

static bool SaveSceneToGLTF(picojson::object *out, const Scene &scene)
{
  {
    std::map<std::string, Node*>::const_iterator it(scene.node_map.begin());
    std::map<std::string, Node*>::const_iterator it_end(scene.node_map.end());

    picojson::object nodes;

    for (; it != it_end; it++) {
      picojson::object node;

      // FIXME: Hierarchial
      picojson::array  meshes;

      meshes.push_back(picojson::value(std::string("mesh_1")));

      node["meshes"] = picojson::value(meshes);

      // Xform
      // TODO(syoyo): Support Translation, Scale, Rotate, etc.
      {
        picojson::array xform;
        for (size_t i = 0; i < 16; i++) {
          xform.push_back(picojson::value((it->second)->xform[i]));
        }
        node["matrix"] = picojson::value(xform);
      }

      nodes["node_1"] = picojson::value(node);
      (*out)["nodes"] = picojson::value(nodes);
    }
  }

  {
    picojson::object defaultScene;
    picojson::array nodes;
    
    nodes.push_back(picojson::value(std::string("node_1")));

    defaultScene["nodes"] = picojson::value(nodes);

    (*out)["scene"] = picojson::value("defaultScene");
    picojson::object scenes;
    scenes["defaultScene"] = picojson::value(defaultScene);
    (*out)["scenes"] = picojson::value(scenes);
  }


  // @todo {}
  picojson::object shaders;
  picojson::object programs;
  picojson::object techniques;
  picojson::object materials;
  picojson::object skins;
  (*out)["shaders"] = picojson::value(shaders);
  (*out)["programs"] = picojson::value(programs);
  (*out)["techniques"] = picojson::value(techniques);
  (*out)["materials"] = picojson::value(materials);
  (*out)["skins"] = picojson::value(skins);

  return true;
}

static bool SaveMeshToGLTF(const std::string& output_filename,
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

  {
    picojson::object buffers;
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
          picojson::value(static_cast<double>(mesh.faces.size() * sizeof(unsigned int)));
      
      buffers["indices"] = picojson::value(buf); 
    }
    root["buffers"] = picojson::value(buffers);
  }

  {
    picojson::object buffer_views;
    {
      picojson::object buffer_view_vertices;
      buffer_view_vertices["buffer"] = picojson::value(std::string("vertices"));    
      buffer_view_vertices["byteLength"] = picojson::value(static_cast<double>(mesh.vertices.size() * sizeof(float)));
      buffer_view_vertices["byteOffset"] = picojson::value(static_cast<double>(0));
      buffer_view_vertices["target"] = picojson::value(static_cast<int64_t>(TINYGLTF_TARGET_ARRAY_BUFFER));

      buffer_views["bufferView_vertices"] = picojson::value(buffer_view_vertices);
    }

    {
      picojson::object buffer_view_indices;
      buffer_view_indices["buffer"] = picojson::value(std::string("indices"));    
      buffer_view_indices["byteLength"] = picojson::value(static_cast<double>(mesh.faces.size() * sizeof(unsigned int)));
      buffer_view_indices["byteOffset"] = picojson::value(static_cast<double>(0));
      buffer_view_indices["target"] = picojson::value(static_cast<int64_t>(TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER));

      buffer_views["bufferView_indices"] = picojson::value(buffer_view_indices);
    }

    root["bufferViews"] = picojson::value(buffer_views);
  }

  {
    picojson::object attributes;
  
    attributes["POSITION"] = picojson::value(std::string("accessor_vertices"));

    
    picojson::object primitive;
    primitive["attributes"] = picojson::value(attributes);
    primitive["indices"] = picojson::value("accessor_indices");
    primitive["material"] = picojson::value("material_1");
    primitive["mode"] = picojson::value(static_cast<int64_t>(TINYGLTF_MODE_TRIANGLES));

    picojson::array primitive_array;
    primitive_array.push_back(picojson::value(primitive));

    picojson::object m;
    m["primitives"] = picojson::value(primitive_array);

    picojson::object meshes;
    meshes["mesh_1"] = picojson::value(m);

    
    root["meshes"] = picojson::value(meshes);
  }

  {
    picojson::object accessors;
    picojson::object accessor_vertices;
    picojson::object accessor_indices;
    
    accessor_vertices["bufferView"] = picojson::value(std::string("bufferView_vertices"));
    accessor_vertices["byteOffset"] = picojson::value(static_cast<int64_t>(0));
    accessor_vertices["byteStride"] = picojson::value(static_cast<double>(3 * sizeof(float)));
    accessor_vertices["componentType"] = picojson::value(static_cast<int64_t>(TINYGLTF_COMPONENT_TYPE_FLOAT));
    accessor_vertices["count"] = picojson::value(static_cast<int64_t>(mesh.vertices.size()));
    accessor_vertices["type"] = picojson::value(std::string("VEC3"));
    accessors["accessor_vertices"] = picojson::value(accessor_vertices);

    accessor_indices["bufferView"] = picojson::value(std::string("bufferView_indices"));
    accessor_indices["byteOffset"] = picojson::value(static_cast<int64_t>(0));
    accessor_indices["componentType"] = picojson::value(static_cast<int64_t>(TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT));
    accessor_indices["count"] = picojson::value(static_cast<int64_t>(mesh.faces.size()));
    accessor_indices["type"] = picojson::value(std::string("SCALAR"));
    accessors["accessor_indices"] = picojson::value(accessor_indices);
    root["accessors"] = picojson::value(accessors);
  }

  {
    // Use Default Material(Do not supply `material.technique`)
    picojson::object default_material;
    picojson::object materials;

    materials["material_1"] = picojson::value(default_material);

    root["materials"] = picojson::value(materials);

  }

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

static bool SaveCurvesToGLTF(const std::string& output_filename,
              const Curves& curves) {
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

  {
    picojson::object buffers;
    {
      {
        std::string b64data = base64_encode(reinterpret_cast<unsigned char const*>(curves.points.data()), curves.points.size() * sizeof(float));
        picojson::object buf;

        buf["type"] = picojson::value("arraybuffer");
        buf["uri"] = picojson::value(
            std::string("data:application/octet-stream;base64,") + b64data);
        buf["byteLength"] =
            picojson::value(static_cast<double>(curves.points.size() * sizeof(float)));
        
        buffers["points"] = picojson::value(buf); 
      }

      // Out extension
      {
        std::string b64data = base64_encode(reinterpret_cast<unsigned char const*>(curves.nverts.data()), curves.nverts.size() * sizeof(int));
        picojson::object buf;

        buf["type"] = picojson::value("arraybuffer");
        buf["uri"] = picojson::value(
            std::string("data:application/octet-stream;base64,") + b64data);
        buf["byteLength"] =
            picojson::value(static_cast<double>(curves.nverts.size() * sizeof(int)));
        
        buffers["nverts"] = picojson::value(buf); 
      }

    }
    root["buffers"] = picojson::value(buffers);
  }

  {
    picojson::object buffer_views;
    {
      {
        picojson::object buffer_view_points;
        buffer_view_points["buffer"] = picojson::value(std::string("points"));    
        buffer_view_points["byteLength"] = picojson::value(static_cast<double>(curves.points.size() * sizeof(float)));
        buffer_view_points["byteOffset"] = picojson::value(static_cast<double>(0));
        buffer_view_points["target"] = picojson::value(static_cast<int64_t>(TINYGLTF_TARGET_ARRAY_BUFFER));
        buffer_views["bufferView_points"] = picojson::value(buffer_view_points);
      }

      {
        picojson::object buffer_view_nverts;
        buffer_view_nverts["buffer"] = picojson::value(std::string("nverts"));    
        buffer_view_nverts["byteLength"] = picojson::value(static_cast<double>(curves.nverts.size() * sizeof(int)));
        buffer_view_nverts["byteOffset"] = picojson::value(static_cast<double>(0));
        buffer_view_nverts["target"] = picojson::value(static_cast<int64_t>(TINYGLTF_TARGET_ARRAY_BUFFER));
        buffer_views["bufferView_nverts"] = picojson::value(buffer_view_nverts);
      }

    }

    root["bufferViews"] = picojson::value(buffer_views);
  }

  {
    picojson::object attributes;
  
    attributes["POSITION"] = picojson::value(std::string("accessor_points"));
    attributes["NVERTS"] = picojson::value(std::string("accessor_nverts"));

    // Extra information for curves primtive.
    picojson::object extra;
    extra["ext_mode"] = picojson::value("curves");
    
    picojson::object primitive;
    primitive["attributes"] = picojson::value(attributes);
    //primitive["indices"] = picojson::value("accessor_indices");
    primitive["material"] = picojson::value("material_1");
    primitive["mode"] = picojson::value(static_cast<int64_t>(TINYGLTF_MODE_POINTS)); // Use GL_POINTS for backward compatibility
    primitive["extras"] = picojson::value(extra);


    picojson::array primitive_array;
    primitive_array.push_back(picojson::value(primitive));

    picojson::object m;
    m["primitives"] = picojson::value(primitive_array);

    picojson::object meshes;
    meshes["mesh_1"] = picojson::value(m);

    
    root["meshes"] = picojson::value(meshes);
  }

  {
    picojson::object accessors;

    {
      picojson::object accessor_points;
      accessor_points["bufferView"] = picojson::value(std::string("bufferView_points"));
      accessor_points["byteOffset"] = picojson::value(static_cast<int64_t>(0));
      accessor_points["byteStride"] = picojson::value(static_cast<double>(3 * sizeof(float)));
      accessor_points["componentType"] = picojson::value(static_cast<int64_t>(TINYGLTF_COMPONENT_TYPE_FLOAT));
      accessor_points["count"] = picojson::value(static_cast<int64_t>(curves.points.size()));
      accessor_points["type"] = picojson::value(std::string("VEC3"));
      accessors["accessor_points"] = picojson::value(accessor_points);
    }

    {
      picojson::object accessor_nverts;
      accessor_nverts["bufferView"] = picojson::value(std::string("bufferView_nverts"));
      accessor_nverts["byteOffset"] = picojson::value(static_cast<int64_t>(0));
      accessor_nverts["byteStride"] = picojson::value(static_cast<double>(sizeof(int)));
      accessor_nverts["componentType"] = picojson::value(static_cast<int64_t>(TINYGLTF_COMPONENT_TYPE_INT));
      accessor_nverts["count"] = picojson::value(static_cast<int64_t>(curves.nverts.size()));
      accessor_nverts["type"] = picojson::value(std::string("SCALAR"));
      accessors["accessor_nverts"] = picojson::value(accessor_nverts);
    }

    picojson::object accessor_indices;

    root["accessors"] = picojson::value(accessors);
  }

  {
    // Use Default Material(Do not supply `material.technique`)
    picojson::object default_material;
    picojson::object materials;

    materials["material_1"] = picojson::value(default_material);

    root["materials"] = picojson::value(materials);

  }

  {
    picojson::object nodes;
    picojson::object node;
    picojson::array  meshes;

    meshes.push_back(picojson::value(std::string("mesh_1")));

    node["meshes"] = picojson::value(meshes);

    nodes["node_1"] = picojson::value(node);
    root["nodes"] = picojson::value(nodes);
  }

  {
    picojson::object defaultScene;
    picojson::array nodes;
    
    nodes.push_back(picojson::value(std::string("node_1")));

    defaultScene["nodes"] = picojson::value(nodes);

    root["scene"] = picojson::value("defaultScene");
    picojson::object scenes;
    scenes["defaultScene"] = picojson::value(defaultScene);
    root["scenes"] = picojson::value(scenes);
  }


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

#if 0
// Flatten multiple Curves object to one Curves.
static void FlattenCurves(Curves *flatten_curves, const Scene &scene)
{
  flatten_curves->name = "flatten_curves";

  std::map<std::string, Curves*>::const_iterator it(scene.curves_map.begin());
  std::map<std::string, Curves*>::const_iterator itEnd(scene.curves_map.end());

  for (; it != itEnd; it++) {
    const Curves &curves = it->second;
    std::copy(curves.points.begin(), curves.points.end(), std::back_inserter(flatten_curves->points));
    std::copy(curves.nverts.begin(), curves.nverts.end(), std::back_inserter(flatten_curves->nverts));
  }
}
#endif

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

  Scene scene;
  std::stringstream ss;
  Node node;
  VisitObjectAndExtractScene(&node, ss, root, /* indent */"  ");

  std::cout << ss.str() << std::endl;

  if (scene.curves_map.size() > 0) {
  }
#if 0
  if (foundMesh) {
    bool ret = SaveMeshToGLTF(gltf_filename, mesh);
    if (ret) {
      std::cout << "Wrote " << gltf_filename << std::endl;
    } else {
      return EXIT_FAILURE;
    }
  } else {
    std::cout << "No polygon mesh found in Alembic file" << std::endl;
  }

  if (foundCurves) {
    bool ret = SaveCurvesToGLTF(gltf_filename, curves);
    if (ret) {
      std::cout << "Wrote " << gltf_filename << std::endl;
    } else {
      return EXIT_FAILURE;
    }
  }
#endif

  return EXIT_SUCCESS;
}
