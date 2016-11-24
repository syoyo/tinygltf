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

#include "../../tiny_gltf_loader.h"  // To import some TINYGLTF_*** macros.

template <typename T = float>
class real3 {
 public:
  real3() {}
  real3(T xx, T yy, T zz) {
    v[0] = xx;
    v[1] = yy;
    v[2] = zz;
  }
  explicit real3(const T *p) {
    v[0] = p[0];
    v[1] = p[1];
    v[2] = p[2];
  }

  inline T x() const { return v[0]; }
  inline T y() const { return v[1]; }
  inline T z() const { return v[2]; }

  real3 operator*(T f) const { return real3(x() * f, y() * f, z() * f); }
  real3 operator-(const real3 &f2) const {
    return real3(x() - f2.x(), y() - f2.y(), z() - f2.z());
  }
  real3 operator*(const real3 &f2) const {
    return real3(x() * f2.x(), y() * f2.y(), z() * f2.z());
  }
  real3 operator+(const real3 &f2) const {
    return real3(x() + f2.x(), y() + f2.y(), z() + f2.z());
  }
  real3 &operator+=(const real3 &f2) {
    v[0] += f2.x();
    v[1] += f2.y();
    v[2] += f2.z();
    return (*this);
  }
  real3 operator/(const real3 &f2) const {
    return real3(x() / f2.x(), y() / f2.y(), z() / f2.z());
  }
  T operator[](int i) const { return v[i]; }
  T &operator[](int i) { return v[i]; }

  T v[3];
  // T pad;  // for alignment(when T = float)
};

template <typename T>
inline real3<T> operator*(T f, const real3<T> &v) {
  return real3<T>(v.x() * f, v.y() * f, v.z() * f);
}

template <typename T>
inline real3<T> vneg(const real3<T> &rhs) {
  return real3<T>(-rhs.x(), -rhs.y(), -rhs.z());
}

template <typename T>
inline T vlength(const real3<T> &rhs) {
  return std::sqrt(rhs.x() * rhs.x() + rhs.y() * rhs.y() + rhs.z() * rhs.z());
}

template <typename T>
inline real3<T> vnormalize(const real3<T> &rhs) {
  real3<T> v = rhs;
  T len = vlength(rhs);
  if (std::fabs(len) > 1.0e-6f) {
    float inv_len = 1.0f / len;
    v.v[0] *= inv_len;
    v.v[1] *= inv_len;
    v.v[2] *= inv_len;
  }
  return v;
}

template <typename T>
inline real3<T> vcross(real3<T> a, real3<T> b) {
  real3<T> c;
  c[0] = a[1] * b[2] - a[2] * b[1];
  c[1] = a[2] * b[0] - a[0] * b[2];
  c[2] = a[0] * b[1] - a[1] * b[0];
  return c;
}

template <typename T>
inline T vdot(real3<T> a, real3<T> b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

typedef real3<float> float3;

class Node {
 public:
  Node() {}
  virtual ~Node();

  Node(const Node &rhs) {
    name = rhs.name;
    children = rhs.children;
  }

  Node &operator=(const Node &rhs) {
    name = rhs.name;
    children = rhs.children;

    return (*this);
  }

  std::string name;

  std::vector<Node *> children;
};

static void CalcNormal(float3 &N, float3 v0, float3 v1, float3 v2) {
  float3 v10 = v1 - v0;
  float3 v20 = v2 - v0;

  N = vcross(v20, v10);
  N = vnormalize(N);
}

static void ComputeGeometricNormals(std::vector<float> *normals_out,
                                    const unsigned char *data,
                                    const size_t vertex_stride,
                                    const unsigned int *indices,
                                    const size_t num_indices) {
  for (size_t i = 0; i < num_indices / 3; i++) {
    const unsigned int i0 = indices[3 * i + 0];
    const unsigned int i1 = indices[3 * i + 1];
    const unsigned int i2 = indices[3 * i + 2];

    const float *p0 =
        reinterpret_cast<const float *>(&data[vertex_stride * i0]);
    const float *p1 =
        reinterpret_cast<const float *>(&data[vertex_stride * i1]);
    const float *p2 =
        reinterpret_cast<const float *>(&data[vertex_stride * i2]);

    float3 v0(p0[0], p0[1], p0[2]);
    float3 v1(p1[0], p1[1], p1[2]);
    float3 v2(p2[0], p2[1], p2[2]);

    float3 n;
    CalcNormal(n, v0, v1, v2);
    // printf("n = %f, %f, %f\n", n[0], n[1], n[2]);

    (*normals_out)[3 * i0 + 0] += n[0];
    (*normals_out)[3 * i0 + 1] += n[1];
    (*normals_out)[3 * i0 + 2] += n[2];
    (*normals_out)[3 * i1 + 0] += n[0];
    (*normals_out)[3 * i1 + 1] += n[1];
    (*normals_out)[3 * i1 + 2] += n[2];
    (*normals_out)[3 * i2 + 0] += n[0];
    (*normals_out)[3 * i2 + 1] += n[1];
    (*normals_out)[3 * i2 + 2] += n[2];
  }

  // normalize
  for (size_t i = 0; i < normals_out->size() / 3; i++) {
    float3 n((*normals_out)[3 * i + 0], (*normals_out)[3 * i + 1],
             (*normals_out)[3 * i + 2]);
    n = vnormalize(n);
    (*normals_out)[3 * i + 0] = n[0];
    (*normals_out)[3 * i + 1] = n[1];
    (*normals_out)[3 * i + 2] = n[2];
  }
}

Node::~Node() {}

class Xform : public Node {
 public:
  Xform() : Node() {
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

  virtual ~Xform();

  Xform &operator=(const Xform &rhs) {
    if (this != &rhs) {
      Node::operator=(rhs);

      for (size_t i = 0; i < 16; i++) {
        xform[i] = rhs.xform[i];
      }
    }

    return (*this);
  }

  double xform[16];
};

Xform::~Xform() {}

class Mesh : public Node {
 public:
  Mesh() : Node() {}
  virtual ~Mesh();

  Mesh &operator=(const Mesh &rhs) {
    if (this != &rhs) {
      Node::operator=(rhs);

      vertices = rhs.vertices;
      normals = rhs.normals;
      facevarying_normals = rhs.facevarying_normals;
      texcoords = rhs.texcoords;
      facevarying_texcoords = rhs.facevarying_texcoords;
      faces = rhs.faces;
    }

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

Mesh::~Mesh() {}

// Curves are represented as an array of curve.
// i'th curve has nverts[i] points.
// TODO(syoyo) knots, order to support NURBS curve.
class Curves : public Node {
 public:
  Curves() : Node() {}
  virtual ~Curves();

  Curves &operator=(const Curves &rhs) {
    if (this != &rhs) {
      Node::operator=(rhs);

      points = rhs.points;
      nverts = rhs.nverts;
    }

    return (*this);
  }

  std::vector<float> points;
  std::vector<int> nverts;  // # of vertices per strand(curve).
};

Curves::~Curves() {}

// Points represent particle data.
// TODO(syoyo)
class Points : public Node {
 public:
  Points() : Node() {}
  ~Points();

  Points &operator=(const Points &rhs) {
    if (this != &rhs) {
      Node::operator=(rhs);

      points = rhs.points;
      radiuss = rhs.radiuss;
    }

    return (*this);
  }

  std::vector<float> points;
  std::vector<float> radiuss;
};

Points::~Points() {}

// TODO(Nurbs)

class Scene {
 public:
  Scene() : root_node(NULL) {}
  ~Scene() { Destroy(); }

  void Destroy() {
    delete root_node;
    root_node = NULL;

    {
      std::map<std::string, const Xform *>::const_iterator it(
          xform_map.begin());
      for (; it != xform_map.end(); it++) {
        delete it->second;
      }
    }

    {
      std::map<std::string, const Mesh *>::const_iterator it(mesh_map.begin());
      for (; it != mesh_map.end(); it++) {
        delete it->second;
      }
    }

    {
      std::map<std::string, const Curves *>::const_iterator it(
          curves_map.begin());
      for (; it != curves_map.end(); it++) {
        delete it->second;
      }
    }
  }

  std::map<std::string, const Xform *> xform_map;
  std::map<std::string, const Mesh *> mesh_map;
  std::map<std::string, const Curves *> curves_map;
  // std::map<std::string, Points> points_map;

  std::map<std::string, int> id_map;

  const Node *root_node;
  // std::map<std::string, Node*> node_map;
};

// ----------------------------------------------------------------
// writer module
// @todo { move writer code to tiny_gltf_writer.h }

// http://www.adp-gmbh.ch/cpp/common/base64.html
static const char *base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::string base64_encode(unsigned char const *bytes_to_encode,
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

// static bool EncodeFloatArray(picojson::array* arr, const std::vector<float>&
// values) {
//  for (size_t i = 0; i < values.size(); i++) {
//    arr->push_back(picojson::value(values[i]));
//  }
//
//  return true;
//}
//

// ---------------------------------------------------------------------------

static const char *g_sep = ":";

static void VisitProperties(std::stringstream &ss,
                            Alembic::AbcGeom::ICompoundProperty parent,
                            const std::string &indent);

template <class PROP>
static void VisitSimpleProperty(std::stringstream &ss, PROP i_prop,
                                const std::string &indent) {
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

static void VisitCompoundProperty(std::stringstream &ss,
                                  Alembic::Abc::ICompoundProperty i_prop,
                                  const std::string &indent) {
  std::string io_indent = indent + "  ";

  std::string interp = "schema=";
  interp += i_prop.getMetaData().get("schema");

  ss << io_indent << "CompoundProperty "
     << "name=" << i_prop.getName() << g_sep << interp << std::endl;

  VisitProperties(ss, i_prop, io_indent);
}

void VisitProperties(std::stringstream &ss,
                     Alembic::AbcGeom::ICompoundProperty parent,
                     const std::string &indent) {
  for (size_t i = 0; i < parent.getNumProperties(); i++) {
    const Alembic::Abc::PropertyHeader &header = parent.getPropertyHeader(i);

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

static bool BuildFaceSet(std::vector<unsigned int> &faces,
                         Alembic::Abc::P3fArraySamplePtr iP,
                         Alembic::Abc::Int32ArraySamplePtr iIndices,
                         Alembic::Abc::Int32ArraySamplePtr iCounts) {
  faces.clear();

  // Get the number of each thing.
  size_t numFaces = iCounts->size();
  size_t numIndices = iIndices->size();
  size_t numPoints = iP->size();
  if (numFaces < 1 || numIndices < 1 || numPoints < 1) {
    // Invalid.
    std::cerr << "Mesh update quitting because bad arrays"
              << ", numFaces = " << numFaces << ", numIndices = " << numIndices
              << ", numPoints = " << numPoints << std::endl;
    return false;
  }

  // Make triangles.
  size_t faceIndexBegin = 0;
  size_t faceIndexEnd = 0;
  for (size_t face = 0; face < numFaces; ++face) {
    faceIndexBegin = faceIndexEnd;
    size_t count = static_cast<size_t>((*iCounts)[face]);
    faceIndexEnd = faceIndexBegin + count;

    // Check this face is valid
    if (faceIndexEnd > numIndices || faceIndexEnd < faceIndexBegin) {
      std::cerr << "Mesh update quitting on face: " << face
                << " because of wonky numbers"
                << ", faceIndexBegin = " << faceIndexBegin
                << ", faceIndexEnd = " << faceIndexEnd
                << ", numIndices = " << numIndices << ", count = " << count
                << std::endl;

      // Just get out, make no more triangles.
      break;
    }

    // Checking indices are valid.
    bool goodFace = true;
    for (size_t fidx = faceIndexBegin; fidx < faceIndexEnd; ++fidx) {
      if (static_cast<size_t>(((*iIndices)[fidx])) >= numPoints) {
        std::cout << "Mesh update quitting on face: " << face
                  << " because of bad indices"
                  << ", indexIndex = " << fidx
                  << ", vertexIndex = " << (*iIndices)[fidx]
                  << ", numPoints = " << numPoints << std::endl;
        goodFace = false;
        break;
      }
    }

    // Make triangles to fill this face.
    if (goodFace && count > 2) {
      faces.push_back(
          static_cast<unsigned int>((*iIndices)[faceIndexBegin + 0]));
      faces.push_back(
          static_cast<unsigned int>((*iIndices)[faceIndexBegin + 1]));
      faces.push_back(
          static_cast<unsigned int>((*iIndices)[faceIndexBegin + 2]));

      for (size_t c = 3; c < count; ++c) {
        faces.push_back(
            static_cast<unsigned int>((*iIndices)[faceIndexBegin + 0]));
        faces.push_back(
            static_cast<unsigned int>((*iIndices)[faceIndexBegin + c - 1]));
        faces.push_back(
            static_cast<unsigned int>((*iIndices)[faceIndexBegin + c]));
      }
    }
  }

  return true;
}

static void readPolyNormals(std::vector<float> *normals,
                            std::vector<float> *facevarying_normals,
                            Alembic::AbcGeom::IN3fGeomParam normals_param) {
  normals->clear();
  facevarying_normals->clear();

  if (!normals_param) {
    return;
  }

  if ((normals_param.getScope() != Alembic::AbcGeom::kVertexScope) &&
      (normals_param.getScope() != Alembic::AbcGeom::kVaryingScope) &&
      (normals_param.getScope() != Alembic::AbcGeom::kFacevaryingScope)) {
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
static void readPolyUVs(std::vector<float> *uvs,
                        std::vector<float> *facevarying_uvs,
                        Alembic::AbcGeom::IV2fGeomParam uvs_param) {
  uvs->clear();
  facevarying_uvs->clear();

  if (!uvs_param) {
    return;
  }

  if (uvs_param.getNumSamples() > 0) {
    std::string uv_set_name =
        Alembic::Abc::GetSourceName(uvs_param.getMetaData());
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

// Traverse Alembic object tree and extract data.
static void VisitObjectAndExtractNode(Node *node_out, std::stringstream &ss,
                                      const Alembic::AbcGeom::IObject &obj,
                                      const std::string &indent) {
  std::string path = obj.getFullName();
  node_out->name = path;

  if (path.compare("/") != 0) {
    ss << "Object: path = " << path << std::endl;
  }

  Alembic::AbcGeom::ICompoundProperty props = obj.getProperties();
  VisitProperties(ss, props, indent);

  ss << "# of children = " << obj.getNumChildren() << std::endl;

  for (size_t i = 0; i < obj.getNumChildren(); i++) {
    const Alembic::AbcGeom::ObjectHeader &header = obj.getChildHeader(i);
    ss << " Child: header = " << header.getName() << std::endl;

    Alembic::AbcGeom::ICompoundProperty cprops =
        obj.getChild(i).getProperties();
    VisitProperties(ss, props, indent);

    Node *node = NULL;

    if (Alembic::AbcGeom::IXform::matches(header)) {
      ss << "    IXform" << std::endl;

      Alembic::AbcGeom::IXform xform(obj, header.getName());

      if (xform.getSchema().isConstant()) {
        Alembic::AbcGeom::M44d static_matrix =
            xform.getSchema().getValue().getMatrix();
        ss << "IXform static: " << header.getName() << ", " << static_matrix
           << std::endl;

        Xform *xform_node = new Xform();
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
        Alembic::AbcGeom::IXformSchema &ps = xform.getSchema();
        Alembic::AbcGeom::M44d matrix = ps.getValue(samplesel).getMatrix();
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
      Alembic::AbcGeom::IPolyMeshSchema &ps = pmesh.getSchema();

      std::cout << "  # of samples = " << ps.getNumSamples() << std::endl;

      if (ps.getNumSamples() > 0) {
        Mesh *mesh = new Mesh();
        ps.get(psample, samplesel);
        Alembic::Abc::P3fArraySamplePtr P = psample.getPositions();
        std::cout << "  # of positions   = " << P->size() << std::endl;
        std::cout << "  # of face counts = " << psample.getFaceCounts()->size()
                  << std::endl;

        std::vector<unsigned int> faces;  // temp
        bool ret = BuildFaceSet(faces, P, psample.getFaceIndices(),
                                psample.getFaceCounts());
        if (!ret) {
          std::cout << "  No faces in polymesh" << std::endl;
        }

        mesh->vertices.resize(3 * P->size());
        memcpy(mesh->vertices.data(), P->get(), sizeof(float) * 3 * P->size());
        mesh->faces = faces;

        // normals
        Alembic::AbcGeom::IN3fGeomParam normals_param = ps.getNormalsParam();
        std::vector<float> normals;
        std::vector<float> facevarying_normals;
        readPolyNormals(&normals, &facevarying_normals, normals_param);
        std::cout << "  # of normals   = " << (normals.size() / 3) << std::endl;
        std::cout << "  # of facevarying normals   = "
                  << (facevarying_normals.size() / 3) << std::endl;

        // TODO(syoyo): Do not generate normals when `facevarying_normals'
        // exists.
        if (normals.empty() && (!mesh->vertices.empty()) &&
            (!mesh->faces.empty())) {
          // Compute geometric normal.
          normals.resize(3 * P->size(), 0.0f);
          std::cout << "Compute normals" << std::endl;
          ComputeGeometricNormals(
              &normals, reinterpret_cast<const unsigned char *>(P->get()),
              /* stride */ sizeof(float) * 3, &mesh->faces.at(0),
              mesh->faces.size());
        }

        mesh->normals = normals;

        // UV
        Alembic::AbcGeom::IV2fGeomParam uvs_param = ps.getUVsParam();
        std::vector<float> uvs;
        std::vector<float> facevarying_uvs;
        readPolyUVs(&uvs, &facevarying_uvs, uvs_param);
        std::cout << "  # of uvs   = " << (uvs.size() / 2) << std::endl;
        std::cout << "  # of facevarying_uvs   = "
                  << (facevarying_uvs.size() / 2) << std::endl;
        mesh->texcoords = uvs;
        mesh->facevarying_texcoords = facevarying_uvs;

        node = mesh;

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
      Alembic::AbcGeom::ICurvesSchema &ps = curve.getSchema();

      std::cout << "  # of samples = " << ps.getNumSamples() << std::endl;

      if (ps.getNumSamples() > 0) {
        Curves *curves = new Curves();
        ps.get(psample, samplesel);

        const size_t num_curves = psample.getNumCurves();
        std::cout << "  # of curves = " << num_curves << std::endl;

        Alembic::Abc::P3fArraySamplePtr P = psample.getPositions();
        Alembic::Abc::FloatArraySamplePtr knots = psample.getKnots();
        Alembic::Abc::UcharArraySamplePtr orders = psample.getOrders();

        Alembic::Abc::Int32ArraySamplePtr num_vertices =
            psample.getCurvesNumVertices();

        if (knots) std::cout << "  # of knots= " << knots->size() << std::endl;
        if (orders)
          std::cout << "  # of orders= " << orders->size() << std::endl;
        std::cout << "  # of nvs= " << num_vertices->size() << std::endl;

        curves->points.resize(3 * P->size());
        memcpy(curves->points.data(), P->get(), sizeof(float) * 3 * P->size());

#if 0
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
          curves->nverts.resize(num_vertices->size());
          memcpy(curves->nverts.data(), num_vertices->get(),
                 sizeof(int) * num_vertices->size());
        }

        node = curves;

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
      VisitObjectAndExtractNode(
          node, ss,
          Alembic::AbcGeom::IObject(obj, obj.getChildHeader(i).getName()),
          indent);
    }

    node_out->children.push_back(node);
  }
}

static bool ConvertNodeToGLTF(picojson::object *root_out, const Scene &scene,
                              const Node *node) {
  assert(node);
  assert(!node->name.empty());

  // FIXME(syoyo): Add serialization method for each class.
  if (dynamic_cast<const Xform *>(node)) {
    const Xform *xform = dynamic_cast<const Xform *>(node);

    std::cout << "root xform" << node->name << std::endl;

    picojson::object json_node;
    picojson::array json_xform;
    picojson::array json_meshes;
    for (size_t i = 0; i < 16; i++) {
      json_xform.push_back(picojson::value(xform->xform[i]));
    }
    json_node["matrix"] = picojson::value(json_xform);
    json_node["name"] = picojson::value(xform->name);

    if (xform->children.size() > 0) {
      for (size_t i = 0; i < xform->children.size(); i++) {
        if (dynamic_cast<const Mesh *>(xform->children[i])) {
          std::map<std::string, int>::const_iterator it =
              scene.id_map.find(xform->children[i]->name);
          assert(it != scene.id_map.end());
          std::stringstream ss;
          ss << "_" << it->second;
          const std::string prefix = ss.str();

          json_meshes.push_back(picojson::value(std::string("mesh") + prefix));
        } else if (dynamic_cast<const Curves *>(xform->children[i])) {
          std::map<std::string, int>::const_iterator it =
              scene.id_map.find(xform->children[i]->name);
          assert(it != scene.id_map.end());
          std::stringstream ss;
          ss << "_" << it->second;
          const std::string prefix = ss.str();

          json_meshes.push_back(picojson::value(std::string("mesh") + prefix));
        }
      }
      if (json_meshes.size() > 0) {
        json_node["meshes"] = picojson::value(json_meshes);
      }
    }

    picojson::array json_children;
    if (xform->children.size() > 0) {
      for (size_t i = 0; i < xform->children.size(); i++) {
        if (xform->children[i]) {
          picojson::object json_child_node;
          bool ret = ConvertNodeToGLTF(root_out, scene, xform->children[i]);
          if (ret) {
            assert(!(xform->children[i]->name.empty()));
            json_children.push_back(picojson::value(xform->children[i]->name));
          }
        }
      }

      if (!json_children.empty()) {
        json_node["children"] = picojson::value(json_children);
      }
    }

    (*root_out)[node->name] = picojson::value(json_node);
  } else {
    return false;
  }

  return true;
}

static bool ConvertSceneToGLTF(picojson::object *out, const Scene &scene) {
  assert(scene.root_node);

  // Nodes
  picojson::object nodes;
  picojson::array node_names;
  {
    //picojson::object node;

    ConvertNodeToGLTF(&nodes, scene, scene.root_node);

    //nodes[scene.root_node->name] = picojson::value(node);
    node_names.push_back(picojson::value(scene.root_node->name));
  }
  (*out)["nodes"] = picojson::value(nodes);

  picojson::object scenes;
  picojson::object defaultScene;
  defaultScene["nodes"] = picojson::value(node_names);
  scenes["defaultScene"] = picojson::value(defaultScene);

  (*out)["scene"] = picojson::value("defaultScene");

  (*out)["scenes"] = picojson::value(scenes);

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

static bool ConvertMeshToGLTF(picojson::object *buffers_out,
                              picojson::object *buffer_views_out,
                              picojson::object *meshes_out,
                              picojson::object *accessors_out, const Mesh &mesh,
                              const int id) {
  std::stringstream prefix_ss;
  prefix_ss << "_" << id;

  const std::string prefix = prefix_ss.str();

  {
    // Position
    {
      std::string vertices_b64data = base64_encode(
          reinterpret_cast<unsigned char const *>(mesh.vertices.data()),
          mesh.vertices.size() * sizeof(float));
      picojson::object buf;

      buf["type"] = picojson::value("arraybuffer");
      buf["uri"] =
          picojson::value(std::string("data:application/octet-stream;base64,") +
                          vertices_b64data);
      buf["byteLength"] = picojson::value(
          static_cast<double>(mesh.vertices.size() * sizeof(float)));

      (*buffers_out)["vertices" + prefix] = picojson::value(buf);
    }

    // Normal
    if (!mesh.normals.empty()) {
      std::string normals_b64data = base64_encode(
          reinterpret_cast<unsigned char const *>(mesh.normals.data()),
          mesh.normals.size() * sizeof(float));
      picojson::object buf;

      buf["type"] = picojson::value("arraybuffer");
      buf["uri"] =
          picojson::value(std::string("data:application/octet-stream;base64,") +
                          normals_b64data);
      buf["byteLength"] = picojson::value(
          static_cast<double>(mesh.normals.size() * sizeof(float)));

      (*buffers_out)["normals" + prefix] = picojson::value(buf);
    }

    {
      std::string faces_b64data = base64_encode(
          reinterpret_cast<unsigned char const *>(mesh.faces.data()),
          mesh.faces.size() * sizeof(unsigned int));
      picojson::object buf;

      buf["type"] = picojson::value("arraybuffer");
      buf["uri"] = picojson::value(
          std::string("data:application/octet-stream;base64,") + faces_b64data);
      buf["byteLength"] = picojson::value(
          static_cast<double>(mesh.faces.size() * sizeof(unsigned int)));

      (*buffers_out)["indices" + prefix] = picojson::value(buf);
    }
  }

  {
    {
      picojson::object buffer_view_vertices;
      buffer_view_vertices["buffer"] =
          picojson::value(std::string("vertices") + prefix);
      buffer_view_vertices["byteLength"] = picojson::value(
          static_cast<double>(mesh.vertices.size() * sizeof(float)));
      buffer_view_vertices["byteOffset"] =
          picojson::value(static_cast<double>(0));
      buffer_view_vertices["target"] =
          picojson::value(static_cast<int64_t>(TINYGLTF_TARGET_ARRAY_BUFFER));

      (*buffer_views_out)["bufferView_vertices" + prefix] =
          picojson::value(buffer_view_vertices);
    }

    if (!mesh.normals.empty()) {
      picojson::object buffer_view_normals;
      buffer_view_normals["buffer"] =
          picojson::value(std::string("normals") + prefix);
      buffer_view_normals["byteLength"] = picojson::value(
          static_cast<double>(mesh.normals.size() * sizeof(float)));
      buffer_view_normals["byteOffset"] =
          picojson::value(static_cast<double>(0));
      buffer_view_normals["target"] =
          picojson::value(static_cast<int64_t>(TINYGLTF_TARGET_ARRAY_BUFFER));

      (*buffer_views_out)["bufferView_normals" + prefix] =
          picojson::value(buffer_view_normals);
    }

    {
      picojson::object buffer_view_indices;
      buffer_view_indices["buffer"] =
          picojson::value(std::string("indices") + prefix);
      buffer_view_indices["byteLength"] = picojson::value(
          static_cast<double>(mesh.faces.size() * sizeof(unsigned int)));
      buffer_view_indices["byteOffset"] =
          picojson::value(static_cast<double>(0));
      buffer_view_indices["target"] = picojson::value(
          static_cast<int64_t>(TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER));

      (*buffer_views_out)["bufferView_indices" + prefix] =
          picojson::value(buffer_view_indices);
    }
  }

  {
    picojson::object attributes;

    attributes["POSITION"] =
        picojson::value(std::string("accessor_vertices") + prefix);
    if (!mesh.normals.empty()) {
      attributes["NORMAL"] =
          picojson::value(std::string("accessor_normals") + prefix);
    }

    picojson::object primitive;
    primitive["attributes"] = picojson::value(attributes);
    primitive["indices"] = picojson::value("accessor_indices" + prefix);
    primitive["material"] =
        picojson::value("material_1");  // FIXME(syoyo): Assign material.
    primitive["mode"] =
        picojson::value(static_cast<int64_t>(TINYGLTF_MODE_TRIANGLES));

    picojson::array primitive_array;
    primitive_array.push_back(picojson::value(primitive));

    picojson::object m;
    m["name"] = picojson::value(mesh.name);
    m["primitives"] = picojson::value(primitive_array);

    picojson::object meshes;
    (*meshes_out)["mesh" + prefix] = picojson::value(m);
  }

  {
    picojson::object accessors;

    {
      picojson::object accessor_vertices;
      accessor_vertices["bufferView"] =
          picojson::value(std::string("bufferView_vertices" + prefix));
      accessor_vertices["byteOffset"] =
          picojson::value(static_cast<int64_t>(0));
      accessor_vertices["byteStride"] =
          picojson::value(static_cast<double>(3 * sizeof(float)));
      accessor_vertices["componentType"] =
          picojson::value(static_cast<int64_t>(TINYGLTF_COMPONENT_TYPE_FLOAT));
      accessor_vertices["count"] =
          picojson::value(static_cast<int64_t>(mesh.vertices.size()));
      accessor_vertices["type"] = picojson::value(std::string("VEC3"));
      (*accessors_out)["accessor_vertices" + prefix] =
          picojson::value(accessor_vertices);
    }

    if (!mesh.normals.empty()) {
      picojson::object accessor_normals;
      accessor_normals["bufferView"] =
          picojson::value(std::string("bufferView_normals" + prefix));
      accessor_normals["byteOffset"] = picojson::value(static_cast<int64_t>(0));
      accessor_normals["byteStride"] =
          picojson::value(static_cast<double>(3 * sizeof(float)));
      accessor_normals["componentType"] =
          picojson::value(static_cast<int64_t>(TINYGLTF_COMPONENT_TYPE_FLOAT));
      accessor_normals["count"] =
          picojson::value(static_cast<int64_t>(mesh.vertices.size()));
      accessor_normals["type"] = picojson::value(std::string("VEC3"));
      (*accessors_out)["accessor_normals" + prefix] =
          picojson::value(accessor_normals);
    }

    {
      picojson::object accessor_indices;
      accessor_indices["bufferView"] =
          picojson::value(std::string("bufferView_indices" + prefix));
      accessor_indices["byteOffset"] = picojson::value(static_cast<int64_t>(0));
      accessor_indices["componentType"] = picojson::value(
          static_cast<int64_t>(TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT));
      accessor_indices["count"] =
          picojson::value(static_cast<int64_t>(mesh.faces.size()));
      accessor_indices["type"] = picojson::value(std::string("SCALAR"));
      (*accessors_out)["accessor_indices" + prefix] =
          picojson::value(accessor_indices);
    }
  }

  return true;
}

static bool ConvertCurvesToGLTF(picojson::object *buffers_out,
                                picojson::object *buffer_views_out,
                                picojson::object *meshes_out,
                                picojson::object *accessors_out,
                                const Curves &curves, const int id) {
  std::stringstream prefix_ss;
  prefix_ss << "_" << id;

  const std::string prefix = prefix_ss.str();

  {{{std::string b64data = base64_encode(
         reinterpret_cast<unsigned char const *>(curves.points.data()),
         curves.points.size() * sizeof(float));
  picojson::object buf;

  buf["type"] = picojson::value("arraybuffer");
  buf["uri"] = picojson::value(
      std::string("data:application/octet-stream;base64,") + b64data);
  buf["byteLength"] = picojson::value(
      static_cast<double>(curves.points.size() * sizeof(float)));

  (*buffers_out)["points" + prefix] = picojson::value(buf);
}

// Out extension
{
  std::string b64data = base64_encode(
      reinterpret_cast<unsigned char const *>(curves.nverts.data()),
      curves.nverts.size() * sizeof(int));
  picojson::object buf;

  buf["type"] = picojson::value("arraybuffer");
  buf["uri"] = picojson::value(
      std::string("data:application/octet-stream;base64,") + b64data);
  buf["byteLength"] =
      picojson::value(static_cast<double>(curves.nverts.size() * sizeof(int)));

  (*buffers_out)["nverts" + prefix] = picojson::value(buf);
}
}
}

{{{picojson::object buffer_view_points;
buffer_view_points["buffer"] = picojson::value(std::string("points") + prefix);
buffer_view_points["byteLength"] =
    picojson::value(static_cast<double>(curves.points.size() * sizeof(float)));
buffer_view_points["byteOffset"] = picojson::value(static_cast<double>(0));
buffer_view_points["target"] =
    picojson::value(static_cast<int64_t>(TINYGLTF_TARGET_ARRAY_BUFFER));
(*buffer_views_out)["bufferView_points" + prefix] =
    picojson::value(buffer_view_points);
}

{
  picojson::object buffer_view_nverts;
  buffer_view_nverts["buffer"] =
      picojson::value(std::string("nverts") + prefix);
  buffer_view_nverts["byteLength"] =
      picojson::value(static_cast<double>(curves.nverts.size() * sizeof(int)));
  buffer_view_nverts["byteOffset"] = picojson::value(static_cast<double>(0));
  buffer_view_nverts["target"] =
      picojson::value(static_cast<int64_t>(TINYGLTF_TARGET_ARRAY_BUFFER));
  (*buffer_views_out)["bufferView_nverts" + prefix] =
      picojson::value(buffer_view_nverts);
}
}
}

{
  picojson::object attributes;

  attributes["POSITION"] =
      picojson::value(std::string("accessor_points") + prefix);
  attributes["NVERTS"] =
      picojson::value(std::string("accessor_nverts") + prefix);

  // Extra information for curves primtive.
  picojson::object extra;
  extra["ext_mode"] = picojson::value("curves");

  picojson::object primitive;
  primitive["attributes"] = picojson::value(attributes);
  // primitive["indices"] = picojson::value("accessor_indices");
  primitive["material"] = picojson::value("material_1");
  primitive["mode"] = picojson::value(static_cast<int64_t>(
      TINYGLTF_MODE_POINTS));  // Use GL_POINTS for backward compatibility
  primitive["extras"] = picojson::value(extra);

  picojson::array primitive_array;
  primitive_array.push_back(picojson::value(primitive));

  picojson::object m;
  m["name"] = picojson::value(curves.name);
  m["primitives"] = picojson::value(primitive_array);

  picojson::object meshes;
  (*meshes_out)["mesh" + prefix] = picojson::value(m);
}

{
  {
    picojson::object accessor_points;
    accessor_points["bufferView"] =
        picojson::value(std::string("bufferView_points") + prefix);
    accessor_points["byteOffset"] = picojson::value(static_cast<int64_t>(0));
    accessor_points["byteStride"] =
        picojson::value(static_cast<double>(3 * sizeof(float)));
    accessor_points["componentType"] =
        picojson::value(static_cast<int64_t>(TINYGLTF_COMPONENT_TYPE_FLOAT));
    accessor_points["count"] =
        picojson::value(static_cast<int64_t>(curves.points.size()));
    accessor_points["type"] = picojson::value(std::string("VEC3"));
    (*accessors_out)["accessor_points" + prefix] =
        picojson::value(accessor_points);
  }

  {
    picojson::object accessor_nverts;
    accessor_nverts["bufferView"] =
        picojson::value(std::string("bufferView_nverts") + prefix);
    accessor_nverts["byteOffset"] = picojson::value(static_cast<int64_t>(0));
    accessor_nverts["byteStride"] =
        picojson::value(static_cast<double>(sizeof(int)));
    accessor_nverts["componentType"] =
        picojson::value(static_cast<int64_t>(TINYGLTF_COMPONENT_TYPE_INT));
    accessor_nverts["count"] =
        picojson::value(static_cast<int64_t>(curves.nverts.size()));
    accessor_nverts["type"] = picojson::value(std::string("SCALAR"));
    (*accessors_out)["accessor_nverts" + prefix] =
        picojson::value(accessor_nverts);
  }
}

return true;
}

static bool SaveSceneToGLTF(const std::string &output_filename,
                            const Scene &scene) {
  picojson::object root;

  ConvertSceneToGLTF(&root, scene);

  // Write header.
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
  picojson::object buffer_views;
  picojson::object meshes;
  picojson::object accessors;

  // PolyMesh
  std::map<std::string, const Mesh *>::const_iterator mesh_it(
      scene.mesh_map.begin());
  for (; mesh_it != scene.mesh_map.end(); mesh_it++) {
    const Mesh &mesh = *(mesh_it->second);

    assert(scene.id_map.find(mesh.name) != scene.id_map.end());

    const int id = (scene.id_map.find(mesh.name))->second;

    bool ret = ConvertMeshToGLTF(&buffers, &buffer_views, &meshes, &accessors,
                                 mesh, id);
    assert(ret);
    (void)ret;
  }

  // Curves
  std::map<std::string, const Curves *>::const_iterator curves_it(
      scene.curves_map.begin());
  for (; curves_it != scene.curves_map.end(); curves_it++) {
    const Curves &curves = *(curves_it->second);

    assert(scene.id_map.find(curves_it->first) != scene.id_map.end());

    const int id = (scene.id_map.find(curves.name))->second;

    std::cout << "Curves: " << curves.name << std::endl;
    bool ret = ConvertCurvesToGLTF(&buffers, &buffer_views, &meshes, &accessors,
                                   curves, id);
    assert(ret);
    (void)ret;
  }

  root["buffers"] = picojson::value(buffers);
  root["bufferViews"] = picojson::value(buffer_views);
  root["meshes"] = picojson::value(meshes);
  root["accessors"] = picojson::value(accessors);

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

  std::string s = v.serialize(/* pretty */ true);
  ifs.write(s.data(), static_cast<ssize_t>(s.size()));
  ifs.close();

  return true;
}

// Flatten node tree.
// Also assign ID for each graphics primitive.
static void ConvertNodeToScene(Scene *scene, int *id, const Node *node) {
  if (!node) return;

  std::cout << "node " << node->name << std::endl;

  if (dynamic_cast<const Xform *>(node)) {
    // Allow only one root node in the scene.
    if (scene->root_node == NULL) {
      scene->root_node = node;
    } else {
      // Assume child xform node
      assert(scene->xform_map.find(node->name) == scene->xform_map.end());
      scene->xform_map[node->name] = dynamic_cast<const Xform *>(node);
      // scene->id_map[node->name] = (*id)++;
    }

  } else if (dynamic_cast<const Mesh *>(node)) {
    assert(scene->mesh_map.find(node->name) == scene->mesh_map.end());
    scene->mesh_map[node->name] = dynamic_cast<const Mesh *>(node);
    scene->id_map[node->name] = (*id)++;
  } else if (dynamic_cast<const Curves *>(node)) {
    assert(scene->curves_map.find(node->name) == scene->curves_map.end());
    scene->curves_map[node->name] = dynamic_cast<const Curves *>(node);
    scene->id_map[node->name] = (*id)++;
  }

  for (size_t i = 0; i < node->children.size(); i++) {
    ConvertNodeToScene(scene, id, node->children[i]);
  }
}

int main(int argc, char **argv) {
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
  Node *node = new Node();
  VisitObjectAndExtractNode(node, ss, root, /* indent */ "  ");

  // std::cout << ss.str() << std::endl;

  int id = 1;  // ID starts from 1.
  ConvertNodeToScene(&scene, &id, node);
  delete node;

  SaveSceneToGLTF(gltf_filename, scene);

  return EXIT_SUCCESS;
}
